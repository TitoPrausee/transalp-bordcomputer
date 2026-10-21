#ifndef _DRIVER_GPIO_H_
#define _DRIVER_GPIO_H_

/*
 * driver/gpio.h — Mock for host-side unit tests.
 *
 * Provides the GPIO config types/macros/functions used by
 * power_manager.c (power-gating pins) and the sensor drivers
 * (rpm.c, temperature.c) as no-ops. esp_err_t/ESP_OK are provided
 * by the aggregated esp_idf_mock.h (freertos_mock.h).
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

#define GPIO_MODE_OUTPUT           2
#define GPIO_MODE_INPUT            3
#define GPIO_MODE_INPUT_OUTPUT_OD  4
#define GPIO_MODE_OUTPUT_OD        5
#define GPIO_PULLUP_DISABLE        0
#define GPIO_PULLUP_ENABLE         1
#define GPIO_PULLUP_ONLY           2
#define GPIO_PULLDOWN_DISABLE      0
#define GPIO_PULLDOWN_ENABLE       1
#define GPIO_PULLDOWN_ONLY         3
#define GPIO_INTR_DISABLE          0

#define GPIO_NUM_5  5
#define GPIO_NUM_6  6

/* Controllable gpio_get_level return — tests set this to simulate
 * pin states (e.g. DS18B20 1-Wire presence detection). Defaults to 0,
 * matching the original behavior, so existing tests are unaffected.
 * Static so each test translation unit owns its own copy (the test
 * and its included source share the same TU). */
static int g_mock_gpio_level = 0;

static inline esp_err_t gpio_config(const gpio_config_t *cfg) {
    (void)cfg;
    return ESP_OK;
}

static inline esp_err_t gpio_set_level(int gpio_num, int level) {
    (void)gpio_num; (void)level;
    return ESP_OK;
}

static inline esp_err_t gpio_set_direction(int gpio_num, int mode) {
    (void)gpio_num; (void)mode;
    return ESP_OK;
}

static inline esp_err_t gpio_set_pull_mode(int gpio_num, int pull) {
    (void)gpio_num; (void)pull;
    return ESP_OK;
}

static inline int gpio_get_level(int gpio_num) {
    (void)gpio_num;
    return g_mock_gpio_level;
}

#endif /* _DRIVER_GPIO_H_ */
