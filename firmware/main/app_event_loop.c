/*
 * app_event_loop.c — Central Event-Loop with Subscriber Pattern
 *
 * A single FreeRTOS task drains an event queue and calls every
 * registered subscriber whose type mask matches. Producers call
 * app_event_dispatch() from any task; the data is copied into the
 * queue so the producer does not need to hold the buffer.
 */

#include "app_event_loop.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "event_loop";

/* ============================================================
 * Subscriber table
 * ============================================================ */

#define APP_EVENT_MAX_SUBSCRIBERS  16

typedef struct {
    bool                 active;
    uint32_t             type_mask;
    app_event_callback_t callback;
    void                *user_ctx;
} subscriber_t;

static subscriber_t s_subs[APP_EVENT_MAX_SUBSCRIBERS];
static SemaphoreHandle_t s_sub_mutex = NULL;

/* ============================================================
 * Event queue
 *
 * APP_EVENT_QUEUE_LEN is overridable from board_config.h so the
 * depth can be tuned without editing this source.  If the
 * diagnostics counters show non-zero dropped_events under load,
 * increase the queue depth there (e.g. to 64) and rebuild.
 * ============================================================ */

#ifndef APP_EVENT_QUEUE_LEN
#define APP_EVENT_QUEUE_LEN  32
#endif

/* Rate-limit window for drop warnings: at most one ESP_LOGW per
 * interval to avoid log spam under sustained burst load. */
#define APP_EVENT_DROP_WARN_INTERVAL_MS  1000

static QueueHandle_t s_event_queue = NULL;
static TaskHandle_t   s_loop_task   = NULL;
static bool           s_running     = false;

/* Drop/overflow counters (see app_event_loop_stats_t in the header). */
static uint32_t s_dropped_events      = 0;
static uint32_t s_dispatched_events   = 0;
static uint32_t s_drop_warn_count     = 0;
static uint32_t s_last_drop_type      = 0;
static uint32_t s_last_drop_time_ms   = 0;
static uint32_t s_last_drop_warn_time_ms = 0;

/* ============================================================
 * Event-loop task
 * ============================================================ */

static void app_event_loop_task(void *arg)
{
    (void)arg;
    app_event_t ev;

    ESP_LOGI(TAG, "Event loop task started on core %d", xPortGetCoreID());

    while (1) {
        if (xQueueReceive(s_event_queue, &ev, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        /* Iterate subscriber table under mutex */
        xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
        for (int i = 0; i < APP_EVENT_MAX_SUBSCRIBERS; i++) {
            if (!s_subs[i].active)
                continue;
            if (s_subs[i].type_mask & (1U << ev.type)) {
                /* Release mutex during callback to avoid deadlock
                 * if a callback itself calls dispatch/subscribe. */
                subscriber_t cb_copy = s_subs[i];
                xSemaphoreGive(s_sub_mutex);
                cb_copy.callback(&ev, cb_copy.user_ctx);
                xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
            }
        }
        xSemaphoreGive(s_sub_mutex);
    }
}

/* ============================================================
 * API Implementation
 * ============================================================ */

void app_event_loop_init(void)
{
    if (s_running) {
        ESP_LOGW(TAG, "Event loop already initialised");
        return;
    }

    /* Create subscriber mutex */
    s_sub_mutex = xSemaphoreCreateMutex();
    if (s_sub_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create subscriber mutex");
        return;
    }

    /* Clear subscriber table */
    memset(s_subs, 0, sizeof(s_subs));

    /* Reset statistics counters (fresh init starts clean) */
    s_dropped_events         = 0;
    s_dispatched_events       = 0;
    s_drop_warn_count        = 0;
    s_last_drop_type         = 0;
    s_last_drop_time_ms      = 0;
    s_last_drop_warn_time_ms = 0;

    /* Create event queue */
    s_event_queue = xQueueCreate(APP_EVENT_QUEUE_LEN, sizeof(app_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        vSemaphoreDelete(s_sub_mutex);
        s_sub_mutex = NULL;
        return;
    }

    /* Spawn the loop task on Core 0 (UI/Comms core) */
    BaseType_t ret = xTaskCreatePinnedToCore(
        app_event_loop_task,
        "event_loop",
        TASK_STACK_EVENT_LOOP,
        NULL,
        TASK_PRIO_EVENT_LOOP,
        &s_loop_task,
        TASK_CORE_UI);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create event loop task");
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
        vSemaphoreDelete(s_sub_mutex);
        s_sub_mutex = NULL;
        return;
    }

    s_running = true;
    ESP_LOGI(TAG, "Event loop initialised (queue=%d, subs=%d)",
             APP_EVENT_QUEUE_LEN, APP_EVENT_MAX_SUBSCRIBERS);
}

esp_err_t app_event_subscribe(uint32_t type_mask,
                              app_event_callback_t callback,
                              void *user_ctx)
{
    if (callback == NULL || s_sub_mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t result = ESP_ERR_NO_MEM;

    xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
    for (int i = 0; i < APP_EVENT_MAX_SUBSCRIBERS; i++) {
        if (!s_subs[i].active) {
            s_subs[i].active    = true;
            s_subs[i].type_mask = type_mask;
            s_subs[i].callback  = callback;
            s_subs[i].user_ctx  = user_ctx;
            result = ESP_OK;
            break;
        }
    }
    xSemaphoreGive(s_sub_mutex);

    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Subscriber table full (max %d)",
                 APP_EVENT_MAX_SUBSCRIBERS);
    }

    return result;
}

void app_event_unsubscribe(app_event_callback_t callback, void *user_ctx)
{
    if (callback == NULL || s_sub_mutex == NULL) {
        return;
    }

    xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
    for (int i = 0; i < APP_EVENT_MAX_SUBSCRIBERS; i++) {
        if (s_subs[i].active &&
            s_subs[i].callback == callback &&
            s_subs[i].user_ctx == user_ctx) {
            s_subs[i].active = false;
            break;
        }
    }
    xSemaphoreGive(s_sub_mutex);
}

void app_event_dispatch(app_event_type_t type,
                        const void *data,
                        uint16_t data_len)
{
    if (s_event_queue == NULL || !s_running) {
        return;
    }

    app_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type          = type;
    ev.timestamp_ms  = (uint32_t)(esp_timer_get_time() / 1000);
    ev.data_len      = (data_len > APP_EVENT_PAYLOAD_SIZE)
                       ? APP_EVENT_PAYLOAD_SIZE : data_len;
    if (data != NULL && ev.data_len > 0) {
        memcpy(ev.data, data, ev.data_len);
    }

    /* Try to enqueue; drop event if queue is full to avoid blocking
     * the caller (typically a real-time sensor task). */
    BaseType_t ret = xQueueSend(s_event_queue, &ev, 0);
    if (ret != pdTRUE) {
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        s_dropped_events++;
        s_last_drop_type    = (uint32_t)type;
        s_last_drop_time_ms = now_ms;

        /* Rate-limit the warning to one per APP_EVENT_DROP_WARN_INTERVAL_MS
         * so a sustained burst does not flood the log. */
        if ((int32_t)(now_ms - s_last_drop_warn_time_ms)
                >= (int32_t)APP_EVENT_DROP_WARN_INTERVAL_MS) {
            s_last_drop_warn_time_ms = now_ms;
            s_drop_warn_count++;
            ESP_LOGW(TAG,
                     "Event queue full, dropping event type %d "
                     "(total drops=%u, queue depth=%d)",
                     (int)type, (unsigned)s_dropped_events,
                     (int)APP_EVENT_QUEUE_LEN);
        }
    } else {
        s_dispatched_events++;
    }
}

void app_event_dispatch_simple(app_event_type_t type)
{
    app_event_dispatch(type, NULL, 0);
}

bool app_event_loop_is_running(void)
{
    return s_running;
}

void app_event_loop_get_stats(app_event_loop_stats_t *out)
{
    if (out == NULL) {
        return;
    }
    out->dropped_events     = s_dropped_events;
    out->dispatched_events   = s_dispatched_events;
    out->drop_warn_count    = s_drop_warn_count;
    out->last_drop_type     = s_last_drop_type;
    out->last_drop_time_ms  = s_last_drop_time_ms;
    out->queue_depth        = (uint32_t)APP_EVENT_QUEUE_LEN;
}

void app_event_loop_reset_stats(void)
{
    s_dropped_events         = 0;
    s_dispatched_events       = 0;
    s_drop_warn_count        = 0;
    s_last_drop_type         = 0;
    s_last_drop_time_ms      = 0;
    s_last_drop_warn_time_ms = 0;
    ESP_LOGI(TAG, "Event-loop statistics reset");
}

/* ============================================================
 * Event-Bus Bridge
 *
 * Maps overlapping EVT_* bit-flags to APP_EVENT_* dispatches
 * so that subscribers on the payload event loop see events
 * fired through the simple bit-flag API.  This closes gap H1
 * from the architecture review: producers that call
 * event_bus_set(EVT_BLE_CONNECTED) now also trigger every
 * subscriber registered for APP_EVENT_BLE_CONNECTED.
 *
 * The forwarder is registered with event_bus via
 * event_bus_set_bridge_cb() and runs in the caller's task
 * context (same as event_bus_set itself).
 * ============================================================ */

typedef struct {
    uint32_t          evt_bit;
    app_event_type_t  app_type;
} bridge_map_entry_t;

static const bridge_map_entry_t s_bridge_map[] = {
    { EVT_GPS_FIX,              APP_EVENT_GPS_FIX_ACQUIRED },
    { EVT_GPS_LOST_FIX,         APP_EVENT_GPS_FIX_LOST },
    { EVT_CRASH_DETECTED,       APP_EVENT_CRASH_DETECTED },
    { EVT_BATTERY_LOW,          APP_EVENT_BATTERY_LOW },
    { EVT_BATTERY_CRITICAL,     APP_EVENT_BATTERY_CRITICAL },
    { EVT_SDCARD_MOUNTED,       APP_EVENT_SDCARD_MOUNTED },
    { EVT_SDCARD_ERROR,         APP_EVENT_SDCARD_ERROR },
    { EVT_BLE_CONNECTED,        APP_EVENT_BLE_CONNECTED },
    { EVT_BLE_DISCONNECTED,     APP_EVENT_BLE_DISCONNECTED },
    { EVT_DISPLAY_READY,        APP_EVENT_DISPLAY_READY },
    { EVT_SHUTDOWN,             APP_EVENT_SHUTDOWN },
    { EVT_OTA_START,            APP_EVENT_OTA_START },
    { EVT_OTA_DONE,             APP_EVENT_OTA_DONE },
    { EVT_OVERTEMPERATURE,      APP_EVENT_OVERTEMPERATURE },
    { EVT_FUEL_LOW,             APP_EVENT_LOW_FUEL },
    { EVT_RPM_TIMEOUT,          APP_EVENT_RPM_TIMEOUT },
    { EVT_CARPLAY_CONNECTED,    APP_EVENT_CARPLAY_CONNECTED },
    { EVT_CARPLAY_DISCONNECTED, APP_EVENT_CARPLAY_DISCONNECTED },
};

#define BRIDGE_MAP_LEN  (sizeof(s_bridge_map) / sizeof(s_bridge_map[0]))

static void app_event_bridge_forwarder(uint32_t bits)
{
    for (int i = 0; i < (int)BRIDGE_MAP_LEN; i++) {
        if (bits & s_bridge_map[i].evt_bit) {
            app_event_dispatch_simple(s_bridge_map[i].app_type);
        }
    }
}

void app_event_bridge_init(void)
{
    event_bus_set_bridge_cb(app_event_bridge_forwarder);
    ESP_LOGI(TAG, "Event-bus bridge initialised (%d mappings)",
             (int)BRIDGE_MAP_LEN);
}
