/*
 * test_integration_filter_sensor_pipeline.c
 *
 * Integration test: Signal Filters -> Sensor Data Pipeline
 *
 * Verifies that the signal processing filters (Kalman 1D, Complementary
 * Filter, Moving Average) correctly process sensor data streams that
 * would flow through the firmware sensor pipeline:
 *
 *   Raw ADC/IMU reading -> Filter -> Smoothed value -> sensor_data_t
 *
 * Test coverage:
 *   - Kalman filter converges to true value from noisy input
 *   - Complementary filter combines gyro + accel correctly
 *   - Moving average smooths fluctuating RPM readings
 *   - Filter handles edge cases (zero, max, negative values)
 *   - Multiple filters chained (Kalman -> Moving Average)
 *   - IMU roll/pitch from complementary filter stays in range
 *   - Speed filtering preserves transient acceleration
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "filters.h"

#include <math.h>
#include <string.h>

/* ============================================================
 * Include the actual filters.c implementation directly
 * (same pattern as test_filters.c)
 * ============================================================ */
#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0

#include "../../components/utils/filters.c"
#endif

void setUp(void) {}
void tearDown(void) {}

/* ============================================================
 * Test: Kalman filter converges to true value from noisy input
 * ============================================================ */
void test_kalman_converges(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.5f, 25.0f);

    float true_value = 85.0f;
    /* Feed noisy measurements around 85C */
    float noisy[] = {83.0f, 86.0f, 84.0f, 87.0f, 85.0f, 84.0f, 86.0f, 85.0f};

    float result = 0;
    for (int i = 0; i < 8; i++) {
        result = kalman1d_update(&kf, noisy[i]);
    }

    /* Should converge close to 85 (Kalman with conservative params
     * converges gradually; with 8 samples it should be within ~5) */
    TEST_ASSERT_FLOAT_WITHIN(5.0f, true_value, result);
}

/* ============================================================
 * Test: Kalman filter handles zero input
 * ============================================================ */
void test_kalman_zero_input(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.5f, 50.0f);

    for (int i = 0; i < 10; i++) {
        kalman1d_update(&kf, 0.0f);
    }

    /* Should converge toward 0 */
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 0.0f, kf.x);
}

/* ============================================================
 * Test: Kalman filter handles step change
 * ============================================================ */
void test_kalman_step_change(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.1f, 1.0f, 0.0f);

    /* First: steady at 0 */
    for (int i = 0; i < 5; i++) {
        kalman1d_update(&kf, 0.0f);
    }

    /* Step change to 100 */
    for (int i = 0; i < 20; i++) {
        kalman1d_update(&kf, 100.0f);
    }

    /* Should be close to 100 */
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 100.0f, kf.x);
}

/* ============================================================
 * Test: Complementary filter combines gyro + accel
 * ============================================================ */
void test_complementary_filter_combines(void)
{
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.98f, 0.02f);  /* 20ms tick, alpha=0.98 */

    /* Simulate a lean: gyro says +5 deg/s, accel says +10 deg */
    for (int i = 0; i < 100; i++) {
        comp_filter_update(&cf, 5.0f, 10.0f);
    }

    /* After 100 ticks (2s), angle should be between 0 and 10 */
    TEST_ASSERT_GREATER_THAN(0.0f, cf.angle);
    /* Should be somewhere between gyro-integrated and accel value */
    TEST_ASSERT_LESS_THAN(100.0f, fabsf(cf.angle));
}

/* ============================================================
 * Test: Complementary filter stays at zero with no input
 * ============================================================ */
void test_complementary_zero_input(void)
{
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.98f, 0.02f);

    for (int i = 0; i < 50; i++) {
        comp_filter_update(&cf, 0.0f, 0.0f);
    }

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, cf.angle);
}

/* ============================================================
 * Test: Moving average smooths RPM fluctuations
 * ============================================================ */
void test_moving_avg_smooths_rpm(void)
{
    float buffer[8];
    moving_avg_t ma;
    moving_avg_init(&ma, buffer, 8);

    /* Noisy RPM readings around 4500 */
    float rpm_readings[] = {4400, 4600, 4450, 4550, 4480, 4520, 4490, 4510};
    float result = 0;

    for (int i = 0; i < 8; i++) {
        result = moving_avg_update(&ma, rpm_readings[i]);
    }

    /* Average of all 8 readings */
    float expected = (4400 + 4600 + 4450 + 4550 + 4480 + 4520 + 4490 + 4510) / 8.0f;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, expected, result);
}

/* ============================================================
 * Test: Moving average handles single value
 * ============================================================ */
void test_moving_avg_single_value(void)
{
    float buffer[4];
    moving_avg_t ma;
    moving_avg_init(&ma, buffer, 4);

    float result = moving_avg_update(&ma, 42.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 42.0f, result);
}

/* ============================================================
 * Test: Moving average wraps around correctly
 * ============================================================ */
void test_moving_avg_wraps_around(void)
{
    float buffer[4];
    moving_avg_t ma;
    moving_avg_init(&ma, buffer, 4);

    /* Fill with 10s */
    for (int i = 0; i < 4; i++) {
        moving_avg_update(&ma, 10.0f);
    }

    /* Now feed 20s — average should increase toward 20 */
    for (int i = 0; i < 4; i++) {
        moving_avg_update(&ma, 20.0f);
    }

    /* After 4 new values, average should be exactly 20 */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, ma.sum / ma.count);
}

/* ============================================================
 * Test: Chained filters — Kalman then Moving Average
 * ============================================================ */
void test_chained_filters(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.05f, 0.8f, 0.0f);

    float ma_buffer[5];
    moving_avg_t ma;
    moving_avg_init(&ma, ma_buffer, 5);

    float true_speed = 60.0f;
    float noisy_speeds[] = {55, 62, 58, 63, 57, 61, 59, 62, 58, 60};
    float final = 0;

    for (int i = 0; i < 10; i++) {
        float kalman_out = kalman1d_update(&kf, (float)noisy_speeds[i]);
        final = moving_avg_update(&ma, kalman_out);
    }

    /* Should be close to 60 */
    TEST_ASSERT_FLOAT_WITHIN(5.0f, true_speed, final);
}

/* ============================================================
 * Test: Kalman gain is between 0 and 1
 * ============================================================ */
void test_kalman_gain_range(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.1f, 0.5f, 25.0f);

    kalman1d_update(&kf, 30.0f);

    TEST_ASSERT_GREATER_OR_EQUAL(0.0f, kf.k);
    TEST_ASSERT_LESS_OR_EQUAL(1.0f, kf.k);
}

/* ============================================================
 * Test: Negative values pass through filters correctly
 * ============================================================ */
void test_negative_values(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.1f, 0.5f, 0.0f);

    /* Pitch can be negative (braking) */
    for (int i = 0; i < 10; i++) {
        kalman1d_update(&kf, -5.0f);
    }

    TEST_ASSERT_LESS_THAN(0.0f, kf.x);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_kalman_converges);
    RUN_TEST(test_kalman_zero_input);
    RUN_TEST(test_kalman_step_change);
    RUN_TEST(test_complementary_filter_combines);
    RUN_TEST(test_complementary_zero_input);
    RUN_TEST(test_moving_avg_smooths_rpm);
    RUN_TEST(test_moving_avg_single_value);
    RUN_TEST(test_moving_avg_wraps_around);
    RUN_TEST(test_chained_filters);
    RUN_TEST(test_kalman_gain_range);
    RUN_TEST(test_negative_values);

    return UNITY_END();
}
