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

/* esp_fill_random was moved to esp_random.h in newer ESP-IDF; include it
 * here so existing code that only includes esp_system.h still works. */
#include "esp_random.h"

#endif /* _ESP_SYSTEM_H_ */
