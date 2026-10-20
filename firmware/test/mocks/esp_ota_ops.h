#ifndef _ESP_OTA_OPS_H_
#define _ESP_OTA_OPS_H_

/*
 * esp_ota_ops.h — Mock for host-side unit tests of the OTA component.
 *
 * Provides the ESP-IDF OTA API surface used by ota_update.c with a functional
 * in-RAM backing store so the BLE OTA transfer path can be exercised:
 *   - esp_ota_begin / esp_ota_write / esp_ota_end / esp_ota_abort
 *   - esp_ota_get_next_update_partition / esp_ota_get_running_partition
 *   - esp_ota_set_boot_partition / esp_ota_get_state_partition
 *   - esp_ota_mark_app_valid_cancel_rollback / mark_app_invalid_rollback_and_reboot
 *
 * esp_ota_write appends to g_ota_flash (defined in esp_partition.h mock) so the
 * test can verify written bytes and so esp_partition_get_sha256 derives a
 * matching hash over the actual written data.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_partition.h"
#include "esp_err.h"

#define OTA_SIZE_UNKNOWN     0xFFFFFFFFu

typedef int   esp_ota_handle_t;
typedef int   esp_ota_img_states_t;

#define ESP_OTA_IMG_NEW             0
#define ESP_OTA_IMG_PENDING_VERIFY  1
#define ESP_OTA_IMG_VALID           2
#define ESP_OTA_IMG_INVALID         3

/* Controllable mock outcomes (defaults: success). */
extern int  g_ota_begin_result;        /* esp_ota_begin return */
extern int  g_ota_end_result;          /* esp_ota_end return */
extern int  g_ota_write_result;        /* esp_ota_write return */
extern int  g_ota_set_boot_result;     /* esp_ota_set_boot_partition return */
extern int  g_restart_called;          /* set by esp_restart (esp_system.h) */
extern int  g_ota_abort_called;
extern int  g_ota_mark_valid_called;
extern int  g_ota_mark_invalid_called;

/* Reset all mock counters/outcomes to defaults. Defined in the test. */
void ota_mock_reset(void);

static inline const esp_partition_t *esp_ota_get_next_update_partition(const void *args)
{
    (void)args;
    static esp_partition_t p = {
        .type = ESP_PARTITION_TYPE_APP,
        .subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1,
        .address = 0x10000,
        .size = 3 * 1024 * 1024,
        .label = "ota_1",
        .encrypted = false,
    };
    return &p;
}

static inline const esp_partition_t *esp_ota_get_running_partition(void)
{
    static esp_partition_t p = {
        .type = ESP_PARTITION_TYPE_APP,
        .subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0,
        .address = 0x10000,
        .size = 3 * 1024 * 1024,
        .label = "ota_0",
        .encrypted = false,
    };
    return &p;
}

static inline esp_err_t esp_ota_begin(const esp_partition_t *partition,
                                       size_t image_size,
                                       esp_ota_handle_t *out_handle)
{
    (void)partition; (void)image_size;
    if (out_handle) *out_handle = 1;
    g_ota_flash_len = 0;
    return (esp_err_t)g_ota_begin_result;
}

static inline esp_err_t esp_ota_write(esp_ota_handle_t handle,
                                       const void *data, size_t size)
{
    (void)handle;
    if (g_ota_write_result != ESP_OK) return (esp_err_t)g_ota_write_result;
    if (data && size && g_ota_flash_len + size <= OTA_MOCK_FLASH_SIZE) {
        memcpy(&g_ota_flash[g_ota_flash_len], data, size);
        g_ota_flash_len += (uint32_t)size;
    }
    return ESP_OK;
}

static inline esp_err_t esp_ota_end(esp_ota_handle_t handle)
{
    (void)handle;
    return (esp_err_t)g_ota_end_result;
}

static inline esp_err_t esp_ota_abort(esp_ota_handle_t handle)
{
    (void)handle;
    g_ota_abort_called = 1;
    return ESP_OK;
}

static inline esp_err_t esp_ota_set_boot_partition(const esp_partition_t *partition)
{
    (void)partition;
    return (esp_err_t)g_ota_set_boot_result;
}

static inline esp_err_t esp_ota_get_state_partition(const esp_partition_t *partition,
                                                     esp_ota_img_states_t *state)
{
    (void)partition;
    if (state) *state = (esp_ota_img_states_t)g_ota_partition_state;
    return g_ota_get_state_partition_ok ? ESP_OK : ESP_FAIL;
}

static inline esp_err_t esp_ota_mark_app_valid_cancel_rollback(void)
{
    g_ota_mark_valid_called = 1;
    return ESP_OK;
}

static inline esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void)
{
    g_ota_mark_invalid_called = 1;
    return ESP_OK;
}

#endif /* _ESP_OTA_OPS_H_ */
