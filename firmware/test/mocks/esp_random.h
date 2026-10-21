#ifndef _ESP_RANDOM_H_
#define _ESP_RANDOM_H_

/*
 * esp_random.h — Mock for host-side unit tests.
 *
 * In real ESP-IDF, esp_fill_random() lives in esp_random.h (it was split out
 * of esp_system.h).  The mock esp_system.h includes this header so existing
 * tests that only include esp_system.h still get esp_fill_random, while source
 * files that now include esp_random.h directly resolve it here too.
 *
 * Fills the buffer with a deterministic, non-zero pattern so host tests can
 * assert that a value was generated (IRK / identity address) and inspect the
 * static-address marker bits.  The real implementation uses the hardware RNG.
 */

#include <stdint.h>
#include <stddef.h>

static inline void esp_fill_random(void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    for (size_t i = 0; i < len; i++) {
        p[i] = (uint8_t)(0xA5 ^ (uint8_t)(i * 31));
    }
}

#endif /* _ESP_RANDOM_H_ */
