#ifndef _DRIVER_PULSE_CNT_H_
#define _DRIVER_PULSE_CNT_H_

/*
 * pulse_cnt.h — Mock header for host-side unit tests.
 *
 * Provides stubs for the ESP-IDF PCNT (Pulse Counter) API used by
 * the RPM sensor driver. The mock makes pcnt_unit_get_count()
 * controllable via a global so tests can inject pulse counts.
 */

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct pcnt_unit_t *pcnt_unit_handle_t;
typedef struct pcnt_channel_t *pcnt_channel_handle_t;

/* Action constants (match ESP-IDF pcnt_channel_edge_action_t) */
#define PCNT_CHANNEL_EDGE_ACTION_INCREASE  1
#define PCNT_CHANNEL_EDGE_ACTION_DECREASE  (-1)
#define PCNT_CHANNEL_EDGE_ACTION_HOLD      0

/* Level action constants */
#define PCNT_CHANNEL_LEVEL_ACTION_KEEP     0
#define PCNT_CHANNEL_LEVEL_ACTION_INVERSE  1
#define PCNT_CHANNEL_LEVEL_ACTION_HOLD     2

typedef struct {
    int low_limit;
    int high_limit;
    struct {
        bool accum_count;
    } flags;
} pcnt_unit_config_t;

typedef struct {
    uint32_t max_glitch_ns;
} pcnt_glitch_filter_config_t;

typedef struct {
    int edge_gpio_num;
    int level_gpio_num;
} pcnt_chan_config_t;

/* ---- Controllable mock state ----
 * Tests set g_mock_pcnt_count before calling rpm_read(); the mock
 * returns it from pcnt_unit_get_count(). g_mock_pcnt_get_ret
 * controls the return code (0 = ESP_OK). */
extern int g_mock_pcnt_count;
extern int g_mock_pcnt_get_ret;

static inline esp_err_t pcnt_new_unit(const pcnt_unit_config_t *config,
                                        pcnt_unit_handle_t *unit)
{
    (void)config;
    if (unit) *unit = (pcnt_unit_handle_t)1;
    return ESP_OK;
}

static inline esp_err_t pcnt_unit_set_glitch_filter(pcnt_unit_handle_t unit,
                                                     const pcnt_glitch_filter_config_t *cfg)
{
    (void)unit; (void)cfg;
    return ESP_OK;
}

static inline esp_err_t pcnt_new_channel(pcnt_unit_handle_t unit,
                                          const pcnt_chan_config_t *config,
                                          pcnt_channel_handle_t *chan)
{
    (void)unit; (void)config;
    if (chan) *chan = (pcnt_channel_handle_t)1;
    return ESP_OK;
}

static inline esp_err_t pcnt_channel_set_edge_action(pcnt_channel_handle_t chan,
                                                      int pos, int neg)
{
    (void)chan; (void)pos; (void)neg;
    return ESP_OK;
}

static inline esp_err_t pcnt_channel_set_level_action(pcnt_channel_handle_t chan,
                                                        int high, int low)
{
    (void)chan; (void)high; (void)low;
    return ESP_OK;
}

static inline esp_err_t pcnt_unit_enable(pcnt_unit_handle_t unit)
{
    (void)unit;
    return ESP_OK;
}

static inline esp_err_t pcnt_unit_clear_count(pcnt_unit_handle_t unit)
{
    (void)unit;
    return ESP_OK;
}

static inline esp_err_t pcnt_unit_start(pcnt_unit_handle_t unit)
{
    (void)unit;
    return ESP_OK;
}

static inline esp_err_t pcnt_unit_get_count(pcnt_unit_handle_t unit, int *count)
{
    (void)unit;
    if (count) *count = g_mock_pcnt_count;
    return (esp_err_t)g_mock_pcnt_get_ret;
}

#endif /* _DRIVER_PULSE_CNT_H_ */
