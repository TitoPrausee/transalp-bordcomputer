#ifndef _BLE_H_
#define _BLE_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * ble.h — BLE GATT-Server (NimBLE)
 *
 * Services (siehe docs/research/03-software-stack.md Abschnitt 7.2):
 *   1. Telemetry-Service (GPS, RPM, Sensors, Battery — notify)
 *   2. Config-Service (Display-Mode, Log-Interval, Calibration — read/write)
 *   3. Navigation-Service (Next-Turn, ETA, Route-Active — write)
 *   4. OTA-Service (OTA-Control, OTA-Data, OTA-Status — write/notify)
 */

/* API */
void ble_init(void);
void ble_task(void *arg);

/* Notify-Helfer (von anderen Tasks aufgerufen) */
void ble_notify_gps_data(void *data, uint16_t len);
void ble_notify_sensor_data(void *data, uint16_t len);

/* Connection state */
bool ble_is_connected(void);

#endif /* _BLE_H_ */
