/*
 * fuel_level.c — Kraftstoffstand ueber ADC
 *
 * Kraftstoffgeber (Widerstandsschwimmer) ueber Spannungsteiler an ADC.
 * Pin: ADC1_CH2 (siehe board_config.h FUEL_ADC_CHANNEL)
 *
 * Kalibrierungstabelle (ADC-Wert -> Liter) fuer 17-Liter-Tank.
 * Glaettung: gleitender Mittelwert ueber 10 Messungen.
 * Warnung bei <3 Liter Reserve.
 */

#include "sensors.h"
#include "board_config.h"
#include "esp_log.h"
#include "event_bus.h"
#include "diagnostics.h"
#include "filters.h"
#include <string.h>

static const char *TAG = "fuel";

#define FUEL_MULTISAMPLE        16
#define FUEL_SMOOTH_WINDOW      10

/* Kalibrierungstabelle: ADC-Rohwert -> Liter
 * Diese Werte muessen am Fahrzeug gemessen werden!
 * Standard: European 10-190 Ohm Geber mit OPV-Buffer. */
typedef struct {
    float liters;
    int adc_raw;
} fuel_cal_point_t;

static const fuel_cal_point_t s_fuel_cal[] = {
    { 0.0f,   120 },   /* leer */
    { 2.0f,   200 },
    { 4.0f,   290 },
    { 6.0f,   370 },
    { 8.0f,   450 },
    {10.0f,   530 },
    {12.0f,   620 },
    {14.0f,   720 },
    {16.0f,   800 },
    {17.0f,   860 },   /* voll */
};
#define FUEL_CAL_SIZE  (sizeof(s_fuel_cal) / sizeof(s_fuel_cal[0]))

/* Moving-Average-Puffer */
static float s_fuel_buf[FUEL_SMOOTH_WINDOW];
static moving_avg_t s_fuel_avg;
static bool s_initialized = false;

/*
 * Lese ADC-Rohwert (16x Oversampling).
 */
static int fuel_read_adc_raw(void)
{
    if (!g_adc1_handle) {
        return 0;
    }

    uint32_t sum = 0;
    for (int i = 0; i < FUEL_MULTISAMPLE; i++) {
        int raw = 0;
        esp_err_t ret = adc_oneshot_read(g_adc1_handle,
                                         (adc_channel_t)FUEL_ADC_CHANNEL, &raw);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            return -1;
        }
        sum += raw;
    }
    return (int)(sum / FUEL_MULTISAMPLE);
}

/*
 * Interpolation: ADC-Rohwert -> Liter via Kalibrierungstabelle.
 */
static float fuel_adc_to_liters(int raw)
{
    if (raw < 0) {
        return -1.0f;
    }

    /* Unterhalb der Tabelle -> 0 Liter */
    if (raw <= s_fuel_cal[0].adc_raw) {
        return s_fuel_cal[0].liters;
    }

    /* Oberhalb der Tabelle -> voller Tank */
    if (raw >= s_fuel_cal[FUEL_CAL_SIZE - 1].adc_raw) {
        return s_fuel_cal[FUEL_CAL_SIZE - 1].liters;
    }

    /* Lineare Interpolation zwischen Stuetzstellen */
    for (int i = 0; i < FUEL_CAL_SIZE - 1; i++) {
        if (raw >= s_fuel_cal[i].adc_raw && raw <= s_fuel_cal[i + 1].adc_raw) {
            float span = (float)(s_fuel_cal[i + 1].adc_raw - s_fuel_cal[i].adc_raw);
            if (span < 1.0f) {
                return s_fuel_cal[i].liters;
            }
            float t = (float)(raw - s_fuel_cal[i].adc_raw) / span;
            return s_fuel_cal[i].liters +
                   t * (s_fuel_cal[i + 1].liters - s_fuel_cal[i].liters);
        }
    }

    return 0.0f;
}

void fuel_level_init(void)
{
    /* Moving-Average-Filter initialisieren */
    memset(s_fuel_buf, 0, sizeof(s_fuel_buf));
    moving_avg_init(&s_fuel_avg, s_fuel_buf, FUEL_SMOOTH_WINDOW);

    s_initialized = true;
    ESP_LOGI(TAG, "Fuel level ADC init (CH%d, tank=%.0fL, reserve=%.0fL)",
             FUEL_ADC_CHANNEL, (double)TANK_CAPACITY_L, (double)FUEL_RESERVE_L);
}

float fuel_level_read_liters(void)
{
    if (!s_initialized) {
        return -1.0f;
    }

    int raw = fuel_read_adc_raw();
    if (raw < 0) {
        ESP_LOGE(TAG, "Fuel ADC read failed");
        dtc_report(DTC_S0201);
        return -1.0f;
    }

    float liters = fuel_adc_to_liters(raw);
    if (liters < 0.0f) {
        ESP_LOGE(TAG, "Fuel calibration conversion failed");
        dtc_report(DTC_S0201);
        return -1.0f;
    }

    /* Glaettung: Moving Average gegen Schwappen */
    liters = moving_avg_update(&s_fuel_avg, liters);

    /* Reserve-Warnung */
    if (liters < FUEL_RESERVE_L) {
        event_bus_set(EVT_FUEL_LOW);
    }

    ESP_LOGD(TAG, "Fuel: raw=%d -> %.1fL (smoothed)", raw, (double)liters);
    return liters;
}
