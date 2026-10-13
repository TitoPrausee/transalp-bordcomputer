#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * storage.h — SD-Karte, Logger, Konfiguration
 *
 * SD-Karte ueber SPI (shared mit Display).
 * FATFS fuer CSV/GPX/Binaer-Logging.
 */

#define SD_MOUNT_POINT  "/sdcard"

/* API */
void storage_init(void);

/* SD-Karte */
bool sd_card_mount(void);
bool sd_card_is_mounted(void);

/* Logger */
void logger_init(void);
void logger_task(void *arg);

/* Konfiguration (JSON auf SD) */
void config_init(void);
int config_load(void *cfg, uint16_t max_len);
int config_save(const void *cfg, uint16_t len);

#endif /* _STORAGE_H_ */
