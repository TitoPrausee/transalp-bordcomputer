#ifndef _EVENT_BUS_H_
#define _EVENT_BUS_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/*
 * Event-Bus: Pub/Sub System fuer Inter-Task-Kommunikation
 *
 * Verwendet FreeRTOS EventGroups fuer einfache State-Flags.
 * Fuer Datenuebergabe siehe Queues in den jeweiligen Modulen.
 */

/* Event-Bits */
#define EVT_GPS_FIX            (1 << 0)
#define EVT_GPS_LOST_FIX       (1 << 1)
#define EVT_SENSOR_READY       (1 << 2)
#define EVT_CRASH_DETECTED     (1 << 3)
#define EVT_BATTERY_LOW        (1 << 4)
#define EVT_BATTERY_CRITICAL   (1 << 5)
#define EVT_SDCARD_MOUNTED     (1 << 6)
#define EVT_SDCARD_ERROR       (1 << 7)
#define EVT_BLE_CONNECTED      (1 << 8)
#define EVT_BLE_DISCONNECTED   (1 << 9)
#define EVT_DISPLAY_READY     (1 << 10)
#define EVT_SHUTDOWN           (1 << 11)
#define EVT_OTA_START          (1 << 12)
#define EVT_OTA_DONE           (1 << 13)
#define EVT_OVERTEMPERATURE     (1 << 14)
#define EVT_FUEL_LOW            (1 << 15)
#define EVT_RPM_TIMEOUT         (1 << 16)
#define EVT_CARPLAY_CONNECTED   (1 << 17)
#define EVT_CARPLAY_DISCONNECTED (1 << 18)

/* API */
void event_bus_init(void);
void event_bus_set(uint32_t bits);
void event_bus_clear(uint32_t bits);
bool event_bus_wait(uint32_t bits, uint32_t timeout_ms);

/* Globale Event-Group Handle */
extern EventGroupHandle_t g_event_group;

#endif /* _EVENT_BUS_H_ */
