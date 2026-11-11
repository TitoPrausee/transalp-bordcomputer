/*
 * logger.c -- Daten-Logger mit Ringbuffer und Batch-Writes
 *
 * Strategie:
 *   1. GPS/Sensor-Daten im RAM sammeln (Ringbuffer)
 *   2. Bei Erreichen der Batch-Groesse oder Timeout auf SD schreiben
 *   3. Schreiben im Logger-Task (niedrige Prioritaet, Core 0)
 *
 * Formate:
 *   - CSV: menschenlesbar (timestamp,lat,lon,speed,rpm,temp,alt,...)
 *   - GPX: GPS-Track-Export (XML)
 *
 * Komprimierung:
 *   Nur Punkte speichern wenn sich Position >10m geaendert hat.
 *   Maximum 10 Hz Logging-Rate, konfigurierbar.
 *
 * Dateinamen-Konvention: BOOTnnn_HHMMSS.csv / .gpx
 *
 * SPI-Mutex wird beim Schreiben genommen (shared mit Display).
 */

#include "storage.h"
#include "board_config.h"
#include "gps.h"
#include "sensors.h"
#include "event_bus.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

static const char *TAG = "logger";

/* ---- Konstanten ---- */
#define LOG_BATCH_SIZE        64
#define LOG_DIR               SD_MOUNT_POINT "/logs"
#define TRACK_DIR             SD_MOUNT_POINT "/tracks"

/* ---- Ringbuffer ---- */
static log_entry_t s_ring_buffer[LOGGER_RINGBUF_SIZE];
static volatile uint16_t s_ring_head = 0;
static volatile uint16_t s_ring_tail = 0;
static volatile uint16_t s_ring_count = 0;

/* Batch-Buffer fuer SD-Schreibvorgaenge */
static log_entry_t s_batch_buffer[LOG_BATCH_SIZE];

/* ---- Tour-Kontext ---- */
static tour_context_t s_tour;
static logger_stats_t s_stats;

/* ---- Mutex ---- */
static SemaphoreHandle_t s_ring_mutex = NULL;
static SemaphoreHandle_t s_tour_mutex = NULL;

/* ---- Konfiguration ---- */
static uint8_t s_log_rate_hz = LOGGER_MAX_RATE_HZ;
static uint32_t s_log_interval_ms = 100;

/* ---- Hilfsfunktionen ---- */

static float haversine_m(float lat1, float lon1, float lat2, float lon2)
{
    const float R = 6371000.0f;
    float phi1 = lat1 * (float)M_PI / 180.0f;
    float phi2 = lat2 * (float)M_PI / 180.0f;
    float dphi = (lat2 - lat1) * (float)M_PI / 180.0f;
    float dlam = (lon2 - lon1) * (float)M_PI / 180.0f;

    float a = sinf(dphi / 2) * sinf(dphi / 2) +
              cosf(phi1) * cosf(phi2) *
              sinf(dlam / 2) * sinf(dlam / 2);
    float c = 2 * atan2f(sqrtf(a), sqrtf(1 - a));
    return R * c;
}

static void make_filename(char *buf, size_t buflen, const char *suffix)
{
    uint32_t ms = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t sec = ms / 1000;
    uint32_t days = sec / 86400;
    uint32_t h = (sec % 86400) / 3600;
    uint32_t m = (sec % 3600) / 60;
    uint32_t s = sec % 60;

    snprintf(buf, buflen, "%s/BOOT%03lu_%02lu%02lu%02lu.%s",
             suffix, (unsigned long)days,
             (unsigned long)h, (unsigned long)m, (unsigned long)s, suffix);
}

/* ---- Ringbuffer Operationen ---- */

static bool ring_push(const log_entry_t *entry)
{
    if (!s_ring_mutex || xSemaphoreTake(s_ring_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false;
    }

    if (s_ring_count >= LOGGER_RINGBUF_SIZE) {
        s_ring_tail = (s_ring_tail + 1) % LOGGER_RINGBUF_SIZE;
        s_ring_count--;
        s_stats.points_skipped++;
    }

    s_ring_buffer[s_ring_head] = *entry;
    s_ring_head = (s_ring_head + 1) % LOGGER_RINGBUF_SIZE;
    s_ring_count++;

    xSemaphoreGive(s_ring_mutex);
    return true;
}

static uint16_t ring_pop_batch(log_entry_t *batch, uint16_t max)
{
    uint16_t count = 0;

    if (!s_ring_mutex || xSemaphoreTake(s_ring_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return 0;
    }

    while (s_ring_count > 0 && count < max) {
        batch[count] = s_ring_buffer[s_ring_tail];
        s_ring_tail = (s_ring_tail + 1) % LOGGER_RINGBUF_SIZE;
        s_ring_count--;
        count++;
    }

    xSemaphoreGive(s_ring_mutex);
    return count;
}

/* ---- CSV-Format ---- */

int logger_write_csv_header(FILE *f)
{
    if (!f) return -1;

    const char *header =
        "timestamp_ms,lat,lon,speed_kmh,course_deg,altitude_m,"
        "rpm,temp_c,roll_deg,pitch_deg,fuel_l\n";

    size_t n = fwrite(header, 1, strlen(header), f);
    return (n == strlen(header)) ? 0 : -1;
}

int logger_write_csv_row(FILE *f, const log_entry_t *entry)
{
    if (!f || !entry) return -1;

    char line[256];
    int len = snprintf(line, sizeof(line),
        "%lu,%.6f,%.6f,%.1f,%.1f,%.1f,%u,%.1f,%.1f,%.1f,%.1f\n",
        (unsigned long)entry->timestamp_ms,
        entry->lat, entry->lon,
        entry->speed_kmh, entry->course_deg, entry->altitude_m,
        (unsigned)entry->rpm, entry->temp_c,
        entry->roll_deg, entry->pitch_deg, entry->fuel_l);

    if (len <= 0 || len >= (int)sizeof(line)) return -1;

    size_t n = fwrite(line, 1, (size_t)len, f);
    return (n == (size_t)len) ? 0 : -1;
}

/* ---- Batch-Flush auf SD-Karte ---- */

static int flush_batch_to_csv(log_entry_t *batch, uint16_t count)
{
    if (!sd_card_is_mounted() || count == 0) return -1;

    char path[SD_MAX_PATH_LEN];
    make_filename(path, sizeof(path), "csv");

    if (!s_tour_mutex || xSemaphoreTake(s_tour_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return -1;
    }

    if (spi_mutex) {
        xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100));
    }

    FILE *f = fopen(path, "a");
    if (!f) {
        f = fopen(path, "w");
        if (f) {
            logger_write_csv_header(f);
        }
    }

    if (!f) {
        ESP_LOGE(TAG, "Cannot open %s for writing", path);
        s_stats.write_errors++;
        if (spi_mutex) xSemaphoreGive(spi_mutex);
        xSemaphoreGive(s_tour_mutex);
        return -1;
    }

    if (ftell(f) == 0) {
        logger_write_csv_header(f);
    }

    for (uint16_t i = 0; i < count; i++) {
        if (logger_write_csv_row(f, &batch[i]) != 0) {
            s_stats.write_errors++;
            break;
        }
        s_stats.points_written++;
    }

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    if (spi_mutex) xSemaphoreGive(spi_mutex);

    s_stats.flush_count++;
    xSemaphoreGive(s_tour_mutex);

    ESP_LOGD(TAG, "Flushed %u entries to %s", (unsigned)count, path);
    return count;
}

/* ---- GPX-Export ---- */

static int write_gpx_header(FILE *f)
{
    if (!f) return -1;

    const char *hdr =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<gpx version=\"1.1\" creator=\"transalp-bordcomputer\" "
        "xmlns=\"http://www.topografix.com/GPX/1/1\">\n"
        "  <metadata>\n"
        "    <name>Tour Track</name>\n"
        "  </metadata>\n"
        "  <trk>\n"
        "    <name>Tour</name>\n"
        "    <trkseg>\n";

    size_t n = fwrite(hdr, 1, strlen(hdr), f);
    return (n == strlen(hdr)) ? 0 : -1;
}

static int write_gpx_footer(FILE *f)
{
    if (!f) return -1;

    const char *ftr =
        "    </trkseg>\n"
        "  </trk>\n"
        "</gpx>\n";

    size_t n = fwrite(ftr, 1, strlen(ftr), f);
    return (n == strlen(ftr)) ? 0 : -1;
}

static int write_gpx_point(FILE *f, const log_entry_t *entry)
{
    if (!f || !entry) return -1;

    char line[256];
    int len = snprintf(line, sizeof(line),
        "      <trkpt lat=\"%.6f\" lon=\"%.6f\">\n"
        "        <ele>%.1f</ele>\n"
        "        <time>%lu</time>\n"
        "      </trkpt>\n",
        entry->lat, entry->lon,
        entry->altitude_m,
        (unsigned long)entry->timestamp_ms);

    if (len <= 0 || len >= (int)sizeof(line)) return -1;

    size_t n = fwrite(line, 1, (size_t)len, f);
    return (n == (size_t)len) ? 0 : -1;
}

int logger_export_gpx(const char *path)
{
    if (!path) return -1;
    if (!sd_card_is_mounted()) {
        ESP_LOGW(TAG, "Cannot export GPX: SD not mounted");
        return -1;
    }

    char full[SD_MAX_PATH_LEN];
    snprintf(full, sizeof(full), "%s/%s", SD_MOUNT_POINT, path);

    sd_card_mkdir("tracks");

    if (spi_mutex) {
        xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100));
    }

    FILE *f = fopen(full, "w");
    if (!f) {
        ESP_LOGE(TAG, "Cannot create GPX: %s", full);
        if (spi_mutex) xSemaphoreGive(spi_mutex);
        return -1;
    }

    write_gpx_header(f);

    if (s_ring_mutex && xSemaphoreTake(s_ring_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint16_t idx = s_ring_tail;
        for (uint16_t i = 0; i < s_ring_count; i++) {
            write_gpx_point(f, &s_ring_buffer[idx]);
            idx = (idx + 1) % LOGGER_RINGBUF_SIZE;
        }
        xSemaphoreGive(s_ring_mutex);
    }

    write_gpx_footer(f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);

    if (spi_mutex) xSemaphoreGive(spi_mutex);

    ESP_LOGI(TAG, "GPX exported to %s", full);
    return 0;
}

/* ---- Tour-Verwaltung ---- */

int logger_start_tour(void)
{
    if (!s_tour_mutex || xSemaphoreTake(s_tour_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }

    if (s_tour.state == TOUR_ACTIVE) {
        xSemaphoreGive(s_tour_mutex);
        return 0;
    }

    s_tour.state = TOUR_ACTIVE;
    s_tour.start_time_ms = (uint32_t)(esp_timer_get_time() / 1000);
    s_tour.point_count = 0;
    s_tour.segment_count = 1;
    s_tour.has_last_point = false;
    s_tour.last_lat = 0;
    s_tour.last_lon = 0;

    make_filename(s_tour.csv_path, sizeof(s_tour.csv_path), "csv");

    char fname[64];
    make_filename(fname, sizeof(fname), "gpx");
    snprintf(s_tour.gpx_path, sizeof(s_tour.gpx_path), "%s/%s",
             TRACK_DIR, strrchr(fname, '/') ? strrchr(fname, '/') + 1 : fname);

    s_stats.tours_started++;

    xSemaphoreGive(s_tour_mutex);

    ESP_LOGI(TAG, "Tour started: %s", s_tour.csv_path);
    return 0;
}

int logger_stop_tour(void)
{
    if (!s_tour_mutex || xSemaphoreTake(s_tour_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }

    if (s_tour.state != TOUR_ACTIVE && s_tour.state != TOUR_PAUSED) {
        xSemaphoreGive(s_tour_mutex);
        return 0;
    }

    s_tour.state = TOUR_IDLE;
    xSemaphoreGive(s_tour_mutex);

    logger_flush();
    logger_export_gpx(s_tour.gpx_path);

    s_stats.tours_stopped++;

    ESP_LOGI(TAG, "Tour stopped: %u points, %u segments",
             (unsigned)s_tour.point_count, (unsigned)s_tour.segment_count);
    return 0;
}

int logger_pause_tour(void)
{
    if (!s_tour_mutex || xSemaphoreTake(s_tour_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }

    if (s_tour.state == TOUR_ACTIVE) {
        s_tour.state = TOUR_PAUSED;
        ESP_LOGI(TAG, "Tour paused");
    }

    xSemaphoreGive(s_tour_mutex);
    return 0;
}

int logger_resume_tour(void)
{
    if (!s_tour_mutex || xSemaphoreTake(s_tour_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }

    if (s_tour.state == TOUR_PAUSED) {
        s_tour.state = TOUR_ACTIVE;
        s_tour.segment_count++;
        s_tour.has_last_point = false;
        ESP_LOGI(TAG, "Tour resumed (segment %u)", (unsigned)s_tour.segment_count);
    }

    xSemaphoreGive(s_tour_mutex);
    return 0;
}

/* ---- Track-Point hinzufuegen ---- */

int logger_add_track_point(const log_entry_t *entry)
{
    if (!entry) return -1;

    if (s_tour.state != TOUR_ACTIVE) return 0;

    if (s_tour.has_last_point) {
        float dist = haversine_m(s_tour.last_lat, s_tour.last_lon,
                                 entry->lat, entry->lon);
        if (dist < LOGGER_MIN_DIST_M) {
            s_stats.points_skipped++;
            return 0;
        }
    }

    log_entry_t e = *entry;
    if (e.timestamp_ms == 0) {
        e.timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000);
    }

    if (!ring_push(&e)) {
        ESP_LOGW(TAG, "Ring buffer push failed");
        return -1;
    }

    if (s_tour_mutex && xSemaphoreTake(s_tour_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        s_tour.point_count++;
        s_tour.last_lat = entry->lat;
        s_tour.last_lon = entry->lon;
        s_tour.has_last_point = true;
        xSemaphoreGive(s_tour_mutex);
    }

    s_stats.total_points++;
    return 0;
}

void logger_set_rate(uint8_t rate_hz)
{
    if (rate_hz == 0) rate_hz = 1;
    if (rate_hz > LOGGER_MAX_RATE_HZ) rate_hz = LOGGER_MAX_RATE_HZ;

    s_log_rate_hz = rate_hz;
    s_log_interval_ms = 1000 / rate_hz;

    ESP_LOGI(TAG, "Log rate set to %u Hz (interval=%lums)",
             (unsigned)rate_hz, (unsigned long)s_log_interval_ms);
}

int logger_flush(void)
{
    uint16_t count = ring_pop_batch(s_batch_buffer, LOG_BATCH_SIZE);
    if (count == 0) return 0;
    return flush_batch_to_csv(s_batch_buffer, count);
}

/* ---- Initialisierung ---- */

void logger_init(void)
{
    s_ring_mutex = xSemaphoreCreateMutex();
    s_tour_mutex = xSemaphoreCreateMutex();

    if (!s_ring_mutex || !s_tour_mutex) {
        ESP_LOGE(TAG, "Failed to create mutexes");
        return;
    }

    s_ring_head = 0;
    s_ring_tail = 0;
    s_ring_count = 0;
    s_log_rate_hz = LOGGER_MAX_RATE_HZ;
    s_log_interval_ms = 1000 / s_log_rate_hz;

    memset(&s_tour, 0, sizeof(s_tour));
    memset(&s_stats, 0, sizeof(s_stats));
    memset(s_ring_buffer, 0, sizeof(s_ring_buffer));
    s_tour.state = TOUR_IDLE;

    ESP_LOGI(TAG, "Logger initialized (ringbuf=%d, batch=%d, rate=%uHz)",
             LOGGER_RINGBUF_SIZE, LOG_BATCH_SIZE, (unsigned)s_log_rate_hz);
}

/* ---- Logger-Task ---- */

void logger_task(void *arg)
{
    (void)arg;

    gps_data_t gps;
    sensor_data_t sens;
    uint32_t last_flush_ms = 0;
    uint32_t last_log_ms = 0;

    ESP_LOGI(TAG, "Logger task started on core %d", xPortGetCoreID());

    while (1) {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

        if ((now - last_log_ms) < s_log_interval_ms) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        bool have_gps = (gps_data_queue &&
                         xQueuePeek(gps_data_queue, &gps, pdMS_TO_TICKS(50)) == pdTRUE);
        bool have_sens = (sensor_data_queue &&
                          xQueuePeek(sensor_data_queue, &sens, 0) == pdTRUE);

        if (have_gps && have_sens && s_tour.state == TOUR_ACTIVE) {
            log_entry_t entry;
            memset(&entry, 0, sizeof(entry));

            entry.timestamp_ms = now;
            entry.lat = gps.lat;
            entry.lon = gps.lon;
            entry.speed_kmh = gps.speed_kmh;
            entry.altitude_m = gps.altitude_m;
            entry.course_deg = gps.course_deg;
            entry.rpm = sens.rpm;
            entry.temp_c = sens.temp_c;
            entry.roll_deg = sens.roll_deg;
            entry.pitch_deg = sens.pitch_deg;
            entry.fuel_l = sens.fuel_level_l;

            logger_add_track_point(&entry);
            last_log_ms = now;
        }

        bool flush_timeout = ((now - last_flush_ms) >= LOGGER_FLUSH_TIMEOUT_MS);
        bool flush_full = (s_ring_count >= LOG_BATCH_SIZE);

        if (flush_full || (flush_timeout && s_ring_count > 0)) {
            if (sd_card_is_mounted()) {
                logger_flush();
            }
            last_flush_ms = now;
        }

        if (event_bus_wait(EVT_SDCARD_MOUNTED, 0) && s_ring_count > 0 &&
            (now - last_flush_ms) >= 1000) {
            logger_flush();
            last_flush_ms = now;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ---- Statistik / Kontext ---- */

const logger_stats_t *logger_get_stats(void)
{
    return &s_stats;
}

const tour_context_t *logger_get_tour_context(void)
{
    return &s_tour;
}
