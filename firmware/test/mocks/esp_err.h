#ifndef _ESP_ERR_H_
#define _ESP_ERR_H_

/*
 * Mock header for host-side unit tests.
 */

#include <stdint.h>

typedef int esp_err_t;

#define ESP_OK          0
#define ESP_FAIL        -1
#define ESP_ERR_NO_MEM  0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_NOT_FOUND 0x104
#define ESP_ERR_NOT_SUPPORTED 0x105
#define ESP_ERR_TIMEOUT 0x107

#define ESP_ERROR_CHECK(x) do { esp_err_t __e = (x); (void)__e; } while(0)

#endif /* _ESP_ERR_H_ */
