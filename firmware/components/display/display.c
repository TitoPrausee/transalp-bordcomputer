/*
 * display.c — Display-Task: 1Hz Refresh-Zyklus
 *
 * Laeuft auf Core 0. Pullt GPS- und Sensor-Daten aus den Queues
 * und rendert die UI ueber den Renderer.
 */

#include "display.h"
#include "board_config.h"
#include "gps.h"
#include "sensors.h"
#include "esp_log.h"

static const char *TAG = "display";

void display_init(void)
{
    /* Renderer und Treiber initialisieren.
     * Default: E-Paper (wird zur Laufzeit aus NVS geladen in Zukunft).
     * Fuer TFT-Boards: renderer_init(DISPLAY_TYPE_TFT).
     */
    renderer_init(DISPLAY_TYPE_EPAPER);
    ESP_LOGI(TAG, "Display subsystem initialized");
}

void display_task(void *arg)
{
    gps_data_t gps;
    sensor_data_t sens;

    while (1) {
        /* Aktuelle Daten pullen (non-blocking) */
        bool have_gps = (gps_data_queue &&
                         xQueuePeek(gps_data_queue, &gps, 0) == pdTRUE);
        bool have_sens = (sensor_data_queue &&
                          xQueuePeek(sensor_data_queue, &sens, 0) == pdTRUE);

        /* Framebuffer loeschen und neu zeichnen */
        renderer_clear();

        if (have_gps) {
            widgets_draw_speed((uint16_t)gps.speed_kmh, 5, 10, 4);
            widgets_draw_altitude(gps.altitude_m, 180, 10);
        }

        if (have_sens) {
            widgets_draw_rpm_bar(sens.rpm, 8000, 60, 50, 180, 16);
            widgets_draw_gear(sens.gear, 200, 50);
            widgets_draw_temp((int8_t)sens.coolant_temp_c, 5, 90);
            widgets_draw_fuel((uint8_t)sens.fuel_level_l, 20, 5, 90);
        }

        /* Framebuffer an Display senden (Partial-Refresh bei E-Paper) */
        renderer_flush(true);

        /* 1Hz Refresh-Rate fuer E-Paper (30 FPS fuer TFT spaeter) */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
