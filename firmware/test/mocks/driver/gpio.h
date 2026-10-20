#ifndef _DRIVER_GPIO_H_
#define _DRIVER_GPIO_H_

/*
 * driver/gpio.h — Mock for host-side unit tests.
 *
 * Provides the GPIO config types/macros/functions used by
 * power_manager.c (power-gating pins) as no-ops. esp_err_t/ESP_OK
 * are provided by the aggregated esp_idf_mock.h (freertos_mock.h).
 */

#include <stdint.h>

typedef int gpio_num_t;

typedef struct {
    uint64_t pin_bit_mask;
    int      mode;
    int      pull_up_en;
    int      pull_down_en;
    int      intr_type;
} gpio_config_t;

#define GPIO_MODE_OUTPUT       2
#define GPIO_PULLUP_DISABLE    0
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_INTR_DISABLE      0

#define GPIO_NUM_5  5
#define GPIO_NUM_6  6

static inline esp_err_t gpio_config(const gpio_config_t *cfg) {
    (void)cfg;
    return ESP_OK;
}

static inline esp_err_t gpio_set_level(int gpio_num, int level) {
    (void)gpio_num; (void)level;
    return ESP_OK;
}

#endif /* _DRIVER_GPIO_H_ */
