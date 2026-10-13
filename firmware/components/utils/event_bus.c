/*
 * event_bus.c — Pub/Sub Event-System ueber FreeRTOS EventGroups
 *
 * TODO: Fuer komplexe Events (mit Payload) eine zweite Ebene mit
 *       Queues oder esp_event_loop hinzufuegen.
 */

#include "event_bus.h"
#include "esp_log.h"

static const char *TAG = "event_bus";
EventGroupHandle_t g_event_group = NULL;

void event_bus_init(void)
{
    g_event_group = xEventGroupCreate();
    if (g_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }
    ESP_LOGI(TAG, "Event bus initialized");
}

void event_bus_set(uint32_t bits)
{
    if (g_event_group) {
        xEventGroupSetBits(g_event_group, (EventBits_t)bits);
    }
}

void event_bus_clear(uint32_t bits)
{
    if (g_event_group) {
        xEventGroupClearBits(g_event_group, (EventBits_t)bits);
    }
}

bool event_bus_wait(uint32_t bits, uint32_t timeout_ms)
{
    if (!g_event_group) return false;
    EventBits_t result = xEventGroupWaitBits(
        g_event_group,
        (EventBits_t)bits,
        pdTRUE,         /* clear on exit */
        pdFALSE,        /* any bit */
        pdMS_TO_TICKS(timeout_ms));
    return (result & bits) != 0;
}
