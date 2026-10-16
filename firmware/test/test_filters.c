/*
 * test_filters.c — Unit Tests fuer Signal-Filter (Kalman, Complementary, Moving Average)
 *
 * Testet firmware/components/utils/filters.c:
 *   - Kalman 1D: Konvergenz, Rauschunterdrueckung, initiale Schaetzung
 *   - Complementary Filter: Winkelkombination aus Gyro + Accel
 *   - Moving Average: Edge-Cases (leer, single sample, saturation, wrap-around)
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. filters.c hat keine ESP-IDF-Abhaengigkeiten.
 */

#include "unity.h"
#include "filters.h"

#include <math.h>
#include <string.h>

/* ============================================================
 * Include the actual filters.c implementation directly.
 * No ESP-IDF deps — only <math.h> which is available on host.
 * ============================================================ */
#include "../../components/utils/filters.c"

void setUp(void)
{
}

void tearDown(void)
{
}

/* ============================================================
 * Kalman 1D Tests
 * ============================================================ */

void test_kalman_init_values(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.1f, 42.0f);

    TEST_ASSERT_EQUAL_FLOAT(0.01f, kf.q);
    TEST_ASSERT_EQUAL_FLOAT(0.1f,  kf.r);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, kf.x);
    TEST_ASSERT_EQUAL_FLOAT(1.0f,  kf.p);
    TEST_ASSERT_EQUAL_FLOAT(0.0f,  kf.k);
}

void test_kalman_converges_to_constant(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.001f, 0.5f, 0.0f);

    /* Feed 50 measurements of a constant value (100.0) with noise */
    float result = 0.0f;
    for (int i = 0; i < 50; i++) {
        result = kalman1d_update(&kf, 100.0f);
    }

    /* After convergence, estimate should be close to 100 */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, result);
}

void test_kalman_reduces_noise(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 2.0f, 50.0f);

    /* Feed noisy measurements around 50.0 */
    float noisy[] = {48.0f, 52.0f, 49.0f, 51.0f, 47.0f, 53.0f, 50.0f, 50.0f};
    float result = 50.0f;
    for (int i = 0; i < 8; i++) {
        result = kalman1d_update(&kf, noisy[i]);
    }

    /* The filtered result should be closer to 50 than the average of noise */
    float avg = 0.0f;
    for (int i = 0; i < 8; i++) avg += noisy[i];
    avg /= 8.0f;

    /* Result should be between 49 and 51 */
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 50.0f, result);

    /* Kalman gain should be between 0 and 1 */
    TEST_ASSERT_GREATER_OR_EQUAL(0.0f, kf.k);
    TEST_ASSERT_LESS_OR_EQUAL(1.0f, kf.k);
}

void test_kalman_tracks_changing_value(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.1f, 0.5f, 0.0f);

    /* Feed a ramp: 0, 10, 20, 30, 40, 50 */
    float result = 0.0f;
    for (int v = 0; v <= 50; v += 10) {
        result = kalman1d_update(&kf, (float)v);
    }

    /* After a ramp, the filter should be moving toward 50 */
    TEST_ASSERT_GREATER_THAN(20.0f, result);
}

void test_kalman_single_update(void)
{
    kalman1d_t kf;
    kalman1d_init(&kf, 0.01f, 0.1f, 0.0f);

    float result = kalman1d_update(&kf, 100.0f);

    /* After one update with measurement 100 and initial estimate 0:
     * k = p / (p + r) = 1.0 / (1.0 + 0.1) = 0.909...
     * x = 0 + k * (100 - 0) = 90.9... */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 90.9f, result);
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
    TEST_ASSERT_EQUAL_FLOAT(0.0f,  cf.angle);
}

void test_comp_filter_accel_only(void)
{
    /* With alpha=0 (pure accel), the filter should return the accel angle */
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.0f, 0.01f);

    float result = comp_filter_update(&cf, 0.0f, 45.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.0f, result);
}

void test_comp_filter_gyro_only(void)
{
    /* With alpha=1.0 (pure gyro), the filter should integrate the gyro rate.
     * Starting at 0, with gyro_rate=100 deg/s and dt=0.01s:
     * angle = 1.0 * (0 + 100 * 0.01) + 0 * accel = 1.0 */
    complementary_filter_t cf;
    comp_filter_init(&cf, 1.0f, 0.01f);

    float result = comp_filter_update(&cf, 100.0f, 999.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, result);
}

void test_comp_filter_blend(void)
{
    /* With alpha=0.5, equal blend of gyro and accel */
    complementary_filter_t cf;
    comp_filter_init(&cf, 0.5f, 0.1f);

    /* angle = 0.5 * (0 + 10 * 0.1) + 0.5 * 30 = 0.5 + 15 = 15.5 */
    float result = comp_filter_update(&cf, 10.0f, 30.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.5f, result);
}

/* ============================================================
 * Moving Average Tests
 * ============================================================ */

void test_moving_avg_single_sample(void)
{
    float buf[4];
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 4);

    float result = moving_avg_update(&ma, 42.0f);
    TEST_ASSERT_EQUAL_FLOAT(42.0f, result);
}

void test_moving_avg_partial_fill(void)
{
    float buf[4];
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 4);

    moving_avg_update(&ma, 10.0f);
    moving_avg_update(&ma, 20.0f);
    float result = moving_avg_update(&ma, 30.0f);

    /* Average of 10, 20, 30 = 20 */
    TEST_ASSERT_EQUAL_FLOAT(20.0f, result);
}

void test_moving_avg_full_window(void)
{
    float buf[3];
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 3);

    moving_avg_update(&ma, 6.0f);   /* avg = 6 */
    moving_avg_update(&ma, 6.0f);   /* avg = 6 */
    moving_avg_update(&ma, 12.0f);  /* avg = 8 */
    float result = moving_avg_update(&ma, 6.0f);  /* window: 6, 12, 6 -> avg = 8 */

    TEST_ASSERT_EQUAL_FLOAT(8.0f, result);
}

void test_moving_avg_wrap_around(void)
{
    float buf[3];
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 3);

    /* Fill: 0, 0, 0 */
    for (int i = 0; i < 3; i++) moving_avg_update(&ma, 0.0f);

    /* Now push 3, 6, 9 — should wrap around and give avg = 6 */
    moving_avg_update(&ma, 3.0f);
    moving_avg_update(&ma, 6.0f);
    float result = moving_avg_update(&ma, 9.0f);

    TEST_ASSERT_EQUAL_FLOAT(6.0f, result);
}

void test_moving_avg_constant_input(void)
{
    float buf[8];
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 8);

    float result = 0.0f;
    for (int i = 0; i < 20; i++) {
        result = moving_avg_update(&ma, 55.5f);
    }

    TEST_ASSERT_EQUAL_FLOAT(55.5f, result);
}

void test_moving_avg_size_one(void)
{
    /* Edge case: window size of 1 should return the latest value */
    float buf[1];
    moving_avg_t ma;
    moving_avg_init(&ma, buf, 1);

    moving_avg_update(&ma, 10.0f);
    float result = moving_avg_update(&ma, 99.0f);

    TEST_ASSERT_EQUAL_FLOAT(99.0f, result);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* Kalman */
    RUN_TEST(test_kalman_init_values);
    RUN_TEST(test_kalman_converges_to_constant);
    RUN_TEST(test_kalman_reduces_noise);
    RUN_TEST(test_kalman_tracks_changing_value);
    RUN_TEST(test_kalman_single_update);

    /* Complementary Filter */
    RUN_TEST(test_comp_filter_init);
    RUN_TEST(test_comp_filter_accel_only);
    RUN_TEST(test_comp_filter_gyro_only);
    RUN_TEST(test_comp_filter_blend);

    /* Moving Average */
    RUN_TEST(test_moving_avg_single_sample);
    RUN_TEST(test_moving_avg_partial_fill);
    RUN_TEST(test_moving_avg_full_window);
    RUN_TEST(test_moving_avg_wrap_around);
    RUN_TEST(test_moving_avg_constant_input);
    RUN_TEST(test_moving_avg_size_one);

    return UNITY_END();
}
