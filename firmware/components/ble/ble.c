#include "ble.h"
#include "esp_log.h"

static const char *TAG = "ble";

void ble_init(void)
{
    /* TODO: Bluedroid oder NimBLE stack init */
    /* TODO: GATT services:
     *   - GPS Data (read/notify)
     *   - Sensor Data (read/notify)
     *   - Config (read/write)
     *   - OTA Control (write)
     */
    ESP_LOGI(TAG, "BLE init (stub)");
}

void ble_task(void *arg)
{
    while (1) {
        /* TODO: BLE events verarbeiten */
        /* TODO: Smartphone-Kommunikation:
         *   - Navigation-Targets empfangen
         *   - Wetterdaten empfangen
         *   - Phone-Notifications durchreichen
         *   - OTA-Firmware-Chunks schreiben
         */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
