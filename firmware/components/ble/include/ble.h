#ifndef _BLE_H_
#define _BLE_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * ble.h — BLE GATT-Server (NimBLE)
 *
 * Services (siehe docs/research/13-connectivity-app.md Abschnitt 2):
 *   1. Navigation  (0x1000) — Maneuver, Route, Road, Status, Cmd
 *   2. Media       (0x2000) — Now-Playing, Control, Volume
 *   3. Phone       (0x3000) — Incoming-Call, Status, Control, Notif
 *   4. Weather     (0x4000) — Current, Forecast, Alert
 *   5. Telemetry   (0x5000) — Live, Snapshot, Track-Export
 *   6. Config      (0x6000) — Read, Write, Export, Import, Bond-Mgmt
 *   7. Diagnose    (0x7000) — DTC-List, Sensor-Status, Self-Test, Log
 *   8. OTA         (0x8000) — Control, Data, Verify
 *
 * Thread-safety: BLE callbacks run in the NimBLE host task.
 * Data from sensor/GPS tasks is copied into thread-safe caches
 * protected by mutexes. Notify calls are safe from any task.
 */

/* ============================================================
 * Lifecycle
 * ============================================================ */

/* Initialise NimBLE stack, register GATT services, start advertising. */
void ble_init(void);

/* BLE task: handles the write-command queue and periodic notifies. */
void ble_task(void *arg);

/* ============================================================
 * Connection state
 * ============================================================ */

bool ble_is_connected(void);

/* ============================================================
 * Notify helpers — callable from any task
 * ============================================================ */

/* Push live telemetry (22 bytes packed, TLM_LIVE 0x5001) at 5 Hz. */
void ble_notify_telemetry_live(void);

/* Push GPS data (triggered by GPS fix event). */
void ble_notify_gps_data(void *data, uint16_t len);

/* Push sensor data (triggered by sensor update). */
void ble_notify_sensor_data(void *data, uint16_t len);

/* Push DTC list on new DTC (indicate). */
void ble_notify_dtc_list(void);

/* Push sensor status on status change (notify). */
void ble_notify_sensor_status(void);

/* ============================================================
 * Event-bus callbacks — registered by ble_service.c
 * ============================================================ */

/* Called by event_bus when sensor data is updated. */
void ble_on_sensor_update(void *data, uint16_t len);

/* Called by event_bus when GPS data is updated. */
void ble_on_gps_update(void *data, uint16_t len);

#endif /* _BLE_H_ */
