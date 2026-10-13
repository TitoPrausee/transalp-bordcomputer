/*
 * ble_service.c — NimBLE GATT-Server Setup
 *
 * Initialisiert den NimBLE-Stack und registriert die GATT-Services.
 *
 * TODO: NimBLE initialisieren (nimble_port_init).
 * TODO: Security Manager konfigurieren (Just Works, Secure Connections).
 * TODO: Advertising starten (General Discoverable, Connectable).
 * TODO: MTU auf 247 verhandeln.
 * TODO: Connection-Parameter nach Connect optimieren (7.5ms-20ms).
 *
 * Siehe docs/research/03-software-stack.md Abschnitt 7.
 */

#include "ble.h"
#include "gatt_uuids.h"
#include "esp_log.h"

/* NimBLE headers — in ESP-IDF v5.x:
 * #include "esp_bt.h"
 * #include "esp_bt_main.h"
 * #include "nimble/nimble_port.h"
 * #include "nimble/nimble_port_freertos.h"
 * #include "host/ble_hs.h"
 */

static const char *TAG = "ble";

/* Active connection handle (0 = no connection) */
static uint16_t s_conn_handle = 0;

/* TODO: GATT Service-Table definieren.
 *
 * static const struct ble_gatt_svc_def gatt_services[] = {
 *     {
 *         .type = BLE_GATT_SVC_TYPE_PRIMARY,
 *         .uuid = &svc_telemetry_uuid.u,
 *         .characteristics = (struct ble_gatt_chr_def[]) {
 *             {
 *                 .uuid = &chr_gps_uuid.u,
 *                 .access_cb = ble_gps_data_access,
 *                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
 *             },
 *             { 0 },
 *         },
 *     },
 *     { 0 },
 * };
 */

void ble_init(void)
{
    /* TODO: NimBLE Stack initialisieren.
     *
     * ESP_ERROR_CHECK(nimble_port_init());
     * ble_hs_cfg.sync_cb = ble_on_sync;
     * ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
     * ble_hs_cfg.sm_bonding = 1;
     * ble_hs_cfg.sm_mitm = 0;
     * ble_hs_cfg.sm_sc = 1;
     *
     * ble_svc_gap_device_name_set("Transalp-BC");
     * ble_svc_gatt_init();
     * ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_services));
     * ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_services));
     *
     * nimble_port_freertos_init(ble_host_task);
     */

    ESP_LOGI(TAG, "BLE init (stub)");
}

bool ble_is_connected(void)
{
    return s_conn_handle != 0;
}

/*
 * TODO: BLE Host-Task (NimBLE)
 *
 * static void ble_host_task(void *param) {
 *     nimble_port_run();
 *     nimble_port_freertos_deinit();
 * }
 *
 * static void ble_on_sync(void) {
 *     uint8_t addr_type;
 *     ble_hs_id_infer_auto(0, &addr_type);
 *     // Advertising starten
 *     ble_gap_adv_start(addr_type, NULL, BLE_HS_FOREVER, &adv_params,
 *                       ble_gap_event_handler, NULL);
 * }
 */
