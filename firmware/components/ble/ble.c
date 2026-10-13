/*
 * ble.c — BLE-Task: Event-Loop
 *
 * NimBLE laeuft intern in einem eigenen Host-Task (nimble_port_freertos_init).
 * Dieser Task ist fuer anwendungsseitige BLE-Events zustaendig.
 */

#include "ble.h"
#include "esp_log.h"

static const char *TAG = "ble_task";

void ble_task(void *arg)
{
    /*
     * NimBLE-Host-Task wird in ble_init() gestartet.
     * Hier: anwendungsseitige Event-Behandlung.
     */

    while (1) {
        /* TODO: BLE-Events verarbeiten:
         *   - Connect/Disconnect -> Event-Bus
         *   - Neue GPS/Sensor-Daten -> Notify an App
         *   - OTA-Chunks empfangen -> esp_ota_write
         *   - Navigation-Daten empfangen -> an Display-Task
         */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
