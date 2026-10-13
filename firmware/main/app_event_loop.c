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
 * ============================================================ */

#define APP_EVENT_QUEUE_LEN  32

static QueueHandle_t s_event_queue = NULL;
static TaskHandle_t   s_loop_task   = NULL;
static bool           s_running     = false;

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
        ESP_LOGW(TAG, "Event queue full, dropping event type %d", (int)type);
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
