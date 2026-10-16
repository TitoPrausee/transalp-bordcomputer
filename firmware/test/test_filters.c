/*
 * test_filters.c — Unit Tests fuer Signal-Filter (Kalman, Complementary, Moving-Avg)
 *
 * Testet:
 *   - Kalman 1D: Konvergenz, Rauschunterdrueckung, Edge-Cases
 *   - Complementary Filter: Winkel-Berechnung, Gewichtung
 *   - Moving Average: Fuell-Verhalten, Ueberlauf, Mittelwert
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. Keine ESP-IDF-Abhaengigkeiten.
 */

#include "unity.h"
#include "filters.h"

#include <string.h>
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

/* ============================================================
 * Kalman 1D Tests
 * ============================================================ */

void test_kalman_init(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.1f, 50.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.01f, kf.q);
    TEST_ASSERT_EQUAL_FLOAT(0.1f, kf.r);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, kf.x);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, kf.p);
}

void test_kalman_convergence(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.1f, 0.0f);

    /* Feed a constant value of 100 — Kalman should converge toward it */
    float result = 0.0f;
    for (int i = 0; i < 100; i++) {
        result = kalman1d_update(&kf, 100.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result);
}

void test_kalman_noise_suppression(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.001f, 1.0f, 50.0f);

    /* Feed noisy data around 50 — Kalman should pull toward 50 */
    float raw_values[] = {45, 55, 48, 52, 47, 53, 49, 51, 46, 54};
    float result = 50.0f;
    for (int i = 0; i < 10; i++) {
        result = kalman1d_update(&kf, raw_values[i]);
    }
    /* With only 10 samples and high r, result should be between 49 and 51 */
    TEST_ASSERT(result > 49.0f && result < 51.0f);
}

void test_kalman_single_update(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.1f, 0.0f);
    float result = kalman1d_update(&kf, 10.0f);
    /* After one update, result should be between initial (0) and measurement (10) */
    TEST_ASSERT(result > 0.0f && result < 10.0f);
}

void test_kalman_zero_measurement(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.1f, 100.0f);
    float result = kalman1d_update(&kf, 0.0f);
    /* Should move toward 0 but not jump */
    TEST_ASSERT(result < 100.0f && result >= 0.0f);
}

/* ============================================================
 * Complementary Filter Tests
 * ============================================================ */

void test_comp_filter_init(void)
{
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.98f, 0.01f);
    TEST_ASSERT_EQUAL_FLOAT(0.98f, cf.alpha);
    TEST_ASSERT_EQUAL_FLOAT(0.01f, cf.dt);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cf.angle);
}

void test_comp_filter_steady_state(void)
{
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.98f, 0.01f);

    /* No rotation, constant accel angle of 45 degrees */
    float result = 0.0f;
    for (int i = 0; i < 1000; i++) {
        result = comp_filter_update(&cf, 0.0f, 45.0f);
    }
    /* Should converge to the accel angle */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 45.0f, result);
}

void test_comp_filter_gyro_dominant(void)
{
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.98f, 0.01f);

    /* High alpha = gyro dominant. With constant gyro rate and zero accel angle,
     * the angle should drift in the direction of the gyro rate. */
    float result = 0.0f;
    for (int i = 0; i < 100; i++) {
        result = comp_filter_update(&cf, 10.0f, 0.0f);
    }
    /* Angle should be positive (drifting from gyro) */
    TEST_ASSERT(result > 0.0f);
}

void test_comp_filter_accel_dominant(void)
{
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.5f, 0.01f);

    /* Low alpha = accel dominant */
    float result = comp_filter_update(&cf, 0.0f, 30.0f);
    /* With alpha=0.5, result should be closer to accel angle */
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 15.0f, result);
}

/* ============================================================
 * Moving Average Tests
 * ============================================================ */

void test_moving_avg_init(void)
{
    float buf[5] = {0};
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 5);
    TEST_ASSERT_EQUAL(5, ma.size);
    TEST_ASSERT_EQUAL(0, ma.count);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ma.sum);
}

void test_moving_avg_partial_fill(void)
{
    float buf[5] = {0};
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 5);

    float result = moving_avg_update(&ma, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);

    result = moving_avg_update(&ma, 20.0f);
    TEST_ASSERT_EQUAL_FLOAT(15.0f, result);

    result = moving_avg_update(&ma, 30.0f);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, result);
}

void test_moving_avg_full_buffer(void)
{
    float buf[3] = {0};
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 3);

    moving_avg_update(&ma, 10.0f);
    moving_avg_update(&ma, 20.0f);
    moving_avg_update(&ma, 30.0f);
    float result = moving_avg_update(&ma, 40.0f);

    /* Buffer: 20, 30, 40 -> avg = 30 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, result);
}

void test_moving_avg_wraparound(void)
{
    float buf[3] = {0};
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 3);

    /* Fill and wrap around multiple times */
    for (int i = 0; i < 10; i++) {
        moving_avg_update(&ma, (float)(i + 1));
    }
    /* Last 3 values: 8, 9, 10 -> avg = 9 */
    float result = ma.sum / ma.count;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.0f, result);
}

void test_moving_avg_single_sample(void)
{
    float buf[1] = {0};
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 1);

    float result = moving_avg_update(&ma, 42.0f);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, result);

    result = moving_avg_update(&ma, 84.0f);
    TEST_ASSERT_EQUAL_FLOAT(84.0f, result);
}

void test_moving_avg_constant_input(void)
{
    float buf[5] = {0};
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 5);

    for (int i = 0; i < 10; i++) {
        moving_avg_update(&ma, 100.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, ma.sum / ma.count);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* Kalman */
    RUN_TEST(test_kalman_init);
    RUN_TEST(test_kalman_convergence);
    RUN_TEST(test_kalman_noise_suppression);
    RUN_TEST(test_kalman_single_update);
    RUN_TEST(test_kalman_zero_measurement);

    /* Complementary */
    RUN_TEST(test_comp_filter_init);
    RUN_TEST(test_comp_filter_steady_state);
    RUN_TEST(test_comp_filter_gyro_dominant);
    RUN_TEST(test_comp_filter_accel_dominant);

    /* Moving Average */
    RUN_TEST(test_moving_avg_init);
    RUN_TEST(test_moving_avg_partial_fill);
    RUN_TEST(test_moving_avg_full_buffer);
    RUN_TEST(test_moving_avg_wraparound);
    RUN_TEST(test_moving_avg_single_sample);
    RUN_TEST(test_moving_avg_constant_input);

    return UNITY_END();
}
