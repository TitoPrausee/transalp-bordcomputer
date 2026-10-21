/*
 * carplay_client.c — GATT-Client fuer CarPlay-Display
 *
 * Der Bordcomputer verbindet sich als GATT-Client (Central) mit dem
 * gekoppelten CarPlay-Display, entdeckt dessen Service 0x9100 und
 * abonniert die Notify-Characteristics fuer Navigations-Status und
 * Musik-Metadaten. Auf derselben Verbindung dient der Bordcomputer
 * gleichzeitig als GATT-Server (Telemetrie-Service 0x9000).
 *
 * Aufbau (NimBLE Client API):
 *   1. ble_gap_connect zur gespeicherten Display-Adresse
 *   2. BLE_GAP_EVENT_CONNECT  -> disc_all_chrs ueber den vollen Handle-Bereich
 *   3. chr_disc_cb             -> Ziel-UUIDs matchen
 *   3b. disc_all_dscs         -> CCCD-Deskriptoren dynamisch finden (+1 Fallback)
 *   4. BLE_GAP_EVENT_NOTIFY_RX -> Daten an carplay_client_on_notify
 *   5. BLE_GAP_EVENT_DISCONNECT-> Backoff-Reconnect ueber carplay_client_poll
 *
 * Siehe docs/CARPLAY-INTEGRATION.md Abschnitt GATT-Client.
 */

#include "carplay_internal.h"
#include "carplay_uuids.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "nimble/nimble_port.h"
#include "host/ble_hs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "carplay_cli";

/* ============================================================
 * Client state
 * ============================================================ */

static carplay_client_state_t s_state = CP_CLIENT_IDLE;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static SemaphoreHandle_t s_state_mutex = NULL;

/* Paired peer for (re)connect */
static carplay_peer_t s_peer;
static bool s_peer_valid = false;

/* Discovered characteristic value handles (peer-side 0x9100 service) */
static uint16_t s_h_nav_status = 0;
static uint16_t s_h_media_now  = 0;
static uint16_t s_h_media_ctrl = 0;  /* CHR_CP_MEDIA_CTRL (0x9104), write target */

/* Discovered CCCD descriptor handles (0 = not discovered; cp_subscribe
 * falls back to val_handle+1).  Resolved dynamically via
 * ble_gattc_disc_all_dscs instead of assuming the conventional +1 offset. */
static uint16_t s_cccd_nav_status = 0;
static uint16_t s_cccd_media_now  = 0;

/* Reconnect backoff */
static int64_t s_reconnect_at_us = 0;
static uint8_t s_backoff_step = 0;
#define CP_RECONNECT_BASE_MS   1000
#define CP_RECONNECT_MAX_MS    30000

/* Target UUIDs for discovery matching (peer display service 0x9100) */
static ble_uuid128_t cp_uuid_nav_status = CARPLAY_UUID128(CHR_CP_NAV_STATUS);
static ble_uuid128_t cp_uuid_media_now  = CARPLAY_UUID128(CHR_CP_MEDIA_NOW);
static ble_uuid128_t cp_uuid_media_ctrl = CARPLAY_UUID128(CHR_CP_MEDIA_CTRL);

/* CCCD descriptor UUID (Client Characteristic Configuration, 0x2902).
 * Used to discover the subscription handle dynamically rather than
 * assuming the conventional val_handle + 1 offset. */
static ble_uuid16_t cp_uuid_cccd = {
    .u = { .type = BLE_UUID_TYPE_16 },
    .value = BLE_GATT_DSC_CLT_CFG_UUID16,
};

/* ============================================================
 * State helpers
 * ============================================================ */

static void cp_set_state(carplay_client_state_t st)
{
    xSemaphoreTake(s_state_mutex, portMAX_DELAY);
    s_state = st;
    xSemaphoreGive(s_state_mutex);
}

carplay_client_state_t carplay_client_get_state(void)
{
    carplay_client_state_t st;
    xSemaphoreTake(s_state_mutex, portMAX_DELAY);
    st = s_state;
    xSemaphoreGive(s_state_mutex);
    return st;
}

uint16_t carplay_client_get_conn_handle(void)
{
    uint16_t h;
    xSemaphoreTake(s_state_mutex, portMAX_DELAY);
    h = s_conn_handle;
    xSemaphoreGive(s_state_mutex);
    return h;
}

/* Validate that conn_handle is still the CarPlay-display connection.
 * Guards against conn-handle reuse (H4): if the CarPlay link dropped
 * and NimBLE reused the handle for a companion-app peripheral
 * connection, this returns false so carplay_notify() skips the send
 * instead of pushing CarPlay telemetry to the companion app. */
bool carplay_client_conn_is_valid(uint16_t conn_handle)
{
    uint8_t expected_addr[6];
    uint8_t expected_addr_type;
    bool peer_valid;

    xSemaphoreTake(s_state_mutex, portMAX_DELAY);
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE ||
        s_conn_handle != conn_handle) {
        xSemaphoreGive(s_state_mutex);
        return false;
    }
    peer_valid = s_peer_valid;
    memcpy(expected_addr, s_peer.addr, 6);
    expected_addr_type = s_peer.addr_type;
    xSemaphoreGive(s_state_mutex);

    /* If we don't know the peer address (shouldn't happen after start),
     * fall back to checking the handle is still tracked as ours. */
    if (!peer_valid) {
        return true;
    }

    struct ble_gap_conn_desc desc;
    if (ble_gap_conn_find(conn_handle, &desc) != 0) {
        return false;
    }

    return memcmp(desc.peer_id_addr.val, expected_addr, 6) == 0 &&
           desc.peer_id_addr.type == expected_addr_type;
}

/* ============================================================
 * CCCD subscription: enable notifications on a characteristic.
 *
 * Writes 0x0001 (LE) to the characteristic's CCCD descriptor.  The
 * descriptor handle is discovered dynamically via
 * ble_gattc_disc_all_dscs (see cp_desc_disc_cb).  If discovery did
 * not find a CCCD (discovered_cccd == 0), we fall back to the
 * conventional (val_handle + 1) offset for backward compatibility
 * with the EKIY/Jiuyin display and any peer following the standard
 * GATT layout.
 * ============================================================ */

static void cp_subscribe(uint16_t conn, uint16_t chr_val_handle,
                         uint16_t discovered_cccd)
{
    static const uint8_t cccd_notify_le[2] = { 0x01, 0x00 };

    bool discovered = (discovered_cccd != 0);
    uint16_t cccd_handle = discovered
        ? discovered_cccd
        : (uint16_t)(chr_val_handle + 1);

    int rc = ble_gattc_write_flat(conn, cccd_handle,
                                  cccd_notify_le, sizeof(cccd_notify_le),
                                  NULL, NULL);
    if (rc != 0) {
        ESP_LOGW(TAG, "CCCD write (chr=%u cccd=%u %s) rc=%d",
                 (unsigned)chr_val_handle, (unsigned)cccd_handle,
                 discovered ? "discovered" : "fallback+1", rc);
    } else {
        ESP_LOGI(TAG, "CCCD subscribe (chr=%u -> cccd=%u, %s)",
                 (unsigned)chr_val_handle, (unsigned)cccd_handle,
                 discovered ? "discovered" : "fallback+1");
    }
}

/* ============================================================
 * Finalize the subscription phase.
 *
 * Subscribes (CCCD write) to every discovered notify target using the
 * dynamically discovered CCCD handle, falling back to (val_handle + 1)
 * when no descriptor was found for a given characteristic.  Then
 * transitions to CP_CLIENT_CONNECTED and notifies the bridge.
 * ============================================================ */

static void cp_finalize_subscription(uint16_t conn)
{
    if (s_h_nav_status != 0) {
        cp_subscribe(conn, s_h_nav_status, s_cccd_nav_status);
    }
    if (s_h_media_now != 0) {
        cp_subscribe(conn, s_h_media_now, s_cccd_media_now);
    }

    cp_set_state(CP_CLIENT_CONNECTED);
    carplay_client_on_connected();
    ESP_LOGI(TAG, "Discovery done (nav=%u media=%u ctrl=%u, "
             "cccd_nav=%u cccd_media=%u)",
             (unsigned)s_h_nav_status, (unsigned)s_h_media_now,
             (unsigned)s_h_media_ctrl,
             (unsigned)s_cccd_nav_status, (unsigned)s_cccd_media_now);
}

/* ============================================================
 * Descriptor discovery callback (CCCD)
 *
 * Called once per discovered descriptor.  chr_val_handle identifies
 * which characteristic the descriptor belongs to.  We match the CCCD
 * UUID (0x2902) against our notify targets (nav_status, media_now)
 * and remember the descriptor handle.  On completion (status != 0)
 * we finalize the subscription using discovered handles (+1 fallback).
 * ============================================================ */

static int cp_desc_disc_cb(uint16_t conn_handle,
                           const struct ble_gatt_error *error,
                           uint16_t chr_val_handle,
                           const struct ble_gatt_dsc *dsc, void *arg)
{
    (void)arg;
    (void)conn_handle;

    if (error == NULL) {
        return 0;
    }

    /* Non-zero status => descriptor discovery complete or failed */
    if (error->status != 0) {
        uint16_t conn = carplay_client_get_conn_handle();
        if (conn == BLE_HS_CONN_HANDLE_NONE) {
            return 0;
        }
        /* Subscribe using whatever CCCD handles we found (0 => +1). */
        cp_finalize_subscription(conn);
        return 0;
    }

    if (dsc == NULL) {
        return 0;
    }

    /* Match CCCD descriptor (0x2902) for our notify targets */
    if (ble_uuid_cmp(&cp_uuid_cccd.u, (const ble_uuid_t *)&dsc->uuid) == 0) {
        if (chr_val_handle == s_h_nav_status) {
            s_cccd_nav_status = dsc->handle;
            ESP_LOGD(TAG, "CCCD found for nav_status: chr=%u cccd=%u",
                     (unsigned)chr_val_handle, (unsigned)dsc->handle);
        } else if (chr_val_handle == s_h_media_now) {
            s_cccd_media_now = dsc->handle;
            ESP_LOGD(TAG, "CCCD found for media_now: chr=%u cccd=%u",
                     (unsigned)chr_val_handle, (unsigned)dsc->handle);
        }
    }

    return 0;
}

/* ============================================================
 * Characteristic discovery callback
 *
 * Called once per discovered characteristic. status != 0 signals
 * completion (BLE_HS_EDONE) or an error; in that case chr is NULL
 * and we finalize the subscription phase.
 * ============================================================ */

static int cp_chr_disc_cb(uint16_t conn_handle,
                          const struct ble_gatt_error *error,
                          const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    (void)conn_handle;

    if (error == NULL) {
        return 0;
    }

    /* Non-zero status => discovery complete or failed */
    if (error->status != 0) {
        uint16_t conn = carplay_client_get_conn_handle();
        if (conn == BLE_HS_CONN_HANDLE_NONE) {
            return 0;
        }

        /* If we found at least one notify target, discover their CCCD
         * descriptors dynamically instead of assuming the +1 offset. */
        if (s_h_nav_status != 0 || s_h_media_now != 0) {
            cp_set_state(CP_CLIENT_SUBSCRIBING);
            int rc = ble_gattc_disc_all_dscs(conn, 1, 0xFFFF,
                                             cp_desc_disc_cb, NULL);
            if (rc == 0) {
                ESP_LOGD(TAG, "CCCD descriptor discovery started");
                return 0;  /* cp_desc_disc_cb finalizes subscription */
            }
            ESP_LOGW(TAG, "disc_all_dscs rc=%d, falling back to +1 CCCD", rc);
        }

        /* No notify targets, or desc discovery failed to start:
         * subscribe with the conventional +1 fallback and finalize. */
        cp_finalize_subscription(conn);
        return 0;
    }

    if (chr == NULL) {
        return 0;
    }

    /* Match target UUIDs */
    if (ble_uuid_cmp(&cp_uuid_nav_status.u, (const ble_uuid_t *)&chr->uuid) == 0) {
        s_h_nav_status = chr->val_handle;
    } else if (ble_uuid_cmp(&cp_uuid_media_now.u, (const ble_uuid_t *)&chr->uuid) == 0) {
        s_h_media_now = chr->val_handle;
    } else if (ble_uuid_cmp(&cp_uuid_media_ctrl.u, (const ble_uuid_t *)&chr->uuid) == 0) {
        s_h_media_ctrl = chr->val_handle;
    }

    return 0;
}

/* ============================================================
 * GAP event handler (CarPlay client connection)
 * ============================================================ */

int carplay_client_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    /* NimBLE BLE_GAP_EVENT_* values are not modeled in the host-analysis
     * mocks, so clang-tidy collapses the case labels. The real handler
     * dispatches on all BLE_GAP_EVENT_* cases. Suppressed per TOT-61. */
    // NOLINTNEXTLINE(readability-trivial-switch)
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            struct ble_gap_conn_desc desc;
            bool peer_known = (ble_gap_conn_find(event->connect.conn_handle,
                                                &desc) == 0);

            xSemaphoreTake(s_state_mutex, portMAX_DELAY);
            s_conn_handle = event->connect.conn_handle;
            xSemaphoreGive(s_state_mutex);

            cp_set_state(CP_CLIENT_DISCOVERING);
            if (peer_known) {
                ESP_LOGI(TAG, "CarPlay display connected: handle=%d "
                         "peer=%02X:%02X:%02X:%02X:%02X:%02X type=%d",
                         s_conn_handle,
                         desc.peer_id_addr.val[5], desc.peer_id_addr.val[4],
                         desc.peer_id_addr.val[3], desc.peer_id_addr.val[2],
                         desc.peer_id_addr.val[1], desc.peer_id_addr.val[0],
                         desc.peer_id_addr.type);
            } else {
                ESP_LOGI(TAG, "CarPlay display connected: handle=%d",
                         s_conn_handle);
            }

            /* Discover all characteristics, match by UUID */
            int rc = ble_gattc_disc_all_chrs(s_conn_handle,
                                             1, 0xFFFF,
                                             cp_chr_disc_cb, NULL);
            if (rc != 0) {
                ESP_LOGW(TAG, "disc_all_chrs rc=%d", rc);
                ble_gap_terminate(s_conn_handle,
                                  BLE_ERR_REM_USER_CONN_TERM);
            }
        } else {
            ESP_LOGW(TAG, "CarPlay connect failed: status=%d",
                     event->connect.status);
            cp_set_state(CP_CLIENT_RECONNECTING);
            s_reconnect_at_us = esp_timer_get_time() +
                                (int64_t)CP_RECONNECT_BASE_MS * 1000;
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        xSemaphoreTake(s_state_mutex, portMAX_DELAY);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        xSemaphoreGive(s_state_mutex);

        s_h_nav_status = 0;
        s_h_media_now = 0;
        s_h_media_ctrl = 0;
        s_cccd_nav_status = 0;
        s_cccd_media_now = 0;
        s_backoff_step = 0;

        carplay_client_on_disconnected();

        if (s_peer_valid) {
            cp_set_state(CP_CLIENT_RECONNECTING);
            s_reconnect_at_us = esp_timer_get_time() +
                                (int64_t)CP_RECONNECT_BASE_MS * 1000;
            ESP_LOGI(TAG, "CarPlay display disconnected: reason=%d, "
                     "will reconnect", event->disconnect.reason);
        } else {
            cp_set_state(CP_CLIENT_IDLE);
        }
        break;

    case BLE_GAP_EVENT_NOTIFY_RX: {
        /* Incoming notification from the CarPlay display */
        uint16_t conn = carplay_client_get_conn_handle();
        if (conn == BLE_HS_CONN_HANDLE_NONE) {
            break;
        }

        uint8_t buf[128];
        uint16_t len = 0;
        int rc = ble_hs_mbuf_to_flat(event->notify_rx.om,
                                     buf, sizeof(buf), &len);
        if (rc == 0 && len > 0) {
            carplay_client_on_notify(event->notify_rx.attr_handle, buf, len);
        }
        break;
    }

    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGI(TAG, "Conn params updated");
        break;

    default:
        break;
    }

    return 0;
}

/* ============================================================
 * Initiate a connection to the paired CarPlay display
 * ============================================================ */

static void cp_connect(const carplay_peer_t *peer)
{
    if (peer == NULL || !peer->valid) {
        return;
    }

    uint8_t own_addr_type;
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGW(TAG, "id_infer_auto rc=%d", rc);
        return;
    }

    ble_addr_t addr;
    addr.type = peer->addr_type;
    memcpy(addr.val, peer->addr, CARPLAY_PEER_ADDR_LEN);

    struct ble_gap_conn_params params = {
        .itvl_min = 12,   /* 15 ms */
        .itvl_max = 24,   /* 30 ms */
        .latency = 0,
        .supervision_timeout = 400,  /* 4 s */
    };

    rc = ble_gap_connect(own_addr_type, &addr, 30000 /* 30 s */,
                         &params, carplay_client_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGW(TAG, "ble_gap_connect rc=%d", rc);
        cp_set_state(CP_CLIENT_RECONNECTING);
        /* Backoff schedule is owned by carplay_client_poll(); seed an initial
         * base delay so the first poll does not fire immediately. */
        s_reconnect_at_us = esp_timer_get_time() +
                            (int64_t)CP_RECONNECT_BASE_MS * 1000;
    } else {
        cp_set_state(CP_CLIENT_CONNECTING);
        ESP_LOGI(TAG, "Connecting to CarPlay display...");
    }
}

/* ============================================================
 * Public client API
 * ============================================================ */

void carplay_client_init(void)
{
    s_state_mutex = xSemaphoreCreateMutex();
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_state = CP_CLIENT_IDLE;
    s_peer_valid = false;
    s_h_nav_status = 0;
    s_h_media_now = 0;
    s_h_media_ctrl = 0;
    s_cccd_nav_status = 0;
    s_cccd_media_now = 0;
    s_backoff_step = 0;
}

void carplay_client_start(const carplay_peer_t *peer)
{
    if (peer == NULL || !peer->valid) {
        return;
    }

    memcpy(&s_peer, peer, sizeof(s_peer));
    s_peer_valid = true;
    s_backoff_step = 0;

    cp_connect(&s_peer);
}

void carplay_client_stop(void)
{
    s_peer_valid = false;
    cp_set_state(CP_CLIENT_IDLE);

    uint16_t conn = carplay_client_get_conn_handle();
    if (conn != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
        xSemaphoreTake(s_state_mutex, portMAX_DELAY);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        xSemaphoreGive(s_state_mutex);
    }
}

/*
 * Reconnect poller — called from carplay_bridge_task.
 * Implements exponential backoff (1s, 2s, 4s, 8s, max 30s).
 */
void carplay_client_poll(void)
{
    if (carplay_client_get_state() != CP_CLIENT_RECONNECTING) {
        return;
    }
    if (!s_peer_valid) {
        return;
    }

    int64_t now = esp_timer_get_time();
    if (now < s_reconnect_at_us) {
        return;
    }

    /* Backoff: 1s, 2s, 4s, 8s, 16s, capped at 30s */
    uint32_t delay_ms = CP_RECONNECT_BASE_MS << s_backoff_step;
    if (delay_ms > CP_RECONNECT_MAX_MS) {
        delay_ms = CP_RECONNECT_MAX_MS;
    }
    if (s_backoff_step < 5) {
        s_backoff_step++;
    }

    cp_connect(&s_peer);

    /* Schedule the next reconnect attempt with exponential backoff. Applied
     * after cp_connect() so it overrides the fixed base delay seeded on
     * synchronous connect failure; it is a no-op when cp_connect() transitioned
     * to CONNECTING, since the state gates the poller. */
    s_reconnect_at_us = now + (int64_t)delay_ms * 1000;
}

/* ============================================================
 * Media control forward path (gap C4 / FU-4)
 *
 * Writes a single command byte (carplay_media_cmd) to the peer
 * display's CHR_CP_MEDIA_CTRL (0x9104).  Called by the CarPlay
 * bridge when an APP_EVENT_CTRL_MEDIA_* is received from the
 * Lenker-Controller.  Safe to call from any task; no-ops when
 * not connected or the characteristic was not discovered.
 * ============================================================ */

void carplay_client_media_ctrl(uint8_t cmd)
{
    uint16_t conn = carplay_client_get_conn_handle();
    if (conn == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGD(TAG, "media_ctrl: not connected, drop cmd=0x%02X", cmd);
        return;
    }
    if (s_h_media_ctrl == 0) {
        ESP_LOGW(TAG, "media_ctrl: CHR_CP_MEDIA_CTRL not discovered, drop cmd=0x%02X",
                 cmd);
        return;
    }

    int rc = ble_gattc_write_flat(conn, s_h_media_ctrl,
                                  &cmd, sizeof(cmd), NULL, NULL);
    if (rc != 0 && rc != BLE_HS_ENOTCONN) {
        ESP_LOGW(TAG, "media_ctrl write (handle=%u cmd=0x%02X) rc=%d",
                 (unsigned)s_h_media_ctrl, cmd, rc);
    } else {
        ESP_LOGI(TAG, "media_ctrl -> display (cmd=0x%02X)", cmd);
    }
}
