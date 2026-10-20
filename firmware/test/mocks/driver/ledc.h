#ifndef _DRIVER_LEDC_H_
#define _DRIVER_LEDC_H_

/*
 * driver/ledc.h — Mock for host-side unit tests.
 *
 * Provides the LEDC PWM types, config structs and functions used by
 * power_manager.c (backlight PWM) as no-ops. esp_err_t/ESP_OK are
 * provided by the aggregated esp_idf_mock.h (freertos_mock.h) which
 * is included before this header in host test builds.
 */

#include <stdint.h>

typedef int ledc_timer_t;
typedef int ledc_channel_t;
typedef int ledc_mode_t;
typedef int ledc_timer_bit_t;
typedef int ledc_intr_type_t;
typedef int ledc_clk_cfg_t;

#define LEDC_TIMER_1          1
#define LEDC_LOW_SPEED_MODE   0
#define LEDC_CHANNEL_1        1
#define LEDC_TIMER_10_BIT     10
#define LEDC_AUTO_CLK         0
#define LEDC_INTR_DISABLE      0

typedef struct {
    int       speed_mode;
    int       timer_num;
    int       duty_resolution;
    int       freq_hz;
    int       clk_cfg;
} ledc_timer_config_t;

typedef struct {
    int       speed_mode;
    int       channel;
    int       timer_sel;
    int       intr_type;
    int       gpio_num;
    uint32_t  duty;
    int       hpoint;
} ledc_channel_config_t;

static inline esp_err_t ledc_timer_config(const ledc_timer_config_t *timer_conf) {
    (void)timer_conf;
    return ESP_OK;
}

static inline esp_err_t ledc_channel_config(const ledc_channel_config_t *ledc_conf) {
    (void)ledc_conf;
    return ESP_OK;
}

static inline esp_err_t ledc_set_duty(int speed_mode, int channel, uint32_t duty) {
    (void)speed_mode; (void)channel; (void)duty;
    return ESP_OK;
}

static inline esp_err_t ledc_update_duty(int speed_mode, int channel) {
    (void)speed_mode; (void)channel;
    return ESP_OK;
}

#endif /* _DRIVER_LEDC_H_ */
