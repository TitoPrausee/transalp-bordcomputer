#ifndef _ESP_IDF_MOCK_H_
#define _ESP_IDF_MOCK_H_

/*
 * esp_idf_mock.h — Aggregated mock header for host-side unit tests.
 *
 * This header intercepts ESP-IDF includes and replaces them with
 * POSIX-based mock implementations, so the same firmware source
 * files can be compiled with host GCC without the ESP-IDF toolchain.
 *
 * Usage: compile with -include "mocks/esp_idf_mock.h"
 */

/* ---- Intercept FreeRTOS header includes ---- */
/* When the compiler sees #include "freertos/FreeRTOS.h" etc.,
 * it checks if the header guard is already defined. We define
 * all guards here and provide our mock types, so the actual
 * FreeRTOS headers are never included. */

#ifndef _FREERTOS_H_
#define _FREERTOS_H_
#endif

#ifndef _FREERTOS_TASK_H_
#define _FREERTOS_TASK_H_
#endif

#ifndef _FREERTOS_QUEUE_H_
#define _FREERTOS_QUEUE_H_
#endif

#ifndef _FREERTOS_SEMPHR_H_
#define _FREERTOS_SEMPHR_H_
#endif

#ifndef _FREERTOS_EVENT_GROUPS_H_
#define _FREERTOS_EVENT_GROUPS_H_
#endif

/* Now provide the FreeRTOS types and functions */
#include "freertos/freertos_mock.h"
#include "esp_log_mock.h"

#endif /* _ESP_IDF_MOCK_H_ */
