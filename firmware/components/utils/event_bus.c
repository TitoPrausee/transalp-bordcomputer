/*
 * event_bus.c — Pub/Sub Event-System ueber FreeRTOS EventGroups
 *
 * Bit-flag events with optional bridge forwarding to the
 * payload-capable app_event_loop.  When a bridge callback is
 * registered, event_bus_set() forwards the raw bits after
 * updating the EventGroup, so subscribers on app_event_loop
 * see overlapping events (BLE_CONNECTED, OTA_START, etc.)
 * without each producer needing to call both APIs.
 */

#include "event_bus.h"
#include "esp_log.h"

static const char *TAG = "event_bus";
EventGroupHandle_t g_event_group = NULL;

static event_bus_bridge_cb_t s_bridge_cb = NULL;

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
    if (s_bridge_cb) {
        s_bridge_cb(bits);
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

void event_bus_set_bridge_cb(event_bus_bridge_cb_t cb)
{
    s_bridge_cb = cb;
    ESP_LOGI(TAG, "Bridge callback %s", cb ? "registered" : "cleared");
}
