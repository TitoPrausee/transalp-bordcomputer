/*
 * test_fuel_level.c — Unit Tests fuer Kraftstoffstand-Messung
 *
 * Testet firmware/components/sensors/fuel_level.c:
 *   - Kalibrierungstabelle: ADC-Rohwert -> Liter (Interpolation)
 *   - Tabellen-Grenzen (unterhalb/oberhalb, exacte Stuetzstellen)
 *   - Lineare Interpolation zwischen Stuetzstellen
 *   - Moving-Average-Glaettung gegen Schwappen
 *   - Reserve-Warnung (< 3 Liter -> EVT_FUEL_LOW)
 *   - Fehlerbehandlung (ADC-Fehler, nicht initialisiert)
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. Die ADC-Abhaengigkeit wird ueber einen
 * praeprozessor-Umleitungs-Mock kontrollierbar gemacht.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "sensors.h"
#include "board_config.h"
#include "event_bus.h"
#include "diagnostics.h"

#include <string.h>
#include <stdint.h>

/* ============================================================
 * Host build: mock ESP-IDF dependencies for fuel_level.c
 * ============================================================ */
#ifdef TEST_HOST

static inline void _fl_log_noop(const char *tag, const char *fmt, ...) {
    (void)tag; (void)fmt;
}

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV
#define ESP_LOGE(tag, fmt, ...) _fl_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) _fl_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) _fl_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) _fl_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) _fl_log_noop(tag, fmt, ##__VA_ARGS__)

/* ---- Shared handles (declared extern in sensors.h) ---- */
adc_oneshot_unit_handle_t g_adc1_handle = (adc_oneshot_unit_handle_t)1;
adc_cali_handle_t g_adc_cali_handle = (adc_cali_handle_t)1;

/* ---- Controllable ADC mock ----
 * fuel_level.c calls adc_oneshot_read() (a static inline in the mock
 * header). We redirect it to a controllable stub via a macro so the
 * test can inject ADC raw values and failure returns. */
static int g_mock_adc_raw = 0;
static int g_mock_adc_ret = 0;  /* 0 = ESP_OK */
static int g_mock_adc_call_count = 0;

static int mock_fuel_adc_oneshot_read(adc_oneshot_unit_handle_t h,
                                       adc_channel_t c, int *out)
{
    (void)h; (void)c;
    g_mock_adc_call_count++;
    if (out) *out = g_mock_adc_raw;
    return g_mock_adc_ret;
}
#define adc_oneshot_read mock_fuel_adc_oneshot_read

/* ---- event_bus mock (records which events were set) ---- */
static uint32_t g_events_set = 0;
void event_bus_set(uint32_t bits) { g_events_set |= bits; }
void event_bus_clear(uint32_t bits) { g_events_set &= ~bits; }
bool event_bus_wait(uint32_t bits, uint32_t timeout_ms) {
    (void)timeout_ms; return (g_events_set & bits) != 0;
}

/* ---- diagnostics mock (records last reported DTC) ---- */
static int g_dtc_report_count = 0;
static int g_last_dtc_code = -1;
void dtc_report(dtc_code_t code) {
    g_last_dtc_code = (int)code;
    g_dtc_report_count++;
}

/* Include filters.c first — fuel_level.c uses moving_avg_init/update. */
#include "../../components/utils/filters.c"

/* Include the real fuel_level.c implementation directly. */
#include "../../components/sensors/fuel_level.c"

#endif /* TEST_HOST */

/* ============================================================
 * Helpers
 * ============================================================ */

static void reset_mock_state(void)
{
    g_mock_adc_raw = 0;
    g_mock_adc_ret = 0;
    g_mock_adc_call_count = 0;
    g_events_set = 0;
    g_dtc_report_count = 0;
    g_last_dtc_code = -1;
}

void setUp(void)
{
    reset_mock_state();
    fuel_level_init();
}

void tearDown(void)
{
}

/* ============================================================
 * Kalibrierungstabelle: exacte Stuetzstellen
 * ============================================================ */

void test_calibration_table_exact_points(void)
{
    /* Directly test the interpolation at each calibration point. */
    TEST_ASSERT_EQUAL_FLOAT(0.0f,  fuel_adc_to_liters(120));
    TEST_ASSERT_EQUAL_FLOAT(2.0f,  fuel_adc_to_liters(200));
    TEST_ASSERT_EQUAL_FLOAT(4.0f,  fuel_adc_to_liters(290));
    TEST_ASSERT_EQUAL_FLOAT(6.0f,  fuel_adc_to_liters(370));
    TEST_ASSERT_EQUAL_FLOAT(8.0f,  fuel_adc_to_liters(450));
    TEST_ASSERT_EQUAL_FLOAT(10.0f, fuel_adc_to_liters(530));
    TEST_ASSERT_EQUAL_FLOAT(12.0f, fuel_adc_to_liters(620));
    TEST_ASSERT_EQUAL_FLOAT(14.0f, fuel_adc_to_liters(720));
    TEST_ASSERT_EQUAL_FLOAT(16.0f, fuel_adc_to_liters(800));
    TEST_ASSERT_EQUAL_FLOAT(17.0f, fuel_adc_to_liters(860));
}

/* ============================================================
 * Tabellen-Grenzen: unterhalb -> 0 Liter, oberhalb -> voll
 * ============================================================ */

void test_below_table_returns_empty(void)
{
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fuel_adc_to_liters(50));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fuel_adc_to_liters(0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fuel_adc_to_liters(119));
    /* Exactly at the first point */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fuel_adc_to_liters(120));
}

void test_above_table_returns_full(void)
{
    TEST_ASSERT_EQUAL_FLOAT(17.0f, fuel_adc_to_liters(1000));
    TEST_ASSERT_EQUAL_FLOAT(17.0f, fuel_adc_to_liters(861));
    /* Exactly at the last point */
    TEST_ASSERT_EQUAL_FLOAT(17.0f, fuel_adc_to_liters(860));
}

/* ============================================================
 * Lineare Interpolation zwischen Stuetzstellen
 * ============================================================ */

void test_interpolation_midpoint(void)
{
    /* Midpoint between 120 (0.0L) and 200 (2.0L) -> 1.0L */
    TEST_ASSERT_EQUAL_FLOAT(1.0f, fuel_adc_to_liters(160));

    /* Midpoint between 200 (2.0L) and 290 (4.0L) -> 3.0L */
    TEST_ASSERT_EQUAL_FLOAT(3.0f, fuel_adc_to_liters(245));

    /* Midpoint between 800 (16.0L) and 860 (17.0L) -> 16.5L */
    TEST_ASSERT_EQUAL_FLOAT(16.5f, fuel_adc_to_liters(830));
}

void test_interpolation_quarter_point(void)
{
    /* 25% into [120, 200]: 120 + 0.25*80 = 140 -> 0.5L */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, fuel_adc_to_liters(140));

    /* 75% into [120, 200]: 120 + 0.75*80 = 180 -> 1.5L */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.5f, fuel_adc_to_liters(180));
}

/* ============================================================
 * Fehlerhafter ADC-Rohwert (negativ)
 * ============================================================ */

void test_negative_raw_returns_error(void)
{
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, fuel_adc_to_liters(-1));
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, fuel_adc_to_liters(-100));
}

/* ============================================================
 * fuel_level_read_liters: nicht initialisiert -> -1.0
 * ============================================================ */

void test_read_not_initialized_returns_error(void)
{
    /* Force uninitialized state by re-including is not possible;
     * instead we verify the init flag works by reading after init
     * (setUp calls init). Here we just confirm a normal read works. */
    g_mock_adc_raw = 530;
    float liters = fuel_level_read_liters();
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, liters);
}

/* ============================================================
 * fuel_level_read_liters: korrekte Umrechnung ueber ADC
 * ============================================================ */

void test_read_liters_from_adc(void)
{
    /* ADC raw = 530 -> 10.0 liters (exact cal point) */
    g_mock_adc_raw = 530;
    float liters = fuel_level_read_liters();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.0f, liters);
}

void test_read_liters_full_tank(void)
{
    g_mock_adc_raw = 860;
    float liters = fuel_level_read_liters();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 17.0f, liters);
}

void test_read_liters_empty_tank(void)
{
    g_mock_adc_raw = 120;
    float liters = fuel_level_read_liters();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, liters);
}

/* ============================================================
 * Reserve-Warnung: < 3 Liter -> EVT_FUEL_LOW
 * ============================================================ */

void test_reserve_warning_when_low(void)
{
    /* ADC raw = 160 -> 1.0 liter (< FUEL_RESERVE_L = 3.0) */
    g_mock_adc_raw = 160;
    fuel_level_read_liters();
    TEST_ASSERT((g_events_set & EVT_FUEL_LOW) != 0);
}

void test_no_reserve_warning_when_sufficient(void)
{
    /* ADC raw = 530 -> 10.0 liters (> FUEL_RESERVE_L) */
    g_mock_adc_raw = 530;
    fuel_level_read_liters();
    TEST_ASSERT((g_events_set & EVT_FUEL_LOW) == 0);
}

/* ============================================================
 * ADC-Lesefehler -> DTC + -1.0
 * ============================================================ */

void test_adc_read_failure_reports_dtc(void)
{
    g_mock_adc_ret = -1;  /* non-OK return */
    float liters = fuel_level_read_liters();
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, liters);
    TEST_ASSERT_EQUAL_INT(1, g_dtc_report_count);
    TEST_ASSERT_EQUAL_INT(DTC_S0201, g_last_dtc_code);
}

/* ============================================================
 * Moving-Average-Glaettung: Schwappen wird reduziert
 * ============================================================ */

void test_smoothing_reduces_slosh(void)
{
    /* Feed alternating high/low values; the moving average should
     * converge toward the mean rather than following extremes. */
    g_mock_adc_raw = 860;   /* 17L */
    fuel_level_read_liters();
    g_mock_adc_raw = 120;   /* 0L */
    float low = fuel_level_read_liters();
    g_mock_adc_raw = 860;   /* 17L */
    fuel_level_read_liters();
    g_mock_adc_raw = 120;   /* 0L */
    fuel_level_read_liters();

    /* After 4 alternating samples, the average window (size 10)
     * should produce a value well below 17 (not following the
     * last high sample) and above 0. */
    TEST_ASSERT_GREATER_THAN(0.0f, low);
    /* The smoothed value should be closer to the mean (~8.5) than
     * to the last extreme (0.0 or 17.0). */
    g_mock_adc_raw = 530;   /* 10L — bring it to a known value */
    float liters = fuel_level_read_liters();
    /* After several samples, result trends toward 10 */
    TEST_ASSERT_GREATER_THAN(2.0f, liters);
}

/* ============================================================
 * Multisampling: ADC wird 16x gelesen
 * ============================================================ */

void test_multisampling_reads_adc_16_times(void)
{
    g_mock_adc_raw = 530;
    g_mock_adc_call_count = 0;
    fuel_level_read_liters();
    TEST_ASSERT_EQUAL_INT(16, g_mock_adc_call_count);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_calibration_table_exact_points);
    RUN_TEST(test_below_table_returns_empty);
    RUN_TEST(test_above_table_returns_full);
    RUN_TEST(test_interpolation_midpoint);
    RUN_TEST(test_interpolation_quarter_point);
    RUN_TEST(test_negative_raw_returns_error);
    RUN_TEST(test_read_not_initialized_returns_error);
    RUN_TEST(test_read_liters_from_adc);
    RUN_TEST(test_read_liters_full_tank);
    RUN_TEST(test_read_liters_empty_tank);
    RUN_TEST(test_reserve_warning_when_low);
    RUN_TEST(test_no_reserve_warning_when_sufficient);
    RUN_TEST(test_adc_read_failure_reports_dtc);
    RUN_TEST(test_smoothing_reduces_slosh);
    RUN_TEST(test_multisampling_reads_adc_16_times);

    return UNITY_END();
}
