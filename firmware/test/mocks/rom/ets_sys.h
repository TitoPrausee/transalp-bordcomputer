#ifndef _ROM_ETS_SYS_H_
#define _ROM_ETS_SYS_H_

/*
 * ets_sys.h — Mock header for host-side unit tests.
 *
 * Provides a no-op ets_delay_us() used by the DS18B20 1-Wire
 * bit-banging implementation in temperature.c.
 */

#include <stdint.h>

static inline void ets_delay_us(uint32_t us)
{
    (void)us;
}

#endif /* _ROM_ETS_SYS_H_ */
