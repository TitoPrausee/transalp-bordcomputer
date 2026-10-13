/*
 * storage.c — Storage-Modul: SD-Karte + Logger + Config initialisieren
 */

#include "storage.h"
#include "esp_log.h"

static const char *TAG = "storage";

void storage_init(void)
{
    /* SD-Karte initialisieren und mounten */
    sd_card_init();

    /* Konfiguration aus NVS/JSON laden */
    config_init();

    ESP_LOGI(TAG, "Storage subsystem initialized");
}
