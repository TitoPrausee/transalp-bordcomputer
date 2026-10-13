/*
 * rpm.c — Drehzahl ueber Pulse-Counter (PCNT)
 *
 * Hardware: PCNT-Unit zaehlt Pulse vom Tacho-Signal (Hall/Reed/Induktiv).
 * Pin: GPIO 3 (Reserve, siehe PINMAP.md).
 *
 * TODO: Puls-pro-Umdrehung fuer Transalp XL600V ermitteln.
 * TODO: PCNT-Glitch-Filter an Vibration anpassen.
 * TODO: Speed aus Rad-Umfang und Pulsen berechnen (falls Tacho-Signal
 *       gleichzeitig fuer Geschwindigkeit genutzt wird).
 */

#include "sensors.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"

static const char *TAG = "rpm";

#define PULSES_PER_REV      2       /* TODO: an Fahrzeug anpassen */
#define RPM_SAMPLE_PERIOD_MS 100    /* 10Hz Update-Rate */
#define PCNT_HIGH_LIMIT     10000

static pcnt_unit_handle_t pcnt_unit = NULL;

void rpm_init(void)
{
    /* TODO: PCNT-Unit erstellen, Channel auf Tacho-Pin konfigurieren.
     *       Glitch-Filter: 1us (max_glitch_ns = 1000).
     *       Edge-Action: rising edge -> increase.
     *
     * pcnt_unit_config_t unit_config = {
     *     .low_limit = -100,
     *     .high_limit = PCNT_HIGH_LIMIT,
     *     .accum_count = true,
     * };
     * pcnt_new_unit(&unit_config, &pcnt_unit);
     *
     * pcnt_chan_config_t chan_config = {
     *     .edge_gpio_num = 3,   // Reserve-Pin fuer Tacho
     *     .level_gpio_num = -1,
     * };
     * pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan);
     * pcnt_channel_set_edge_action(pcnt_chan,
     *     PCNT_CHANNEL_EDGE_ACTION_INCREASE,
     *     PCNT_CHANNEL_EDGE_ACTION_HOLD);
     * pcnt_unit_enable(pcnt_unit);
     * pcnt_unit_start(pcnt_unit);
     */

    ESP_LOGI(TAG, "RPM PCNT init (stub)");
}

uint16_t rpm_read(void)
{
    if (!pcnt_unit) return 0;

    int pulse_count = 0;
    /* TODO: pcnt_unit_get_count(pcnt_unit, &pulse_count); */
    /* TODO: pcnt_unit_clear_count(pcnt_unit); */

    /* RPM = (pulse_count / sample_period_s) * (60 / pulses_per_rev) */
    float period_s = RPM_SAMPLE_PERIOD_MS / 1000.0f;
    float rpm = (pulse_count / period_s) * (60.0f / PULSES_PER_REV);
    return (uint16_t)rpm;
}
