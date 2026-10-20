/*
 * temperature.c — Kuehlmittel-Temperatur
 *
 * Primaer: DS18B20 1-Wire Sensor (digital, ±0.5°C).
 * Fallback: NTC 10k B3950 Thermistor ueber ADC mit Spannungsteiler.
 *
 * Pin NTC:  ADC1_CH6 (siehe board_config.h TEMP_NTC_ADC_CHANNEL)
 * Pin DS18B20: GPIO 15 (siehe board_config.h TEMP_ONEWIRE_PIN)
 *
 * Lese Temperatur alle 500ms (wird vom Sensor-Task aufgerufen).
 * Ueberlaufschutz bei >120°C -> Warning Event + DTC.
 *
 * DS18B20 1-Wire wird hier ohne externe Library implementiert
 * (Bit-Banging ueber GPIO).
 */

#include "sensors.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "event_bus.h"
#include "diagnostics.h"
#include <math.h>

static const char *TAG = "temp";

/* DS18B20 1-Wire Commands */
#define DS18B20_CMD_SKIP_ROM        0xCC
#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_READ_POWER      0xB4

/* NTC Parameter */
#define NTC_VREF_MV             3300.0f
#define NTC_MULTISAMPLE         16

static bool s_ds18b20_present = false;
static bool s_initialized = false;

/* --- DS18B20 1-Wire Bit-Banging --- */

#define ONEWIRE_PIN             TEMP_ONEWIRE_PIN
#define ONEWIRE_INPUT           gpio_get_level(ONEWIRE_PIN)
#define ONEWIRE_OUTPUT_LOW()    gpio_set_level(ONEWIRE_PIN, 0)
#define ONEWIRE_OUTPUT_HIGH()   gpio_set_level(ONEWIRE_PIN, 1)

static void onewire_set_input(void)
{
    gpio_set_direction(ONEWIRE_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ONEWIRE_PIN, GPIO_PULLUP_ONLY);
}

static void onewire_set_output(void)
{
    gpio_set_direction(ONEWIRE_PIN, GPIO_MODE_OUTPUT_OD);
}

static void onewire_delay_us(uint32_t us)
{
    ets_delay_us(us);
}

static bool onewire_reset(void)
{
    bool presence;

    onewire_set_output();
    ONEWIRE_OUTPUT_LOW();
    onewire_delay_us(480);
    onewire_set_input();
    onewire_delay_us(70);

    presence = (ONEWIRE_INPUT == 0);

    onewire_delay_us(410);
    return presence;
}

static void onewire_write_bit(uint8_t bit)
{
    onewire_set_output();
    ONEWIRE_OUTPUT_LOW();

    if (bit) {
        onewire_delay_us(6);
        onewire_set_input();
        onewire_delay_us(64);
    } else {
        onewire_delay_us(60);
        onewire_set_input();
        onewire_delay_us(10);
    }
}

static uint8_t onewire_read_bit(void)
{
    uint8_t bit;

    onewire_set_output();
    ONEWIRE_OUTPUT_LOW();
    onewire_delay_us(4);
    onewire_set_input();
    onewire_delay_us(10);

    bit = ONEWIRE_INPUT ? 1 : 0;
    onewire_delay_us(50);
    return bit;
}

static void onewire_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(data & 0x01);
        data >>= 1;
    }
}

static uint8_t onewire_read_byte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data >>= 1;
        if (onewire_read_bit()) {
            data |= 0x80;
        }
    }
    return data;
}

/*
 * DS18B20 Temperatur lesen.
 * Gibt Temperatur in °C zurueck, oder -1000.0f bei Fehler.
 */
static float ds18b20_read_temp(void)
{
    if (!onewire_reset()) {
        return -1000.0f;
    }

    /* Skip ROM + Convert T */
    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_CONVERT_T);

    /* Warte auf Conversion (max 750ms bei 12-bit) */
    for (int i = 0; i < 100; i++) {
        if (onewire_read_bit()) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!onewire_reset()) {
        return -1000.0f;
    }

    /* Skip ROM + Read Scratchpad */
    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_READ_SCRATCHPAD);

    uint8_t temp_lsb = onewire_read_byte();
    uint8_t temp_msb = onewire_read_byte();

    int16_t raw_temp = (temp_msb << 8) | temp_lsb;
    /* 12-bit Resolution: 0.0625 °C per LSB */
    return (float)raw_temp * 0.0625f;
}

/*
 * NTC Fallback: Lese Temperatur ueber ADC und Spannungsteiler.
 *
 * Schaltung:
 *   3.3V --[10k Referenz]--+-- ADC Pin
 *                           |
 *                       [NTC 10k]
 *                           |
 *                          GND
 *
 * R_ntc = R_ref * V / (Vcc - V)
 * T = 1 / (1/T25 + ln(R/R25)/B) - 273.15
 */
static float ntc_read_temp(void)
{
    if (!g_adc1_handle || !g_adc_cali_handle) {
        return -1000.0f;
    }

    /* Multisampling: 16 Samples mitteln */
    uint32_t sum = 0;
    for (int i = 0; i < NTC_MULTISAMPLE; i++) {
        int raw = 0;
        esp_err_t ret = adc_oneshot_read(g_adc1_handle,
                                         (adc_channel_t)TEMP_NTC_ADC_CHANNEL, &raw);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            return -1000.0f;
        }
        sum += raw;
    }
    int avg_raw = (int)(sum / NTC_MULTISAMPLE);

    /* ADC-Kalibrierung: raw -> Spannung (mV) */
    int voltage_mv = 0;
    esp_err_t ret = adc_cali_raw_to_voltage(g_adc_cali_handle, avg_raw, &voltage_mv);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC cali failed: %s", esp_err_to_name(ret));
        return -1000.0f;
    }

    /* Spannungsteiler aufloesen: R_ntc = R_ref * V / (Vcc - V) */
    float v = voltage_mv / 1000.0f;  /* -> Volt */
    float vcc = NTC_VREF_MV / 1000.0f;

    if (v >= vcc - 0.01f) {
        /* NTC kurzgeschlossen oder nicht angeschlossen */
        return -1000.0f;
    }

    float r_ntc = NTC_SERIES_R * v / (vcc - v);

    /* Steinhart-Hart (Beta-Gleichung):
     * 1/T = 1/T25 + (1/B) * ln(R/R25)
     * T = 1 / (1/T25 + ln(R/R25)/B) - 273.15
     */
    float t25 = 298.15f;  /* 25°C in Kelvin */
    float temp_k = 1.0f / (1.0f / t25 + logf(r_ntc / NTC_R25) / NTC_B_VALUE);
    float temp_c = temp_k - 273.15f;

    return temp_c;
}

void temperature_init(void)
{
    /* DS18B20 Pin konfigurieren */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TEMP_ONEWIRE_PIN),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* Pruefe ob DS18B20 am Bus vorhanden */
    s_ds18b20_present = onewire_reset();

    if (s_ds18b20_present) {
        ESP_LOGI(TAG, "DS18B20 detected on pin %d", TEMP_ONEWIRE_PIN);
    } else {
        ESP_LOGW(TAG, "DS18B20 not found — falling back to NTC ADC on CH%d",
                 TEMP_NTC_ADC_CHANNEL);
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Temperature sensor initialized");
}

float temperature_read_coolant(void)
{
    if (!s_initialized) {
        return -1.0f;
    }

    float temp_c = -1000.0f;

    /* Primaer: DS18B20 (digital) */
    if (s_ds18b20_present) {
        temp_c = ds18b20_read_temp();
        if (temp_c < -999.0f) {
            ESP_LOGW(TAG, "DS18B20 read failed — trying NTC fallback");
            s_ds18b20_present = false;
            temp_c = ntc_read_temp();
        }
    } else {
        /* Fallback: NTC ueber ADC */
        temp_c = ntc_read_temp();
    }

    if (temp_c < -999.0f) {
        ESP_LOGE(TAG, "All temperature sensors failed");
        dtc_report(DTC_S0101);
        return -1.0f;
    }

    /* Ueberlaufschutz: >120°C -> Warning */
    if (temp_c > TEMP_OVERTEMP_C) {
        ESP_LOGW(TAG, "Coolant overtemperature: %.1f°C", (double)temp_c);
        event_bus_set(EVT_OVERTEMPERATURE);
        dtc_report(DTC_S0103);
    }

    return temp_c;
}
