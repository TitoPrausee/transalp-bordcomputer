#ifndef _DRIVER_RTC_IO_H_
#define _DRIVER_RTC_IO_H_

/*
 * driver/rtc_io.h — Mock for host-side unit tests.
 *
 * Provides the RTC GPIO helpers used by power_manager.c as no-ops.
 */

#include <stdbool.h>

static inline bool rtc_gpio_is_valid_gpio(int gpio_num) {
    (void)gpio_num;
    return false;
}

static inline void rtc_gpio_isolate(int gpio_num) {
    (void)gpio_num;
}

#endif /* _DRIVER_RTC_IO_H_ */
