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
    /* Renderer und Treiber initialisieren */
    renderer_init();
    ESP_LOGI(TAG, "Display subsystem initialized (stub)");
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
            widgets_draw_speed((uint16_t)gps.speed_kmh);
            widgets_draw_nav_arrow((uint16_t)gps.course_deg, 0);
        }

        if (have_sens) {
            widgets_draw_rpm_bar(sens.rpm, 8000);
            widgets_draw_gear(sens.gear);
            widgets_draw_altitude(sens.altitude_m);
        }

        /* Framebuffer an Display senden (Partial-Refresh bei E-Paper) */
        renderer_flush(true);

        /* 1Hz Refresh-Rate */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
