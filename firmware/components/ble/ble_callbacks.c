/*
 * ble_callbacks.c — GATT Read/Write/Notify Handler
 *
 * TODO: access_cb Funktionen fuer jede Characteristic implementieren.
 * TODO: Notify bei neuen GPS/Sensor-Daten (von Display- oder Sensor-Task).
 * TODO: OTA-Data-Handler: Firmware-Chunks entgegennehmen und esp_ota_write.
 * TODO: Navigation-Handler: Next-Turn/ETA-Daten empfangen und an Display weiterleiten.
 */

#include "ble.h"
#include "gatt_uuids.h"
#include "esp_log.h"

static const char *TAG = "ble_cb";

/* Extern connection handle from ble_service.c */
extern uint16_t s_conn_handle;

void ble_notify_gps_data(void *data, uint16_t len)
{
    /* TODO: GPS-Daten per Notify an verbundene App senden.
     *
     * if (s_conn_handle == 0) return;
     * struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
     * ble_gattc_notify(s_conn_handle, gps_attr_handle, om);
     */

    (void)data;
    (void)len;
}

void ble_notify_sensor_data(void *data, uint16_t len)
{
    /* TODO: Sensor-Daten per Notify senden. */
    (void)data;
    (void)len;
}

/*
 * TODO: GATT Access Callbacks
 *
 * static int ble_gps_data_access(uint16_t conn_handle, uint16_t attr_handle,
 *                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
 *     if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
 *         gps_data_t gps;
 *         // ... aus Queue lesen ...
 *         os_mbuf_append(ctxt->om, &gps, sizeof(gps));
 *         return 0;
 *     }
 *     return BLE_ATT_ERR_UNLIKELY;
 * }
 *
 * static int ble_nav_write_access(...) {
 *     // Navigation-Data von Smartphone empfangen
 *     // -> an Display-Task weiterleiten
 * }
 *
 * static int ble_ota_data_access(...) {
 *     // OTA-Chunks schreiben
 *     // -> esp_ota_write()
 * }
 */
