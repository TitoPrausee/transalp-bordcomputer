#ifndef _ESP_PARTITION_H_
#define _ESP_PARTITION_H_

/*
 * esp_partition.h — Mock for host-side unit tests of the OTA component.
 * Provides the esp_partition_t type and a controllable esp_partition_get_sha256
 * used by ota_finish_and_verify() to compute the image hash.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define ESP_PARTITION_TYPE_APP     0
#define ESP_PARTITION_SUBTYPE_APP_OTA_0 0x10
#define ESP_PARTITION_SUBTYPE_APP_OTA_1 0x11

typedef struct {
    uint8_t  type;
    uint8_t  subtype;
    uint32_t address;
    uint32_t size;
    char     label[20];
    bool     encrypted;
} esp_partition_t;

/* Mock flash backing store (written by the esp_ota_ops mock). */
#define OTA_MOCK_FLASH_SIZE (4 * 1024 * 1024)
extern uint8_t  g_ota_flash[];
extern uint32_t g_ota_flash_len;

/* Controllable partition-state for ota_check_on_boot. */
extern uint8_t  g_ota_partition_state;        /* esp_ota_img_states_t */
extern uint8_t  g_ota_get_state_partition_ok;  /* 1 = ESP_OK */

/*
 * Mock esp_partition_get_sha256: derives a deterministic 32-byte hash from the
 * flash backing store so the OTA verify path can be exercised. The OTA code
 * delegates hashing to this ESP-IDF function, so a deterministic derivation is
 * sufficient to test the comparison logic in ota_finish_and_verify().
 */
static inline int esp_partition_get_sha256(const esp_partition_t *part,
                                            uint8_t *sha256)
{
    (void)part;
    if (sha256 == NULL) return -1;
    uint32_t len = g_ota_flash_len;
    for (int i = 0; i < 32; i++) {
        if (len > 0) {
            sha256[i] = (uint8_t)(g_ota_flash[i % len] ^ (uint8_t)(i * 0x5Au));
        } else {
            sha256[i] = (uint8_t)(i * 0x5Au);
        }
    }
    return 0;
}

#endif /* _ESP_PARTITION_H_ */
