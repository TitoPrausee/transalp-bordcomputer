#ifndef _ESP_EFUSE_H_
#define _ESP_EFUSE_H_

/*
 * esp_efuse.h — Host mock for unit tests.
 * Provides controllable eFuse read/write for the anti-rollback tests.
 *
 * Controllable globals (defined in the test):
 *   g_efuse_read_result     — esp_efuse_read_field_blob return (ESP_OK = OK)
 *   g_efuse_write_result    — esp_efuse_write_field_bit return
 *   g_efuse_raw_value       — raw value returned by read_field_blob
 *   g_efuse_write_call_count — incremented by each write_field_bit
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_err.h"

extern esp_err_t g_efuse_read_result;
extern esp_err_t g_efuse_write_result;
extern uint32_t  g_efuse_raw_value;
extern int       g_efuse_write_call_count;

static inline esp_err_t esp_efuse_read_field_blob(const void *field,
                                                   void *dst, size_t bit_count)
{
    (void)field; (void)bit_count;
    if (dst) memcpy(dst, &g_efuse_raw_value, sizeof(uint32_t));
    return g_efuse_read_result;
}

static inline esp_err_t esp_efuse_write_field_bit(const void *field)
{
    (void)field;
    g_efuse_write_call_count++;
    g_efuse_raw_value |= (1u << (g_efuse_write_call_count - 1));
    return g_efuse_write_result;
}

#endif /* _ESP_EFUSE_H_ */
