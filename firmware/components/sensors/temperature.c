/*
 * temperature.c — NTC-Thermistor ueber ADC
 *
 * Kuehlmittel-Temperatur ueber NTC und Spannungsteiler.
 * Pin: Siehe board_config.h (ADC1 Channel).
 *
 * TODO: NTC-Parameter (B-Wert, R25) fuer konkreten Sensor eintragen.
 * TODO: ADC-Kalibrierung mit eFuse Vref (adc_cali_scheme).
 * TODO: Weitere NTC-Sensoren (Oeltemp, Aussenluft) bei Bedarf hinzufuegen.
 */

#include "sensors.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "temp";

/* NTC-Parameter (typisch 10k NTC, B=3950) */
#define NTC_R25         10000.0f    /* Ohm bei 25C */
#define NTC_B_VALUE     3950.0f     /* B-Wert */
#define NTC_SERIES_R    10000.0f    /* Spannungsteiler Vorwiderstand */
#define ADC_VREF_MV     3300.0f     /* 3.3V Referenz */

/* TODO: ADC-Handle initialisieren
 * static adc_oneshot_unit_handle_t adc1_handle;
 * static adc_cali_handle_t adc_cali_handle;
 */

void temperature_init(void)
{
    /* TODO: adc_oneshot_new_unit(...) fuer ADC1.
     * TODO: adc_oneshot_config_channel(...) fuer coolant-temp Channel.
     * TODO: adc_cali_create_scheme_linepoint(...) fuer eFuse Vref.
     */
    ESP_LOGI(TAG, "Temperature ADC init (stub)");
}

float temperature_read_coolant(void)
{
    /* TODO: ADC lesen, Spannung berechnen, NTC-Widerstand aus Spannungsteiler,
     *       dann Beta-Gleichung: T = 1 / (1/T25 + ln(R/R25)/B) - 273.15
     *
     * int raw;
     * adc_oneshot_read(adc1_handle, channel, &raw);
     * float voltage_mv = (raw / 4095.0f) * ADC_VREF_MV;
     * float r_ntc = NTC_SERIES_R * voltage_mv / (ADC_VREF_MV - voltage_mv);
     * float temp_k = 1.0f / (1.0f/298.15f + logf(r_ntc / NTC_R25) / NTC_B_VALUE);
     * return temp_k - 273.15f;
     */
    return -1.0f;  /* Stub */
}
