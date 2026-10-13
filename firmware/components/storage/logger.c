/*
 * logger.c — Daten-Logger mit Ringbuffer und Batch-Writes
 *
 * Strategie:
 *   1. GPS/Sensor-Daten im RAM sammeln (Ringbuffer)
 *   2. Alle N Sekunden als Batch auf SD schreiben
 *   3. Schreiben im Logger-Task (niedrige Prioritaet)
 *
 * Formate:
 *   - CSV: menschenlesbar (timestamp,lat,lon,speed,rpm,temp,alt,...)
 *   - Binaer: kompakt fuer High-Rate IMU (14 Bytes pro Eintrag)
 *   - GPX: GPS-Track-Export
 *
 * TODO: Batch-Buffer implementieren.
 * TODO: Dateinamen-Konvention: YYYYMMDD_HHMMSS.csv
 * TODO: SPI-Mutex beim Schreiben (shared mit Display).
 * TODO: fsync() nach jedem Batch.
 *
 * Siehe docs/research/03-software-stack.md Abschnitt 6.
 */

#include "storage.h"
#include "board_config.h"
#include "gps.h"
#include "sensors.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "logger";

#define LOG_BATCH_SIZE   64
#define LOG_FILE_PATH     SD_MOUNT_POINT "/logs/"

typedef struct {
    uint32_t timestamp;
    float lat, lon;
    float speed_kmh;
    uint16_t rpm;
    float temp_c;
    float altitude_m;
    float roll_deg, pitch_deg;
} log_entry_t;

static log_entry_t s_log_buffer[LOG_BATCH_SIZE];
static volatile int s_log_index = 0;

/* TODO: Mutex fuer Buffer-Zugriff */
static SemaphoreHandle_t s_log_mutex = NULL;

void logger_init(void)
{
    s_log_mutex = xSemaphoreCreateMutex();
    memset(s_log_buffer, 0, sizeof(s_log_buffer));

    /* TODO: Neue Log-Datei oeffnen:
     *   /sdcard/logs/SESSION_YYYYMMDD_HHMMSS.csv
     *   Header schreiben: timestamp,lat,lon,speed_kmh,rpm,temp_c,alt_m,...
     */

    ESP_LOGI(TAG, "Logger initialized (stub), batch=%d", LOG_BATCH_SIZE);
}

void logger_task(void *arg)
{
    gps_data_t gps;
    sensor_data_t sens;

    while (1) {
        /* Daten aus Queues pullen */
        bool have_gps = (gps_data_queue &&
                         xQueuePeek(gps_data_queue, &gps, pdMS_TO_TICKS(50)) == pdTRUE);
        bool have_sens = (sensor_data_queue &&
                          xQueuePeek(sensor_data_queue, &sens, 0) == pdTRUE);

        if (have_gps && have_sens) {
            log_entry_t entry = {
                .timestamp = xthal_get_ccount() / 240000,
                .lat = gps.lat,
                .lon = gps.lon,
                .speed_kmh = gps.speed_kmh,
                .rpm = sens.rpm,
                .temp_c = sens.temp_c,
                .altitude_m = sens.altitude_m,
                .roll_deg = sens.roll_deg,
                .pitch_deg = sens.pitch_deg,
            };

            /* In Buffer schreiben (thread-safe) */
            if (s_log_mutex && xSemaphoreTake(s_log_mutex, pdMS_TO_TICKS(10))) {
                if (s_log_index < LOG_BATCH_SIZE) {
                    s_log_buffer[s_log_index++] = entry;
                }
                xSemaphoreGive(s_log_mutex);
            }
        }

        /* Batch schreiben wenn voll oder Timeout */
        if (s_log_index >= LOG_BATCH_SIZE) {
            /* TODO: Batch auf SD schreiben.
             *
             * if (sd_card_is_mounted() && spi_mutex) {
             *     xSemaphoreTake(spi_mutex, portMAX_DELAY);
             *     FILE *f = fopen(LOG_FILE_PATH "current.csv", "a");
             *     if (f) {
             *         for (int i = 0; i < s_log_index; i++) {
             *             fprintf(f, "%u,%.6f,%.6f,%.1f,%d,%.1f,%.1f,%.1f,%.1f\n",
             *                 s_log_buffer[i].timestamp,
             *                 s_log_buffer[i].lat, s_log_buffer[i].lon,
             *                 s_log_buffer[i].speed_kmh, s_log_buffer[i].rpm,
             *                 s_log_buffer[i].temp_c, s_log_buffer[i].altitude_m,
             *                 s_log_buffer[i].roll_deg, s_log_buffer[i].pitch_deg);
             *         }
             *         fflush(f);
             *         fsync(fileno(f));
             *         fclose(f);
             *     }
             *     xSemaphoreGive(spi_mutex);
             * }
             */

            if (s_log_mutex && xSemaphoreTake(s_log_mutex, pdMS_TO_TICKS(10))) {
                s_log_index = 0;
                xSemaphoreGive(s_log_mutex);
            }
        }

        /* 5Hz Polling */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
