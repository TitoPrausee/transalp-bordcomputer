/*
 * ble_service.c — NimBLE GATT-Server Setup
 *
 * Initialisiert den NimBLE-Stack, registriert alle GATT-Services
 * und startet Advertising.
 *
 * Services (8 Custom + 2 Standard):
 *   0x1000 Navigation, 0x2000 Media, 0x3000 Phone, 0x4000 Weather,
 *   0x5000 Telemetry, 0x6000 Config, 0x7000 Diagnose, 0x8000 OTA
 *
 * Siehe docs/research/13-connectivity-app.md Abschnitt 2.
 */

#include "ble.h"
#include "ble_internal.h"
#include "gatt_uuids.h"
#include "carplay_bridge.h"

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "sensors.h"
#include "gps.h"
#include "power.h"
#include "diagnostics.h"
#include "event_bus.h"

static const char *TAG = "ble_svc";

/* ============================================================
 * Device configuration
 * ============================================================ */

#define BLE_DEVICE_NAME         "Transalp-BC"
#define BLE_MTU_SIZE            247
#define BLE_CONN_INTERVAL_MIN   12   /* 15 ms (12 * 1.25 ms) */
#define BLE_CONN_INTERVAL_MAX   24   /* 30 ms */
#define BLE_CONN_LATENCY        0
#define BLE_CONN_TIMEOUT        400  /* 4 s (400 * 10 ms) */
#define BLE_MAX_CONNECTIONS     2  /* 1 companion-app (peripheral) + 1 CarPlay (central) */

/* ============================================================
 * Connection state
 * ============================================================ */

static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool s_initialized = false;
static SemaphoreHandle_t s_conn_mutex = NULL;

/* Peer address of the active companion-app connection.  Stored when a
 * peripheral-role connection comes up and used by ble_conn_is_valid()
 * to guard against conn-handle reuse races (H4).  When the companion
 * app disconnects, the handle is freed and NimBLE may reuse the same
 * numeric handle for the CarPlay central connection.  Comparing the
 * peer address prevents server-side notifies from being sent to the
 * wrong connection. */
static uint8_t s_peer_addr[6] = {0};
static uint8_t s_peer_addr_type = 0;

/* CCCD subscription tracking */
static bool s_cccd_telemetry = false;
static bool s_cccd_gps = false;
static bool s_cccd_dtc = false;
static bool s_cccd_sensor_status = false;

/* ============================================================
 * 128-bit UUID definitions
 * ============================================================ */

/* Service UUIDs */
static ble_uuid128_t svc_nav_uuid     = GATT_UUID128(SVC_NAVIGATION);
static ble_uuid128_t svc_media_uuid   = GATT_UUID128(SVC_MEDIA);
static ble_uuid128_t svc_phone_uuid    = GATT_UUID128(SVC_PHONE);
static ble_uuid128_t svc_weather_uuid  = GATT_UUID128(SVC_WEATHER);
static ble_uuid128_t svc_telemetry_uuid = GATT_UUID128(SVC_TELEMETRY);
static ble_uuid128_t svc_config_uuid   = GATT_UUID128(SVC_CONFIG);
static ble_uuid128_t svc_diag_uuid     = GATT_UUID128(SVC_DIAGNOSE);
static ble_uuid128_t svc_ota_uuid      = GATT_UUID128(SVC_OTA);

/* Characteristic UUIDs */
static ble_uuid128_t chr_nav_maneuver   = GATT_UUID128(CHR_NAV_MANEUVER);
static ble_uuid128_t chr_nav_route      = GATT_UUID128(CHR_NAV_ROUTE);
static ble_uuid128_t chr_nav_road        = GATT_UUID128(CHR_NAV_ROAD);
static ble_uuid128_t chr_nav_status     = GATT_UUID128(CHR_NAV_STATUS);
static ble_uuid128_t chr_nav_cmd         = GATT_UUID128(CHR_NAV_CMD);

static ble_uuid128_t chr_media_nowplaying = GATT_UUID128(CHR_MEDIA_NOWPLAYING);
static ble_uuid128_t chr_media_control   = GATT_UUID128(CHR_MEDIA_CONTROL);
static ble_uuid128_t chr_media_volume    = GATT_UUID128(CHR_MEDIA_VOLUME);

static ble_uuid128_t chr_phone_incoming  = GATT_UUID128(CHR_PHONE_INCOMING);
static ble_uuid128_t chr_phone_status    = GATT_UUID128(CHR_PHONE_STATUS);
static ble_uuid128_t chr_phone_control   = GATT_UUID128(CHR_PHONE_CONTROL);
static ble_uuid128_t chr_phone_notif     = GATT_UUID128(CHR_PHONE_NOTIF);

static ble_uuid128_t chr_wx_current      = GATT_UUID128(CHR_WX_CURRENT);
static ble_uuid128_t chr_wx_forecast     = GATT_UUID128(CHR_WX_FORECAST);
static ble_uuid128_t chr_wx_alert        = GATT_UUID128(CHR_WX_ALERT);

static ble_uuid128_t chr_tlm_live        = GATT_UUID128(CHR_TLM_LIVE);
static ble_uuid128_t chr_tlm_snapshot    = GATT_UUID128(CHR_TLM_SNAPSHOT);
static ble_uuid128_t chr_track_export    = GATT_UUID128(CHR_TRACK_EXPORT);
static ble_uuid128_t chr_track_export_ctrl = GATT_UUID128(CHR_TRACK_EXPORT_CTRL);

static ble_uuid128_t chr_config_read     = GATT_UUID128(CHR_CONFIG_READ);
static ble_uuid128_t chr_config_write    = GATT_UUID128(CHR_CONFIG_WRITE);
static ble_uuid128_t chr_config_export   = GATT_UUID128(CHR_CONFIG_EXPORT);
static ble_uuid128_t chr_config_import   = GATT_UUID128(CHR_CONFIG_IMPORT);
static ble_uuid128_t chr_bond_mgmt       = GATT_UUID128(CHR_BOND_MGMT);

static ble_uuid128_t chr_dtc_list        = GATT_UUID128(CHR_DTC_LIST);
static ble_uuid128_t chr_sensor_status   = GATT_UUID128(CHR_SENSOR_STATUS);
static ble_uuid128_t chr_self_test       = GATT_UUID128(CHR_SELF_TEST);
static ble_uuid128_t chr_log_export      = GATT_UUID128(CHR_LOG_EXPORT);
static ble_uuid128_t chr_log_export_ctrl = GATT_UUID128(CHR_LOG_EXPORT_CTRL);

static ble_uuid128_t chr_ota_control     = GATT_UUID128(CHR_OTA_CONTROL);
static ble_uuid128_t chr_ota_data       = GATT_UUID128(CHR_OTA_DATA);
static ble_uuid128_t chr_ota_verify      = GATT_UUID128(CHR_OTA_VERIFY);

/* ============================================================
 * GATT Service Table
 *
 * Each service defines an array of characteristics terminated by {0}.
 * Each characteristic points to the shared access callback, which
 * dispatches based on the attribute handle.
 * ============================================================ */

static const struct ble_gatt_chr_def chr_nav[] = {
    { .uuid = &chr_nav_maneuver.u,  .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_NAV_MANEUVER] },
    { .uuid = &chr_nav_route.u,     .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_NAV_ROUTE] },
    { .uuid = &chr_nav_road.u,      .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_NAV_ROAD] },
    { .uuid = &chr_nav_status.u,    .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_NAV_STATUS] },
    { .uuid = &chr_nav_cmd.u,        .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
      .val_handle = &ble_val_handles[CHR_H_NAV_CMD] },
    { 0 },
};

static const struct ble_gatt_chr_def chr_media[] = {
    { .uuid = &chr_media_nowplaying.u, .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_MEDIA_NOWPLAYING] },
    { .uuid = &chr_media_control.u,   .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
      .val_handle = &ble_val_handles[CHR_H_MEDIA_CONTROL] },
    { .uuid = &chr_media_volume.u,    .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_WRITE,
      .val_handle = &ble_val_handles[CHR_H_MEDIA_VOLUME] },
    { 0 },
};

static const struct ble_gatt_chr_def chr_phone[] = {
    { .uuid = &chr_phone_incoming.u,  .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_PHONE_INCOMING] },
    { .uuid = &chr_phone_status.u,   .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_PHONE_STATUS] },
    { .uuid = &chr_phone_control.u,   .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &ble_val_handles[CHR_H_PHONE_CONTROL] },
    { .uuid = &chr_phone_notif.u,     .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_PHONE_NOTIF] },
    { 0 },
};

static const struct ble_gatt_chr_def chr_weather[] = {
    { .uuid = &chr_wx_current.u,      .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_WX_CURRENT] },
    { .uuid = &chr_wx_forecast.u,     .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_WX_FORECAST] },
    { .uuid = &chr_wx_alert.u,        .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_WX_ALERT] },
    { 0 },
};

static const struct ble_gatt_chr_def chr_telemetry[] = {
    { .uuid = &chr_tlm_live.u,        .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_TLM_LIVE] },
    { .uuid = &chr_tlm_snapshot.u,    .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ,
      .val_handle = &ble_val_handles[CHR_H_TLM_SNAPSHOT] },
    { .uuid = &chr_track_export.u,    .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_TRACK_EXPORT] },
    { .uuid = &chr_track_export_ctrl.u, .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &ble_val_handles[CHR_H_TRACK_EXPORT_CTRL] },
    { 0 },
};

static const struct ble_gatt_chr_def chr_config[] = {
    { .uuid = &chr_config_read.u,     .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_CONFIG_READ] },
    { .uuid = &chr_config_write.u,    .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &ble_val_handles[CHR_H_CONFIG_WRITE] },
    { .uuid = &chr_config_export.u,   .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_CONFIG_EXPORT] },
    { .uuid = &chr_config_import.u,   .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &ble_val_handles[CHR_H_CONFIG_IMPORT] },
    { .uuid = &chr_bond_mgmt.u,       .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &ble_val_handles[CHR_H_BOND_MGMT] },
    { 0 },
};

static const struct ble_gatt_chr_def chr_diag[] = {
    { .uuid = &chr_dtc_list.u,        .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE,
      .val_handle = &ble_val_handles[CHR_H_DTC_LIST] },
    { .uuid = &chr_sensor_status.u,   .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_SENSOR_STATUS] },
    { .uuid = &chr_self_test.u,       .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_SELF_TEST] },
    { .uuid = &chr_log_export.u,      .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_LOG_EXPORT] },
    { .uuid = &chr_log_export_ctrl.u, .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE,
      .val_handle = &ble_val_handles[CHR_H_LOG_EXPORT_CTRL] },
    { 0 },
};

static const struct ble_gatt_chr_def chr_ota[] = {
    { .uuid = &chr_ota_control.u,    .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_OTA_CONTROL] },
    { .uuid = &chr_ota_data.u,        .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
      .val_handle = &ble_val_handles[CHR_H_OTA_DATA] },
    { .uuid = &chr_ota_verify.u,      .access_cb = ble_gatt_access_cb,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &ble_val_handles[CHR_H_OTA_VERIFY] },
    { 0 },
};

static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_nav_uuid.u,
        .characteristics = chr_nav,
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_media_uuid.u,
        .characteristics = chr_media,
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_phone_uuid.u,
        .characteristics = chr_phone,
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_weather_uuid.u,
        .characteristics = chr_weather,
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_telemetry_uuid.u,
        .characteristics = chr_telemetry,
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_config_uuid.u,
        .characteristics = chr_config,
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_diag_uuid.u,
        .characteristics = chr_diag,
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_ota_uuid.u,
        .characteristics = chr_ota,
    },
    { 0 },  /* terminator */
};

/* ============================================================
 * Attribute handle table (filled by NimBLE after registration)
 *
 * Indexed by the CHR_H_* enum from ble_internal.h.
 * Used by notify/indicate calls to know which handle to send to.
 * ============================================================ */

uint16_t ble_val_handles[CHR_H_COUNT] = {0};

/* ============================================================
 * Advertising data
 * ============================================================ */

static void
ble_start_advertising(void)
{
    struct ble_hs_adv_fields adv_fields;
    struct ble_hs_adv_fields rsp_fields;
    uint8_t svc_uuid16[2];
    int rc;

    memset(&adv_fields, 0, sizeof(adv_fields));

    /* Flags: LE General Discoverable + BD/LE */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Device name in scan response (shorter adv packet) */
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.name = (uint8_t *)BLE_DEVICE_NAME;
    rsp_fields.name_len = strlen(BLE_DEVICE_NAME);
    rsp_fields.name_is_complete = 1;

    /* Advertise the Navigation Service UUID (16-bit encoded) */
    svc_uuid16[0] = SVC_NAVIGATION & 0xFF;
    svc_uuid16[1] = (SVC_NAVIGATION >> 8) & 0xFF;
    adv_fields.svc_data_uuid16 = svc_uuid16;
    adv_fields.svc_data_uuid16_len = sizeof(svc_uuid16);

    /* TX power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv set_fields rc=%d", rc);
        return;
    }

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv rsp_set_fields rc=%d", rc);
        return;
    }

    /* Connectable, undirected advertising */
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    uint8_t own_addr_type;
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "id_infer_auto rc=%d", rc);
        return;
    }

    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "Advertising started as \"%s\"", BLE_DEVICE_NAME);
}

/* ============================================================
 * Sync callback — called when NimBLE is ready
 * ============================================================ */

static void
ble_on_sync(void)
{
    ESP_LOGI(TAG, "NimBLE synced");
    ble_start_advertising();
}

static void
ble_on_reset(int reason)
{
    ESP_LOGW(TAG, "NimBLE reset (reason=%d)", reason);
}

/* ============================================================
 * GAP event handler — connect, disconnect, MTU
 * ============================================================ */

int
ble_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            struct ble_gap_conn_desc desc;
            bool peer_known = (ble_gap_conn_find(event->connect.conn_handle,
                                                &desc) == 0);

            xSemaphoreTake(s_conn_mutex, portMAX_DELAY);
            s_conn_handle = event->connect.conn_handle;
            if (peer_known) {
                memcpy(s_peer_addr, desc.peer_id_addr, 6);
                s_peer_addr_type = desc.peer_id_addr_type;
            } else {
                memset(s_peer_addr, 0, sizeof(s_peer_addr));
                s_peer_addr_type = 0;
            }
            xSemaphoreGive(s_conn_mutex);

            event_bus_set(EVT_BLE_CONNECTED);

            if (peer_known) {
                ESP_LOGI(TAG, "Companion-app connected: handle=%d "
                         "peer=%02X:%02X:%02X:%02X:%02X:%02X type=%d",
                         s_conn_handle,
                         desc.peer_id_addr[5], desc.peer_id_addr[4],
                         desc.peer_id_addr[3], desc.peer_id_addr[2],
                         desc.peer_id_addr[1], desc.peer_id_addr[0],
                         desc.peer_id_addr_type);
            } else {
                ESP_LOGI(TAG, "Companion-app connected: handle=%d "
                         "(peer addr unavailable)", s_conn_handle);
            }

            /* Stop advertising so a second peripheral connection cannot
             * overwrite s_conn_handle.  The second NimBLE slot is reserved
             * for the CarPlay central connection (carplay_client.c). */
            ble_gap_adv_stop();

            /* Request larger MTU for chunked transfers */
            ble_att_set_preferred_mtu(BLE_MTU_SIZE);
            int rc = ble_gattc_exchange_mtu(s_conn_handle, NULL, NULL);
            if (rc != 0 && rc != BLE_HS_ENOTCONN) {
                ESP_LOGW(TAG, "MTU exchange rc=%d", rc);
            }

            /* Update connection parameters: 15-30ms, 0 latency, 4s timeout */
            struct ble_gap_upd_params params = {
                .itvl_min = BLE_CONN_INTERVAL_MIN,
                .itvl_max = BLE_CONN_INTERVAL_MAX,
                .latency = BLE_CONN_LATENCY,
                .supervision_timeout = BLE_CONN_TIMEOUT,
            };
            rc = ble_gap_update_params(s_conn_handle, &params);
            if (rc != 0) {
                ESP_LOGW(TAG, "update_params rc=%d", rc);
            }
        } else {
            ESP_LOGW(TAG, "Companion-app connect failed: status=%d",
                     event->connect.status);
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT: {
        uint16_t old_handle;
        uint8_t old_addr[6];
        uint8_t old_addr_type;

        xSemaphoreTake(s_conn_mutex, portMAX_DELAY);
        old_handle = s_conn_handle;
        memcpy(old_addr, s_peer_addr, 6);
        old_addr_type = s_peer_addr_type;
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        memset(s_peer_addr, 0, sizeof(s_peer_addr));
        s_peer_addr_type = 0;
        xSemaphoreGive(s_conn_mutex);

        /* Reset CCCD subscriptions */
        s_cccd_telemetry = false;
        s_cccd_gps = false;
        s_cccd_dtc = false;
        s_cccd_sensor_status = false;

        event_bus_set(EVT_BLE_DISCONNECTED);
        ESP_LOGI(TAG, "Companion-app disconnected: handle=%d reason=%d "
                 "peer=%02X:%02X:%02X:%02X:%02X:%02X",
                 old_handle, event->disconnect.reason,
                 old_addr[5], old_addr[4], old_addr[3],
                 old_addr[2], old_addr[1], old_addr[0]);

        /* Auto-reconnect: restart advertising */
        ble_start_advertising();
        break;
    }

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU: conn=%d mtu=%d",
                 event->mtu.conn_handle, event->mtu.value);
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "Subscribe: handle=%d, notify=%d, indicate=%d",
                 event->subscribe.attr_handle,
                 event->subscribe.cur_notify,
                 event->subscribe.cur_indicate);

        /* Track CCCD state per characteristic */
        if (event->subscribe.attr_handle == ble_val_handles[CHR_H_TLM_LIVE]) {
            s_cccd_telemetry = event->subscribe.cur_notify;
        } else if (event->subscribe.attr_handle ==
                   ble_val_handles[CHR_H_DTC_LIST]) {
            s_cccd_dtc = event->subscribe.cur_notify ||
                         event->subscribe.cur_indicate;
        } else if (event->subscribe.attr_handle ==
                   ble_val_handles[CHR_H_SENSOR_STATUS]) {
            s_cccd_sensor_status = event->subscribe.cur_notify;
        }
        break;

    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGI(TAG, "Conn params updated");
        break;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* Remove old bond, accept new pairing */
        {
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0) {
                ble_store_util_delete_peer(&desc.peer_id_addr);
            }
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    default:
        break;
    }

    return 0;
}

/* ============================================================
 * CCCD state queries (used by ble_callbacks.c)
 * ============================================================ */

bool ble_cccd_telemetry_enabled(void)  { return s_cccd_telemetry; }
bool ble_cccd_gps_enabled(void)        { return s_cccd_gps; }
bool ble_cccd_dtc_enabled(void)        { return s_cccd_dtc; }
bool ble_cccd_sensor_status_enabled(void) { return s_cccd_sensor_status; }

uint16_t ble_get_conn_handle(void)
{
    uint16_t handle;
    xSemaphoreTake(s_conn_mutex, portMAX_DELAY);
    handle = s_conn_handle;
    xSemaphoreGive(s_conn_mutex);
    return handle;
}

/* ============================================================
 * Connection-handle validation (H4 race-fix)
 *
 * Returns true iff conn_handle is still the active companion-app
 * connection AND the peer address matches the one stored at connect
 * time.  This guards the notify/indicate TOCTOU window: after reading
 * s_conn_handle, the connection could drop and NimBLE could reuse the
 * same numeric handle for the CarPlay central link.  Comparing the
 * peer address via ble_gap_conn_find() prevents server-side notifies
 * from being sent to the wrong connection.
 * ============================================================ */

bool ble_conn_is_valid(uint16_t conn_handle)
{
    uint8_t expected_addr[6];
    uint8_t expected_type;

    xSemaphoreTake(s_conn_mutex, portMAX_DELAY);
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE ||
        s_conn_handle != conn_handle) {
        xSemaphoreGive(s_conn_mutex);
        return false;
    }
    memcpy(expected_addr, s_peer_addr, 6);
    expected_type = s_peer_addr_type;
    xSemaphoreGive(s_conn_mutex);

    struct ble_gap_conn_desc desc;
    if (ble_gap_conn_find(conn_handle, &desc) != 0) {
        return false;
    }

    return memcmp(desc.peer_id_addr, expected_addr, 6) == 0 &&
           desc.peer_id_addr_type == expected_type;
}

/* ============================================================
 * NimBLE host task
 * ============================================================ */

static void
ble_host_task(void *param)
{
    (void)param;
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();  /* This blocks until nimble_port_stop() */
    nimble_port_freertos_deinit();
}

/* ============================================================
 * Security Manager configuration
 * ============================================================ */

static void
ble_configure_security(void)
{
    /* LE Secure Connections with bonding */
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_YES_NO;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC |
                                  BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC |
                                    BLE_SM_PAIR_KEY_DIST_ID;

    /* Store bonding keys in NVS */
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
}

/* ============================================================
 * Public API
 * ============================================================ */

void ble_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "BLE already initialized");
        return;
    }

    /* Create connection mutex */
    s_conn_mutex = xSemaphoreCreateMutex();
    if (s_conn_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create conn mutex");
        return;
    }

    /* Initialize NimBLE port */
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %s", esp_err_to_name(err));
        return;
    }

    /* Configure security / bonding */
    ble_configure_security();

    /* Set device name */
    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);

    /* Initialize GATT and GAP services */
    ble_svc_gap_init();
    ble_svc_gatt_init();

    /* Count and register our custom services */
    int rc = ble_gatts_count_cfg(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "gatts_count_cfg rc=%d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "gatts_add_svcs rc=%d", rc);
        return;
    }

    /* Register CarPlay-Bridge GATT service (0x9000) before host start */
    carplay_bridge_register_gatt();

    /* Set sync and reset callbacks */
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    /* Set preferred MTU */
    ble_att_set_preferred_mtu(BLE_MTU_SIZE);

    /* Start NimBLE host task */
    nimble_port_freertos_init(ble_host_task);

    s_initialized = true;
    ESP_LOGI(TAG, "BLE initialized: device=\"%s\", MTU=%d",
             BLE_DEVICE_NAME, BLE_MTU_SIZE);
}

bool ble_is_connected(void)
{
    return ble_get_conn_handle() != BLE_HS_CONN_HANDLE_NONE;
}
