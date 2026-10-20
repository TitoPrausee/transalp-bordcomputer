#ifndef _NVS_H_
#define _NVS_H_

/*
 * Mock header for host-side unit tests.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "nvs_flash.h"

#define NVS_READONLY 0
#define NVS_READWRITE 1

static inline int nvs_open(const char *name, int mode, nvs_handle_t *h) {
    static int nvs_mock_handle_sentinel;
    (void)name; (void)mode; if (h) *h = &nvs_mock_handle_sentinel; return 0;
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

/* ============================================================
 * Functional u32 / u8 / str store for tests that need NVS
 * round-trip behaviour (e.g. OTA resume state).
 *
 * The store is a small fixed-size key/value table keyed by the
 * NVS key string. It is shared across all handles/namespaces in
 * the mock (sufficient for host unit tests).
 * ============================================================ */

#define NVS_MOCK_STORE_SIZE 32

typedef struct {
    char     key[32];
    uint8_t  valid;
    uint32_t u32;
    uint8_t  u8;
    char     str[64];
    size_t   str_len;
    uint8_t  has_u32;
    uint8_t  has_u8;
    uint8_t  has_str;
} nvs_mock_entry_t;

static nvs_mock_entry_t nvs_mock_store[NVS_MOCK_STORE_SIZE];

static inline nvs_mock_entry_t *nvs_mock_find(const char *k) {
    for (int i = 0; i < NVS_MOCK_STORE_SIZE; i++) {
        if (nvs_mock_store[i].valid && k &&
            strncmp(nvs_mock_store[i].key, k, sizeof(nvs_mock_store[i].key)) == 0) {
            return &nvs_mock_store[i];
        }
    }
    return NULL;
}

static inline nvs_mock_entry_t *nvs_mock_alloc(const char *k) {
    nvs_mock_entry_t *e = nvs_mock_find(k);
    if (e != NULL) return e;
    for (int i = 0; i < NVS_MOCK_STORE_SIZE; i++) {
        if (!nvs_mock_store[i].valid) {
            nvs_mock_store[i].valid = 1;
            strncpy(nvs_mock_store[i].key, k ? k : "", sizeof(nvs_mock_store[i].key) - 1);
            nvs_mock_store[i].key[sizeof(nvs_mock_store[i].key) - 1] = '\0';
            return &nvs_mock_store[i];
        }
    }
    return NULL;
}

static inline void nvs_mock_reset(void) {
    memset(nvs_mock_store, 0, sizeof(nvs_mock_store));
}

static inline int nvs_set_u32(nvs_handle_t h, const char *k, uint32_t v) {
    (void)h; nvs_mock_entry_t *e = nvs_mock_alloc(k);
    if (e) { e->u32 = v; e->has_u32 = 1; return 0; }
    return -1;
}
static inline int nvs_get_u32(nvs_handle_t h, const char *k, uint32_t *v) {
    (void)h; nvs_mock_entry_t *e = nvs_mock_find(k);
    if (e && e->has_u32) { if (v) *v = e->u32; return 0; }
    return -1;
}
static inline int nvs_set_u8(nvs_handle_t h, const char *k, uint8_t v) {
    (void)h; nvs_mock_entry_t *e = nvs_mock_alloc(k);
    if (e) { e->u8 = v; e->has_u8 = 1; return 0; }
    return -1;
}
static inline int nvs_get_u8(nvs_handle_t h, const char *k, uint8_t *v) {
    (void)h; nvs_mock_entry_t *e = nvs_mock_find(k);
    if (e && e->has_u8) { if (v) *v = e->u8; return 0; }
    return -1;
}
static inline int nvs_set_str(nvs_handle_t h, const char *k, const char *v) {
    (void)h; nvs_mock_entry_t *e = nvs_mock_alloc(k);
    if (!e || !v) return -1;
    strncpy(e->str, v, sizeof(e->str) - 1);
    e->str[sizeof(e->str) - 1] = '\0';
    e->str_len = strlen(e->str);
    e->has_str = 1;
    return 0;
}
static inline int nvs_get_str(nvs_handle_t h, const char *k, char *out, size_t *len) {
    (void)h; nvs_mock_entry_t *e = nvs_mock_find(k);
    if (!(e && e->has_str)) return -1;
    size_t need = e->str_len + 1;
    if (len) {
        if (*len < need) { *len = need; return -1; }
        *len = need;
    }
    if (out) memcpy(out, e->str, need);
    return 0;
}

#endif /* _NVS_H_ */
