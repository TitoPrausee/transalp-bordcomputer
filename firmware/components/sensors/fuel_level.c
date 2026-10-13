/*
 * fuel_level.c — Kraftstoffstand ueber ADC
 *
 * Kraftstoffgeber (Widerstandsschwimmer) ueber Spannungsteiler an ADC.
 *
 * TODO: Kennlinie des Gebers messen (Widerstand -> Fuellstand).
 * TODO: Tankvolumen der Transalp XL600V eintragen (ca. 22 Liter).
 * TODO: Filterung (Moving Average) gegen Schwappen.
 */

#include "sensors.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "fuel";

#define TANK_CAPACITY_L      22.0f   /* TODO: verifizieren */
#define FUEL_ADC_CHANNEL     ADC_CHANNEL_4   /* TODO: korrekten Channel pruefen */

/* TODO: ADC-Handles (shared mit temperature.c oder separat) */

void fuel_level_init(void)
{
    /* TODO: ADC-Channel fuer Kraftstoffgeber konfigurieren.
     * TODO: Kennlinie kalibrieren (leer/voll Messwerte).
     */
    ESP_LOGI(TAG, "Fuel level ADC init (stub)");
}

float fuel_level_read_liters(void)
{
    /* TODO: ADC lesen, Spannung -> Widerstand -> Liter ueber Kennlinie.
     * TODO: Moving-Average-Filter gegen Schwappen beim Kurvenfahren.
     */
    return -1.0f;  /* Stub */
}
