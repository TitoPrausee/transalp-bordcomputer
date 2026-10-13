/*
 * gear_indicator.c — Gang-Erkennung aus Speed und RPM
 *
 * Berechnet den aktuellen Gang aus dem Verhaeltnis von
 * Geschwindigkeit zu Motordrehzahl.
 *
 * Uebersetzungskette:
 *   Primaer:  2.733 (Kurbelwelle -> Kupplung)
 *   Gang:     2.571, 1.750, 1.316, 1.045, 0.893, 0.800
 *   Sekundaer: 2.933 (15/44 Zaehne)
 *   Radumfang: ~1.96m (130/80-17 Reifen)
 *
 * Gesamtuebersetzung pro Gang = Primaer * Gang * Sekundaer
 * RPM = speed_kmh * Gesamt * 1000 / (60 * Radumfang)
 *
 * Neutral-Erkennung: RPM/Speed-Ratio sehr niedrig oder
 * Speed ~0 bei laufendem Motor.
 *
 * Toleranzfenster ±5% fuer Zuordnung.
 */

#include "sensors.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "gear";

/* Getriebe-Uebersetzungen (6-Gang) */
#define GEAR_COUNT           6
#define PRIMARY_RATIO        2.733f
#define FINAL_RATIO          2.933f   /* 15/44 Zaehne */
#define WHEEL_CIRCUMFERENCE  1.96f    /* 130/80-17 Reifen, ~1.96m */

/* Toleranzfenster: 5% relative Abweichung */
#define GEAR_TOLERANCE       0.05f

/* Schwellwerte fuer Neutral-Erkennung */
#define GEAR_MIN_RPM         500      /* Mindest-Drehzahl fuer Gang-Erkennung */
#define GEAR_MIN_SPEED_KMH   1.0f    /* Mindest-Speed fuer Gang-Erkennung */

static const float s_gear_ratios[GEAR_COUNT] = {
    2.571f,   /* Gang 1 */
    1.750f,   /* Gang 2 */
    1.316f,   /* Gang 3 */
    1.045f,   /* Gang 4 */
    0.893f,   /* Gang 5 */
    0.800f,   /* Gang 6 */
};

void gear_indicator_init(void)
{
    ESP_LOGI(TAG, "Gear indicator init (gears=%d, primary=%.3f, final=%.3f, wheel=%.2fm)",
             GEAR_COUNT, (double)PRIMARY_RATIO, (double)FINAL_RATIO,
             (double)WHEEL_CIRCUMFERENCE);
}

uint8_t gear_indicator_calc(float speed_kmh, uint16_t rpm)
{
    /* Neutral/Stand: Speed zu gering oder Motor dreht nicht */
    if (speed_kmh < GEAR_MIN_SPEED_KMH || rpm < GEAR_MIN_RPM) {
        return 0;  /* Neutral */
    }

    /* Erwartete RPM pro Gang bei aktueller Geschwindigkeit berechnen
     * und mit tatsaechlicher RPM vergleichen.
     *
     * v = (n_mot / (i_prim * i_final * i_gear)) * U_reifen * 60 / 1000
     * -> n_mot = v * i_ges * 1000 / (60 * U_reifen)
     */
    float best_diff = 1.0f;  /* relative Abweichung (initial > Toleranz) */
    uint8_t best_gear = 0;

    for (int g = 0; g < GEAR_COUNT; g++) {
        float total_ratio = PRIMARY_RATIO * s_gear_ratios[g] * FINAL_RATIO;
        float expected_rpm = speed_kmh * total_ratio * 1000.0f
                              / (60.0f * WHEEL_CIRCUMFERENCE);

        if (expected_rpm < 1.0f) {
            continue;
        }

        float diff = fabsf((float)rpm - expected_rpm) / expected_rpm;

        if (diff < best_diff) {
            best_diff = diff;
            best_gear = (uint8_t)(g + 1);
        }
    }

    /* Nur wenn Abweichung im Toleranzfenster — sonst Neutral */
    if (best_diff > GEAR_TOLERANCE) {
        return 0;  /* Kupplung getreten / Schaltvorgang -> Neutral */
    }

    return best_gear;
}
