#ifndef _ESP_EFUSE_TABLE_H_
#define _ESP_EFUSE_TABLE_H_

/*
 * esp_efuse_table.h — Host mock for unit tests.
 * ESP_EFUSE_SECURE_VERSION is just a dummy pointer; the mock
 * esp_efuse_read_field_blob/esp_efuse_write_field_bit ignore it.
 */

#include <stdint.h>

static const uint32_t s_efuse_secure_version_dummy = 0;
#define ESP_EFUSE_SECURE_VERSION  (&s_efuse_secure_version_dummy)

#endif /* _ESP_EFUSE_TABLE_H_ */
