/*
 * test_rpm.c — Unit Tests fuer Drehzahl-Messung (PCNT)
 *
 * Testet firmware/components/sensors/rpm.c:
 *   - RPM-Formel: pulse_count / period_s * (60 / pulses_per_rev)
 *   - Gleitender Mittelwert (Window 5) Konvergenz
 *   - Clamping auf 0..8000 (Redline)
 *   - Null-Pulse / Motor steht
 *   - PCNT-Lesefehler -> 0
 *   - Nicht initialisiert -> 0
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. Die PCNT-Abhaengigkeit wird ueber den Mock
 * test/mocks/driver/pulse_cnt.h kontrollierbar gemacht.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "sensors.h"
#include "board_config.h"

#include <string.h>
#include <stdint.h>

/* ============================================================
 * Host build: mock ESP-IDF dependencies for rpm.c
 * ============================================================ */
#ifdef TEST_HOST

static inline void _rpm_log_noop(const char *tag, const char *fmt, ...) {
    (void)tag; (void)fmt;
}

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV
#define ESP_LOGE(tag, fmt, ...) _rpm_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) _rpm_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) _rpm_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) _rpm_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) _rpm_log_noop(tag, fmt, ##__VA_ARGS__)

/* Define the controllable PCNT mock state (declared extern in the
 * mock header so tests can set the injected pulse count). */
int g_mock_pcnt_count = 0;
int g_mock_pcnt_get_ret = 0;  /* 0 = ESP_OK */

/* Include the real rpm.c implementation directly. */
#include "../../components/sensors/rpm.c"

#endif /* TEST_HOST */

/* ============================================================
 * Helpers
 *
 * RPM formula: rpm = (pulse_count / 0.1) * (60 / 1) = pulse_count * 600
 * After smoothing (first sample = the value itself), then clamped to 8000.
 * ============================================================ */
#define RPM_FORMULA(pc) ((uint16_t)((float)(pc) / 0.1f * (60.0f / RPM_PULSES_PER_REV)))

void setUp(void)
{
    g_mock_pcnt_count = 0;
    g_mock_pcnt_get_ret = 0;
    /* rpm_init resets the smoothing buffer and configures the mock PCNT
     * unit (which sets s_pcnt_unit non-NULL). */
    rpm_init();
}

void tearDown(void)
{
}

/* ============================================================
 * Null-Pulse: Motor steht -> 0
 * ============================================================ */

void test_zero_pulses_returns_zero(void)
{
    g_mock_pcnt_count = 0;
    TEST_ASSERT_EQUAL_UINT16(0, rpm_read());
}

void test_negative_pulses_returns_zero(void)
{
    g_mock_pcnt_count = -5;
    TEST_ASSERT_EQUAL_UINT16(0, rpm_read());
}

/* ============================================================
 * RPM-Formel: korrekte Umrechnung
 * ============================================================ */

void test_rpm_formula_single_sample(void)
{
    /* pulse_count = 10 -> 6000 rpm (first sample, smoothing = value itself) */
    g_mock_pcnt_count = 10;
    TEST_ASSERT_EQUAL_UINT16(6000, rpm_read());
}

void test_rpm_formula_low_rpm(void)
{
    /* pulse_count = 1 -> 600 rpm */
    g_mock_pcnt_count = 1;
    TEST_ASSERT_EQUAL_UINT16(600, rpm_read());
}

void test_rpm_formula_high_rpm(void)
{
    /* pulse_count = 12 -> 7200 rpm */
    g_mock_pcnt_count = 12;
    TEST_ASSERT_EQUAL_UINT16(7200, rpm_read());
}

/* ============================================================
 * Clamping: Ueber Redline -> 8000
 * ============================================================ */

void test_clamping_at_redline(void)
{
    /* pulse_count = 14 -> 8400 -> clamped to 8000 */
    g_mock_pcnt_count = 14;
    TEST_ASSERT_EQUAL_UINT16(RPM_MAX_VALUE, rpm_read());
}

void test_clamping_far_above_redline(void)
{
    /* pulse_count = 100 -> 60000 -> clamped to 8000 */
    g_mock_pcnt_count = 100;
    TEST_ASSERT_EQUAL_UINT16(RPM_MAX_VALUE, rpm_read());
}

void test_clamping_just_below_redline(void)
{
    /* pulse_count = 13 -> 7800 rpm (below 8000, not clamped) */
    g_mock_pcnt_count = 13;
    TEST_ASSERT_EQUAL_UINT16(7800, rpm_read());
}

/* ============================================================
 * PCNT-Lesefehler -> 0
 * ============================================================ */

void test_pcnt_read_failure_returns_zero(void)
{
    g_mock_pcnt_get_ret = -1;  /* non-OK return */
    g_mock_pcnt_count = 10;
    TEST_ASSERT_EQUAL_UINT16(0, rpm_read());
}

/* ============================================================
 * Glaettung: gleitender Mittelwert (Window 5)
 * ============================================================ */

void test_smoothing_constant_input(void)
{
    /* 5 identical reads should return the same value */
    g_mock_pcnt_count = 10;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_UINT16(6000, rpm_read());
    }
}

void test_smoothing_averages_multiple_samples(void)
{
    /* First read: 6000. Second read: 7200 (pc=12).
     * After 2 samples, avg = (6000 + 7200) / 2 = 6600. */
    g_mock_pcnt_count = 10;
    rpm_read();  /* buffer: [6000], avg = 6000 */

    g_mock_pcnt_count = 12;
    uint16_t result = rpm_read();  /* buffer: [6000, 7200], avg = 6600 */
    TEST_ASSERT_EQUAL_UINT16(6600, result);
}

void test_smoothing_window_size_5(void)
{
    /* Fill the window with 5 samples of 6000, then inject one 7200.
     * Window (ring buffer of 5): after 6 reads with [6000,6000,6000,6000,6000]
     * then 7200 -> oldest 6000 replaced by 7200.
     * avg = (6000*4 + 7200) / 5 = 31200/5 = 6240 */
    g_mock_pcnt_count = 10;
    for (int i = 0; i < 5; i++) {
        rpm_read();
    }

    g_mock_pcnt_count = 12;  /* 7200 rpm */
    uint16_t result = rpm_read();
    TEST_ASSERT_EQUAL_UINT16(6240, result);
}

void test_smoothing_reduces_single_spike(void)
{
    /* 4 reads at 6000, then 1 spike at 8000 (clamped from pc=14->8400).
     * After 5 reads: buffer = [6000,6000,6000,6000,8000]
     * avg = (6000*4 + 8000)/5 = 32000/5 = 6400
     * (clamping happens after smoothing, but each individual value
     * is clamped before being added to the buffer) */
    g_mock_pcnt_count = 10;
    for (int i = 0; i < 4; i++) {
        rpm_read();
    }

    g_mock_pcnt_count = 14;  /* 8400 -> smoothed first, then... see below */
    uint16_t result = rpm_read();

    /* Smoothing: buffer has [6000,6000,6000,6000], add 8400
     * avg = (6000*4 + 8400)/5 = 32400/5 = 6480 -> clamped to 6480 */
    TEST_ASSERT_EQUAL_UINT16(6480, result);
}

/* ============================================================
 * Init: Puffer wird zurueckgesetzt
 * ============================================================ */

void test_init_resets_smoothing_buffer(void)
{
    /* Fill buffer with high values */
    g_mock_pcnt_count = 13;  /* 7800 rpm */
    for (int i = 0; i < 5; i++) {
        rpm_read();
    }

    /* Re-init should reset the buffer */
    rpm_init();

    /* After re-init, a single low sample should not be averaged
     * with the old high values */
    g_mock_pcnt_count = 1;  /* 600 rpm */
    uint16_t result = rpm_read();
    TEST_ASSERT_EQUAL_UINT16(600, result);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_zero_pulses_returns_zero);
    RUN_TEST(test_negative_pulses_returns_zero);
    RUN_TEST(test_rpm_formula_single_sample);
    RUN_TEST(test_rpm_formula_low_rpm);
    RUN_TEST(test_rpm_formula_high_rpm);
    RUN_TEST(test_clamping_at_redline);
    RUN_TEST(test_clamping_far_above_redline);
    RUN_TEST(test_clamping_just_below_redline);
    RUN_TEST(test_pcnt_read_failure_returns_zero);
    RUN_TEST(test_smoothing_constant_input);
    RUN_TEST(test_smoothing_averages_multiple_samples);
    RUN_TEST(test_smoothing_window_size_5);
    RUN_TEST(test_smoothing_reduces_single_spike);
    RUN_TEST(test_init_resets_smoothing_buffer);

    return UNITY_END();
}
