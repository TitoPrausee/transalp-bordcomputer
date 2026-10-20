#ifndef _ESP_APP_DESC_H_
#define _ESP_APP_DESC_H_

/*
 * esp_app_desc.h — Mock for host-side unit tests of the OTA component.
 */

#include <stdint.h>

#define ESP_APP_DESC_MAGIC_WORD 0xABCD1234

typedef struct {
    uint32_t magic_word;
    uint32_t secure_version;
    char     version[32];
    char     project_name[16];
    char     time[16];
    char     date[16];
    char     idf_ver[16];
    uint8_t  app_elf_sha256[32];
} esp_app_desc_t;

static inline const esp_app_desc_t *esp_app_get_description(void)
{
    static esp_app_desc_t desc;
    desc.magic_word = ESP_APP_DESC_MAGIC_WORD;
    strncpy(desc.version, "1.0.0-test", sizeof(desc.version) - 1);
    desc.version[sizeof(desc.version) - 1] = '\0';
    strncpy(desc.project_name, "transalp", sizeof(desc.project_name) - 1);
    return &desc;
}

#endif /* _ESP_APP_DESC_H_ */
