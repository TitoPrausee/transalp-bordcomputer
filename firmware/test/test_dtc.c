/*
 * test_dtc.c — Unit Tests fuer das DTC-System (Diagnostic Trouble Codes)
 *
 * Testet die in firmware/components/diagnostics/dtc.c implementierten
 * Funktionen:
 *   - dtc_init / dtc_report / dtc_clear / dtc_clear_all
 *   - dtc_get_active
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. Da dtc.c ESP-IDF-Abhaengigkeiten hat (esp_log,
 * xthal_get_ccount), werden diese durch Mocks ersetzt.
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

/* xthal_get_ccount is an Xtensa intrinsic; provide a mock */
static uint32_t s_mock_ccount = 0;
static inline uint32_t xthal_get_ccount(void) {
    s_mock_ccount += 240000; /* simulate 1ms per call */
    return s_mock_ccount;
}

/* Include the actual dtc.c implementation directly */
#include "../../components/diagnostics/dtc.c"

/* dtc.c has a static init function that's not in the header.
 * The header declares dtc_report etc. but dtc.c defines dtc_init().
 * We need to call it, so declare it here. */
#endif

void setUp(void)
{
    dtc_init();
}

void tearDown(void)
{
}

/* ============================================================
 * Test: DTC-Code-Enumeration
 * ============================================================ */
void test_dtc_code_enum_values(void)
{
    /* DTC_NONE should be 0 */
    TEST_ASSERT_EQUAL(0, (int)DTC_NONE);

    /* Each subsequent code should increment by 1 */
    TEST_ASSERT_EQUAL(1, (int)DTC_GPS_NO_FIX);
    TEST_ASSERT_EQUAL(2, (int)DTC_GPS_UART_ERROR);
    TEST_ASSERT_EQUAL(3, (int)DTC_IMU_NOT_RESPONDING);
    TEST_ASSERT_EQUAL(4, (int)DTC_BARO_NOT_RESPONDING);
    TEST_ASSERT_EQUAL(5, (int)DTC_SD_MOUNT_FAILED);
    TEST_ASSERT_EQUAL(6, (int)DTC_SD_WRITE_FAILED);
    TEST_ASSERT_EQUAL(7, (int)DTC_BATTERY_LOW);
    TEST_ASSERT_EQUAL(8, (int)DTC_BATTERY_CRITICAL);
    TEST_ASSERT_EQUAL(9, (int)DTC_BLE_INIT_FAILED);
    TEST_ASSERT_EQUAL(10, (int)DTC_DISPLAY_INIT_FAILED);
    TEST_ASSERT_EQUAL(11, (int)DTC_SPI_BUS_ERROR);
    TEST_ASSERT_EQUAL(12, (int)DTC_OVERTEMPERATURE);
}

/* ============================================================
 * Test: DTC melden (neu)
 * ============================================================ */
void test_dtc_report_new(void)
{
    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(0, count);

    dtc_report(DTC_GPS_NO_FIX);

    count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(DTC_GPS_NO_FIX, entries[0].code);
    TEST_ASSERT_EQUAL(1, entries[0].count);
    TEST_ASSERT_TRUE(entries[0].active);
}

/* ============================================================
 * Test: DTC wiederholt melden (count++)
 * ============================================================ */
void test_dtc_report_reoccurrence(void)
{
    dtc_report(DTC_SD_MOUNT_FAILED);
    dtc_report(DTC_SD_MOUNT_FAILED);
    dtc_report(DTC_SD_MOUNT_FAILED);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);

    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(3, entries[0].count);
    TEST_ASSERT_TRUE(entries[0].active);
}

/* ============================================================
 * Test: Mehrere verschiedene DTCs
 * ============================================================ */
void test_dtc_multiple_codes(void)
{
    dtc_report(DTC_GPS_NO_FIX);
    dtc_report(DTC_BATTERY_LOW);
    dtc_report(DTC_OVERTEMPERATURE);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);

    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(DTC_GPS_NO_FIX, entries[0].code);
    TEST_ASSERT_EQUAL(DTC_BATTERY_LOW, entries[1].code);
    TEST_ASSERT_EQUAL(DTC_OVERTEMPERATURE, entries[2].code);
}

/* ============================================================
 * Test: DTC loeschen (clear)
 * ============================================================ */
void test_dtc_clear_single(void)
{
    dtc_report(DTC_BATTERY_LOW);
    dtc_report(DTC_OVERTEMPERATURE);

    dtc_clear(DTC_BATTERY_LOW);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);

    /* OVERTEMPERATURE still active, BATTERY_LOW cleared */
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(DTC_OVERTEMPERATURE, entries[0].code);
}

/* ============================================================
 * Test: Alle DTCs loeschen
 * ============================================================ */
void test_dtc_clear_all(void)
{
    dtc_report(DTC_GPS_NO_FIX);
    dtc_report(DTC_SD_WRITE_FAILED);
    dtc_report(DTC_BLE_INIT_FAILED);

    dtc_clear_all();

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(0, count);
}

/* ============================================================
 * Test: DTC-Tabelle voll (DTC_MAX_ENTRIES)
 * ============================================================ */
void test_dtc_list_full(void)
{
    /* Fill all 16 slots with distinct codes */
    for (int i = 1; i <= DTC_MAX_ENTRIES; i++) {
        dtc_report((dtc_code_t)i);
    }

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(DTC_MAX_ENTRIES, count);

    /* Try to add one more — should not crash, should be ignored */
    dtc_report((dtc_code_t)(DTC_MAX_ENTRIES + 1));
    count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(DTC_MAX_ENTRIES, count);
}

/* ============================================================
 * Test: DTC-Entry-Struktur-Layout
 * ============================================================ */
void test_dtc_entry_struct(void)
{
    dtc_entry_t entry;
    memset(&entry, 0, sizeof(entry));

    entry.code = DTC_SPI_BUS_ERROR;
    entry.timestamp_ms = 12345;
    entry.count = 5;
    entry.active = true;

    TEST_ASSERT_EQUAL(DTC_SPI_BUS_ERROR, entry.code);
    TEST_ASSERT_EQUAL(12345, entry.timestamp_ms);
    TEST_ASSERT_EQUAL(5, entry.count);
    TEST_ASSERT_TRUE(entry.active);
}

/* ============================================================
 * Test: DTC-Timestamp wird aktualisiert bei Wiederholung
 * ============================================================ */
void test_dtc_timestamp_updated(void)
{
    dtc_report(DTC_GPS_UART_ERROR);
    
    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, count);
    uint32_t first_ts = entries[0].timestamp_ms;

    /* Report again — timestamp should be updated (later) */
    dtc_report(DTC_GPS_UART_ERROR);
    count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_GREATER_OR_EQUAL(first_ts, entries[0].timestamp_ms);
    TEST_ASSERT_EQUAL(2, entries[0].count);
}

/* ============================================================
 * Test: dtc_clear fuer nicht vorhandenen Code
 * ============================================================ */
void test_dtc_clear_nonexistent(void)
{
    /* Should not crash */
    dtc_clear(DTC_OVERTEMPERATURE);

    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);
    TEST_ASSERT_EQUAL(0, count);
}

/* ============================================================
 * Test: dtc_get_active mit kleiner max_entries
 * ============================================================ */
void test_dtc_get_active_limited_buffer(void)
{
    dtc_report(DTC_GPS_NO_FIX);
    dtc_report(DTC_BATTERY_LOW);
    dtc_report(DTC_OVERTEMPERATURE);

    dtc_entry_t entries[2]; /* Smaller than number of DTCs */
    uint8_t count = dtc_get_active(entries, 2);
    TEST_ASSERT_EQUAL(2, count);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_dtc_code_enum_values);
    RUN_TEST(test_dtc_report_new);
    RUN_TEST(test_dtc_report_reoccurrence);
    RUN_TEST(test_dtc_multiple_codes);
    RUN_TEST(test_dtc_clear_single);
    RUN_TEST(test_dtc_clear_all);
    RUN_TEST(test_dtc_list_full);
    RUN_TEST(test_dtc_entry_struct);
    RUN_TEST(test_dtc_timestamp_updated);
    RUN_TEST(test_dtc_clear_nonexistent);
    RUN_TEST(test_dtc_get_active_limited_buffer);

    return UNITY_END();
}
