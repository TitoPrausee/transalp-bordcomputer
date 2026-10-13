/*
 * gear_indicator.c — Gang-Erkennung aus Speed und RPM
 *
 * Berechnet den aktuellen Gang aus dem Verhaeltnis von
 * Geschwindigkeit zu Drehzahl (Wheel-RPM / Engine-RPM Ratio).
 *
 * Transalp XL600V: 5-Gang-Getriebe.
 *
 * TODO: Uebersetzungstabelle fuer jeden Gang messen (Rollerpruefstand).
 * TODO: Toleranzen fuer Kupplungsrutschen/Schlupf einbauen.
 * TODO: Neutral-Erkennung (Neutral-Schalter falls vorhanden, sonst Ratio ~= 0).
 */

#include "sensors.h"
#include "esp_log.h"

static const char *TAG = "gear";

/* Typische Ratio-Bereiche (Speed_kmh / RPM) pro Gang.
 * Diese Werte muessen am Fahrzeug gemessen werden! */
static const float gear_ratios[6] = {
    0.0f,   /* Neutral (kein definiertes Verhaeltnis) */
    0.008f, /* Gang 1 */
    0.014f, /* Gang 2 */
    0.020f, /* Gang 3 */
    0.026f, /* Gang 4 */
    0.032f, /* Gang 5 */
};
#define GEAR_RATIO_TOLERANCE  0.003f

void gear_indicator_init(void)
{
    ESP_LOGI(TAG, "Gear indicator init (stub)");
}

uint8_t gear_indicator_calc(float speed_kmh, uint16_t rpm)
{
    /* TODO: Ratio berechnen und gegen Tabelle matchen.
     * if (rpm < 500 || speed_kmh < 1.0f) return 0;  // Neutral/Stand
     * float ratio = speed_kmh / (float)rpm;
     * for (int g = 1; g <= 5; g++) {
     *     if (fabs(ratio - gear_ratios[g]) < GEAR_RATIO_TOLERANCE) return g;
     * }
     */
    return 0;  /* Stub: Neutral */
}
