/*
 * rpm.c — Drehzahl-Messung ueber Pulse-Counter (PCNT)
 *
 * Hardware: PCNT-Unit zaehlt Pulse vom Pickup-Signal (Optokoppler).
 * Pin: GPIO 16 (siehe board_config.h RPM_PCNT_PIN).
 *
 * Der Pickup-Sensor liefert 1 Puls pro Kurbelwellenumdrehung.
 * RPM = (Pulse im Messfenster) * 60 / (Messfenster_s * PULSES_PER_REV)
 *
 * Glättung: gleitender Mittelwert (Window 5) + Clamping auf 0-8000.
 */

#include "sensors.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "rpm";

#define RPM_SAMPLE_PERIOD_MS 100       /* 10Hz Update-Rate */
#define RPM_SMOOTH_WINDOW    5         /* Moving Average Window */

static pcnt_unit_handle_t s_pcnt_unit = NULL;
static pcnt_channel_handle_t s_pcnt_chan = NULL;

/* Gleitender Mittelwert-Puffer */
static float s_rpm_buf[RPM_SMOOTH_WINDOW];
static uint8_t s_rpm_buf_idx = 0;
static uint8_t s_rpm_buf_count = 0;

/*
 * Lese den PCNT-Zaehler und setze ihn zurueck.
 * Gibt die Anzahl Pulse im letzten Messfenster zurueck.
 */
static int rpm_take_count(void)
{
    if (!s_pcnt_unit) {
        return 0;
    }

    int pulse_count = 0;
    esp_err_t ret = pcnt_unit_get_count(s_pcnt_unit, &pulse_count);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "pcnt_unit_get_count failed: %s", esp_err_to_name(ret));
        return 0;
    }

    /* Zaehler zuruecksetzen fuer naechstes Fenster */
    pcnt_unit_clear_count(s_pcnt_unit);
    return pulse_count;
}

/*
 * Glaettung: gleitender Mittelwert ueber RPM_SMOOTH_WINDOW Werte.
 */
static float rpm_smooth(float new_rpm)
{
    s_rpm_buf[s_rpm_buf_idx] = new_rpm;
    s_rpm_buf_idx = (s_rpm_buf_idx + 1) % RPM_SMOOTH_WINDOW;
    if (s_rpm_buf_count < RPM_SMOOTH_WINDOW) {
        s_rpm_buf_count++;
    }

    float sum = 0.0f;
    for (uint8_t i = 0; i < s_rpm_buf_count; i++) {
        sum += s_rpm_buf[i];
    }
    return sum / s_rpm_buf_count;
}

void rpm_init(void)
{
    /* PCNT-Unit konfigurieren */
    pcnt_unit_config_t unit_config = {
        .low_limit = -1,
        .high_limit = RPM_PCNT_HIGH_LIMIT,
        .flags.accum_count = false,
    };
    esp_err_t ret = pcnt_new_unit(&unit_config, &s_pcnt_unit);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_new_unit failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Glitch-Filter konfigurieren (1us — ignoriert Noise-Spikes) */
    pcnt_glitch_filter_config_t filter_cfg = {
        .max_glitch_ns = 1000,
    };
    pcnt_unit_set_glitch_filter(s_pcnt_unit, &filter_cfg);

    /* Channel auf RPM-Pin konfigurieren */
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = RPM_PCNT_PIN,
        .level_gpio_num = -1,
    };
    ret = pcnt_new_channel(s_pcnt_unit, &chan_config, &s_pcnt_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_new_channel failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Rising Edge -> Increment */
    pcnt_channel_set_edge_action(s_pcnt_chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD);

    /* Level-Action nicht benoetigt */
    pcnt_channel_set_level_action(s_pcnt_chan,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP);

    /* Unit aktivieren und starten */
    pcnt_unit_enable(s_pcnt_unit);
    pcnt_unit_clear_count(s_pcnt_unit);
    pcnt_unit_start(s_pcnt_unit);

    /* GPIO-Pin als Input mit Pull-Up (Optokoppler Open-Collector) */
    gpio_set_direction(RPM_PCNT_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(RPM_PCNT_PIN, GPIO_PULLUP_ONLY);

    /* Puffer initialisieren */
    memset(s_rpm_buf, 0, sizeof(s_rpm_buf));
    s_rpm_buf_idx = 0;
    s_rpm_buf_count = 0;

    ESP_LOGI(TAG, "RPM PCNT initialized (pin=%d, pulses/rev=%d, window=%d)",
             RPM_PCNT_PIN, RPM_PULSES_PER_REV, RPM_SMOOTH_WINDOW);
}

uint16_t rpm_read(void)
{
    if (!s_pcnt_unit) {
        return 0;
    }

    int pulse_count = rpm_take_count();
    if (pulse_count <= 0) {
        /* Keine Pulse -> Motor steht oder Sensor aus */
        return 0;
    }

    /* RPM = (pulse_count / sample_period_s) * (60 / pulses_per_rev) */
    float period_s = RPM_SAMPLE_PERIOD_MS / 1000.0f;
    float rpm = (pulse_count / period_s) * (60.0f / RPM_PULSES_PER_REV);

    /* Glaettung */
    rpm = rpm_smooth(rpm);

    /* Clamping: 0..8000 */
    if (rpm < 0.0f) rpm = 0.0f;
    if (rpm > RPM_MAX_VALUE) rpm = RPM_MAX_VALUE;

    return (uint16_t)rpm;
}
