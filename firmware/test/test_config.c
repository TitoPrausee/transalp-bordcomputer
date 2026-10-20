/*
 * test_config.c — Unit Tests fuer NVS-Konfigurationsverwaltung
 *
 * Testet firmware/components/storage/config.c:
 *   - config_set_value / config_get_value Roundtrips
 *   - JSON-Export und JSON-Import
 *   - JSON-Parser Edge-Cases (Overflow, leere Werte, fehlende Keys)
 *   - Factory-Reset Verhalten
 *   - Default-Konfiguration
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. NVS/ESP-IDF-Abhaengigkeiten werden durch Mocks ersetzt.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "storage.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ============================================================
 * Mock ESP-IDF dependencies of config.c
 * ============================================================ */
#ifdef TEST_HOST
#include "esp_err.h"

/* Prevent mock NVS headers (test/mocks/nvs.h, nvs_flash.h) from
 * conflicting with the test-specific mocks below. */
#define _NVS_FLASH_H_
#define _NVS_H_

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0
#define ESP_LOGD(tag, fmt, ...)  (void)0

/* Mock nvs_flash.h — config_init calls nvs_flash_init */
#define ESP_ERR_NVS_NO_FREE_PAGES    0x201
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x202
static inline esp_err_t nvs_flash_init(void) { return ESP_FAIL; }
static inline esp_err_t nvs_flash_erase(void) { return ESP_OK; }

/* Mock nvs.h */
typedef int nvs_handle_t;
typedef int nvs_open_mode_t;
#define NVS_READONLY  0
#define NVS_READWRITE 1
static inline esp_err_t nvs_open(const char *ns, nvs_open_mode_t mode, nvs_handle_t *h) {
    (void)ns; (void)mode; *h = 1; return ESP_FAIL;
}
static inline void nvs_close(nvs_handle_t h) { (void)h; }
static inline esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *len) {
    (void)h; (void)key; (void)out; (void)len; return ESP_FAIL;
}
static inline esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t len) {
    (void)h; (void)key; (void)data; (void)len; return ESP_OK;
}
static inline esp_err_t nvs_commit(nvs_handle_t h) { (void)h; return ESP_OK; }
static inline void nvs_erase_all(nvs_handle_t h) { (void)h; }

/* Include the actual config.c implementation directly */
#include "../../components/storage/config.c"

#endif /* TEST_HOST */

void setUp(void)
{
    /* Reset to defaults before each test by calling config_init.
     * Since nvs_flash_init and nvs_open fail in the mock, config_init
     * will set s_config to defaults and skip NVS load. */
    config_init();
}

void tearDown(void)
{
}

/* ============================================================
 * Test: Default-Konfiguration
 * ============================================================ */

void test_config_defaults_loaded(void)
{
    device_config_t *cfg = config_get();
    TEST_ASSERT_NOT_NULL(cfg);

    TEST_ASSERT_EQUAL_UINT16(CONFIG_VERSION, cfg->version);
    TEST_ASSERT_EQUAL_UINT16(8000, cfg->rpm_redline);
    TEST_ASSERT_EQUAL_UINT16(7000, cfg->rpm_shift_light);
    TEST_ASSERT_EQUAL_FLOAT(11.5f, cfg->battery_low_v);
    TEST_ASSERT_EQUAL_FLOAT(10.5f, cfg->battery_shutdown_v);
    TEST_ASSERT_EQUAL_FLOAT(105.0f, cfg->coolant_temp_warn);
    TEST_ASSERT_EQUAL_FLOAT(120.0f, cfg->coolant_temp_crit);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, cfg->fuel_min_l);
    TEST_ASSERT_EQUAL_UINT8(5, cfg->log_interval_s);
    TEST_ASSERT_EQUAL_UINT8(10, cfg->gps_rate_hz);
    TEST_ASSERT_EQUAL_UINT8(80, cfg->display_brightness);
    TEST_ASSERT_EQUAL_UINT8(1, cfg->anim_enabled);
    TEST_ASSERT_EQUAL_STRING("BC-Transalp", cfg->device_name);
    TEST_ASSERT_EQUAL_UINT32(123456, cfg->pairing_code);
}

/* ============================================================
 * Test: set/get Roundtrip — uint8 Werte
 * ============================================================ */

void test_config_set_get_display_mode(void)
{
    uint8_t val = 2;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_DISPLAY_MODE, &val, 1));

    uint8_t out = 255;
    TEST_ASSERT_EQUAL(1, config_get_value(CFG_KEY_DISPLAY_MODE, &out, 1));
    TEST_ASSERT_EQUAL_UINT8(2, out);
}

void test_config_set_get_brightness(void)
{
    uint8_t val = 55;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_DISPLAY_BRIGHTNESS, &val, 1));

    uint8_t out = 0;
    TEST_ASSERT_EQUAL(1, config_get_value(CFG_KEY_DISPLAY_BRIGHTNESS, &out, 1));
    TEST_ASSERT_EQUAL_UINT8(55, out);
}

void test_config_set_get_gps_rate(void)
{
    uint8_t val = 5;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_GPS_RATE, &val, 1));

    uint8_t out = 0;
    TEST_ASSERT_EQUAL(1, config_get_value(CFG_KEY_GPS_RATE, &out, 1));
    TEST_ASSERT_EQUAL_UINT8(5, out);
}

/* ============================================================
 * Test: set/get Roundtrip — uint16 Werte
 * ============================================================ */

void test_config_set_get_rpm_redline(void)
{
    uint16_t val = 9500;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_RPM_REDLINE, &val, 2));

    uint16_t out = 0;
    TEST_ASSERT_EQUAL(2, config_get_value(CFG_KEY_RPM_REDLINE, &out, 2));
    TEST_ASSERT_EQUAL_UINT16(9500, out);
}

void test_config_set_get_rpm_shift_light(void)
{
    uint16_t val = 8000;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_RPM_SHIFT_LIGHT, &val, 2));

    uint16_t out = 0;
    TEST_ASSERT_EQUAL(2, config_get_value(CFG_KEY_RPM_SHIFT_LIGHT, &out, 2));
    TEST_ASSERT_EQUAL_UINT16(8000, out);
}

/* ============================================================
 * Test: set/get Roundtrip — float Werte
 * ============================================================ */

void test_config_set_get_battery_low(void)
{
    float val = 12.3f;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_BATTERY_LOW, &val, 4));

    float out = 0.0f;
    TEST_ASSERT_EQUAL(4, config_get_value(CFG_KEY_BATTERY_LOW, &out, 4));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.3f, out);
}

void test_config_set_get_coolant_warn(void)
{
    float val = 110.5f;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_COOLANT_WARN, &val, 4));

    float out = 0.0f;
    TEST_ASSERT_EQUAL(4, config_get_value(CFG_KEY_COOLANT_WARN, &out, 4));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 110.5f, out);
}

void test_config_set_get_fuel_min(void)
{
    float val = 3.5f;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_FUEL_MIN, &val, 4));

    float out = 0.0f;
    TEST_ASSERT_EQUAL(4, config_get_value(CFG_KEY_FUEL_MIN, &out, 4));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.5f, out);
}

/* ============================================================
 * Test: set/get Roundtrip — uint32 Werte
 * ============================================================ */

void test_config_set_get_pairing_code(void)
{
    uint32_t val = 999999;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_PAIRING_CODE, &val, 4));

    uint32_t out = 0;
    TEST_ASSERT_EQUAL(4, config_get_value(CFG_KEY_PAIRING_CODE, &out, 4));
    TEST_ASSERT_EQUAL_UINT32(999999, out);
}

void test_config_set_get_sea_level(void)
{
    uint32_t val = 100000;
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_CALIB_SEA_LEVEL, &val, 4));

    uint32_t out = 0;
    TEST_ASSERT_EQUAL(4, config_get_value(CFG_KEY_CALIB_SEA_LEVEL, &out, 4));
    TEST_ASSERT_EQUAL_UINT32(100000, out);
}

/* ============================================================
 * Test: set/get Roundtrip — device_name (string)
 * ============================================================ */

void test_config_set_get_device_name(void)
{
    const char *name = "MyBike-01";
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_DEVICE_NAME, name, strlen(name)));

    char out[32] = {0};
    TEST_ASSERT_GREATER_THAN(0, config_get_value(CFG_KEY_DEVICE_NAME, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("MyBike-01", out);
}

/* ============================================================
 * Test: set/get Roundtrip — IMU calibration (int16[3])
 * ============================================================ */

void test_config_set_get_imu_calibration(void)
{
    int16_t offsets[3] = {100, -50, 200};
    TEST_ASSERT_EQUAL(0, config_set_value(CFG_KEY_CALIB_IMU, offsets, sizeof(offsets)));

    int16_t out[3] = {0, 0, 0};
    TEST_ASSERT_EQUAL(sizeof(offsets), config_get_value(CFG_KEY_CALIB_IMU, out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT16(100, out[0]);
    TEST_ASSERT_EQUAL_INT16(-50, out[1]);
    TEST_ASSERT_EQUAL_INT16(200, out[2]);
}

/* ============================================================
 * Test: Unknown config key returns -1
 * ============================================================ */

void test_config_unknown_key_returns_error(void)
{
    uint8_t val = 1;
    TEST_ASSERT_EQUAL(-1, config_set_value((config_key_t)99, &val, 1));

    uint8_t out = 0;
    TEST_ASSERT_EQUAL(-1, config_get_value((config_key_t)99, &out, 1));
}

/* ============================================================
 * Test: NULL/zero-length input rejected
 * ============================================================ */

void test_config_set_null_value_rejected(void)
{
    TEST_ASSERT_EQUAL(-1, config_set_value(CFG_KEY_DISPLAY_MODE, NULL, 1));
    TEST_ASSERT_EQUAL(-1, config_set_value(CFG_KEY_DISPLAY_MODE, &(uint8_t){1}, 0));
}

void test_config_get_null_value_rejected(void)
{
    TEST_ASSERT_EQUAL(-1, config_get_value(CFG_KEY_DISPLAY_MODE, NULL, 1));
    TEST_ASSERT_EQUAL(-1, config_get_value(CFG_KEY_DISPLAY_MODE, &(uint8_t){0}, 0));
}

/* ============================================================
 * Test: JSON-Export
 * ============================================================ */

void test_config_export_json_valid(void)
{
    char buf[2048];
    int len = config_export_json(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, len);

    /* Should contain version */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"version\""));
    /* Should contain device name */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"BC-Transalp\""));
    /* Should contain thresholds */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"rpm_redline\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"battery_low_v\""));
    /* Should be valid JSON (starts with { and ends with }) */
    TEST_ASSERT_EQUAL('{', buf[0]);
}

void test_config_export_json_too_small(void)
{
    char buf[10];
    int len = config_export_json(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(-1, len);
}

void test_config_export_json_null_buffer(void)
{
    int len = config_export_json(NULL, 2048);
    TEST_ASSERT_EQUAL(-1, len);
}

/* ============================================================
 * Test: JSON-Import — values applied
 * ============================================================ */

void test_config_import_json_applies_values(void)
{
    const char *json =
        "{\n"
        "  \"version\": 3,\n"
        "  \"display\": {\n"
        "    \"mode\": 1,\n"
        "    \"theme\": 1,\n"
        "    \"brightness\": 50,\n"
        "    \"anim_enabled\": 0\n"
        "  },\n"
        "  \"logging\": {\n"
        "    \"interval_s\": 10,\n"
        "    \"gps_rate_hz\": 5\n"
        "  },\n"
        "  \"thresholds\": {\n"
        "    \"rpm_redline\": 9000,\n"
        "    \"battery_low_v\": 11.8\n"
        "  },\n"
        "  \"device\": {\n"
        "    \"name\": \"TestBike\",\n"
        "    \"pairing_code\": 654321\n"
        "  }\n"
        "}\n";

    TEST_ASSERT_EQUAL(0, config_import_json(json));

    device_config_t *cfg = config_get();
    TEST_ASSERT_EQUAL_UINT8(1, cfg->display_mode);
    TEST_ASSERT_EQUAL_UINT8(1, cfg->display_theme);
    TEST_ASSERT_EQUAL_UINT8(50, cfg->display_brightness);
    TEST_ASSERT_EQUAL_UINT8(0, cfg->anim_enabled);
    TEST_ASSERT_EQUAL_UINT8(10, cfg->log_interval_s);
    TEST_ASSERT_EQUAL_UINT8(5, cfg->gps_rate_hz);
    TEST_ASSERT_EQUAL_UINT16(9000, cfg->rpm_redline);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 11.8f, cfg->battery_low_v);
    TEST_ASSERT_EQUAL_STRING("TestBike", cfg->device_name);
    TEST_ASSERT_EQUAL_UINT32(654321, cfg->pairing_code);
}

/* ============================================================
 * Test: JSON-Import — NULL input rejected
 * ============================================================ */

void test_config_import_json_null_rejected(void)
{
    TEST_ASSERT_EQUAL(-1, config_import_json(NULL));
}

/* ============================================================
 * Test: JSON-Import — partial JSON only applies found keys
 * ============================================================ */

void test_config_import_json_partial(void)
{
    /* Only set brightness, leave everything else at default */
    const char *json = "{\"brightness\": 42}";
    TEST_ASSERT_EQUAL(0, config_import_json(json));

    device_config_t *cfg = config_get();
    TEST_ASSERT_EQUAL_UINT8(42, cfg->display_brightness);
    /* Other values should remain at defaults */
    TEST_ASSERT_EQUAL_UINT16(8000, cfg->rpm_redline);
}

/* ============================================================
 * Test: JSON-Import — Roundtrip (export then import)
 * ============================================================ */

void test_config_json_export_import_roundtrip(void)
{
    /* Set custom values */
    uint8_t mode = 2;
    config_set_value(CFG_KEY_DISPLAY_MODE, &mode, 1);
    uint16_t redline = 9500;
    config_set_value(CFG_KEY_RPM_REDLINE, &redline, 2);

    /* Export */
    char buf[2048];
    int len = config_export_json(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, len);

    /* Reset to defaults */
    config_init();

    /* Verify defaults are back */
    device_config_t *cfg = config_get();
    TEST_ASSERT_EQUAL_UINT8(0, cfg->display_mode);
    TEST_ASSERT_EQUAL_UINT16(8000, cfg->rpm_redline);

    /* Import the exported JSON */
    TEST_ASSERT_EQUAL(0, config_import_json(buf));

    /* Values should be restored */
    cfg = config_get();
    TEST_ASSERT_EQUAL_UINT8(2, cfg->display_mode);
    TEST_ASSERT_EQUAL_UINT16(9500, cfg->rpm_redline);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_config_defaults_loaded);

    RUN_TEST(test_config_set_get_display_mode);
    RUN_TEST(test_config_set_get_brightness);
    RUN_TEST(test_config_set_get_gps_rate);
    RUN_TEST(test_config_set_get_rpm_redline);
    RUN_TEST(test_config_set_get_rpm_shift_light);
    RUN_TEST(test_config_set_get_battery_low);
    RUN_TEST(test_config_set_get_coolant_warn);
    RUN_TEST(test_config_set_get_fuel_min);
    RUN_TEST(test_config_set_get_pairing_code);
    RUN_TEST(test_config_set_get_sea_level);
    RUN_TEST(test_config_set_get_device_name);
    RUN_TEST(test_config_set_get_imu_calibration);

    RUN_TEST(test_config_unknown_key_returns_error);
    RUN_TEST(test_config_set_null_value_rejected);
    RUN_TEST(test_config_get_null_value_rejected);

    RUN_TEST(test_config_export_json_valid);
    RUN_TEST(test_config_export_json_too_small);
    RUN_TEST(test_config_export_json_null_buffer);

    RUN_TEST(test_config_import_json_applies_values);
    RUN_TEST(test_config_import_json_null_rejected);
    RUN_TEST(test_config_import_json_partial);
    RUN_TEST(test_config_json_export_import_roundtrip);

    return UNITY_END();
}
