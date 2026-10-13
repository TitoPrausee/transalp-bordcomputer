#ifndef _NVS_FLASH_H_
#define _NVS_FLASH_H_

/*
 * Mock header for host-side unit tests.
 */

typedef void *nvs_handle_t;

static inline int nvs_flash_init(void) { return 0; }
static inline int nvs_flash_erase(void) { return 0; }
static inline int nvs_flash_deinit(void) { return 0; }

#endif /* _NVS_FLASH_H_ */
