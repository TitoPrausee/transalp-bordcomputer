/*
 * test_ble_callbacks.c — Unit Tests fuer BLE GATT Handler
 *
 * Testet firmware/components/ble/ble_callbacks.c:
 *   - Telemetry-Live Packet (22 bytes packed, Little-Endian)
 *   - Sensor-Status (5 bytes packed)
 *   - DTC Binary Packing (7 bytes pro Eintrag)
 *   - BLE Command Queue (enqueue)
 *   - Static asserts fuer struct sizes
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "ble.h"
#include "ble_internal.h"
#include "gatt_uuids.h"
#include "diagnostics.h"
#include "ota_update.h"

#include <string.h>
#include <stdint.h>

/* ============================================================
 * Mock ESP-IDF dependencies for host build
 * ============================================================ */
#ifdef TEST_HOST

/* Host-side logging no-op. Routing the format args through a helper
 * (rather than (void)0) keeps variables that are only referenced in log
 * calls marked as used, avoiding -Wunused-variable noise that the
 * mocked-out logging would otherwise produce in the source under test. */
static inline void _log_noop(const char *tag, const char *fmt, ...) {
    (void)tag; (void)fmt;
}

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#define ESP_LOGE(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)

/* esp_timer_get_time, vTaskDelayUntil and the FreeRTOS primitives come
 * from freertos_mock.h (via esp_idf_mock.h). esp_restart() and the
 * heap helpers come from the shared esp_system.h mock, which references
 * g_restart_called — define it here so the test links. NimBLE types
 * (host/ble_hs.h) and services/gap/ble_svc_gap.h are provided as shared
 * mocks under test/mocks. nvs_flash.h / nvs.h resolve to the shared
 * mocks (ble_callbacks.c includes them but does not call any NVS API). */
int g_restart_called = 0;

/* Mock power.h functions referenced by ble_callbacks.c */
static float s_mock_voltage = 12.6f;
float power_get_battery_voltage(void) { return s_mock_voltage; }
uint8_t power_get_battery_percent(void) { return 80; }

/* Mock diagnostics.h: ble_callbacks.c calls dtc_get_active(); the other
 * dtc_* helpers are declared in diagnostics.h but unused here, so they
 * are left undefined. The signature must match diagnostics.h exactly. */
uint8_t dtc_get_active(dtc_entry_t *out, int max_count)
{
    (void)out; (void)max_count;
    return 0;
}

/* Mock selftest (declared in diagnostics.h) */
bool selftest_run(void) { return true; }

/* ble_service.c stubs */
uint16_t ble_val_handles[CHR_H_COUNT] = {0};
bool g_ble_cccd_telemetry = false;
bool g_ble_cccd_dtc = false;
bool ble_cccd_telemetry_enabled(void) { return g_ble_cccd_telemetry; }
bool ble_cccd_gps_enabled(void) { return false; }
bool ble_cccd_dtc_enabled(void) { return g_ble_cccd_dtc; }
bool ble_cccd_sensor_status_enabled(void) { return false; }

/* Controllable conn-handle stub.  Default BLE_HS_CONN_HANDLE_NONE so
 * existing tests that don't care about the notify path are unaffected.
 * Tests that exercise the notify validation guard set g_ble_conn_handle
 * to a valid handle (e.g. 1). */
uint16_t g_ble_conn_handle = 0xFFFF;
uint16_t ble_get_conn_handle(void) { return g_ble_conn_handle; }

/* ble_conn_is_valid stub — controllable by tests via g_ble_conn_valid.
 * Default true so existing notify tests pass unchanged.  Tests that
 * exercise the conn-handle reuse guard set g_ble_conn_valid = false. */
bool g_ble_conn_valid = true;
bool ble_conn_is_valid(uint16_t conn_handle) {
    (void)conn_handle;
    return g_ble_conn_valid;
}
void ble_store_clear(void) {}

/* OTA + event_bus stubs: ble_callbacks.c calls these but the test does
 * not exercise OTA or the event bus, so they are no-ops returning the
 * "idle / success" value. Signatures match ota_update.h / event_bus.h. */
ota_state_t ota_get_state(void) { return OTA_STATE_IDLE; }
ota_error_t ota_get_error(void) { return OTA_ERR_NONE; }
uint8_t ota_get_progress(void) { return 0; }
esp_err_t ota_update_from_ble(uint32_t total_size) { (void)total_size; return ESP_OK; }
esp_err_t ota_ble_write_chunk(const ota_chunk_t *chunk) { (void)chunk; return ESP_OK; }
esp_err_t ota_ble_commit(const uint8_t *expected_hash) { (void)expected_hash; return ESP_OK; }
esp_err_t ota_ble_get_verify_result(ota_verify_result_t *out) { (void)out; return ESP_OK; }
esp_err_t ota_abort(void) { return ESP_OK; }
bool event_bus_wait(uint32_t bits, uint32_t timeout_ms) { (void)bits; (void)timeout_ms; return false; }

#endif /* TEST_HOST */

/* ============================================================
 * Include the actual ble_callbacks.c implementation directly.
 * We include it to test the internal helper functions.
 * ============================================================ */
#ifdef TEST_HOST
#include "../../components/ble/ble_callbacks.c"
#endif

void setUp(void)
{
    /* Create the data mutex and command queue for each test */
    if (s_data_mutex == NULL) {
        s_data_mutex = xSemaphoreCreateMutex();
    }
    if (ble_cmd_queue == NULL) {
        ble_cmd_queue = xQueueCreate(8, sizeof(ble_cmd_t));
    }
    memset(&s_sensor_cache, 0, sizeof(s_sensor_cache));
    memset(&s_gps_cache, 0, sizeof(s_gps_cache));

    /* Reset mock state for conn-handle validation tests (H4) */
    g_ble_conn_handle = 0xFFFF;
    g_ble_conn_valid = true;
    g_ble_cccd_telemetry = false;
    g_ble_cccd_dtc = false;
    g_mock_mbuf_alloc_ok = false;
    g_mock_notify_call_count = 0;
    g_mock_indicate_call_count = 0;
}

void tearDown(void)
{
}

/* ============================================================
 * Test: telemetry_live_t is exactly 22 bytes (packed)
 * ============================================================ */

void test_telemetry_live_packed_size(void)
{
    TEST_ASSERT_EQUAL(22, sizeof(telemetry_live_t));
}

/* ============================================================
 * Test: sensor_status_t is exactly 5 bytes (packed)
 * ============================================================ */

void test_sensor_status_packed_size(void)
{
    TEST_ASSERT_EQUAL(5, sizeof(sensor_status_t));
}

/* ============================================================
 * Test: dtc_binary_t is exactly 6 bytes (packed)
 * ============================================================ */

void test_dtc_binary_packed_size(void)
{
    TEST_ASSERT_EQUAL(7, sizeof(dtc_binary_t));
}

/* ============================================================
 * Test: telemetry_live field offsets
 *
 * Verify the byte layout matches the spec:
 *   speed=0, rpm=1-2, gear=3, coolant=4, ambient=5,
 *   fuel=6, voltage=7, lat=8-11, lon=12-15,
 *   altitude=16-17, heading=18-19, sats=20, hdop=21
 * ============================================================ */

void test_telemetry_live_layout(void)
{
    telemetry_live_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    /* Set distinctive values */
    pkt.speed_kmh = 123;
    pkt.rpm = 4567;
    pkt.gear = 3;
    pkt.coolant_temp_c = 85;
    pkt.ambient_temp_c = 20;
    pkt.fuel_percent = 75;
    pkt.voltage_tenth = 126;
    pkt.latitude_e7 = 509353900;
    pkt.longitude_e7 = 115872400;
    pkt.altitude_m = 412;
    pkt.heading_tenth = 2700;
    pkt.satellites = 9;
    pkt.hdop_tenth = 12;

    /* Verify via byte-level access (Little-Endian) */
    uint8_t *raw = (uint8_t *)&pkt;
    TEST_ASSERT_EQUAL_UINT8(123, raw[0]);    /* speed */
    TEST_ASSERT_EQUAL_UINT8(3, raw[3]);      /* gear */
    TEST_ASSERT_EQUAL_UINT8(85, raw[4]);     /* coolant */
    TEST_ASSERT_EQUAL_UINT8(20, raw[5]);     /* ambient */
    TEST_ASSERT_EQUAL_UINT8(75, raw[6]);     /* fuel */
    TEST_ASSERT_EQUAL_UINT8(126, raw[7]);    /* voltage */
    TEST_ASSERT_EQUAL_UINT8(9, raw[20]);     /* satellites */
    TEST_ASSERT_EQUAL_UINT8(12, raw[21]);    /* hdop */

    /* RPM is uint16 at offset 1 (Little-Endian) */
    uint16_t rpm_le = raw[1] | (raw[2] << 8);
    TEST_ASSERT_EQUAL_UINT16(4567, rpm_le);
}

/* ============================================================
 * Test: build_telemetry_live from caches
 * ============================================================ */

void test_build_telemetry_live_from_caches(void)
{
    /* Populate sensor cache */
    s_sensor_cache.speed_kmh = 100.4f;
    s_sensor_cache.rpm = 5000;
    s_sensor_cache.gear = 4;
    s_sensor_cache.coolant_temp_c = 90;
    s_sensor_cache.temp_c = 25;
    s_sensor_cache.fuel_level_l = 8;

    /* Populate GPS cache */
    s_gps_cache.valid = true;
    s_gps_cache.lat = 50.93539f;
    s_gps_cache.lon = 11.58724f;
    s_gps_cache.altitude_m = 300;
    s_gps_cache.course_deg = 270;
    s_gps_cache.satellites = 8;
    s_gps_cache.hdop = 1.2f;

    telemetry_live_t pkt;
    build_telemetry_live(&pkt);

    TEST_ASSERT_EQUAL_UINT8(100, pkt.speed_kmh);  /* 100.4 + 0.5 -> 100 */
    TEST_ASSERT_EQUAL_UINT16(5000, pkt.rpm);
    TEST_ASSERT_EQUAL_INT8(4, pkt.gear);
    TEST_ASSERT_EQUAL_INT8(90, pkt.coolant_temp_c);
    TEST_ASSERT_EQUAL_INT8(25, pkt.ambient_temp_c);
    TEST_ASSERT_EQUAL_UINT8(8, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(8, pkt.satellites);
}

/* ============================================================
 * Test: build_telemetry_live with no GPS fix
 * ============================================================ */

void test_build_telemetry_live_no_gps(void)
{
    memset(&s_sensor_cache, 0, sizeof(s_sensor_cache));
    memset(&s_gps_cache, 0, sizeof(s_gps_cache));
    s_gps_cache.valid = false;

    telemetry_live_t pkt;
    build_telemetry_live(&pkt);

    /* GPS fields should be zero when no fix */
    TEST_ASSERT_EQUAL_INT32(0, pkt.latitude_e7);
    TEST_ASSERT_EQUAL_INT32(0, pkt.longitude_e7);
    TEST_ASSERT_EQUAL_INT16(0, pkt.altitude_m);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.heading_tenth);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.satellites);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.hdop_tenth);
}

/* ============================================================
 * Test: build_sensor_status
 * ============================================================ */

void test_build_sensor_status_ok(void)
{
    memset(&s_sensor_cache, 0, sizeof(s_sensor_cache));
    s_sensor_cache.temp_c = 80.0f;
    s_gps_cache.valid = true;

    sensor_status_t st;
    build_sensor_status(&st);

    TEST_ASSERT_EQUAL_UINT8(0, st.gps);   /* ok */
    TEST_ASSERT_EQUAL_UINT8(0, st.rpm);   /* ok */
    TEST_ASSERT_EQUAL_UINT8(0, st.temp);  /* ok */
    TEST_ASSERT_EQUAL_UINT8(0, st.fuel);  /* ok */
}

void test_build_sensor_status_temp_error(void)
{
    memset(&s_sensor_cache, 0, sizeof(s_sensor_cache));
    s_sensor_cache.temp_c = 200.0f;  /* out of range */

    sensor_status_t st;
    build_sensor_status(&st);

    TEST_ASSERT_EQUAL_UINT8(2, st.temp);  /* error */
}

void test_build_sensor_status_temp_cold_error(void)
{
    memset(&s_sensor_cache, 0, sizeof(s_sensor_cache));
    s_sensor_cache.temp_c = -50.0f;  /* below -40 */

    sensor_status_t st;
    build_sensor_status(&st);

    TEST_ASSERT_EQUAL_UINT8(2, st.temp);  /* error */
}

/* ============================================================
 * Test: ble_cmd_enqueue
 * ============================================================ */

void test_ble_cmd_enqueue_valid(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    bool result = ble_cmd_enqueue(BLE_CMD_NAV_CMD, 1, data, 3);
    TEST_ASSERT_TRUE(result);

    /* Verify the command was enqueued */
    ble_cmd_t cmd;
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(ble_cmd_queue, &cmd, 0));
    TEST_ASSERT_EQUAL(BLE_CMD_NAV_CMD, cmd.type);
    TEST_ASSERT_EQUAL_UINT16(1, cmd.conn_handle);
    TEST_ASSERT_EQUAL_UINT16(3, cmd.len);
    TEST_ASSERT_EQUAL_UINT8(0x01, cmd.data[0]);
}

void test_ble_cmd_enqueue_too_long(void)
{
    uint8_t data[300] = {0};
    bool result = ble_cmd_enqueue(BLE_CMD_OTA_DATA, 1, data, 300);
    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: ble_on_sensor_update copies data
 * ============================================================ */

void test_ble_on_sensor_update_copies_data(void)
{
    sensor_data_t sd;
    memset(&sd, 0, sizeof(sd));
    sd.speed_kmh = 120;
    sd.rpm = 6500;
    sd.coolant_temp_c = 95;

    ble_on_sensor_update(&sd, sizeof(sd));

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 120, s_sensor_cache.speed_kmh);
    TEST_ASSERT_EQUAL_UINT16(6500, s_sensor_cache.rpm);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 95, s_sensor_cache.coolant_temp_c);
}

void test_ble_on_sensor_update_null_rejected(void)
{
    ble_on_sensor_update(NULL, sizeof(sensor_data_t));
    /* Should not crash, cache unchanged */
    TEST_ASSERT_EQUAL_UINT16(0, s_sensor_cache.rpm);
}

/* ============================================================
 * Test: ble_on_gps_update copies data
 * ============================================================ */

void test_ble_on_gps_update_copies_data(void)
{
    gps_data_t gd;
    memset(&gd, 0, sizeof(gd));
    gd.lat = 50.9f;
    gd.lon = 11.5f;
    gd.satellites = 7;

    ble_on_gps_update(&gd, sizeof(gd));

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.9f, s_gps_cache.lat);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 11.5f, s_gps_cache.lon);
    TEST_ASSERT_EQUAL_UINT8(7, s_gps_cache.satellites);
}

/* ============================================================
 * Tests: conn-handle validation guard (H4 — conn-handle reuse race)
 *
 * ble_notify() and ble_indicate() must validate the connection handle
 * via ble_conn_is_valid() before sending.  If the companion-app link
 * dropped and NimBLE reused the handle for the CarPlay central link,
 * the validation must return false and the notify must be skipped.
 * ============================================================ */

void test_notify_skipped_when_conn_handle_none(void)
{
    g_ble_conn_handle = 0xFFFF;  /* BLE_HS_CONN_HANDLE_NONE */
    g_ble_cccd_telemetry = true;
    g_mock_mbuf_alloc_ok = true;

    ble_notify_telemetry_live();

    /* No notify should be sent when there is no connection */
    TEST_ASSERT_EQUAL(0, g_mock_notify_call_count);
}

void test_notify_sent_when_conn_valid(void)
{
    g_ble_conn_handle = 1;        /* valid handle */
    g_ble_conn_valid = true;      /* validation passes */
    g_ble_cccd_telemetry = true;
    g_mock_mbuf_alloc_ok = true;  /* mbuf alloc succeeds */

    ble_notify_telemetry_live();

    /* Notify should be sent when connection is valid */
    TEST_ASSERT_EQUAL(1, g_mock_notify_call_count);
}

void test_notify_skipped_when_conn_handle_reused(void)
{
    g_ble_conn_handle = 1;        /* handle is "valid" per stub */
    g_ble_conn_valid = false;     /* but validation fails (handle reused) */
    g_ble_cccd_telemetry = true;
    g_mock_mbuf_alloc_ok = true;  /* mbuf alloc would succeed */

    ble_notify_telemetry_live();

    /* Notify must NOT be sent — conn-handle reuse guard (H4) */
    TEST_ASSERT_EQUAL(0, g_mock_notify_call_count);
}

void test_indicate_skipped_when_conn_handle_reused(void)
{
    g_ble_conn_handle = 1;
    g_ble_conn_valid = false;
    g_mock_mbuf_alloc_ok = true;

    /* Call ble_indicate directly (static, visible because the test
     * includes ble_callbacks.c).  ble_notify_dtc_list would early-return
     * because dtc_get_active returns 0 in the mock. */
    uint8_t dummy = 0;
    ble_indicate(ble_val_handles[CHR_H_DTC_LIST], &dummy, 1);

    /* Indicate must NOT be sent — conn-handle reuse guard (H4) */
    TEST_ASSERT_EQUAL(0, g_mock_indicate_call_count);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_telemetry_live_packed_size);
    RUN_TEST(test_sensor_status_packed_size);
    RUN_TEST(test_dtc_binary_packed_size);

    RUN_TEST(test_telemetry_live_layout);

    RUN_TEST(test_build_telemetry_live_from_caches);
    RUN_TEST(test_build_telemetry_live_no_gps);

    RUN_TEST(test_build_sensor_status_ok);
    RUN_TEST(test_build_sensor_status_temp_error);
    RUN_TEST(test_build_sensor_status_temp_cold_error);

    RUN_TEST(test_ble_cmd_enqueue_valid);
    RUN_TEST(test_ble_cmd_enqueue_too_long);

    RUN_TEST(test_ble_on_sensor_update_copies_data);
    RUN_TEST(test_ble_on_sensor_update_null_rejected);

    RUN_TEST(test_ble_on_gps_update_copies_data);

    RUN_TEST(test_notify_skipped_when_conn_handle_none);
    RUN_TEST(test_notify_sent_when_conn_valid);
    RUN_TEST(test_notify_skipped_when_conn_handle_reused);
    RUN_TEST(test_indicate_skipped_when_conn_handle_reused);

    return UNITY_END();
}
