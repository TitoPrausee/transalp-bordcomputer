/*
 * power_manager.c — Akku-Ueberwachung und Sleep-Modi
 *
 * ADC: GPIO4 (ADC1_CH4), Spannungsteiler 2:1 (max 3.3V).
 * Power-Control: GPS_EN=GPIO5, BOOST_EN=GPIO6.
 *
 * TODO: Neue ADC-API (v5.x: adc_oneshot + adc_cali) verwenden.
 * TODO: State-Machine: ACTIVE -> IDLE (Timeout) -> SLEEP -> SHUTDOWN.
 * TODO: Deep-Sleep mit Wakeup-Source (GPIO-Button, Timer).
 * TODO: Battery-Percentage aus Ladekurve (Li-Ion: 3.0V-4.2V).
 *
 * Siehe docs/research/03-software-stack.md Abschnitt 5.5.
 */

#include "power.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "power";
static power_state_t s_state = POWER_STATE_ACTIVE;

#define BATT_LOW_CUTOFF    3.2f
#define BATT_FULL          4.2f
#define BATT_EMPTY         3.0f

/* TODO: ADC-Handles
 * static adc_oneshot_unit_handle_t adc1_handle;
 * static adc_cali_handle_t adc_cali_handle;
 */

void power_init(void)
{
    /* TODO: ADC initialisieren (neue v5.x API).
     *
     * adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1 };
     * adc_oneshot_new_unit(&init_config, &adc1_handle);
     *
     * adc_oneshot_chan_cfg_t chan_config = {
     *     .atten = ADC_ATTEN_DB_12,
     *     .bitwidth = ADC_BITWIDTH_12,
     * };
     * adc_oneshot_config_channel(adc1_handle, BATT_ADC_CHANNEL, &chan_config);
     *
     * adc_cali_linepoint_val_config_t cali_config = {
     *     .unit_id = ADC_UNIT_1,
     *     .chan = BATT_ADC_CHANNEL,
     *     .atten = ADC_ATTEN_DB_12,
     *     .bitwidth = ADC_BITWIDTH_12,
     * };
     * adc_cali_create_scheme_linepoint(&cali_config, &adc_cali_handle);
     */

    /* Power-Control GPIOs */
    /* TODO: gpio_set_direction(GPS_POWER_EN_PIN, GPIO_MODE_OUTPUT);
     * gpio_set_direction(BOOST_EN_PIN, GPIO_MODE_OUTPUT);
     * gpio_set_level(GPS_POWER_EN_PIN, 1);   // GPS an
     * gpio_set_level(BOOST_EN_PIN, 1);       // 5V-Rail an
     */

    ESP_LOGI(TAG, "Power management initialized (stub)");
}

float power_get_battery_voltage(void)
{
    /* TODO: ADC lesen und Spannung berechnen.
     *
     * int raw, voltage_mv;
     * adc_oneshot_read(adc1_handle, BATT_ADC_CHANNEL, &raw);
     * adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_mv);
     * return (float)voltage_mv * BATT_VOLTAGE_DIV / 1000.0f;
     */
    return 4.0f;  /* Stub: assume 4V */
}

uint8_t power_get_battery_percent(void)
{
    float v = power_get_battery_voltage();
    if (v <= BATT_EMPTY) return 0;
    if (v >= BATT_FULL) return 100;
    float pct = ((v - BATT_EMPTY) / (BATT_FULL - BATT_EMPTY)) * 100.0f;
    return (uint8_t)pct;
}

power_state_t power_get_state(void)
{
    return s_state;
}

void power_task(void *arg)
{
    while (1) {
        float vbat = power_get_battery_voltage();

        if (vbat < BATT_LOW_CUTOFF) {
            ESP_LOGW(TAG, "Battery low: %.2fV -> SHUTDOWN", vbat);
            s_state = POWER_STATE_SHUTDOWN;
            event_bus_set(EVT_BATTERY_CRITICAL | EVT_SHUTDOWN);

            /* TODO: SD flush, GPS backup, dann deep sleep */
            /* esp_deep_sleep_start(); */
        }

        /* TODO: State-Machine fuer ACTIVE -> IDLE -> SLEEP
         * (Timeout-basiert, siehe Architecture-Doc).
         */

        /* 1Hz */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
