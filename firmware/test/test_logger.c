/*
 * test_logger.c — Unit Tests fuer Telemetry-Logger (CSV + GPX)
 *
 * Testet firmware/components/storage/logger.c:
 *   - CSV-Header Format (alle Spalten korrekt)
 *   - CSV-Row Format (Werte, Komma-Separierung, Zeilenumbruch)
 *   - GPX-Header/Footer/Point Format (XML-Struktur)
 *   - Haversine-Distanz (Track-Point-Filterung < 10m)
 *   - Tour-Statemachine (start/stop/pause/resume)
 *   - Track-Point-Distanzfilterung (Punkte < 10m werden verworfen)
 *   - Logger-Statistik (points_written, points_skipped, tours_*)
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. Die SD-Karten-Abhaengigkeit wird ueber Stubs
 * deaktiviert (sd_card_is_mounted -> false), so dass die Format-
 * funktionen und die Tour-Logik isoliert getestet werden.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "storage.h"
#include "board_config.h"
#include "gps.h"
#include "sensors.h"
#include "event_bus.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

/* ============================================================
 * Host build: mock ESP-IDF dependencies for logger.c
 * ============================================================ */
#ifdef TEST_HOST

static inline void _log_noop(const char *tag, const char *fmt, ...) {
    (void)tag; (void)fmt;
}

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV
#define ESP_LOGE(tag, fmt, ...) _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) _log_noop(tag, fmt, ##__VA_ARGS__)

/* ---- SD card stubs: disable SD-dependent paths ---- */
bool sd_card_is_mounted(void) { return false; }
int sd_card_mkdir(const char *path) { (void)path; return 0; }

/* ---- Shared mutex (declared extern in storage.h) ---- */
SemaphoreHandle_t spi_mutex = NULL;

/* ---- Queues (declared extern in sensors.h / gps.h) ---- */
QueueHandle_t sensor_data_queue = NULL;
QueueHandle_t gps_data_queue = NULL;

/* ---- event_bus stub ---- */
static uint32_t g_events_set = 0;
void event_bus_set(uint32_t bits) { g_events_set |= bits; }
void event_bus_clear(uint32_t bits) { g_events_set &= ~bits; }
bool event_bus_wait(uint32_t bits, uint32_t timeout_ms) {
    (void)timeout_ms; return (g_events_set & bits) != 0;
}

/* Include the real logger.c implementation directly. */
#include "../../components/storage/logger.c"

#endif /* TEST_HOST */

/* ============================================================
 * Helpers
 * ============================================================ */

static log_entry_t make_entry(uint32_t ts, float lat, float lon,
                               float speed, uint16_t rpm)
{
    log_entry_t e;
    memset(&e, 0, sizeof(e));
    e.timestamp_ms = ts;
    e.lat = lat;
    e.lon = lon;
    e.speed_kmh = speed;
    e.rpm = rpm;
    e.temp_c = 85.0f;
    e.altitude_m = 300.0f;
    e.course_deg = 180.0f;
    e.roll_deg = 2.0f;
    e.pitch_deg = -1.0f;
    e.fuel_l = 10.0f;
    return e;
}

void setUp(void)
{
    g_events_set = 0;
    logger_init();
}

void tearDown(void)
{
}

/* ============================================================
 * CSV-Header Format
 * ============================================================ */

void test_csv_header_format(void)
{
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);

    int ret = logger_write_csv_header(f);
    TEST_ASSERT_EQUAL(0, ret);

    /* Read back and verify the header contains all expected columns */
    rewind(f);
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';

    TEST_ASSERT_NOT_NULL(strstr(buf, "timestamp_ms"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "lat"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "lon"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "speed_kmh"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "course_deg"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "altitude_m"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "rpm"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "temp_c"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "roll_deg"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "pitch_deg"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "fuel_l"));

    fclose(f);
}

void test_csv_header_null_file(void)
{
    TEST_ASSERT_EQUAL(-1, logger_write_csv_header(NULL));
}

/* ============================================================
 * CSV-Row Format
 * ============================================================ */

void test_csv_row_format(void)
{
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);

    log_entry_t e = make_entry(12345, 50.935390, 11.587240, 100.5, 5000);
    int ret = logger_write_csv_row(f, &e);
    TEST_ASSERT_EQUAL(0, ret);

    rewind(f);
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';

    /* Verify the row starts with the timestamp */
    TEST_ASSERT_NOT_NULL(strstr(buf, "12345"));
    /* Verify lat/lon are present with 6 decimal places */
    TEST_ASSERT_NOT_NULL(strstr(buf, "50.935390"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "11.587240"));
    /* Verify RPM */
    TEST_ASSERT_NOT_NULL(strstr(buf, "5000"));
    /* Verify the row ends with newline */
    TEST_ASSERT_EQUAL('\n', buf[n - 1]);

    fclose(f);
}

void test_csv_row_null_args(void)
{
    log_entry_t e = make_entry(1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL(-1, logger_write_csv_row(NULL, &e));
    TEST_ASSERT_EQUAL(-1, logger_write_csv_row(stdout, NULL));
}

void test_csv_header_and_rows_sequence(void)
{
    /* Write header + 3 rows, read back and verify structure */
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);

    logger_write_csv_header(f);
    for (int i = 0; i < 3; i++) {
        log_entry_t e = make_entry(1000 + i, 50.0 + i * 0.001, 11.0, 80.0, 4000 + i);
        logger_write_csv_row(f, &e);
    }

    rewind(f);
    char buf[2048];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';

    /* Count newlines: 1 header + 3 rows = 4 */
    int newlines = 0;
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\n') newlines++;
    }
    TEST_ASSERT_EQUAL(4, newlines);

    fclose(f);
}

/* ============================================================
 * GPX Format (static helpers, accessible via .c inclusion)
 * ============================================================ */

void test_gpx_header_format(void)
{
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);

    int ret = write_gpx_header(f);
    TEST_ASSERT_EQUAL(0, ret);

    rewind(f);
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';

    /* Verify GPX 1.1 XML structure */
    TEST_ASSERT_NOT_NULL(strstr(buf, "<?xml version=\"1.0\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "<gpx"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "version=\"1.1\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "xmlns=\"http://www.topografix.com/GPX/1/1\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "<trk>"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "<trkseg>"));

    fclose(f);
}

void test_gpx_footer_format(void)
{
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);

    int ret = write_gpx_footer(f);
    TEST_ASSERT_EQUAL(0, ret);

    rewind(f);
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';

    TEST_ASSERT_NOT_NULL(strstr(buf, "</trkseg>"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "</trk>"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "</gpx>"));

    fclose(f);
}

void test_gpx_point_format(void)
{
    FILE *f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);

    log_entry_t e = make_entry(99999, 50.935390, 11.587240, 0, 0);
    e.altitude_m = 245.3f;

    int ret = write_gpx_point(f, &e);
    TEST_ASSERT_EQUAL(0, ret);

    rewind(f);
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';

    /* Verify trkpt element with lat/lon attributes */
    TEST_ASSERT_NOT_NULL(strstr(buf, "<trkpt"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "lat=\"50.935390\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "lon=\"11.587240\""));
    /* Verify elevation */
    TEST_ASSERT_NOT_NULL(strstr(buf, "<ele>245.3</ele>"));
    /* Verify timestamp */
    TEST_ASSERT_NOT_NULL(strstr(buf, "99999"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "</trkpt>"));

    fclose(f);
}

void test_gpx_point_null_args(void)
{
    TEST_ASSERT_EQUAL(-1, write_gpx_point(NULL, NULL));
    log_entry_t e = make_entry(1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL(-1, write_gpx_point(NULL, &e));
}

/* ============================================================
 * Haversine-Distanz
 * ============================================================ */

void test_haversine_zero_distance(void)
{
    /* Same point -> 0 meters */
    float d = haversine_m(50.0f, 11.0f, 50.0f, 11.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, d);
}

void test_haversine_known_distance(void)
{
    /* 1 degree latitude ~= 111.195 km (6371000 * pi/180) */
    float d = haversine_m(50.0f, 11.0f, 51.0f, 11.0f);
    TEST_ASSERT_FLOAT_WITHIN(500.0f, 111195.0f, d);
}

void test_haversine_small_distance_below_threshold(void)
{
    /* Points ~5m apart (< LOGGER_MIN_DIST_M = 10m) */
    /* 5m / 111195m per degree ~= 0.000045 degrees */
    float d = haversine_m(50.0f, 11.0f, 50.0f, 11.000045f);
    TEST_ASSERT_LESS_THAN(10.0f, d);
}

void test_haversine_above_threshold(void)
{
    /* Points ~100m apart (> 10m threshold) */
    float d = haversine_m(50.0f, 11.0f, 50.0f, 11.001f);
    TEST_ASSERT_GREATER_THAN(10.0f, d);
}

/* ============================================================
 * Tour-Statemachine
 * ============================================================ */

void test_tour_start_sets_active(void)
{
    TEST_ASSERT_EQUAL(0, logger_start_tour());
    const tour_context_t *tc = logger_get_tour_context();
    TEST_ASSERT_EQUAL(TOUR_ACTIVE, tc->state);
    TEST_ASSERT_EQUAL(1, tc->segment_count);
    TEST_ASSERT_EQUAL(0, tc->point_count);
}

void test_tour_start_idempotent(void)
{
    logger_start_tour();
    TEST_ASSERT_EQUAL(0, logger_start_tour());  /* already active */
    const tour_context_t *tc = logger_get_tour_context();
    TEST_ASSERT_EQUAL(TOUR_ACTIVE, tc->state);
}

void test_tour_stop_sets_idle(void)
{
    logger_start_tour();
    TEST_ASSERT_EQUAL(0, logger_stop_tour());
    const tour_context_t *tc = logger_get_tour_context();
    TEST_ASSERT_EQUAL(TOUR_IDLE, tc->state);
}

void test_tour_pause_resume(void)
{
    logger_start_tour();
    TEST_ASSERT_EQUAL(0, logger_pause_tour());
    const tour_context_t *tc = logger_get_tour_context();
    TEST_ASSERT_EQUAL(TOUR_PAUSED, tc->state);

    TEST_ASSERT_EQUAL(0, logger_resume_tour());
    tc = logger_get_tour_context();
    TEST_ASSERT_EQUAL(TOUR_ACTIVE, tc->state);
    TEST_ASSERT_EQUAL(2, tc->segment_count);  /* resumed increments segment */
}

void test_tour_stop_when_idle(void)
{
    /* Stopping when already idle should be a no-op (return 0) */
    TEST_ASSERT_EQUAL(0, logger_stop_tour());
}

/* ============================================================
 * Track-Point-Distanzfilterung
 * ============================================================ */

void test_add_track_point_when_idle_ignored(void)
{
    /* Without starting a tour, points are not added */
    log_entry_t e = make_entry(1000, 50.0, 11.0, 80, 4000);
    TEST_ASSERT_EQUAL(0, logger_add_track_point(&e));
    TEST_ASSERT_EQUAL_UINT32(0, logger_get_stats()->total_points);
}

void test_add_track_point_accepted_when_active(void)
{
    logger_start_tour();
    log_entry_t e = make_entry(1000, 50.0, 11.0, 80, 4000);
    TEST_ASSERT_EQUAL(0, logger_add_track_point(&e));
    TEST_ASSERT_EQUAL_UINT32(1, logger_get_stats()->total_points);
    TEST_ASSERT_EQUAL_UINT32(1, logger_get_tour_context()->point_count);
}

void test_add_track_point_skipped_below_min_distance(void)
{
    logger_start_tour();

    /* First point always accepted (no last point) */
    log_entry_t e1 = make_entry(1000, 50.0, 11.0, 80, 4000);
    logger_add_track_point(&e1);

    /* Second point < 10m away -> skipped */
    log_entry_t e2 = make_entry(2000, 50.0, 11.00001f, 80, 4000);
    logger_add_track_point(&e2);

    TEST_ASSERT_EQUAL_UINT32(1, logger_get_stats()->total_points);
    TEST_ASSERT_EQUAL_UINT32(1, logger_get_stats()->points_skipped);
}

void test_add_track_point_accepted_above_min_distance(void)
{
    logger_start_tour();

    log_entry_t e1 = make_entry(1000, 50.0, 11.0, 80, 4000);
    logger_add_track_point(&e1);

    /* Second point > 10m away -> accepted */
    log_entry_t e2 = make_entry(2000, 50.0, 11.001f, 90, 4500);
    logger_add_track_point(&e2);

    TEST_ASSERT_EQUAL_UINT32(2, logger_get_stats()->total_points);
    TEST_ASSERT_EQUAL_UINT32(0, logger_get_stats()->points_skipped);
}

void test_add_track_point_null_rejected(void)
{
    logger_start_tour();
    TEST_ASSERT_EQUAL(-1, logger_add_track_point(NULL));
}

/* ============================================================
 * Logger-Statistik
 * ============================================================ */

void test_stats_initial_state(void)
{
    const logger_stats_t *s = logger_get_stats();
    TEST_ASSERT_EQUAL_UINT32(0, s->total_points);
    TEST_ASSERT_EQUAL_UINT32(0, s->points_skipped);
    TEST_ASSERT_EQUAL_UINT32(0, s->points_written);
    TEST_ASSERT_EQUAL_UINT32(0, s->tours_started);
}

void test_stats_tours_started_incremented(void)
{
    logger_start_tour();
    TEST_ASSERT_EQUAL_UINT32(1, logger_get_stats()->tours_started);
    logger_stop_tour();
    TEST_ASSERT_EQUAL_UINT32(1, logger_get_stats()->tours_stopped);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_header_format);
    RUN_TEST(test_csv_header_null_file);
    RUN_TEST(test_csv_row_format);
    RUN_TEST(test_csv_row_null_args);
    RUN_TEST(test_csv_header_and_rows_sequence);

    RUN_TEST(test_gpx_header_format);
    RUN_TEST(test_gpx_footer_format);
    RUN_TEST(test_gpx_point_format);
    RUN_TEST(test_gpx_point_null_args);

    RUN_TEST(test_haversine_zero_distance);
    RUN_TEST(test_haversine_known_distance);
    RUN_TEST(test_haversine_small_distance_below_threshold);
    RUN_TEST(test_haversine_above_threshold);

    RUN_TEST(test_tour_start_sets_active);
    RUN_TEST(test_tour_start_idempotent);
    RUN_TEST(test_tour_stop_sets_idle);
    RUN_TEST(test_tour_pause_resume);
    RUN_TEST(test_tour_stop_when_idle);

    RUN_TEST(test_add_track_point_when_idle_ignored);
    RUN_TEST(test_add_track_point_accepted_when_active);
    RUN_TEST(test_add_track_point_skipped_below_min_distance);
    RUN_TEST(test_add_track_point_accepted_above_min_distance);
    RUN_TEST(test_add_track_point_null_rejected);

    RUN_TEST(test_stats_initial_state);
    RUN_TEST(test_stats_tours_started_incremented);

    return UNITY_END();
}
