#include "display.h"
#include "esp_log.h"
#include "gps.h"
#include "sensors.h"

static const char *TAG = "display";

void display_init(void)
{
    /* TODO: EPD-Treiber init (Waveshare 2.9") */
    /* SPI: MOSI=11, SCLK=12, CS=10, DC=14, RST=21, BUSY=47 */
    ESP_LOGI(TAG, "Display init (stub)");
}

void display_task(void *arg)
{
    gps_data_t gps;
    sensor_data_t sens;

    while (1) {
        /* Aktuelle Daten pullen */
        if (gps_data_queue && xQueuePeek(gps_data_queue, &gps, 0) == pdTRUE) {
            /* TODO: Speed, Position rendern */
        }
        if (sensor_data_queue && xQueuePeek(sensor_data_queue, &sens, 0) == pdTRUE) {
            /* TODO: Temp, Höhe, Neigung rendern */
        }
        /* TODO: EPD Partial-Refresh */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
