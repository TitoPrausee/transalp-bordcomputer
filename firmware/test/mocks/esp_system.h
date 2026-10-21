#ifndef _ESP_SYSTEM_H_
#define _ESP_SYSTEM_H_

/*
 * esp_system.h — Mock for host-side unit tests.
 * esp_restart does NOT actually restart; it sets a flag so the OTA
 * success path (ota_finish_and_verify -> esp_restart) is observable.
 */

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

extern int g_restart_called;

static inline void esp_restart(void) { g_restart_called = 1; }

static inline uint32_t esp_get_free_heap_size(void) { return 200 * 1024; }

static inline uint32_t esp_get_minimum_free_heap_size(void) { return 100 * 1024; }

/* Mock for esp_fill_random (ESP-IDF esp_system.h).  Fills the buffer with a
 * deterministic, non-zero pattern so host tests can assert that a value was
 * generated (IRK / identity address) and inspect the static-address marker
 * bits.  The real implementation uses the hardware RNG. */
static inline void esp_fill_random(void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    for (size_t i = 0; i < len; i++) {
        p[i] = (uint8_t)(0xA5 ^ (uint8_t)(i * 31));
    }
}

#endif /* _ESP_SYSTEM_H_ */
