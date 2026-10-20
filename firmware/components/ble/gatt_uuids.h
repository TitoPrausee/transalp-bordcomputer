#ifndef _GATT_UUIDS_H_
#define _GATT_UUIDS_H_

/*
 * gatt_uuids.h — GATT Service/Characteristic UUIDs
 *
 * MUESSEN mit der Companion-App uebereinstimmen!
 * Siehe docs/research/13-connectivity-app.md Abschnitt 2.
 * Siehe companion-app/src/core/ble/gatt-uuids.ts (Referenz).
 *
 * Format: 128-bit UUID, Base = 6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
 * Die 16-bit-ID XXXX identifiziert Service oder Characteristic.
 *
 * Services:    0x1000 Schritte (0x1000-0x8000)
 * Chars:       Service-ID + 0x01 bis Service-ID + 0x0F
 */

#include <stdint.h>

/* ============================================================
 * 128-bit UUID construction for NimBLE
 *
 * NimBLE uses ble_uuid128_t with the UUID stored little-endian.
 * The base UUID 6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e maps to:
 *   bytes [15..0] = {0x6e,0x40, ID_HI, ID_LO, 0xb5,0xa3, 0xf3,0x93,
 *                     0xe0,0xa9, 0xe5,0x0e, 0x24,0xdc, 0xca,0x9e}
 * but ble_uuid128_t.value is little-endian, so we reverse:
 *   value[0]=0x9e ... value[15]=0x6e
 * ============================================================ */

#define BLE_UUID128(id16) \
    { \
        .u = { .type = BLE_UUID_TYPE_128 }, \
        .value = { \
            0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, \
            0x93, 0xf3, 0xa3, 0xb5, \
            (uint8_t)((id16) & 0xFF), (uint8_t)(((id16) >> 8) & 0xFF), \
            0x40, 0x6e \
        } \
    }

/* ============================================================
 * Service UUIDs (16-bit IDs)
 * ============================================================ */

#define SVC_NAVIGATION     0x1000
#define SVC_MEDIA          0x2000
#define SVC_PHONE          0x3000
#define SVC_WEATHER        0x4000
#define SVC_TELEMETRY      0x5000
#define SVC_CONFIG         0x6000
#define SVC_DIAGNOSE       0x7000
#define SVC_OTA            0x8000

/* ============================================================
 * Navigation Service (0x1000)
 * ============================================================ */

#define CHR_NAV_MANEUVER   0x1001  /* read+notify: ManeuverType, distance, street */
#define CHR_NAV_ROUTE      0x1002  /* read+notify: total/remaining dist, eta, time */
#define CHR_NAV_ROAD       0x1003  /* read+notify: speed limit, road name */
#define CHR_NAV_STATUS     0x1004  /* read+notify: NavStatus enum */
#define CHR_NAV_CMD        0x1005  /* write+write_nr: Bordcomputer -> App commands */

/* ============================================================
 * Media Service (0x2000)
 * ============================================================ */

#define CHR_MEDIA_NOWPLAYING 0x2001  /* read+notify: artist, title, duration, pos, state */
#define CHR_MEDIA_CONTROL    0x2002  /* write+write_nr: play/pause/next/prev/seek */
#define CHR_MEDIA_VOLUME     0x2003  /* read+notify+write: uint8 0-100 */

/* ============================================================
 * Phone Service (0x3000)
 * ============================================================ */

#define CHR_PHONE_INCOMING  0x3001  /* notify: caller-id, call-type */
#define CHR_PHONE_STATUS    0x3002  /* read+notify: uint8 call status */
#define CHR_PHONE_CONTROL   0x3003  /* write: accept/reject/hangup */
#define CHR_PHONE_NOTIF     0x3004  /* notify: app-name, title, body */

/* ============================================================
 * Weather Service (0x4000)
 * ============================================================ */

#define CHR_WX_CURRENT      0x4001  /* read+notify: temp, humidity, condition, wind */
#define CHR_WX_FORECAST     0x4002  /* read+notify: 6h forecast array */
#define CHR_WX_ALERT        0x4003  /* notify: alert type, severity, time */

/* ============================================================
 * Telemetry Service (0x5000)
 * ============================================================ */

#define CHR_TLM_LIVE        0x5001  /* notify: 22-byte packed live telemetry */
#define CHR_TLM_SNAPSHOT    0x5002  /* read: full sensor snapshot ~60 bytes */
#define CHR_TRACK_EXPORT    0x5003  /* notify: GPX binary transfer */
#define CHR_TRACK_EXPORT_CTRL 0x5004  /* write: start/cancel/status */

/* ============================================================
 * Config Service (0x6000)
 * ============================================================ */

#define CHR_CONFIG_READ     0x6001  /* read+notify: JSON chunk */
#define CHR_CONFIG_WRITE    0x6002  /* write: JSON chunk */
#define CHR_CONFIG_EXPORT   0x6003  /* notify: binary config export */
#define CHR_CONFIG_IMPORT   0x6004  /* write: binary config import */
#define CHR_BOND_MGMT       0x6005  /* write: 0x00=delete all bonds */

/* ============================================================
 * Diagnose Service (0x7000)
 * ============================================================ */

#define CHR_DTC_LIST        0x7001  /* read+notify: DTC array, 6 bytes each */
#define CHR_SENSOR_STATUS   0x7002  /* read+notify: 5-byte sensor health */
#define CHR_SELF_TEST       0x7003  /* write+notify: trigger/result */
#define CHR_LOG_EXPORT      0x7004  /* notify: log binary transfer */
#define CHR_LOG_EXPORT_CTRL 0x7005  /* write: start/cancel log export */

/* ============================================================
 * OTA Service (0x8000)
 * ============================================================ */

#define CHR_OTA_CONTROL     0x8001  /* write+notify: begin/commit/abort + status */
#define CHR_OTA_DATA        0x8002  /* write_nr: firmware image chunks */
#define CHR_OTA_VERIFY      0x8003  /* notify: sha256, size, status */

/* CHR_OTA_VERIFY (0x8003) notify payload — see docs/BLE-INTERFACE-SPEC.md §9.3
 * Layout (little-endian, packed, 41 bytes):
 *   offset 0  : u8     status   (ota_verify_status enum, see ota_update.h)
 *   offset 1  : u8[32] sha256   (computed image hash)
 *   offset 33 : u32    size     (bytes written)
 *   offset 37 : u32    version  (firmware version, best-effort, 0 = unknown)
 */
#define OTA_VERIFY_NOTIFY_SIZE   41u
#define OTA_VERIFY_OFF_STATUS    0u
#define OTA_VERIFY_OFF_SHA256    1u
#define OTA_VERIFY_OFF_SIZE      33u
#define OTA_VERIFY_OFF_VERSION   37u

/* ============================================================
 * Maneuver types (CHR_NAV_MANEUVER byte 0)
 * ============================================================ */

enum maneuver_type {
    MANEUVER_STRAIGHT     = 0,
    MANEUVER_SLIGHT_LEFT  = 1,
    MANEUVER_LEFT         = 2,
    MANEUVER_SHARP_LEFT   = 3,
    MANEUVER_SLIGHT_RIGHT = 4,
    MANEUVER_RIGHT        = 5,
    MANEUVER_SHARP_RIGHT  = 6,
    MANEUVER_UTURN        = 7,
    MANEUVER_ARRIVE       = 8,
    MANEUVER_FORK         = 9,
};

/* ============================================================
 * Nav status (CHR_NAV_STATUS)
 * ============================================================ */

enum nav_status_val {
    NAV_STATUS_INACTIVE   = 0,
    NAV_STATUS_ACTIVE     = 1,
    NAV_STATUS_REROUTING  = 2,
    NAV_STATUS_ARRIVING  = 3,
    NAV_STATUS_OFFLINE    = 4,
};

/* ============================================================
 * DTC severity (CHR_DTC_LIST byte 2)
 * ============================================================ */

enum dtc_severity_wire {
    DTC_WIRE_SEV_INFO     = 0,
    DTC_WIRE_SEV_WARNING  = 1,
    DTC_WIRE_SEV_CRITICAL = 2,
};

#endif /* _GATT_UUIDS_H_ */
