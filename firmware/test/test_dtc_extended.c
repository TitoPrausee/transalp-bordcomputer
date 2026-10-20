/*
 * test_dtc_extended.c — Erweiterte DTC-Tests (Severity, History, String-Mapping)
 *
 * Testet firmware/components/diagnostics/dtc.c zusaetzlich zu test_dtc.c:
 *   - dtc_default_severity Mapping fuer alle Code-Kategorien
 *   - dtc_code_to_string / dtc_severity_to_string / dtc_status_to_string
 *   - dtc_get_history nach dtc_clear
 *   - dtc_get_all (active + history)
 *   - Freeze-Frame Struktur
 *   - DTC_NONE handling
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "diagnostics.h"

#include <string.h>

/* ============================================================
 * Mock ESP-IDF dependencies of dtc.c for host build
 * ============================================================ */
#ifdef TEST_HOST

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0

/* dtc.c references these extern variables from sensors.h and gps.h */
QueueHandle_t sensor_data_queue = NULL;
QueueHandle_t gps_data_queue = NULL;

/* Mock power.h functions referenced by capture_freeze_frame */
static float s_mock_voltage = 12.6f;
float power_get_battery_voltage(void) { return s_mock_voltage; }
uint8_t power_get_battery_percent(void) { return 80; }

/* Mock ble.h function referenced by capture_freeze_frame */
bool ble_is_connected(void) { return false; }

/* Mock storage.h function referenced by capture_freeze_frame */
bool sd_card_is_mounted(void) { return false; }

/* Include the actual dtc.c implementation directly */
#include "../../components/diagnostics/dtc.c"

#endif /* TEST_HOST */

void setUp(void)
{
    dtc_init();
}

void tearDown(void)
{
}

/* ============================================================
 * Test: Severity-Mapping fuer verschiedene Code-Kategorien
 * ============================================================ */

void test_dtc_severity_powertrain_warn(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_P0101));
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_P0103));
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_P0401));
}

void test_dtc_severity_powertrain_error(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_ERROR, dtc_default_severity(DTC_P0102));
    TEST_ASSERT_EQUAL(DTC_SEV_ERROR, dtc_default_severity(DTC_P0202));
}

void test_dtc_severity_powertrain_critical(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_CRITICAL, dtc_default_severity(DTC_P0402));
}

void test_dtc_severity_powertrain_info(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_INFO, dtc_default_severity(DTC_P0404));
}

void test_dtc_severity_sensor_categories(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_ERROR, dtc_default_severity(DTC_S0101));
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_S0102));
    TEST_ASSERT_EQUAL(DTC_SEV_CRITICAL, dtc_default_severity(DTC_S0103));
    TEST_ASSERT_EQUAL(DTC_SEV_INFO, dtc_default_severity(DTC_S0503));
}

void test_dtc_severity_communication(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_ERROR, dtc_default_severity(DTC_C0101));
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_C0102));
    TEST_ASSERT_EQUAL(DTC_SEV_ERROR, dtc_default_severity(DTC_C0203));
}

void test_dtc_severity_body(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_ERROR, dtc_default_severity(DTC_B0101));
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_B0102));
}

void test_dtc_severity_diagnose_system(void)
{
    TEST_ASSERT_EQUAL(DTC_SEV_ERROR, dtc_default_severity(DTC_D0101));
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_D0102));
    TEST_ASSERT_EQUAL(DTC_SEV_INFO, dtc_default_severity(DTC_D0402));
}

void test_dtc_severity_none_fallback(void)
{
    /* DTC_NONE should return default severity (WARN) */
    TEST_ASSERT_EQUAL(DTC_SEV_WARN, dtc_default_severity(DTC_NONE));
}

/* ============================================================
 * Test: String-Mapping
 * ============================================================ */

void test_dtc_code_to_string_known(void)
{
    TEST_ASSERT_EQUAL_STRING("P0101", dtc_code_to_string(DTC_P0101));
    TEST_ASSERT_EQUAL_STRING("S0501", dtc_code_to_string(DTC_S0501));
    TEST_ASSERT_EQUAL_STRING("C0101", dtc_code_to_string(DTC_C0101));
    TEST_ASSERT_EQUAL_STRING("B0101", dtc_code_to_string(DTC_B0101));
    TEST_ASSERT_EQUAL_STRING("D0101", dtc_code_to_string(DTC_D0101));
}

void test_dtc_code_to_string_unknown(void)
{
    TEST_ASSERT_EQUAL_STRING("????", dtc_code_to_string(DTC_NONE));
    TEST_ASSERT_EQUAL_STRING("????", dtc_code_to_string((dtc_code_t)99999));
}

void test_dtc_severity_to_string(void)
{
    TEST_ASSERT_EQUAL_STRING("INFO",     dtc_severity_to_string(DTC_SEV_INFO));
    TEST_ASSERT_EQUAL_STRING("WARN",     dtc_severity_to_string(DTC_SEV_WARN));
    TEST_ASSERT_EQUAL_STRING("ERROR",    dtc_severity_to_string(DTC_SEV_ERROR));
    TEST_ASSERT_EQUAL_STRING("CRITICAL", dtc_severity_to_string(DTC_SEV_CRITICAL));
}

void test_dtc_status_to_string(void)
{
    TEST_ASSERT_EQUAL_STRING("ACTIVE",  dtc_status_to_string(DTC_STATUS_ACTIVE));
    TEST_ASSERT_EQUAL_STRING("HISTORY", dtc_status_to_string(DTC_STATUS_HISTORY));
    TEST_ASSERT_EQUAL_STRING("PENDING", dtc_status_to_string(DTC_STATUS_PENDING));
}

/* ============================================================
 * Test: History nach Clear
 * ============================================================ */

void test_dtc_clear_moves_to_history(void)
{
    dtc_report(DTC_S0501);
    dtc_report(DTC_P0401);

    dtc_clear(DTC_S0501);

    /* Active should have only P0401 */
    dtc_entry_t active[DTC_MAX_ENTRIES];
    uint8_t active_count = dtc_get_active(active, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, active_count);
    TEST_ASSERT_EQUAL(DTC_P0401, active[0].code);

    /* History should have S0501 */
    dtc_entry_t history[DTC_MAX_ENTRIES];
    uint8_t hist_count = dtc_get_history(history, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, hist_count);
    TEST_ASSERT_EQUAL(DTC_S0501, history[0].code);
}

void test_dtc_clear_all_moves_all_to_history(void)
{
    dtc_report(DTC_S0501);
    dtc_report(DTC_P0401);
    dtc_report(DTC_C0101);

    dtc_clear_all();

    /* No active */
    dtc_entry_t active[DTC_MAX_ENTRIES];
    TEST_ASSERT_EQUAL(0, dtc_get_active(active, DTC_MAX_ENTRIES));

    /* 3 in history */
    dtc_entry_t history[DTC_MAX_ENTRIES];
    uint8_t hist_count = dtc_get_history(history, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(3, hist_count);
}

/* ============================================================
 * Test: get_all returns active + history
 * ============================================================ */

void test_dtc_get_all_returns_all(void)
{
    dtc_report(DTC_S0501);   /* active */
    dtc_report(DTC_P0401);   /* active */
    dtc_clear(DTC_S0501);    /* S0501 -> history, P0401 stays active */

    dtc_entry_t all[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_all(all, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(2, count);
}

/* ============================================================
 * Test: Freeze-Frame structure
 * ============================================================ */

void test_dtc_freeze_frame_struct(void)
{
    dtc_freeze_frame_t ff;
    memset(&ff, 0, sizeof(ff));

    ff.battery_v = 12.4f;
    ff.coolant_temp_c = 95.0f;
    ff.rpm = 4500;
    ff.speed_kmh = 80.0f;
    ff.gps_sats = 8;
    ff.gps_fix = true;

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.4f, ff.battery_v);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 95.0f, ff.coolant_temp_c);
    TEST_ASSERT_EQUAL_FLOAT(4500, ff.rpm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 80.0f, ff.speed_kmh);
    TEST_ASSERT_EQUAL_UINT8(8, ff.gps_sats);
    TEST_ASSERT_TRUE(ff.gps_fix);
}

/* ============================================================
 * Test: DTC report uses default severity
 * ============================================================ */

void test_dtc_report_uses_default_severity(void)
{
    /* DTC_P0402 should have CRITICAL severity when reported */
    dtc_report(DTC_P0402);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(DTC_SEV_CRITICAL, entries[0].severity);
}

void test_dtc_report_info_severity(void)
{
    /* DTC_P0404 should have INFO severity */
    dtc_report(DTC_P0404);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(DTC_SEV_INFO, entries[0].severity);
}

/* ============================================================
 * Test: Reoccurrence increments count and keeps severity
 * ============================================================ */

void test_dtc_reoccurrence_keeps_severity(void)
{
    dtc_report(DTC_S0103);  /* CRITICAL */
    dtc_report(DTC_S0103);
    dtc_report(DTC_S0103);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(3, entries[0].count);
    TEST_ASSERT_EQUAL(DTC_SEV_CRITICAL, entries[0].severity);
}

/* ============================================================
 * Test: DTC entry has timestamps
 * ============================================================ */

void test_dtc_entry_has_timestamps(void)
{
    dtc_report(DTC_C0101);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, count);

    /* first_occurrence should be non-zero (xTaskGetTickCount based) */
    TEST_ASSERT_GREATER_THAN(0, entries[0].first_occurrence_ms);
    TEST_ASSERT_GREATER_OR_EQUAL(entries[0].first_occurrence_ms,
                                 entries[0].last_occurrence_ms);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* Severity mapping */
    RUN_TEST(test_dtc_severity_powertrain_warn);
    RUN_TEST(test_dtc_severity_powertrain_error);
    RUN_TEST(test_dtc_severity_powertrain_critical);
    RUN_TEST(test_dtc_severity_powertrain_info);
    RUN_TEST(test_dtc_severity_sensor_categories);
    RUN_TEST(test_dtc_severity_communication);
    RUN_TEST(test_dtc_severity_body);
    RUN_TEST(test_dtc_severity_diagnose_system);
    RUN_TEST(test_dtc_severity_none_fallback);

    /* String mapping */
    RUN_TEST(test_dtc_code_to_string_known);
    RUN_TEST(test_dtc_code_to_string_unknown);
    RUN_TEST(test_dtc_severity_to_string);
    RUN_TEST(test_dtc_status_to_string);

    /* History */
    RUN_TEST(test_dtc_clear_moves_to_history);
    RUN_TEST(test_dtc_clear_all_moves_all_to_history);

    /* Get all */
    RUN_TEST(test_dtc_get_all_returns_all);

    /* Freeze frame */
    RUN_TEST(test_dtc_freeze_frame_struct);

    /* Report severity */
    RUN_TEST(test_dtc_report_uses_default_severity);
    RUN_TEST(test_dtc_report_info_severity);
    RUN_TEST(test_dtc_reoccurrence_keeps_severity);
    RUN_TEST(test_dtc_entry_has_timestamps);

    return UNITY_END();
}
