/*
 * carplay_bridge.c — CarPlay BLE-Bridge (GATT-Server + Lifecycle)
 *
 * Bordcomputer-seitige CarPlay-Bridge. Stellt den GATT-Service 0x9000
 * (Telemetrie- und Status-Characteristics) bereit und integriert den
 * GATT-Client (carplay_client.c) fuer den Empfang von Navigations- und
 * Musik-Daten vom CarPlay-Display.
 *
 * Aufbau analog zu firmware/components/ble/ble_service.c:
 *   - Service-Tabelle mit .val_handle Indirection
 *   - Zentraler access_cb Dispatch nach attr_handle
 *   - Event-Bus-Integration (EVT_CARPLAY_CONNECTED/DISCONNECTED)
 *
 * Siehe docs/CARPLAY-INTEGRATION.md.
 */

#include "carplay_bridge.h"
#include "carplay_internal.h"
#include "carplay_uuids.h"

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"

#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "event_bus.h"
#include "app_event_loop.h"
#include "carplay_media.h"

static const char *TAG = "carplay";

/* ============================================================
 * Connection / state
 * ============================================================ */

static bool s_initialized = false;
static uint8_t s_status = CARPLAY_STATUS_DISCONNECTED;
static SemaphoreHandle_t s_status_mutex = NULL;

/* Telemetry cache (updated by sensor task, read by notify) */
static carplay_telemetry_input_t s_telemetry_input;
SemaphoreHandle_t carplay_data_mutex = NULL;  /* exported, extern in carplay_internal.h */
carplay_inbound_t carplay_inbound_cache;            /* exported */

/* CCCD subscription tracking for our server-side notify char */
static bool s_cccd_telemetry = false;

/* ============================================================
 * Handle table (filled by NimBLE after registration)
 * ============================================================ */

uint16_t carplay_val_handles[CP_H_COUNT] = {0};

/* Forward declaration: referenced by the static service table below,
 * defined later in this file. */
int carplay_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);

/* ============================================================
 * 128-bit UUID definitions
 * ============================================================ */

static ble_uuid128_t svc_carplay_uuid = CARPLAY_UUID128(SVC_CARPLAY_BRIDGE);

static ble_uuid128_t chr_cp_tlm_live     = CARPLAY_UUID128(CHR_CP_TLM_LIVE);
static ble_uuid128_t chr_cp_tlm_snapshot = CARPLAY_UUID128(CHR_CP_TLM_SNAPSHOT);
static ble_uuid128_t chr_cp_status       = CARPLAY_UUID128(CHR_CP_STATUS);
static ble_uuid128_t chr_cp_pairing      = CARPLAY_UUID128(CHR_CP_PAIRING);
static ble_uuid128_t chr_cp_bond_mgmt    = CARPLAY_UUID128(CHR_CP_BOND_MGMT);

/* ============================================================
 * GATT Service Table
 * ============================================================ */

static const struct ble_gatt_chr_def chr_carplay[] = {
    { .uuid = &chr_cp_tlm_live.u, .access_cb = carplay_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &carplay_val_handles[CP_H_TLM_LIVE] },
    { .uuid = &chr_cp_tlm_snapshot.u, .access_cb = carplay_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ,
      .val_handle = &carplay_val_handles[CP_H_TLM_SNAPSHOT] },
    { .uuid = &chr_cp_status.u, .access_cb = carplay_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &carplay_val_handles[CP_H_STATUS] },
    { .uuid = &chr_cp_pairing.u, .access_cb = carplay_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &carplay_val_handles[CP_H_PAIRING] },
    { .uuid = &chr_cp_bond_mgmt.u, .access_cb = carplay_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &carplay_val_handles[CP_H_BOND_MGMT] },
    { 0 },
};

static const struct ble_gatt_svc_def carplay_gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_carplay_uuid.u,
        .characteristics = chr_carplay,
    },
    { 0 },  /* terminator */
};

/* ============================================================
 * GATT registration hook (called from ble_init, before host start)
 * ============================================================ */

void carplay_bridge_register_gatt(void)
{
    int rc = ble_gatts_count_cfg(carplay_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "gatts_count_cfg rc=%d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(carplay_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "gatts_add_svcs rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "CarPlay-Bridge GATT service 0x%04X registered",
             SVC_CARPLAY_BRIDGE);
}

/* ============================================================
 * Notify helper
 * ============================================================ */

static void carplay_notify(uint16_t attr_handle,
                           const void *data, uint16_t len)
{
    /* The CarPlay display shares a single GATT connection that the
     * Bordcomputer established as a client (carplay_client.c). On that
     * same link the Bordcomputer also acts as GATT server. ble_hs_mbuf
     * + ble_gattc_notify are safe to call from any task. */
    uint16_t conn = carplay_client_get_conn_handle();
    if (conn == BLE_HS_CONN_HANDLE_NONE) {
        return;
    }

    /* Validate the handle is still the CarPlay connection before
     * sending.  This closes the conn-handle reuse TOCTOU (H4): if the
     * CarPlay link dropped and NimBLE reused the handle for a
     * companion-app peripheral connection, we skip the notify instead
     * of pushing CarPlay telemetry to the companion app. */
    if (!carplay_client_conn_is_valid(conn)) {
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        ESP_LOGW(TAG, "mbuf alloc failed for notify");
        return;
    }

    int rc = ble_gattc_notify_custom(conn, attr_handle, om);
    if (rc != 0 && rc != BLE_HS_ENOTCONN) {
        ESP_LOGW(TAG, "notify rc=%d handle=%d", rc, attr_handle);
    }
}

/* ============================================================
 * GATT Access Callback — central dispatcher
 * ============================================================ */

int carplay_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)arg;

    uint8_t buf[64];
    uint16_t len = 0;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        if (attr_handle == carplay_val_handles[CP_H_TLM_SNAPSHOT]) {
            /* Full telemetry snapshot (12 bytes) */
            carplay_telemetry_t pkt;
            carplay_telemetry_input_t in;
            xSemaphoreTake(carplay_data_mutex, portMAX_DELAY);
            in = s_telemetry_input;
            xSemaphoreGive(carplay_data_mutex);
            carplay_telemetry_build(&pkt, &in);
            memcpy(buf, &pkt, sizeof(pkt));
            len = sizeof(pkt);
        }
        else if (attr_handle == carplay_val_handles[CP_H_STATUS]) {
            xSemaphoreTake(s_status_mutex, portMAX_DELAY);
            buf[0] = s_status;
            xSemaphoreGive(s_status_mutex);
            len = 1;
        }
        else {
            ESP_LOGW(TAG, "Read on unknown handle %d", attr_handle);
            return BLE_ATT_ERR_UNLIKELY;
        }
    }
    else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t wlen = OS_MBUF_PKTLEN(ctxt->om);
        if (wlen > sizeof(buf)) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &wlen);
        if (rc != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        if (attr_handle == carplay_val_handles[CP_H_PAIRING]) {
            /* Expect 7 bytes: 6 addr + 1 addr_type */
            if (wlen < CARPLAY_PEER_ADDR_LEN + 1) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            carplay_bridge_pair(buf, buf[CARPLAY_PEER_ADDR_LEN]);
        }
        else if (attr_handle == carplay_val_handles[CP_H_BOND_MGMT]) {
            if (wlen >= 1 && buf[0] == 0x00) {
                carplay_bridge_unpair();
            }
        }
        else {
            ESP_LOGW(TAG, "Write on unknown handle %d", attr_handle);
            return BLE_ATT_ERR_UNLIKELY;
        }
    }
    else {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (len > 0) {
        int rc = os_mbuf_append(ctxt->om, buf, len);
        if (rc != 0) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }

    return 0;
}

/* ============================================================
 * Status / connection state helpers
 * ============================================================ */

static void carplay_set_status(uint8_t status)
{
    xSemaphoreTake(s_status_mutex, portMAX_DELAY);
    s_status = status;
    xSemaphoreGive(s_status_mutex);
}

uint8_t carplay_bridge_get_status(void)
{
    uint8_t status;
    xSemaphoreTake(s_status_mutex, portMAX_DELAY);
    status = s_status;
    xSemaphoreGive(s_status_mutex);
    return status;
}

bool carplay_bridge_is_connected(void)
{
    return carplay_bridge_get_status() == CARPLAY_STATUS_CONNECTED;
}

/* Called by carplay_client.c when the client connection comes up */
void carplay_client_on_connected(void)
{
    carplay_set_status(CARPLAY_STATUS_CONNECTED);
    s_cccd_telemetry = true;
    event_bus_set(EVT_CARPLAY_CONNECTED);
    ESP_LOGI(TAG, "CarPlay display connected");

    /* Notify status change */
    uint8_t st = CARPLAY_STATUS_CONNECTED;
    if (s_cccd_telemetry) {
        carplay_notify(carplay_val_handles[CP_H_STATUS], &st, 1);
    }
}

/* Called by carplay_client.c when the client connection drops */
void carplay_client_on_disconnected(void)
{
    s_cccd_telemetry = false;
    carplay_set_status(CARPLAY_STATUS_DISCONNECTED);
    event_bus_set(EVT_CARPLAY_DISCONNECTED);
    ESP_LOGI(TAG, "CarPlay display disconnected");
}

/* Called by carplay_client.c on incoming notify */
void carplay_client_on_notify(uint16_t attr_handle,
                              const uint8_t *data, uint16_t len)
{
    (void)attr_handle;
    if (data == NULL || len == 0) {
        return;
    }

    xSemaphoreTake(carplay_data_mutex, portMAX_DELAY);
    /* Minimal decode: first byte is the status/state enum */
    carplay_inbound_cache.nav_status = data[0];
    xSemaphoreGive(carplay_data_mutex);
}

/* ============================================================
 * Public API: telemetry
 * ============================================================ */

void carplay_bridge_notify_telemetry(void)
{
    if (!s_cccd_telemetry) {
        return;
    }

    carplay_telemetry_t pkt;
    carplay_telemetry_input_t in;
    xSemaphoreTake(carplay_data_mutex, portMAX_DELAY);
    in = s_telemetry_input;
    xSemaphoreGive(carplay_data_mutex);

    carplay_telemetry_build(&pkt, &in);
    carplay_notify(carplay_val_handles[CP_H_TLM_LIVE], &pkt, sizeof(pkt));
}

void carplay_bridge_on_sensor_update(void *data, uint16_t len)
{
    if (data == NULL || len < sizeof(carplay_telemetry_input_t)) {
        return;
    }

    xSemaphoreTake(carplay_data_mutex, portMAX_DELAY);
    memcpy(&s_telemetry_input, data, sizeof(carplay_telemetry_input_t));
    xSemaphoreGive(carplay_data_mutex);
}

bool carplay_bridge_get_inbound(void *out, uint16_t len)
{
    if (out == NULL || len < sizeof(carplay_inbound_t)) {
        return false;
    }

    xSemaphoreTake(carplay_data_mutex, portMAX_DELAY);
    memcpy(out, &carplay_inbound_cache, sizeof(carplay_inbound_t));
    xSemaphoreGive(carplay_data_mutex);
    return true;
}

/* ============================================================
 * Public API: pairing
 * ============================================================ */

bool carplay_bridge_pair(const uint8_t *addr, uint8_t addr_type)
{
    if (!carplay_pairing_store(addr, addr_type)) {
        return false;
    }

    carplay_peer_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.addr, addr, CARPLAY_PEER_ADDR_LEN);
    peer.addr_type = addr_type;
    peer.valid = true;

    /* Start the client connection attempt */
    carplay_client_start(&peer);
    return true;
}

bool carplay_bridge_get_paired(uint8_t *addr_out, uint8_t *addr_type_out)
{
    carplay_peer_t peer;
    if (!carplay_pairing_load(&peer) || !peer.valid) {
        return false;
    }

    if (addr_out) {
        memcpy(addr_out, peer.addr, CARPLAY_PEER_ADDR_LEN);
    }
    if (addr_type_out) {
        *addr_type_out = peer.addr_type;
    }
    return true;
}

void carplay_bridge_unpair(void)
{
    carplay_client_stop();
    (void)carplay_pairing_erase();
    carplay_set_status(CARPLAY_STATUS_DISCONNECTED);
}

/* ============================================================
 * Lenker-Controller -> CarPlay media-control forward path (gap C4 / FU-4)
 *
 * The handlebar controller dispatches APP_EVENT_CTRL_MEDIA_NEXT/PREV on the
 * payload event loop (controller_mapping_dispatch in hid_controller).  This
 * subscriber maps the event to a carplay_media_cmd byte and forwards it to
 * the peer display's CHR_CP_MEDIA_CTRL (0x9104) via the GATT client.
 *
 * Runs in the app_event_loop task context.  The write itself is non-blocking
 * (ble_gattc_write_flat, no callback), so it does not stall the loop.
 * ============================================================ */

static void carplay_on_ctrl_media(const app_event_t *event, void *user_ctx)
{
    (void)user_ctx;

    uint8_t cmd = carplay_media_cmd_from_event(event->type);
    if (cmd == 0) {
        return;  /* unmapped controller event (e.g. VOL_*) — no display protocol */
    }

    if (!carplay_bridge_is_connected()) {
        ESP_LOGD(TAG, "ctrl media evt=%d: CarPlay not connected, drop", event->type);
        return;
    }

    ESP_LOGI(TAG, "ctrl media evt=%d -> cmd=0x%02X", event->type, cmd);
    carplay_client_media_ctrl(cmd);
}

/* ============================================================
 * Lifecycle
 * ============================================================ */

void carplay_bridge_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "CarPlay-Bridge already initialized");
        return;
    }

    s_status_mutex = xSemaphoreCreateMutex();
    carplay_data_mutex = xSemaphoreCreateMutex();
    if (s_status_mutex == NULL || carplay_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutexes");
        return;
    }

    memset(&s_telemetry_input, 0, sizeof(s_telemetry_input));
    memset(&carplay_inbound_cache, 0, sizeof(carplay_inbound_cache));
    carplay_set_status(CARPLAY_STATUS_DISCONNECTED);

    /* Initialise GATT client */
    carplay_client_init();

    /* Subscribe to Lenker-Controller media events and forward them to the
     * CarPlay display's CHR_CP_MEDIA_CTRL (closes gap C4 / FU-4, D2-1/D2-4). */
    app_event_subscribe((1U << APP_EVENT_CTRL_MEDIA_NEXT) |
                        (1U << APP_EVENT_CTRL_MEDIA_PREV),
                        carplay_on_ctrl_media, NULL);

    /* If a paired display exists, start auto-connect */
    carplay_peer_t peer;
    if (carplay_pairing_load(&peer) && peer.valid) {
        ESP_LOGI(TAG, "Found paired CarPlay display, auto-connecting");
        carplay_client_start(&peer);
    } else {
        ESP_LOGI(TAG, "No paired CarPlay display (pair via CHR_CP_PAIRING)");
    }

    s_initialized = true;
    ESP_LOGI(TAG, "CarPlay-Bridge initialized (service 0x%04X)",
             SVC_CARPLAY_BRIDGE);
}

void carplay_bridge_task(void *arg)
{
    (void)arg;

    TickType_t last_notify = xTaskGetTickCount();
    const TickType_t notify_period = pdMS_TO_TICKS(200);  /* 5 Hz */

    ESP_LOGI(TAG, "CarPlay-Bridge task started (telemetry: 5 Hz)");

    while (1) {
        /* Periodic telemetry push when connected + subscribed */
        TickType_t now = xTaskGetTickCount();
        if (carplay_bridge_is_connected() &&
            (now - last_notify) >= notify_period) {
            carplay_bridge_notify_telemetry();
            last_notify = now;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
