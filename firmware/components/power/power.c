#include "power.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_sleep.h"

static const char *TAG = "power";
static power_state_t current_state = POWER_STATE_ACTIVE;

#define BATT_ADC_CHANNEL  ADC1_CHANNEL_3  /* GPIO4 */
#define BATT_VOLTAGE_DIV 2.0f            /* 2:1 Spannungsteiler */
#define BATT_LOW_CUTOFF  3.2f            /* Shutdown-Schwelle */
#define BATT_FULL        4.2f

/* GPIO-Pins für Power-Control */
#define GPS_POWER_EN_GPIO  5
#define BOOST_EN_GPIO      6

void power_init(void)
{
    /* ADC für Akku-Überwachung */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATT_ADC_CHANNEL, ADC_ATTEN_DB_11);

    /* Power-Control GPIOs */
    gpio_set_direction(GPS_POWER_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOOST_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(GPS_POWER_EN_GPIO, 1);  /* GPS an */
    gpio_set_level(BOOST_EN_GPIO, 1);      /* 5V-Rail an */

    ESP_LOGI(TAG, "Power management initialized");
}

float power_get_battery_voltage(void)
{
    int raw = adc1_get_raw(BATT_ADC_CHANNEL);
    float v = (raw / 4095.0f) * 3.3f * BATT_VOLTAGE_DIV;
    return v;
}

power_state_t power_get_state(void)
{
    return current_state;
}

void power_task(void *arg)
{
    while (1) {
        float vbat = power_get_battery_voltage();

        if (vbat < BATT_LOW_CUTOFF) {
            ESP_LOGW(TAG, "Battery low: %.2fV → SHUTDOWN", vbat);
            current_state = POWER_STATE_SHUTDOWN;
            /* TODO: SD flush, GPS backup, deep sleep */
            esp_deep_sleep_start();
        }

        /* TODO: State-Machine für ACTIVE → IDLE → SLEEP */
        /* Siehe ARCHITECTURE.md Power-Management */

        vTaskDelay(pdMS_TO_TICKS(1000)); /* 1Hz */
    }
}
