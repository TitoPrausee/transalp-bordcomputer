#ifndef _NVS_H_
#define _NVS_H_

/*
 * Mock header for host-side unit tests.
 */

#include <stdint.h>
#include <stddef.h>
#include "nvs_flash.h"

#define NVS_READONLY 0
#define NVS_READWRITE 1

static inline int nvs_open(const char *name, int mode, nvs_handle_t *h) {
    (void)name; (void)mode; if (h) *h = (void *)1; return 0;
}
static inline void nvs_close(nvs_handle_t h) { (void)h; }
static inline int nvs_get_i32(nvs_handle_t h, const char *k, int32_t *v) {
    (void)h; (void)k; if (v) *v = 0; return 0;
}
static inline int nvs_set_i32(nvs_handle_t h, const char *k, int32_t v) {
    (void)h; (void)k; (void)v; return 0;
}
static inline int nvs_get_u16(nvs_handle_t h, const char *k, uint16_t *v) {
    (void)h; (void)k; if (v) *v = 0; return 0;
}
static inline int nvs_set_u16(nvs_handle_t h, const char *k, uint16_t v) {
    (void)h; (void)k; (void)v; return 0;
}
static inline int nvs_get_blob(nvs_handle_t h, const char *k, void *d, size_t *l) {
    (void)h; (void)k; (void)d; if (l) *l = 0; return 0;
}
static inline int nvs_set_blob(nvs_handle_t h, const char *k, const void *d, size_t l) {
    (void)h; (void)k; (void)d; (void)l; return 0;
}
static inline int nvs_commit(nvs_handle_t h) { (void)h; return 0; }
static inline int nvs_erase_key(nvs_handle_t h, const char *k) { (void)h; (void)k; return 0; }
static inline int nvs_erase_all(nvs_handle_t h) { (void)h; return 0; }

#endif /* _NVS_H_ */
