/*
 * ble_callbacks.c — GATT Read/Write/Notify Handler
 *
 * Implementiert den zentralen access_cb fuer alle Characteristics.
 * - Read: liefert aktuelle Sensor-, GPS- und Diagnose-Daten
 * - Write: empfängt Konfiguration und Befehle von der App
 * - Notify/Indicate: pusht Daten bei Aenderungen
 *
 * Thread-safety: Der access_cb laeuft im NimBLE-Host-Task.
 * Daten von Sensor/GPS-Tasks werden ueber Mutex-gesicherte
 * Caches gelesen. Schreib-Befehle werden in eine Queue
 * eingereiht und asynchron in ble_task() verarbeitet.
 */

#include "ble.h"
#include "ble_internal.h"
#include "gatt_uuids.h"

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "sensors.h"
#include "gps.h"
#include "power.h"
#include "diagnostics.h"
#include "event_bus.h"
#include "ota_update.h"
#include "storage.h"

static const char *TAG = "ble_cb";

/* ============================================================
 * Thread-safe data caches
 *
 * Sensor and GPS tasks update these via ble_on_sensor_update /
 * ble_on_gps_update. The BLE read/notify callbacks read from
 * these caches under the mutex.
 * ============================================================ */

static sensor_data_t s_sensor_cache;
static gps_data_t     s_gps_cache;
static SemaphoreHandle_t s_data_mutex = NULL;

/* Telemetry live packet (22 bytes packed, TLM_LIVE 0x5001) */
typedef struct __attribute__((packed)) {
    uint8_t  speed_kmh;        /* km/h */
    uint16_t rpm;               /* U/min */
    int8_t   gear;              /* -1 = neutral */
    int8_t   coolant_temp_c;    /* Celsius */
    int8_t   ambient_temp_c;    /* Celsius */
    uint8_t fuel_percent;      /* 0-100 */
    uint8_t voltage_tenth;     /* Zehntelvolt */
    int32_t  latitude_e7;      /* E-7 */
    int32_t  longitude_e7;     /* E-7 */
    int16_t  altitude_m;       /* Meter */
    uint16_t heading_tenth;   /* Zehntelgrad */
    uint8_t satellites;        /* count */
    uint8_t hdop_tenth;       /* Zehntel */
} telemetry_live_t;

_Static_assert(sizeof(telemetry_live_t) == 22,
               "TLM_LIVE packed size mismatch");

/* DTC binary entry (6 bytes) */
typedef struct __attribute__((packed)) {
    uint16_t code;
    uint8_t  severity;
    uint32_t timestamp;
} dtc_binary_t;

_Static_assert(sizeof(dtc_binary_t) == 7,
               "DTC entry must be 7 bytes");

/* Sensor status (5 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t gps;       /* 0=ok, 1=no_fix, 2=error */
    uint8_t rpm;       /* 0=ok, 1=no_signal, 2=error */
    uint8_t temp;      /* 0=ok, 1=disconnected, 2=error */
    uint8_t fuel;      /* 0=ok, 1=error */
    uint8_t sd_card;   /* 0=ok, 1=not_present, 2=full, 3=error */
} sensor_status_t;

_Static_assert(sizeof(sensor_status_t) == 5,
               "Sensor status must be 5 bytes");

/* ============================================================
 * Write-command queue
 * ============================================================ */

QueueHandle_t ble_cmd_queue = NULL;

bool ble_cmd_enqueue(ble_cmd_type_t type, uint16_t conn_handle,
                     const uint8_t *data, uint16_t len)
{
    if (ble_cmd_queue == NULL || len > sizeof(((ble_cmd_t *)0)->data)) {
        return false;
    }

    ble_cmd_t cmd = {
        .type = type,
        .conn_handle = conn_handle,
        .len = len,
    };
    memcpy(cmd.data, data, len);

    return xQueueSend(ble_cmd_queue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

/* ============================================================
 * Helper: build telemetry live packet from caches
 * ============================================================ */

static void build_telemetry_live(telemetry_live_t *pkt)
{
    xSemaphoreTake(s_data_mutex, portMAX_DELAY);

    sensor_data_t *s = &s_sensor_cache;
    gps_data_t    *g = &s_gps_cache;

    pkt->speed_kmh = (uint8_t)(s->speed_kmh + 0.5f);
    pkt->rpm = s->rpm;
    pkt->gear = (s->gear == 0) ? -1 : (int8_t)s->gear;
    pkt->coolant_temp_c = (int8_t)s->coolant_temp_c;
    pkt->ambient_temp_c = (int8_t)s->temp_c;
    pkt->fuel_percent = (uint8_t)(s->fuel_level_l + 0.5f);
    pkt->voltage_tenth = (uint8_t)(power_get_battery_voltage() * 10.0f + 0.5f);

    if (g->valid) {
        pkt->latitude_e7 = (int32_t)(g->lat * 1e7f);
        pkt->longitude_e7 = (int32_t)(g->lon * 1e7f);
        pkt->altitude_m = (int16_t)g->altitude_m;
        pkt->heading_tenth = (uint16_t)(g->course_deg * 10.0f);
        pkt->satellites = g->satellites;
        pkt->hdop_tenth = (uint8_t)(g->hdop * 10.0f + 0.5f);
    } else {
        pkt->latitude_e7 = 0;
        pkt->longitude_e7 = 0;
        pkt->altitude_m = 0;
        pkt->heading_tenth = 0;
        pkt->satellites = 0;
        pkt->hdop_tenth = 0;
    }

    xSemaphoreGive(s_data_mutex);
}

/* ============================================================
 * Helper: build sensor status
 * ============================================================ */

static void build_sensor_status(sensor_status_t *st)
{
    memset(st, 0, sizeof(*st));

    xSemaphoreTake(s_data_mutex, portMAX_DELAY);

    /* GPS status */
    if (s_gps_cache.valid) {
        st->gps = 0;  /* ok */
    } else if (s_gps_cache.fix_quality != GPS_FIX_NONE) {
        st->gps = 1;  /* no_fix */
    } else {
        st->gps = 2;  /* error */
    }

    /* RPM status: check if rpm is plausible (>0 when riding) */
    st->rpm = 0;  /* assume ok */

    /* Temperature: assume ok if temp is within plausible range */
    st->temp = (s_sensor_cache.temp_c < -40.0f ||
                s_sensor_cache.temp_c > 120.0f) ? 2 : 0;

    /* Fuel: assume ok */
    st->fuel = 0;

    xSemaphoreGive(s_data_mutex);

    /* SD card status from event bus */
    if (event_bus_wait(EVT_SDCARD_MOUNTED, 0)) {
        st->sd_card = 0;  /* ok */
    } else if (event_bus_wait(EVT_SDCARD_ERROR, 0)) {
        st->sd_card = 3;  /* error */
    } else {
        st->sd_card = 1;  /* not_present */
    }
}

/* ============================================================
 * Helper: build DTC list binary
 * ============================================================ */

static uint16_t build_dtc_list(dtc_binary_t *out, uint8_t max_entries)
{
    dtc_entry_t entries[DTC_MAX_ENTRIES];
    uint8_t count = dtc_get_active(entries, DTC_MAX_ENTRIES);

    if (count > max_entries) {
        count = max_entries;
    }

    for (uint8_t i = 0; i < count; i++) {
        out[i].code = (uint16_t)entries[i].code;
        out[i].severity = (uint8_t)entries[i].severity;
        out[i].timestamp = (uint32_t)(entries[i].last_occurrence_ms / 1000);
    }

    return count * sizeof(dtc_binary_t);
}

/* ============================================================
 * Helper: send a notification
 * ============================================================ */

static void ble_notify(uint16_t attr_handle, const void *data, uint16_t len)
{
    uint16_t conn = ble_get_conn_handle();
    if (conn == BLE_HS_CONN_HANDLE_NONE) {
        return;
    }

    /* Validate the handle is still the companion-app connection before
     * sending.  This closes the conn-handle reuse TOCTOU (H4): if the
     * companion app disconnected and NimBLE reused the handle for the
     * CarPlay central link, ble_conn_is_valid() returns false and we
     * skip the notify instead of sending companion-app data to the
     * CarPlay display. */
    if (!ble_conn_is_valid(conn)) {
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

static void ble_indicate(uint16_t attr_handle, const void *data, uint16_t len)
{
    uint16_t conn = ble_get_conn_handle();
    if (conn == BLE_HS_CONN_HANDLE_NONE) {
        return;
    }

    if (!ble_conn_is_valid(conn)) {
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        ESP_LOGW(TAG, "mbuf alloc failed for indicate");
        return;
    }

    int rc = ble_gattc_indicate_custom(conn, attr_handle, om);
    if (rc != 0 && rc != BLE_HS_ENOTCONN) {
        ESP_LOGW(TAG, "indicate rc=%d handle=%d", rc, attr_handle);
    }
}

/* ============================================================
 * GATT Access Callback — central dispatcher
 *
 * Dispatches based on attr_handle compared to ble_val_handles[].
 * ============================================================ */

int ble_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)arg;

    /* Handle read operations */
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return ble_handle_read(attr_handle, ctxt);
    }

    /* Handle write operations */
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return ble_handle_write(conn_handle, attr_handle, ctxt);
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/* ============================================================
 * Read handler
 * ============================================================ */

int ble_handle_read(uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt)
{
    uint8_t buf[256];
    uint16_t len = 0;

    /* Navigation Service reads */
    if (attr_handle == ble_val_handles[CHR_H_NAV_MANEUVER]) {
        /* Return last known maneuver (placeholder) */
        buf[0] = MANEUVER_STRAIGHT;
        len = 1;
    }
    else if (attr_handle == ble_val_handles[CHR_H_NAV_ROUTE]) {
        /* Return route info placeholder */
        memset(buf, 0, 16);
        len = 16;
    }
    else if (attr_handle == ble_val_handles[CHR_H_NAV_ROAD]) {
        /* Return road placeholder */
        buf[0] = 0;  /* speed limit unknown */
        len = 1;
    }
    else if (attr_handle == ble_val_handles[CHR_H_NAV_STATUS]) {
        buf[0] = NAV_STATUS_INACTIVE;
        len = 1;
    }

    /* Media Service reads */
    else if (attr_handle == ble_val_handles[CHR_H_MEDIA_NOWPLAYING]) {
        memset(buf, 0, 16);
        len = 16;
    }
    else if (attr_handle == ble_val_handles[CHR_H_MEDIA_VOLUME]) {
        buf[0] = 50;  /* default volume */
        len = 1;
    }

    /* Phone Service reads */
    else if (attr_handle == ble_val_handles[CHR_H_PHONE_STATUS]) {
        buf[0] = 0;  /* idle */
        len = 1;
    }

    /* Weather Service reads */
    else if (attr_handle == ble_val_handles[CHR_H_WX_CURRENT]) {
        memset(buf, 0, 8);
        len = 8;
    }
    else if (attr_handle == ble_val_handles[CHR_H_WX_FORECAST]) {
        memset(buf, 0, 24);  /* 6 entries x 4 bytes */
        len = 24;
    }

    /* Telemetry Service reads */
    else if (attr_handle == ble_val_handles[CHR_H_TLM_SNAPSHOT]) {
        /* Full sensor snapshot (~60 bytes) */
        xSemaphoreTake(s_data_mutex, portMAX_DELAY);
        sensor_data_t *s = &s_sensor_cache;
        gps_data_t    *g = &s_gps_cache;

        /* Pack snapshot */
        memset(buf, 0, sizeof(buf));
        uint16_t pos = 0;
        memcpy(&buf[pos], &s->rpm, sizeof(s->rpm)); pos += 2;
        memcpy(&buf[pos], &s->speed_kmh, sizeof(s->speed_kmh)); pos += 4;
        buf[pos++] = s->gear;
        memcpy(&buf[pos], &s->coolant_temp_c, 4); pos += 4;
        memcpy(&buf[pos], &s->temp_c, 4); pos += 4;
        memcpy(&buf[pos], &s->fuel_level_l, 4); pos += 4;
        memcpy(&buf[pos], &s->roll_deg, 4); pos += 4;
        memcpy(&buf[pos], &s->pitch_deg, 4); pos += 4;
        memcpy(&buf[pos], &s->altitude_m, 4); pos += 4;
        memcpy(&buf[pos], &g->lat, 4); pos += 4;
        memcpy(&buf[pos], &g->lon, 4); pos += 4;
        memcpy(&buf[pos], &g->altitude_m, 4); pos += 4;
        buf[pos++] = g->satellites;
        memcpy(&buf[pos], &g->hdop, 4); pos += 4;
        buf[pos++] = (uint8_t)(power_get_battery_voltage() * 10.0f + 0.5f);
        buf[pos++] = power_get_battery_percent();

        xSemaphoreGive(s_data_mutex);
        len = pos;
    }

    /* Config Service reads */
    else if (attr_handle == ble_val_handles[CHR_H_CONFIG_READ]) {
        /* Return config JSON placeholder */
        const char *json = "{}";
        len = strlen(json);
        memcpy(buf, json, len);
    }

    /* Diagnose Service reads */
    else if (attr_handle == ble_val_handles[CHR_H_DTC_LIST]) {
        len = build_dtc_list((dtc_binary_t *)buf,
                             sizeof(buf) / sizeof(dtc_binary_t));
    }
    else if (attr_handle == ble_val_handles[CHR_H_SENSOR_STATUS]) {
        sensor_status_t st;
        build_sensor_status(&st);
        memcpy(buf, &st, sizeof(st));
        len = sizeof(st);
    }

    /* OTA Service reads — none, OTA is write+notify only */

    else {
        ESP_LOGW(TAG, "Read on unknown handle %d", attr_handle);
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (len > 0) {
        int rc = os_mbuf_append(ctxt->om, buf, len);
        if (rc != 0) {
            ESP_LOGW(TAG, "os_mbuf_append rc=%d", rc);
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }

    return 0;
}

/* ============================================================
 * Write handler — dispatches to command queue
 * ============================================================ */

int ble_handle_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt)
{
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len > 244) {
        ESP_LOGW(TAG, "Write too long: %d bytes", len);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    uint8_t buf[244];
    int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len);
    if (rc != 0) {
        ESP_LOGW(TAG, "mbuf_to_flat rc=%d", rc);
        return BLE_ATT_ERR_UNLIKELY;
    }

    ble_cmd_type_t cmd_type = BLE_CMD_NONE;

    /* Map attr_handle to command type */
    if (attr_handle == ble_val_handles[CHR_H_NAV_CMD]) {
        cmd_type = BLE_CMD_NAV_CMD;
    }
    else if (attr_handle == ble_val_handles[CHR_H_MEDIA_CONTROL]) {
        cmd_type = BLE_CMD_MEDIA_CONTROL;
    }
    else if (attr_handle == ble_val_handles[CHR_H_MEDIA_VOLUME]) {
        cmd_type = BLE_CMD_MEDIA_VOLUME;
    }
    else if (attr_handle == ble_val_handles[CHR_H_PHONE_CONTROL]) {
        cmd_type = BLE_CMD_PHONE_CONTROL;
    }
    else if (attr_handle == ble_val_handles[CHR_H_TRACK_EXPORT_CTRL]) {
        cmd_type = BLE_CMD_TRACK_EXPORT_CTRL;
    }
    else if (attr_handle == ble_val_handles[CHR_H_CONFIG_WRITE]) {
        cmd_type = BLE_CMD_CONFIG_WRITE;
    }
    else if (attr_handle == ble_val_handles[CHR_H_CONFIG_IMPORT]) {
        cmd_type = BLE_CMD_CONFIG_IMPORT;
    }
    else if (attr_handle == ble_val_handles[CHR_H_BOND_MGMT]) {
        cmd_type = BLE_CMD_BOND_DELETE;
    }
    else if (attr_handle == ble_val_handles[CHR_H_SELF_TEST]) {
        cmd_type = BLE_CMD_SELF_TEST;
    }
    else if (attr_handle == ble_val_handles[CHR_H_LOG_EXPORT_CTRL]) {
        cmd_type = BLE_CMD_LOG_EXPORT_CTRL;
    }
    else if (attr_handle == ble_val_handles[CHR_H_OTA_CONTROL]) {
        cmd_type = BLE_CMD_OTA_CONTROL;
    }
    else if (attr_handle == ble_val_handles[CHR_H_OTA_DATA]) {
        cmd_type = BLE_CMD_OTA_DATA;
    }
    else {
        ESP_LOGW(TAG, "Write on unknown handle %d", attr_handle);
        return BLE_ATT_ERR_UNLIKELY;
    }

    /* Enqueue for asynchronous processing */
    if (!ble_cmd_enqueue(cmd_type, conn_handle, buf, len)) {
        ESP_LOGW(TAG, "cmd queue full, dropping %d", cmd_type);
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return 0;
}

/* ============================================================
 * Notify helpers (public API, callable from any task)
 * ============================================================ */

void ble_notify_telemetry_live(void)
{
    if (!ble_cccd_telemetry_enabled()) {
        return;
    }

    telemetry_live_t pkt;
    build_telemetry_live(&pkt);
    ble_notify(ble_val_handles[CHR_H_TLM_LIVE], &pkt, sizeof(pkt));
}

void ble_notify_gps_data(void *data, uint16_t len)
{
    (void)data;
    (void)len;
    /* GPS data is embedded in telemetry_live; this is kept for
     * API compatibility — triggers a telemetry push. */
    ble_notify_telemetry_live();
}

void ble_notify_sensor_data(void *data, uint16_t len)
{
    (void)data;
    (void)len;
    /* Sensor data is embedded in telemetry_live */
    ble_notify_telemetry_live();
}

void ble_notify_dtc_list(void)
{
    if (!ble_cccd_dtc_enabled()) {
        return;
    }

    dtc_binary_t buf[DTC_MAX_ENTRIES];
    uint16_t len = build_dtc_list(buf, DTC_MAX_ENTRIES);

    if (len > 0) {
        /* Use indicate for DTCs — important events need confirmation */
        ble_indicate(ble_val_handles[CHR_H_DTC_LIST], buf, len);
    }
}

void ble_notify_sensor_status(void)
{
    if (!ble_cccd_sensor_status_enabled()) {
        return;
    }

    sensor_status_t st;
    build_sensor_status(&st);
    ble_notify(ble_val_handles[CHR_H_SENSOR_STATUS], &st, sizeof(st));
}

/* ============================================================
 * Event-bus callbacks — called by sensor/GPS tasks
 * ============================================================ */

void ble_on_sensor_update(void *data, uint16_t len)
{
    if (data == NULL || len < sizeof(sensor_data_t)) {
        return;
    }

    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    memcpy(&s_sensor_cache, data, sizeof(sensor_data_t));
    xSemaphoreGive(s_data_mutex);
}

void ble_on_gps_update(void *data, uint16_t len)
{
    if (data == NULL || len < sizeof(gps_data_t)) {
        return;
    }

    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    memcpy(&s_gps_cache, data, sizeof(gps_data_t));
    xSemaphoreGive(s_data_mutex);
}

/* ============================================================
 * Write-command processing (called from ble_task)
 * ============================================================ */

static void process_config_write(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        ESP_LOGW(TAG, "Config write: empty payload");
        return;
    }

    /* Ensure null-terminated JSON string */
    char *json_buf = malloc(len + 1);
    if (json_buf == NULL) {
        ESP_LOGE(TAG, "Config write: out of memory (%d bytes)", len + 1);
        return;
    }
    memcpy(json_buf, data, len);
    json_buf[len] = '\0';

    ESP_LOGI(TAG, "Config write: %d bytes JSON", len);
    int rc = config_import_json(json_buf);
    if (rc != 0) {
        ESP_LOGW(TAG, "Config write: JSON import failed (rc=%d)", rc);
    } else {
        ESP_LOGI(TAG, "Config write: applied successfully");
    }

    free(json_buf);
}

static void process_config_import(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len < sizeof(device_config_t)) {
        ESP_LOGW(TAG, "Config import: payload too small (%d < %d)",
                 len, (int)sizeof(device_config_t));
        return;
    }

    ESP_LOGI(TAG, "Config import: %d bytes binary", len);

    device_config_t imported;
    memcpy(&imported, data, sizeof(device_config_t));

    /* Validate version before applying */
    if (imported.version > CONFIG_VERSION) {
        ESP_LOGW(TAG, "Config import: version %d > current %d, migrating",
                 imported.version, CONFIG_VERSION);
        config_migrate(imported.version);
    }

    /* Apply imported config and persist */
    device_config_t *cfg = config_get();
    memcpy(cfg, &imported, sizeof(device_config_t));
    cfg->version = CONFIG_VERSION;

    esp_err_t err = config_save_all();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Config import: save failed (%s)", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Config import: applied and persisted");
    }
}

static void process_bond_delete(const uint8_t *data, uint16_t len)
{
    if (len < 1 || data[0] != 0x00) {
        ESP_LOGW(TAG, "Bond mgmt: invalid command 0x%02x",
                 len > 0 ? data[0] : 0xFF);
        return;
    }

    ESP_LOGI(TAG, "Deleting all bonding keys");
    ble_store_clear();
    /* Reboot to restart advertising cleanly */
    esp_restart();
}

static void process_nav_cmd(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "Nav command: 0x%02x", cmd);
    /* 0x01=cancel route, 0x02=repeat maneuver, 0x03=alternative route */
}

static void process_media_control(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "Media control: 0x%02x", cmd);
    /* 0x01=play/pause, 0x02=next, 0x03=prev, 0x04=seek+, 0x05=seek- */
}

static void process_media_volume(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t vol = data[0];
    ESP_LOGI(TAG, "Volume set: %d", vol);
}

static void process_phone_control(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "Phone control: 0x%02x", cmd);
    /* 0x01=accept, 0x02=reject, 0x03=hangup */
}

static void process_track_export_ctrl(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "Track export ctrl: 0x%02x", cmd);
    /* 0x01=start, 0x02=cancel, 0x03=status */
}

static void process_self_test(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "Self-test cmd: 0x%02x", cmd);

    if (cmd == 0x01) {
        /* Start self-test */
        bool result = selftest_run();

        /* Notify result */
        uint8_t result_buf[2];
        result_buf[0] = result ? 0 : 2;  /* 0=pass, 2=fail */
        result_buf[1] = 0;  /* details count */
        ble_notify(ble_val_handles[CHR_H_SELF_TEST], result_buf, 2);
    }
}

static void process_log_export_ctrl(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "Log export ctrl: 0x%02x", cmd);
    /* 0x01=start, 0x02=cancel */
}

static uint8_t ota_state_to_spec(ota_state_t s)
{
    switch (s) {
    case OTA_STATE_IDLE:         return 0;  /* idle */
    case OTA_STATE_DOWNLOADING:  return 1;  /* uploading */
    case OTA_STATE_VERIFYING:    return 2;  /* verifying */
    case OTA_STATE_FLASHING:     return 3;  /* done */
    case OTA_STATE_REBOOTING:    return 3;  /* done */
    case OTA_STATE_ERROR:        return 4;  /* failed */
    case OTA_STATE_ROLLBACK:     return 4;  /* failed */
    default:                     return 4;
    }
}

static void notify_ota_status(void)
{
    struct __attribute__((packed)) {
        uint8_t state;
        uint8_t progress;
        uint8_t error;
    } pkt = {
        .state    = ota_state_to_spec(ota_get_state()),
        .progress = ota_get_progress(),
        .error    = (uint8_t)ota_get_error(),
    };
    ble_notify(ble_val_handles[CHR_H_OTA_CONTROL], &pkt, sizeof(pkt));
}

/* CHR_OTA_VERIFY (0x8003) notify — see docs/BLE-INTERFACE-SPEC.md §9.3.
 * Payload: u8 status + u8[32] sha256 + u32 size + u32 version (41 bytes).
 * Emitted on the commit failure path so a subscribed app learns the
 * computed hash and failure reason. On success the device reboots inside
 * ota_ble_commit() before this can be delivered (TOT-33: verify/commit split). */
static void notify_ota_verify(void)
{
    ota_verify_result_t result;
    if (ota_ble_get_verify_result(&result) != ESP_OK) {
        return;
    }

    struct __attribute__((packed)) {
        uint8_t  status;
        uint8_t  sha256[OTA_SHA256_LEN];
        uint32_t size;
        uint32_t version;
    } pkt;

    pkt.status  = result.status;
    memcpy(pkt.sha256, result.sha256, OTA_SHA256_LEN);
    pkt.size    = result.size;
    pkt.version = result.version;

    _Static_assert(sizeof(pkt) == OTA_VERIFY_NOTIFY_SIZE,
                   "OTA_VERIFY notify payload size mismatch (spec §9.3)");

    ble_notify(ble_val_handles[CHR_H_OTA_VERIFY], &pkt, sizeof(pkt));
}

static void process_ota_control(const uint8_t *data, uint16_t len)
{
    if (len < 1) return;
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "OTA control: 0x%02x", cmd);

    /* Command bytes per docs/BLE-INTERFACE-SPEC.md §9.1 */
    switch (cmd) {
    case 0x00: {  /* Start */
        if (len < 1 + sizeof(uint32_t)) {
            ESP_LOGE(TAG, "OTA Start: missing total_size (len=%u)", (unsigned)len);
            notify_ota_status();
            return;
        }
        uint32_t total_size;
        memcpy(&total_size, &data[1], sizeof(total_size));
        esp_err_t err = ota_update_from_ble(total_size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ota_update_from_ble failed: %s", esp_err_to_name(err));
        }
        notify_ota_status();
        break;
    }
    case 0x01: {  /* Cancel */
        esp_err_t err = ota_abort();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ota_abort failed: %s", esp_err_to_name(err));
        }
        notify_ota_status();
        break;
    }
    case 0x02: {  /* Verify — status checkpoint (verify+commit fused in ota_ble_commit) */
        notify_ota_status();
        break;
    }
    case 0x03: {  /* Commit — verify + set boot partition + reboot */
        const uint8_t *expected_hash = NULL;
        if (len >= 1 + OTA_SHA256_LEN) {
            expected_hash = &data[1];
        }
        esp_err_t err = ota_ble_commit(expected_hash);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ota_ble_commit failed: %s", esp_err_to_name(err));
            notify_ota_status();
            /* Emit the verify result on 0x8003 (CHR_OTA_VERIFY) so a
             * subscribed app receives the computed hash and the failure
             * reason (spec §9.3). On success ota_ble_commit() reboots
             * before this can be delivered — see TOT-33. */
            notify_ota_verify();
        }
        break;
    }
    default:
        ESP_LOGW(TAG, "Unknown OTA cmd 0x%02x", cmd);
        notify_ota_status();
        break;
    }
}

static void process_ota_data(const uint8_t *data, uint16_t len)
{
    ESP_LOGD(TAG, "OTA data: %u bytes", (unsigned)len);

    /* Wire payload is a packed ota_chunk_t: offset(4) + length(2) + crc16(2) + data[length].
     * Only 8 + length bytes are valid; ota_ble_write_chunk reads length bytes of data. */
    if (len < 8) {
        ESP_LOGE(TAG, "OTA chunk too short: %u bytes", (unsigned)len);
        notify_ota_status();
        return;
    }

    const ota_chunk_t *chunk = (const ota_chunk_t *)data;

    if (chunk->length > (OTA_BLE_MTU_PAYLOAD - 8) ||
        (uint16_t)(chunk->length + 8) != len) {
        ESP_LOGE(TAG, "OTA chunk length mismatch: length=%u, len=%u",
                 (unsigned)chunk->length, (unsigned)len);
        notify_ota_status();
        return;
    }

    esp_err_t err = ota_ble_write_chunk(chunk);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ota_ble_write_chunk failed: %s", esp_err_to_name(err));
        notify_ota_status();
    }
}

/* ============================================================
 * BLE Task — processes write commands and periodic notifies
 * ============================================================ */

void ble_task(void *arg)
{
    (void)arg;

    /* Create write-command queue */
    ble_cmd_queue = xQueueCreate(8, sizeof(ble_cmd_t));
    if (ble_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create cmd queue");
        vTaskDelete(NULL);
        return;
    }

    /* Create data mutex */
    s_data_mutex = xSemaphoreCreateMutex();
    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create data mutex");
        vTaskDelete(NULL);
        return;
    }

    /* Initialize caches */
    memset(&s_sensor_cache, 0, sizeof(s_sensor_cache));
    memset(&s_gps_cache, 0, sizeof(s_gps_cache));

    TickType_t last_telemetry_notify = xTaskGetTickCount();
    const TickType_t telemetry_period = pdMS_TO_TICKS(200);  /* 5 Hz */

    ESP_LOGI(TAG, "BLE task started (notify rate: 5Hz telemetry)");

    ble_cmd_t cmd;

    while (1) {
        /* Process pending write commands (non-blocking) */
        while (xQueueReceive(ble_cmd_queue, &cmd, 0) == pdTRUE) {
            switch (cmd.type) {
            case BLE_CMD_CONFIG_WRITE:
                process_config_write(cmd.data, cmd.len);
                break;
            case BLE_CMD_CONFIG_IMPORT:
                process_config_import(cmd.data, cmd.len);
                break;
            case BLE_CMD_BOND_DELETE:
                process_bond_delete(cmd.data, cmd.len);
                break;
            case BLE_CMD_NAV_CMD:
                process_nav_cmd(cmd.data, cmd.len);
                break;
            case BLE_CMD_MEDIA_CONTROL:
                process_media_control(cmd.data, cmd.len);
                break;
            case BLE_CMD_MEDIA_VOLUME:
                process_media_volume(cmd.data, cmd.len);
                break;
            case BLE_CMD_PHONE_CONTROL:
                process_phone_control(cmd.data, cmd.len);
                break;
            case BLE_CMD_TRACK_EXPORT_CTRL:
                process_track_export_ctrl(cmd.data, cmd.len);
                break;
            case BLE_CMD_SELF_TEST:
                process_self_test(cmd.data, cmd.len);
                break;
            case BLE_CMD_LOG_EXPORT_CTRL:
                process_log_export_ctrl(cmd.data, cmd.len);
                break;
            case BLE_CMD_OTA_CONTROL:
                process_ota_control(cmd.data, cmd.len);
                break;
            case BLE_CMD_OTA_DATA:
                process_ota_data(cmd.data, cmd.len);
                break;
            default:
                ESP_LOGW(TAG, "Unknown cmd type %d", cmd.type);
                break;
            }
        }

        /* Periodic telemetry notify at 5 Hz */
        TickType_t now = xTaskGetTickCount();
        if ((now - last_telemetry_notify) >= telemetry_period) {
            ble_notify_telemetry_live();
            last_telemetry_notify = now;
        }

        /* Small delay to yield */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
