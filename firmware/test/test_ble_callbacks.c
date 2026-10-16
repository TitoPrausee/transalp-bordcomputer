/*
 * test_ble_callbacks.c — Unit Tests fuer BLE GATT Handler
 *
 * Testet firmware/components/ble/ble_callbacks.c:
 *   - Telemetry-Live Packet (22 bytes packed, Little-Endian)
 *   - Sensor-Status (5 bytes packed)
 *   - DTC Binary Packing (6 bytes pro Eintrag)
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

#include <string.h>
#include <stdint.h>

/* ============================================================
 * Mock ESP-IDF dependencies for host build
 * ============================================================ */
#ifdef TEST_HOST

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0
#define ESP_LOGD(tag, fmt, ...)  (void)0

/* Mock esp_timer.h */
static inline int64_t esp_timer_get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)(ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);
}

/* Mock esp_system.h */
static inline void esp_restart(void) { /* no-op in tests */ }

/* Mock nvs */
typedef int nvs_handle_t;
static inline esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *h) {
    (void)ns; (void)mode; *h = 1; return ESP_FAIL;
}
static inline void nvs_close(nvs_handle_t h) { (void)h; }

/* Mock ble_hs.h types and functions */
struct os_mbuf;
struct ble_gatt_access_ctxt;
#define BLE_GATT_ACCESS_OP_READ_CHR  1
#define BLE_GATT_ACCESS_OP_WRITE_CHR  2
#define BLE_ATT_ERR_UNLIKELY          0x0E
#define BLE_ATT_ERR_INSUFFICIENT_RES  0x0D
#define BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN 0x0D
#define BLE_HS_CONN_HANDLE_NONE       0xFFFF
#define OS_MBUF_PKTLEN(om)            (0)
#define BLE_UUID_TYPE_128             128

static inline struct os_mbuf *ble_hs_mbuf_from_flat(const void *data, int len) {
    (void)data; (void)len; return NULL;
}
static inline int ble_gattc_notify(uint16_t conn, uint16_t handle, struct os_mbuf *om) {
    (void)conn; (void)handle; (void)om; return 0;
}
static inline int ble_gattc_indicate(uint16_t conn, uint16_t handle, struct os_mbuf *om) {
    (void)conn; (void)handle; (void)om; return 0;
}
static inline int ble_hs_mbuf_to_flat(struct os_mbuf *om, uint8_t *buf, uint16_t max, uint16_t *len) {
    (void)om; (void)buf; (void)max; *len = 0; return 0;
}
static inline int os_mbuf_append(struct os_mbuf *om, const void *data, int len) {
    (void)om; (void)data; (void)len; return 0;
}

/* Mock power.h functions referenced by ble_callbacks.c */
static float s_mock_voltage = 12.6f;
float power_get_battery_voltage(void) { return s_mock_voltage; }
uint8_t power_get_battery_percent(void) { return 80; }

/* Mock diagnostics.h */
void dtc_init(void) {}
void dtc_report(int code) { (void)code; }
void dtc_clear(int code) { (void)code; }
void dtc_clear_all(void) {}
uint8_t dtc_get_active(void *out, int max) { (void)out; (void)max; return 0; }

/* ble_service.c stubs */
uint16_t ble_val_handles[CHR_H_COUNT] = {0};
bool ble_cccd_telemetry_enabled(void) { return false; }
bool ble_cccd_gps_enabled(void) { return false; }
bool ble_cccd_dtc_enabled(void) { return false; }
bool ble_cccd_sensor_status_enabled(void) { return false; }
uint16_t ble_get_conn_handle(void) { return 0xFFFF; }
void ble_store_clear(void) {}

/* Mock selftest */
bool selftest_run(void) { return true; }

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
    TEST_ASSERT_EQUAL(6, sizeof(dtc_binary_t));
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

    return UNITY_END();
}
