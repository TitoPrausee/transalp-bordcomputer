#ifndef _APP_EVENT_LOOP_H_
#define _APP_EVENT_LOOP_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/*
 * app_event_loop.h — Central Event-Loop with Subscriber Pattern
 *
 * Components register callbacks for event types. Producers dispatch
 * events through app_event_dispatch(), which enqueues them. The
 * event-loop task dequeues and calls all registered subscribers.
 *
 * Example flow:
 *   GPS fix update -> dispatch(APP_EVENT_GPS_UPDATE, &gps_data)
 *     -> display subscriber refreshes widgets
 *     -> logger subscriber writes GPX track point
 *     -> BLE subscriber sends notify
 */

/* ============================================================
 * Event Types
 * ============================================================ */

typedef enum {
    APP_EVENT_SENSOR_UPDATE = 0,    /* New sensor data available     */
    APP_EVENT_GPS_UPDATE,           /* GPS position/speed/course      */
    APP_EVENT_GPS_FIX_ACQUIRED,     /* Transition: no-fix -> fix      */
    APP_EVENT_GPS_FIX_LOST,         /* Transition: fix -> no-fix      */
    APP_EVENT_DISPLAY_READY,        /* Display initialised and ready   */
    APP_EVENT_SDCARD_MOUNTED,       /* SD card mounted successfully    */
    APP_EVENT_SDCARD_ERROR,         /* SD card I/O failure             */
    APP_EVENT_BLE_CONNECTED,        /* Peer connected                  */
    APP_EVENT_BLE_DISCONNECTED,     /* Peer disconnected               */
    APP_EVENT_BATTERY_LOW,          /* Battery < warning threshold     */
    APP_EVENT_BATTERY_CRITICAL,     /* Battery < critical threshold    */
    APP_EVENT_OVERTEMPERATURE,      /* Coolant above 120 C             */
    APP_EVENT_LOW_FUEL,             /* Fuel below reserve (3 L)        */
    APP_EVENT_CRASH_DETECTED,       /* IMU detected crash event        */
    APP_EVENT_RPM_TIMEOUT,          /* RPM signal lost                 */
    APP_EVENT_POWER_STATE_CHANGED,  /* Power state transition          */
    APP_EVENT_OTA_START,            /* OTA update initiated            */
    APP_EVENT_OTA_DONE,             /* OTA update completed            */
    APP_EVENT_SHUTDOWN,             /* System shutdown requested       */

    /* ---- Lenker-Controller events ---- */
    APP_EVENT_CONTROLLER_BASE,       /* Base offset for controller events */
    APP_EVENT_CTRL_NAV_UP      = APP_EVENT_CONTROLLER_BASE + 0,
    APP_EVENT_CTRL_NAV_DOWN    = APP_EVENT_CONTROLLER_BASE + 1,
    APP_EVENT_CTRL_NAV_LEFT    = APP_EVENT_CONTROLLER_BASE + 2,
    APP_EVENT_CTRL_NAV_RIGHT   = APP_EVENT_CONTROLLER_BASE + 3,
    APP_EVENT_CTRL_SELECT      = APP_EVENT_CONTROLLER_BASE + 4,
    APP_EVENT_CTRL_BACK        = APP_EVENT_CONTROLLER_BASE + 5,
    APP_EVENT_CTRL_HOME        = APP_EVENT_CONTROLLER_BASE + 6,
    APP_EVENT_CTRL_VOL_UP      = APP_EVENT_CONTROLLER_BASE + 7,
    APP_EVENT_CTRL_VOL_DOWN   = APP_EVENT_CONTROLLER_BASE + 8,
    APP_EVENT_CTRL_MEDIA_NEXT = APP_EVENT_CONTROLLER_BASE + 9,
    APP_EVENT_CTRL_MEDIA_PREV = APP_EVENT_CONTROLLER_BASE + 10,

    /* ---- CarPlay bridge events ---- */
    APP_EVENT_CARPLAY_CONNECTED,     /* CarPlay display connected      */
    APP_EVENT_CARPLAY_DISCONNECTED,   /* CarPlay display disconnected   */

    APP_EVENT_MAX,
} app_event_type_t;

/* ============================================================
 * Event Payload
 * ============================================================ */

/*
 * Events carry an opaque pointer + length. Producers pass a pointer
 * to a stack or static buffer; the event loop copies up to
 * APP_EVENT_PAYLOAD_SIZE bytes into its own queue storage so the
 * producer does not need to hold the data alive.
 *
 * For payloads larger than APP_EVENT_PAYLOAD_SIZE, pass a pointer
 * to a heap-allocated struct and manage lifetime manually.
 */
#define APP_EVENT_PAYLOAD_SIZE  128

typedef struct {
    app_event_type_t type;
    uint32_t         timestamp_ms;   /* FreeRTOS tick at dispatch time */
    uint16_t         data_len;       /* Valid bytes in data[]          */
    uint8_t          data[APP_EVENT_PAYLOAD_SIZE];
} app_event_t;

/* ============================================================
 * Subscriber Callback
 * ============================================================ */

typedef void (*app_event_callback_t)(const app_event_t *event, void *user_ctx);

/* ============================================================
 * API
 * ============================================================ */

/*
 * Initialise the event loop: creates the queue, subscriber table,
 * and spawns the event-loop task on Core 0.
 * Call once from app_main before starting other tasks.
 */
void app_event_loop_init(void);

/*
 * Register a subscriber for one or more event types.
 *   types       — bitmask of app_event_type_t values (1 << type)
 *   callback    — function called when a matching event is dispatched
 *   user_ctx    — opaque pointer passed back to the callback
 * Returns ESP_OK on success, ESP_ERR_NO_MEM if the subscriber table is full.
 */
esp_err_t app_event_subscribe(uint32_t type_mask,
                              app_event_callback_t callback,
                              void *user_ctx);

/*
 * Unregister a previously registered subscriber.
 */
void app_event_unsubscribe(app_event_callback_t callback, void *user_ctx);

/*
 * Dispatch an event. Copies up to data_len bytes (max APP_EVENT_PAYLOAD_SIZE)
 * from data into the internal queue. Safe to call from any task or ISR
 * (sets the queue from-task flag appropriately).
 *
 *   type     — event type
 *   data     — pointer to payload (may be NULL if data_len == 0)
 *   data_len — payload length in bytes
 */
void app_event_dispatch(app_event_type_t type,
                        const void *data,
                        uint16_t data_len);

/*
 * Dispatch a simple event with no payload.
 */
void app_event_dispatch_simple(app_event_type_t type);

/*
 * Check whether the event loop task is running.
 */
bool app_event_loop_is_running(void);

/*
 * Register the event-bus bridge forwarder.  After this call,
 * every event_bus_set(EVT_*) that has a mapping in the bridge
 * table also dispatches the corresponding APP_EVENT_* through
 * the subscriber event loop, so subscribers see both bit-flag
 * and payload events regardless of which API the producer used.
 *
 * Call once after event_bus_init() and app_event_loop_init().
 */
void app_event_bridge_init(void);

#endif /* _APP_EVENT_LOOP_H_ */
