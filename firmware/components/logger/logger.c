#include "logger.h"
#include "esp_log.h"
#include "gps.h"
#include "sensors.h"
#include <string.h>

static const char *TAG = "logger";
static FILE *log_file = NULL;

#define MOUNT_POINT "/sdcard"

void logger_init(void)
{
    /* TODO: SPI + SD-Karte mounten (FATFS) */
    /* TODO: Neue Track-Datei öffnen: /sdcard/logs/SESSION_YYYYMMDD_HHMMSS.csv */
    ESP_LOGI(TAG, "Logger init (stub)");
}

void logger_task(void *arg)
{
    gps_data_t gps;
    sensor_data_t sens;

    while (1) {
        if (gps_data_queue && xQueuePeek(gps_data_queue, &gps, 0) == pdTRUE) {
            /* TODO: GPS-Daten in CSV schreiben */
            /* lat,lon,speed,course,alt,sats,fix,timestamp */
        }
        if (sensor_data_queue && xQueuePeek(sensor_data_queue, &sens, 0) == pdTRUE) {
            /* TODO: Sensor-Daten in CSV schreiben */
            /* temp,pressure,altitude,roll,pitch,accel,crash,timestamp */
        }
        vTaskDelay(pdMS_TO_TICKS(200)); /* 5Hz */
    }
}
