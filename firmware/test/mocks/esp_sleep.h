#ifndef _ESP_SLEEP_H_
#define _ESP_SLEEP_H_

/*
 * esp_sleep.h — Mock for host-side unit tests.
 *
 * Provides the esp_sleep API surface used by power_manager.c
 * (deep-sleep wakeup cause + wakeup-source configuration) as no-ops.
 *
 * RTC_NOINIT_ATTR is also defined here: on target it places a
 * variable in RTC slow memory so it survives deep sleep; on host
 * it is a no-op attribute. power_manager.c relies on it being
 * available before the first use.
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef RTC_NOINIT_ATTR
#define RTC_NOINIT_ATTR
#endif

typedef int esp_sleep_wakeup_cause_t;

#define ESP_SLEEP_WAKEUP_UNDEFINED  0
#define ESP_SLEEP_WAKEUP_TIMER      1
#define ESP_SLEEP_WAKEUP_GPIO       2
#define ESP_SLEEP_WAKEUP_UART       3
#define ESP_SLEEP_WAKEUP_TOUCHPAD   4
#define ESP_SLEEP_WAKEUP_ULP        5

static inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void) {
    return ESP_SLEEP_WAKEUP_UNDEFINED;
}

static inline void esp_sleep_enable_timer_wakeup(uint64_t time_in_us) {
    (void)time_in_us;
}

static inline void esp_sleep_enable_ext0_wakeup(int gpio_num, int level) {
    (void)gpio_num; (void)level;
}

static inline void esp_sleep_enable_uart_wakeup(int uart_num) {
    (void)uart_num;
}

static inline void esp_deep_sleep_start(void) {
    /* no-op in host tests */
}

#endif /* _ESP_SLEEP_H_ */
