/*
 * test_gear_indicator.c — Unit Tests fuer Gang-Erkennung
 *
 * Testet firmware/components/sensors/gear_indicator.c:
 *   - Neutral-Erkennung (Speed/RPM unter Schwellwert)
 *   - Jeder Gang 1-6 bei exakter Speed/RPM-Kombination
 *   - Toleranzfenster (+/-5%): gerade noch erkannt, gerade nicht mehr
 *   - Kupplung getreten -> Neutral bei unpassendem Verhaeltnis
 *   - Grenzwerte: Minimal-Speed, Minimal-RPM
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. gear_indicator.c hat nur esp_log als ESP-IDF-Abhaengigkeit,
 * die ueber den Mock ersetzt wird.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "sensors.h"

#include <math.h>

/* ============================================================
 * Host build: neutralize ESP_LOG macros and include the real
 * gear_indicator.c implementation directly. gear_indicator.c
 * depends only on sensors.h, esp_log.h and <math.h>.
 * ============================================================ */
#ifdef TEST_HOST

static inline void _gi_log_noop(const char *tag, const char *fmt, ...) {
    (void)tag; (void)fmt;
}

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV
#define ESP_LOGE(tag, fmt, ...) _gi_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) _gi_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) _gi_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) _gi_log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) _gi_log_noop(tag, fmt, ##__VA_ARGS__)

#include "../../components/sensors/gear_indicator.c"

#endif /* TEST_HOST */

/* ============================================================
 * Helper: expected engine RPM for a given road speed and gear.
 * Mirrors the formula in gear_indicator_calc():
 *   n_mot = v * i_ges * 1000 / (60 * U_reifen)
 * with i_ges = PRIMARY_RATIO * s_gear_ratios[g-1] * FINAL_RATIO
 * ============================================================ */
static float expected_rpm_for_gear(float speed_kmh, int gear)
{
    /* s_gear_ratios is static in the included .c; reuse it directly. */
    float total = PRIMARY_RATIO * s_gear_ratios[gear - 1] * FINAL_RATIO;
    return speed_kmh * total * 1000.0f / (60.0f * WHEEL_CIRCUMFERENCE);
}

void setUp(void)
{
    gear_indicator_init();
}

void tearDown(void)
{
}

/* ============================================================
 * Neutral: Speed zu gering
 * ============================================================ */
void test_neutral_when_speed_below_threshold(void)
{
    /* Engine revving but bike barely moving -> Neutral */
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(0.5f, 3000));
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(0.99f, 6000));
}

/* ============================================================
 * Neutral: RPM unter Schwellwert (Motor dreht nicht)
 * ============================================================ */
void test_neutral_when_rpm_below_threshold(void)
{
    /* Rolling but engine below 500 rpm -> Neutral */
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(50.0f, 499));
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(50.0f, 0));
}

/* ============================================================
 * Jeder Gang 1-6 bei exakter Speed/RPM-Kombination
 * ============================================================ */
void test_each_gear_at_exact_ratio(void)
{
    /* Use a speed that keeps expected RPM realistic (< 8000). */
    const float speed = 60.0f;

    for (int g = 1; g <= GEAR_COUNT; g++) {
        float exp_rpm = expected_rpm_for_gear(speed, g);
        uint16_t rpm = (uint16_t)(exp_rpm + 0.5f);
        uint8_t result = gear_indicator_calc(speed, rpm);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            (uint8_t)g, result, "gear mismatch at exact ratio");
    }
}

/* ============================================================
 * Toleranzfenster: +4% Abweichung wird noch erkannt
 * ============================================================ */
void test_gear_recognized_within_tolerance(void)
{
    const float speed = 80.0f;

    for (int g = 1; g <= GEAR_COUNT; g++) {
        float exp_rpm = expected_rpm_for_gear(speed, g);
        /* +4% deviation — within the 5% tolerance window */
        uint16_t rpm = (uint16_t)(exp_rpm * 1.04f + 0.5f);
        uint8_t result = gear_indicator_calc(speed, rpm);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            (uint8_t)g, result, "gear not recognized within +4% tolerance");
    }
}

/* ============================================================
 * Toleranzfenster: +6% Abweichung -> Neutral (ausserhalb)
 * ============================================================ */
void test_neutral_when_deviation_exceeds_tolerance(void)
{
    const float speed = 80.0f;

    for (int g = 1; g <= GEAR_COUNT; g++) {
        float exp_rpm = expected_rpm_for_gear(speed, g);
        /* +6% deviation — outside the 5% tolerance -> Neutral */
        uint16_t rpm = (uint16_t)(exp_rpm * 1.06f + 0.5f);
        uint8_t result = gear_indicator_calc(speed, rpm);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            0, result, "gear wrongly recognized outside tolerance");
    }
}

/* ============================================================
 * Toleranzfenster: RPM zwischen zwei Gaengen, >5% von beiden
 *
 * Hinweis: Eine einfache prozentuale Abweichung von einem Gang
 * kann innerhalb der 5%-Toleranz des benachbarten Gangs liegen
 * (die Uebersetzungen liegen bei hohen Gaengen dicht beieinander).
 * Daher wird hier ein RPM gewaehlt, das von allen Gaengen mehr
 * als 5% entfernt ist -> Neutral.
 * ============================================================ */
void test_neutral_when_rpm_between_gears(void)
{
    /* At 80 km/h: gear 6 expects ~4362 rpm, gear 5 expects ~4867 rpm.
     * An rpm of 12000 is far from every gear's expected value. */
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(80.0f, 12000));

    /* At 50 km/h, gear 1 expects ~8761 rpm, gear 2 expects ~5964 rpm.
     * An rpm of 7300 sits in the large gap between gears 1 and 2,
     * more than 5% from either -> Neutral. */
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(50.0f, 7300));
}

/* ============================================================
 * Best-Match-Auswahl: RPM passt zu Gang 3, nicht zu Gang 2/4
 * ============================================================ */
void test_best_match_selection(void)
{
    /* At 70 km/h, gear 3 expected rpm should be closest. */
    float speed = 70.0f;
    float exp3 = expected_rpm_for_gear(speed, 3);
    uint16_t rpm = (uint16_t)(exp3 + 0.5f);

    TEST_ASSERT_EQUAL_UINT8(3, gear_indicator_calc(speed, rpm));
}

/* ============================================================
 * Hohe Geschwindigkeit im hoechsten Gang
 * ============================================================ */
void test_sixth_gear_at_highway_speed(void)
{
    /* 130 km/h in 6th gear */
    float speed = 130.0f;
    float exp6 = expected_rpm_for_gear(speed, 6);
    uint16_t rpm = (uint16_t)(exp6 + 0.5f);

    TEST_ASSERT_EQUAL_UINT8(6, gear_indicator_calc(speed, rpm));
}

/* ============================================================
 * Erster Gang bei niedriger Geschwindigkeit
 * ============================================================ */
void test_first_gear_low_speed(void)
{
    /* 25 km/h in 1st gear */
    float speed = 25.0f;
    float exp1 = expected_rpm_for_gear(speed, 1);
    uint16_t rpm = (uint16_t)(exp1 + 0.5f);

    TEST_ASSERT_EQUAL_UINT8(1, gear_indicator_calc(speed, rpm));
}

/* ============================================================
 * Grenzwert: Minimal-Speed bei dem ein Gang erkannt werden kann.
 *
 * Bei 1.0 km/h liegt die erwartete RPM fuer jeden Gang unterhalb
 * von GEAR_MIN_RPM (500), daher ist die Speed-Grenze in der Praxis
 * immer mit Neutral belegt. Stattdessen testen wir die kleinste
 * Speed, bei der Gang 1 eine erwartete RPM >= 500 liefert (~3 km/h).
 * ============================================================ */
void test_boundary_min_speed(void)
{
    /* gear 1 at 3 km/h: exp = 3 * 20.609 * 8.503 ~= 525 rpm (>= 500) */
    float speed = 3.0f;
    float exp1 = expected_rpm_for_gear(speed, 1);
    uint16_t rpm = (uint16_t)(exp1 + 0.5f);

    /* Speed >= GEAR_MIN_SPEED_KMH and rpm >= GEAR_MIN_RPM -> active */
    uint8_t result = gear_indicator_calc(speed, rpm);
    TEST_ASSERT_EQUAL_UINT8(1, result);

    /* Just below the speed threshold -> Neutral regardless of rpm */
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(0.99f, rpm));
}

/* ============================================================
 * Grenzwert: genau Minimal-RPM (500)
 * ============================================================ */
void test_boundary_min_rpm(void)
{
    /* Find a speed/gear where expected_rpm ~= 500 */
    /* Gear 6 at low speed: exp = speed * 6.411 * 8.503 = speed * 54.52
     * For 500 rpm: speed = 500/54.52 = 9.17 km/h */
    float speed = 9.17f;
    float exp6 = expected_rpm_for_gear(speed, 6);
    uint16_t rpm = 500;

    /* rpm == 500 is the boundary (>= GEAR_MIN_RPM). Should be active. */
    uint8_t result = gear_indicator_calc(speed, rpm);
    /* exp6 should be near 500; result should be 6 if within tolerance */
    TEST_ASSERT_EQUAL_UINT8(6, result);
}

/* ============================================================
 * Kupplung getreten: RPM sehr hoch bei niedriger Speed -> Neutral
 * ============================================================ */
void test_clutch_in_neutral(void)
{
    /* Low speed but very high rpm — doesn't match any gear ratio */
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(10.0f, 7000));
    TEST_ASSERT_EQUAL_UINT8(0, gear_indicator_calc(5.0f, 5000));
}

/* ============================================================
 * Negative Toleranz: -4% Abweichung wird noch erkannt
 * (TOT-62: testet die untere Seite des Toleranzfensters)
 * ============================================================ */
void test_gear_recognized_within_negative_tolerance(void)
{
    const float speed = 80.0f;

    for (int g = 1; g <= GEAR_COUNT; g++) {
        float exp_rpm = expected_rpm_for_gear(speed, g);
        /* -4% deviation — within the 5% tolerance window */
        uint16_t rpm = (uint16_t)(exp_rpm * 0.96f + 0.5f);
        uint8_t result = gear_indicator_calc(speed, rpm);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            (uint8_t)g, result, "gear not recognized within -4% tolerance");
    }
}

/* ============================================================
 * Ausserhalb Toleranz (negativ): -10% von Gear 1 -> Neutral
 *
 * Ein -10% deviation von Gear 1 ist weit genug von allen benachbarten
 * Gaengen entfernt, dass kein anderer Gang innerhalb 5% liegt.
 * (TOT-62: negative Seite der Toleranzgrenze)
 * ============================================================ */
void test_neutral_when_negative_deviation_exceeds_tolerance(void)
{
    const float speed = 80.0f;
    float exp1 = expected_rpm_for_gear(speed, 1);
    /* -10% deviation from gear 1 — far from all gears */
    uint16_t rpm = (uint16_t)(exp1 * 0.90f + 0.5f);
    uint8_t result = gear_indicator_calc(speed, rpm);
    TEST_ASSERT_EQUAL_UINT8(0, result);
}

/* ============================================================
 * Grenzwert: +4.5% Abweichung -> noch erkannt (klar innerhalb 5%)
 *
 * Der exakte 5%-Grenzwert ist durch uint16_t-Rundung und Float-
 * Praezision fragil (diff kann 0.0501 werden). Wir testen stattdessen
 * einen Wert, der sicher innerhalb der Toleranz liegt.
 * ============================================================ */
void test_gear_just_inside_tolerance_boundary(void)
{
    const float speed = 80.0f;
    float exp_rpm = expected_rpm_for_gear(speed, 6);
    /* +4.5% — clearly within the 5% tolerance window */
    uint16_t rpm = (uint16_t)(exp_rpm * 1.045f + 0.5f);
    uint8_t result = gear_indicator_calc(speed, rpm);
    TEST_ASSERT_EQUAL_UINT8(6, result);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_neutral_when_speed_below_threshold);
    RUN_TEST(test_neutral_when_rpm_below_threshold);
    RUN_TEST(test_each_gear_at_exact_ratio);
    RUN_TEST(test_gear_recognized_within_tolerance);
    RUN_TEST(test_neutral_when_deviation_exceeds_tolerance);
    RUN_TEST(test_neutral_when_rpm_between_gears);
    RUN_TEST(test_best_match_selection);
    RUN_TEST(test_sixth_gear_at_highway_speed);
    RUN_TEST(test_first_gear_low_speed);
    RUN_TEST(test_boundary_min_speed);
    RUN_TEST(test_boundary_min_rpm);
    RUN_TEST(test_clutch_in_neutral);
    RUN_TEST(test_gear_recognized_within_negative_tolerance);
    RUN_TEST(test_neutral_when_negative_deviation_exceeds_tolerance);
    RUN_TEST(test_gear_just_inside_tolerance_boundary);

    return UNITY_END();
}
