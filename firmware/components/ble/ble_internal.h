#ifndef _BLE_INTERNAL_H_
#define _BLE_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* Forward-declare NimBLE types to avoid pulling host headers here */
struct ble_gap_event;

/*
 * ble_internal.h — Internal definitions shared between
 * ble_service.c and ble_callbacks.c.
 * NOT part of the public BLE API.
 */

/* ============================================================
 * Attribute handle indices
 *
 * NimBLE assigns handles dynamically. We pass &ble_val_handles[idx]
 * as .val_handle in each characteristic definition. After service
 * registration, NimBLE fills these with the actual handles.
 * ============================================================ */

enum {
    CHR_H_NAV_MANEUVER,
    CHR_H_NAV_ROUTE,
    CHR_H_NAV_ROAD,
    CHR_H_NAV_STATUS,
    CHR_H_NAV_CMD,

    CHR_H_MEDIA_NOWPLAYING,
    CHR_H_MEDIA_CONTROL,
    CHR_H_MEDIA_VOLUME,

    CHR_H_PHONE_INCOMING,
    CHR_H_PHONE_STATUS,
    CHR_H_PHONE_CONTROL,
    CHR_H_PHONE_NOTIF,

    CHR_H_WX_CURRENT,
    CHR_H_WX_FORECAST,
    CHR_H_WX_ALERT,

    CHR_H_TLM_LIVE,
    CHR_H_TLM_SNAPSHOT,
    CHR_H_TRACK_EXPORT,
    CHR_H_TRACK_EXPORT_CTRL,

    CHR_H_CONFIG_READ,
    CHR_H_CONFIG_WRITE,
    CHR_H_CONFIG_EXPORT,
    CHR_H_CONFIG_IMPORT,
    CHR_H_BOND_MGMT,

    CHR_H_DTC_LIST,
    CHR_H_SENSOR_STATUS,
    CHR_H_SELF_TEST,
    CHR_H_LOG_EXPORT,
    CHR_H_LOG_EXPORT_CTRL,

    CHR_H_OTA_CONTROL,
    CHR_H_OTA_DATA,
    CHR_H_OTA_VERIFY,

    CHR_H_COUNT,
};

/* Handle table — filled by NimBLE during service registration */
extern uint16_t ble_val_handles[CHR_H_COUNT];

/* ============================================================
 * GAP event handler (defined in ble_service.c)
 * ============================================================ */

int ble_gap_event_handler(struct ble_gap_event *event, void *arg);

/* ============================================================
 * CCCD subscription state queries (defined in ble_service.c)
 * ============================================================ */

bool ble_cccd_telemetry_enabled(void);
bool ble_cccd_gps_enabled(void);
bool ble_cccd_dtc_enabled(void);
bool ble_cccd_sensor_status_enabled(void);

/* Get current connection handle (thread-safe) */
uint16_t ble_get_conn_handle(void);

/* ============================================================
 * GATT access callback and read/write handlers
 * (defined in ble_callbacks.c)
 * ============================================================ */

struct ble_gatt_access_ctxt;

int ble_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt, void *arg);
int ble_handle_read(uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt);
int ble_handle_write(uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt);

/* ============================================================
 * Write-command queue (defined in ble_callbacks.c)
 *
 * Incoming write commands are enqueued here by the GATT access
 * callback and processed asynchronously in ble_task().
 * ============================================================ */

typedef enum {
    BLE_CMD_NONE = 0,
    BLE_CMD_CONFIG_WRITE,
    BLE_CMD_CONFIG_IMPORT,
    BLE_CMD_BOND_DELETE,
    BLE_CMD_NAV_CMD,
    BLE_CMD_MEDIA_CONTROL,
    BLE_CMD_MEDIA_VOLUME,
    BLE_CMD_PHONE_CONTROL,
    BLE_CMD_TRACK_EXPORT_CTRL,
    BLE_CMD_SELF_TEST,
    BLE_CMD_LOG_EXPORT_CTRL,
    BLE_CMD_OTA_CONTROL,
    BLE_CMD_OTA_DATA,
} ble_cmd_type_t;

typedef struct {
    ble_cmd_type_t type;
    uint16_t conn_handle;
    uint16_t len;
    uint8_t data[244];  /* MTU-3 max payload */
} ble_cmd_t;

extern QueueHandle_t ble_cmd_queue;

/* Enqueue a write command (called from NimBLE host task) */
bool ble_cmd_enqueue(ble_cmd_type_t type, uint16_t conn_handle,
                     const uint8_t *data, uint16_t len);

#endif /* _BLE_INTERNAL_H_ */
