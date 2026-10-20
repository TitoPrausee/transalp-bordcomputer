#ifndef _ESP_ERR_H_
#define _ESP_ERR_H_

/*
 * Mock header for host-side unit tests.
 *
 * When the aggregated esp_idf_mock.h is in use, freertos_mock.h already
 * provides esp_err_t, the core ESP_ERR_* codes, and esp_err_to_name().
 * Every definition here is guarded so it only applies when not already
 * provided — keeping esp_err.h usable standalone too.
 */

#include <stdint.h>

typedef int esp_err_t;

#ifndef ESP_OK
#define ESP_OK          0
#endif
#ifndef ESP_FAIL
#define ESP_FAIL        -1
#endif
#ifndef ESP_ERR_NO_MEM
#define ESP_ERR_NO_MEM  0x101
#endif
#ifndef ESP_ERR_INVALID_ARG
#define ESP_ERR_INVALID_ARG 0x102
#endif
#ifndef ESP_ERR_INVALID_STATE
#define ESP_ERR_INVALID_STATE 0x103
#endif
#ifndef ESP_ERR_NOT_FOUND
#define ESP_ERR_NOT_FOUND 0x104
#endif
#ifndef ESP_ERR_NOT_SUPPORTED
#define ESP_ERR_NOT_SUPPORTED 0x105
#endif
#ifndef ESP_ERR_TIMEOUT
#define ESP_ERR_TIMEOUT 0x107
#endif
#ifndef ESP_ERR_INVALID_CRC
#define ESP_ERR_INVALID_CRC 0x108
#endif
#ifndef ESP_ERR_INVALID_VERSION
#define ESP_ERR_INVALID_VERSION 0x109
#endif
#ifndef ESP_ERR_INVALID_SIZE
#define ESP_ERR_INVALID_SIZE 0x10A
#endif
#ifndef ESP_ERR_NVS_NOT_FOUND
#define ESP_ERR_NVS_NOT_FOUND 0x10B
#endif

#ifndef ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x) do { esp_err_t __e = (x); (void)__e; } while(0)
#endif

/*
 * esp_err_to_name() is provided by freertos_mock.h when the aggregated
 * esp_idf_mock.h is in use. It is intentionally NOT redefined here to
 * avoid a static-inline redefinition conflict. Standalone consumers of
 * esp_err.h that need it should include esp_idf_mock.h or freertos_mock.h.
 */

#endif /* _ESP_ERR_H_ */
