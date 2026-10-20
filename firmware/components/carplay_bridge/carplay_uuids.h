#ifndef _CARPLAY_UUIDS_H_
#define _CARPLAY_UUIDS_H_

/*
 * carplay_uuids.h — GATT Service/Characteristic UUIDs fuer CarPlay-Bridge
 *
 * Die CarPlay-Bridge koppelt den Bordcomputer mit einem externen
 * 5 Zoll CarPlay-Display (EKIY/Jiuyin, AliExpress). Der Bordcomputer
 * ist gleichzeitig GATT-Server (pusht Telemetrie ans Display) und
 * GATT-Client (empfaegt Navigations-Status und Musik-Metadaten vom
 * Display).
 *
 * Format: 128-bit UUID, Base = 6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
 * Die 16-bit-ID XXXX identifiziert Service oder Characteristic.
 * Identisch zu gatt_uuids.h — siehe docs/CARPLAY-INTEGRATION.md.
 *
 * Service-Bereich 0x9000 (naechster freier Block nach 0x1000-0x8000).
 * Chars: Service-ID + 0x01 bis Service-ID + 0x0F.
 */

#include <stdint.h>

/* ============================================================
 * 128-bit UUID construction for NimBLE (little-endian value[])
 * Identisch zum Makro in gatt_uuids.h.
 * ============================================================ */

#define CARPLAY_UUID128(id16) \
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
 * Bordcomputer-seitiger CarPlay-Bridge Service (0x9000)
 *
 * Dieser Service wird vom Bordcomputer (GATT-Server) bereitgestellt.
 * Das CarPlay-Display abonniert die Telemetry-Characteristic und
 * empfängt so RPM, Speed, Temp, Fuel, Gear.
 * ============================================================ */

#define SVC_CARPLAY_BRIDGE   0x9000

#define CHR_CP_TLM_LIVE      0x9001  /* notify: 12-byte packed live telemetry */
#define CHR_CP_TLM_SNAPSHOT  0x9002  /* read: full telemetry snapshot */
#define CHR_CP_STATUS        0x9003  /* read+notify: CarPlay connection status */
#define CHR_CP_PAIRING      0x9004  /* write: pair with CarPlay display */
#define CHR_CP_BOND_MGMT    0x9005  /* write: 0x00=clear CarPlay bond */

/* ============================================================
 * Peer-seitiger CarPlay-Display Service (0x9100)
 *
 * Dieser Service wird vom CarPlay-Display bereitgestellt. Der
 * Bordcomputer verbindet sich als GATT-Client, entdeckt diesen
 * Service und abonniert die Notify-Characteristics fuer
 * Navigations-Status und Musik-Metadaten.
 * ============================================================ */

#define SVC_CARPLAY_DISPLAY  0x9100

#define CHR_CP_NAV_STATUS    0x9101  /* notify: NavStatus enum + payload */
#define CHR_CP_NAV_MANEUVER  0x9102  /* notify: ManeuverType, distance, street */
#define CHR_CP_MEDIA_NOW     0x9103  /* notify: artist, title, duration, pos, state */
#define CHR_CP_MEDIA_CTRL    0x9104  /* write: play/pause/next/prev/seek */
#define CHR_CP_PHONE_INCOMING 0x9105 /* notify: caller-id, call-type */

/* ============================================================
 * CarPlay connection status (CHR_CP_STATUS byte 0)
 * ============================================================ */

enum carplay_status_val {
    CARPLAY_STATUS_DISCONNECTED = 0,
    CARPLAY_STATUS_CONNECTING  = 1,
    CARPLAY_STATUS_CONNECTED   = 2,
    CARPLAY_STATUS_DEGRADING   = 3,
    CARPLAY_STATUS_ERROR       = 4,
};

/* ============================================================
 * CarPlay telemetry live packet (12 bytes packed, CHR_CP_TLM_LIVE)
 *
 * Komplettes Layout siehe carplay_internal.h (carplay_telemetry_t).
 * Das Display zeigt diese Felder als sekundaere Anzeige neben
 * dem Bordcomputer-TFT.
 * ============================================================ */

#define CARPLAY_TLM_LIVE_SIZE  12

/* ============================================================
 * Maneuver types (CHR_CP_NAV_MANEUVER byte 0)
 * Identisch zu maneuver_type in gatt_uuids.h.
 * ============================================================ */

enum carplay_maneuver_type {
    CARPLAY_MANEUVER_STRAIGHT     = 0,
    CARPLAY_MANEUVER_SLIGHT_LEFT  = 1,
    CARPLAY_MANEUVER_LEFT         = 2,
    CARPLAY_MANEUVER_SHARP_LEFT   = 3,
    CARPLAY_MANEUVER_SLIGHT_RIGHT = 4,
    CARPLAY_MANEUVER_RIGHT        = 5,
    CARPLAY_MANEUVER_SHARP_RIGHT  = 6,
    CARPLAY_MANEUVER_UTURN        = 7,
    CARPLAY_MANEUVER_ARRIVE       = 8,
    CARPLAY_MANEUVER_FORK         = 9,
};

/* ============================================================
 * Nav status (CHR_CP_NAV_STATUS byte 0)
 * Identisch zu nav_status_val in gatt_uuids.h.
 * ============================================================ */

enum carplay_nav_status_val {
    CARPLAY_NAV_INACTIVE   = 0,
    CARPLAY_NAV_ACTIVE     = 1,
    CARPLAY_NAV_REROUTING  = 2,
    CARPLAY_NAV_ARRIVING   = 3,
    CARPLAY_NAV_OFFLINE    = 4,
};

/* ============================================================
 * Media playback state (CHR_CP_MEDIA_NOW byte 0)
 * ============================================================ */

enum carplay_media_state {
    CARPLAY_MEDIA_STOPPED   = 0,
    CARPLAY_MEDIA_PLAYING   = 1,
    CARPLAY_MEDIA_PAUSED    = 2,
    CARPLAY_MEDIA_BUFFERING = 3,
};

/* ============================================================
 * Media control commands (CHR_CP_MEDIA_CTRL write byte 0)
 * ============================================================ */

enum carplay_media_cmd {
    CARPLAY_MEDIA_CMD_PLAYPAUSE = 0x01,
    CARPLAY_MEDIA_CMD_NEXT      = 0x02,
    CARPLAY_MEDIA_CMD_PREV      = 0x03,
    CARPLAY_MEDIA_CMD_SEEK_FWD  = 0x04,
    CARPLAY_MEDIA_CMD_SEEK_BACK = 0x05,
};

#endif /* _CARPLAY_UUIDS_H_ */
