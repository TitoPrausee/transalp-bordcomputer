#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "gps.h"
#include "display.h"
#include "sensors.h"
#include "logger.h"
#include "ble.h"
#include "power.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Transalp Bordcomputer booting...");

    /* Init顺序: Power zuerst, dann Busse, dann Peripherie */
    power_init();
    sensors_init();
    gps_init();
    display_init();
    logger_init();
    ble_init();

    /* Tasks starten — siehe ARCHITECTURE.md für Prioritäten/Cores */
    xTaskCreatePinnedToCore(gps_task,     "gps",     4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(sensor_task,  "sensors", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(display_task, "display", 6144, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(logger_task,  "logger",  4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(ble_task,     "ble",     4096, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(power_task,   "power",   2048, NULL, 6, NULL, 0);

    ESP_LOGI(TAG, "All tasks started. Free heap: %lu bytes",
             (unsigned long)esp_get_free_heap_size());
}
