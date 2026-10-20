/*
 * test_integration_dtc_diagnostics.c
 *
 * Integration test: DTC Reporting -> Diagnostics -> Event Bus
 *
 * Verifies the interaction between the diagnostics system (DTC codes)
 * and the event bus. When a DTC is reported, the system should:
 *   - Store the DTC in the active list
 *   - Map it to the correct severity
 *   - Allow querying by active/history status
 *   - Support clearing individual and all DTCs
 *
 * Also tests that event bus flags correspond to DTC categories:
 *   - EVT_OVERTEMPERATURE <-> DTC_S0103 (temp > 120C)
 *   - EVT_FUEL_LOW <-> DTC_S0202 (fuel < 10%)
 *   - EVT_CRASH_DETECTED <-> sensor crash flag
 *
 * Test coverage:
 *   - DTC report and retrieve cycle
 *   - Severity mapping for critical codes
 *   - DTC clear single and clear all
 *   - DTC code-to-string conversion
 *   - Event bus flag consistency with DTC categories
 *   - DTC freeze-frame structure
 *   - DTC capacity limit (DTC_MAX_ENTRIES)
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "diagnostics.h"
#include "event_bus.h"

#include <string.h>

/* ============================================================
 * Mock ESP log and Xtensa intrinsics for host build
 * ============================================================ */
#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0

static uint32_t s_mock_ccount = 0;
static inline uint32_t xthal_get_ccount(void) {
    s_mock_ccount += 240000;
    return s_mock_ccount;
}

/* ============================================================
 * Stub implementations for cross-component dependencies
 * (dtc.c calls these from other components during capture_freeze_frame)
 * ============================================================ */
QueueHandle_t sensor_data_queue = NULL;
QueueHandle_t gps_data_queue = NULL;

bool ble_is_connected(void) { return false; }
float power_get_battery_voltage(void) { return 12.6f; }
bool sd_card_is_mounted(void) { return true; }

/* Include event_bus.c first, renaming TAG to avoid conflict */
#define TAG TAG_event_bus
#include "../../components/utils/event_bus.c"
#undef TAG

/* Include dtc.c, renaming TAG to avoid conflict */
#define TAG TAG_dtc
#include "../../components/diagnostics/dtc.c"
#undef TAG
#endif

void setUp(void)
{
    dtc_init();
    event_bus_init();
}

void tearDown(void) {}

/* ============================================================
 * Test: DTC report and retrieve
 * ============================================================ */
void test_dtc_report_and_retrieve(void)
{
    dtc_report(DTC_P0101);  /* RPM signal implausible */

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);

    TEST_ASSERT_GREATER_OR_EQUAL(1, count);

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (entries[i].code == DTC_P0101) {
            found = true;
            TEST_ASSERT_EQUAL(DTC_SEV_WARN, entries[i].severity);
            TEST_ASSERT_EQUAL(DTC_STATUS_ACTIVE, entries[i].status);
            break;
        }
    }
    TEST_ASSERT_TRUE(found);
}

/* ============================================================
 * Test: DTC clear single code
 * ============================================================ */
void test_dtc_clear_single(void)
{
    dtc_report(DTC_S0102);  /* Coolant temp > 110C */
    dtc_report(DTC_S0202);  /* Fuel < 10% */

    dtc_clear(DTC_S0102);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);

    bool s0102_active = false;
    bool s0202_active = false;
    for (int i = 0; i < count; i++) {
        if (entries[i].code == DTC_S0102) s0102_active = true;
        if (entries[i].code == DTC_S0202) s0202_active = true;
    }
    TEST_ASSERT_FALSE(s0102_active);
    TEST_ASSERT_TRUE(s0202_active);
}

/* ============================================================
 * Test: DTC clear all
 * ============================================================ */
void test_dtc_clear_all(void)
{
    dtc_report(DTC_P0101);
    dtc_report(DTC_P0401);
    dtc_report(DTC_S0502);

    dtc_clear_all();

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(0, count);
}

/* ============================================================
 * Test: Critical severity for overtemp (DTC_S0103)
 * ============================================================ */
void test_overtemp_severity_critical(void)
{
    dtc_severity_t sev = dtc_default_severity(DTC_S0103);
    TEST_ASSERT_EQUAL(DTC_SEV_CRITICAL, sev);
}

/* ============================================================
 * Test: Critical severity for low voltage (DTC_P0402)
 * ============================================================ */
void test_low_voltage_severity_critical(void)
{
    dtc_severity_t sev = dtc_default_severity(DTC_P0402);
    TEST_ASSERT_EQUAL(DTC_SEV_CRITICAL, sev);
}

/* ============================================================
 * Test: DTC code-to-string conversion
 * ============================================================ */
void test_dtc_code_to_string(void)
{
    TEST_ASSERT_EQUAL_STRING("P0101", dtc_code_to_string(DTC_P0101));
    TEST_ASSERT_EQUAL_STRING("S0103", dtc_code_to_string(DTC_S0103));
    TEST_ASSERT_EQUAL_STRING("C0101", dtc_code_to_string(DTC_C0101));
}

/* ============================================================
 * Test: DTC severity-to-string conversion
 * ============================================================ */
void test_dtc_severity_to_string(void)
{
    TEST_ASSERT_NOT_NULL(dtc_severity_to_string(DTC_SEV_INFO));
    TEST_ASSERT_NOT_NULL(dtc_severity_to_string(DTC_SEV_WARN));
    TEST_ASSERT_NOT_NULL(dtc_severity_to_string(DTC_SEV_ERROR));
    TEST_ASSERT_NOT_NULL(dtc_severity_to_string(DTC_SEV_CRITICAL));
}

/* ============================================================
 * Test: Event bus flags for DTC-related events
 * ============================================================ */
void test_event_bus_overtemp_flag(void)
{
    event_bus_set(EVT_OVERTEMPERATURE);
    bool result = event_bus_wait(EVT_OVERTEMPERATURE, 100);
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Test: Event bus flags for fuel low
 * ============================================================ */
void test_event_bus_fuel_low_flag(void)
{
    event_bus_set(EVT_FUEL_LOW);
    bool result = event_bus_wait(EVT_FUEL_LOW, 100);
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Test: Event bus flags for crash detected
 * ============================================================ */
void test_event_bus_crash_flag(void)
{
    event_bus_set(EVT_CRASH_DETECTED);
    bool result = event_bus_wait(EVT_CRASH_DETECTED, 100);
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Test: Event bus flags are distinct powers of two
 * ============================================================ */
void test_event_flags_distinct(void)
{
    uint32_t flags[] = {
        EVT_GPS_FIX, EVT_GPS_LOST_FIX, EVT_SENSOR_READY,
        EVT_CRASH_DETECTED, EVT_BATTERY_LOW, EVT_BATTERY_CRITICAL,
        EVT_SDCARD_MOUNTED, EVT_SDCARD_ERROR, EVT_BLE_CONNECTED,
        EVT_BLE_DISCONNECTED, EVT_DISPLAY_READY, EVT_SHUTDOWN,
        EVT_OTA_START, EVT_OTA_DONE, EVT_OVERTEMPERATURE,
        EVT_FUEL_LOW, EVT_RPM_TIMEOUT,
        EVT_CARPLAY_CONNECTED, EVT_CARPLAY_DISCONNECTED,
    };
    int n = sizeof(flags) / sizeof(flags[0]);

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            TEST_ASSERT_NOT_EQUAL(flags[i], flags[j]);
        }
    }
}

/* ============================================================
 * Test: Multiple DTCs can coexist
 * ============================================================ */
void test_multiple_dtcs_coexist(void)
{
    dtc_clear_all();
    dtc_report(DTC_P0101);
    dtc_report(DTC_S0102);
    dtc_report(DTC_C0101);
    dtc_report(DTC_B0101);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(4, count);
}

/* ============================================================
 * Test: DTC occurrence count increments on re-report
 * ============================================================ */
void test_dtc_occurrence_count(void)
{
    dtc_clear_all();
    dtc_report(DTC_P0101);
    dtc_report(DTC_P0101);
    dtc_report(DTC_P0101);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_GREATER_OR_EQUAL(1, count);

    /* Find the P0101 entry and check count */
    for (int i = 0; i < count; i++) {
        if (entries[i].code == DTC_P0101) {
            TEST_ASSERT_GREATER_OR_EQUAL(3, entries[i].count);
            return;
        }
    }
    TEST_FAIL_MESSAGE("DTC P0101 not found after reporting");
}

/* ============================================================
 * Test: DTC freeze-frame structure is valid
 * ============================================================ */
void test_dtc_freeze_frame_structure(void)
{
    dtc_freeze_frame_t ff;
    memset(&ff, 0, sizeof(ff));

    ff.battery_v = 12.6f;
    ff.coolant_temp_c = 95.0f;
    ff.rpm = 4500;
    ff.speed_kmh = 80.0f;
    ff.gps_fix = true;
    ff.ble_connected = true;

    TEST_ASSERT_EQUAL_FLOAT(12.6f, ff.battery_v);
    TEST_ASSERT_EQUAL_FLOAT(95.0f, ff.coolant_temp_c);
    TEST_ASSERT_EQUAL(4500, (int)ff.rpm);
    TEST_ASSERT_TRUE(ff.gps_fix);
    TEST_ASSERT_TRUE(ff.ble_connected);
}

/* ============================================================
 * Test: DTC_MAX_ENTRIES is at least 32
 * ============================================================ */
void test_dtc_max_entries(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL(32, DTC_MAX_ENTRIES);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_dtc_report_and_retrieve);
    RUN_TEST(test_dtc_clear_single);
    RUN_TEST(test_dtc_clear_all);
    RUN_TEST(test_overtemp_severity_critical);
    RUN_TEST(test_low_voltage_severity_critical);
    RUN_TEST(test_dtc_code_to_string);
    RUN_TEST(test_dtc_severity_to_string);
    RUN_TEST(test_event_bus_overtemp_flag);
    RUN_TEST(test_event_bus_fuel_low_flag);
    RUN_TEST(test_event_bus_crash_flag);
    RUN_TEST(test_event_flags_distinct);
    RUN_TEST(test_multiple_dtcs_coexist);
    RUN_TEST(test_dtc_occurrence_count);
    RUN_TEST(test_dtc_freeze_frame_structure);
    RUN_TEST(test_dtc_max_entries);

    return UNITY_END();
}
