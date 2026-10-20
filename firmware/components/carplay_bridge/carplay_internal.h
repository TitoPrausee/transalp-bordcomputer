#ifndef _CARPLAY_INTERNAL_H_
#define _CARPLAY_INTERNAL_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "carplay_uuids.h"

/* Forward-declare NimBLE types to avoid pulling host headers here */
struct ble_gap_event;
struct ble_gatt_access_ctxt;

/*
 * carplay_internal.h — Internal definitions shared between
 * carplay_bridge.c, carplay_client.c, carplay_telemetry.c und
 * carplay_pairing.c. NICHT Teil der oeffentlichen API.
 */

/* ============================================================
 * Attribute handle indices (Bordcomputer-seitiger Service 0x9000)
 *
 * NimBLE assigns handles dynamically. Wir uebergeben
 * &carplay_val_handles[idx] als .val_handle in jeder Characteristic.
 * Nach Service-Registration fuellt NimBLE diese mit den echten Handles.
 * ============================================================ */

enum {
    CP_H_TLM_LIVE,
    CP_H_TLM_SNAPSHOT,
    CP_H_STATUS,
    CP_H_PAIRING,
    CP_H_BOND_MGMT,
    CP_H_COUNT,
};

/* Handle-Tabelle — gefuellt durch NimBLE waehrend Service-Registration */
extern uint16_t carplay_val_handles[CP_H_COUNT];

/* ============================================================
 * Telemetry live packet (12 bytes packed, CHR_CP_TLM_LIVE 0x9001)
 *
 * Layout (Little-Endian):
 *   0     speed_kmh        uint8   km/h
 *   1-2   rpm              uint16  U/min
 *   3     gear             int8    -1 = neutral
 *   4     coolant_temp_c   int8    Celsius
 *   5     fuel_percent     uint8   0-100
 *   6-7   voltage_tenth    uint16  Zehntelvolt
 *   8-9   ambient_temp_c   int16   Celsius
 *   10    fuel_low         uint8   0/1 (unter Reserve)
 *   11    reserved         uint8   0
 * ============================================================ */

typedef struct __attribute__((packed)) {
    uint8_t  speed_kmh;
    uint16_t rpm;
    int8_t   gear;
    int8_t   coolant_temp_c;
    uint8_t  fuel_percent;
    uint16_t voltage_tenth;
    int16_t  ambient_temp_c;
    uint8_t  fuel_low;
    uint8_t  reserved;
} carplay_telemetry_t;

_Static_assert(sizeof(carplay_telemetry_t) == CARPLAY_TLM_LIVE_SIZE,
               "CarPlay TLM_LIVE packed size mismatch");

/* ============================================================
 * Eingangsdaten fuer den Telemetry-Packer.
 *
 * Die Sensor-/GPS-/Power-Tasks kopieren ihre Werte in diese
 * Struktur; carplay_telemetry_build() wandelt sie in das
 * 12-byte Packet um.
 * ============================================================ */

typedef struct {
    float    speed_kmh;
    uint16_t rpm;
    int8_t   gear;           /* 0 = neutral */
    float    coolant_temp_c;
    float    ambient_temp_c;
    float    fuel_level_l;
    float    battery_voltage;
    float    tank_capacity_l;  /* fuer Prozent-Umrechnung */
    float    fuel_reserve_l;   /* fuer fuel_low Flag */
} carplay_telemetry_input_t;

/* ============================================================
 * Paired CarPlay display record (NVS)
 *
 * Speichert die BLE-Adresse des gekoppelten CarPlay-Displays,
 * damit der Bordcomputer beim Start automatisch als Client
 * wieder verbinden kann.
 * ============================================================ */

#define CARPLAY_PEER_ADDR_LEN  6
#define CARPLAY_NVS_NAMESPACE "carplay"
#define CARPLAY_NVS_KEY_PEER   "peer_addr"

typedef struct {
    uint8_t addr[CARPLAY_PEER_ADDR_LEN];  /* little-endian, NimBLE convention */
    uint8_t addr_type;                     /* 0=public, 1=random */
    bool    valid;
} carplay_peer_t;

/* ============================================================
 * Client state machine (carplay_client.c)
 * ============================================================ */

typedef enum {
    CP_CLIENT_IDLE = 0,
    CP_CLIENT_CONNECTING,
    CP_CLIENT_DISCOVERING,
    CP_CLIENT_SUBSCRIBING,
    CP_CLIENT_CONNECTED,
    CP_CLIENT_RECONNECTING,
} carplay_client_state_t;

/* ============================================================
 * Inbound data from CarPlay display (received via GATT notify)
 *
 * Der Client entdeckt die Peer-Characteristics und merkt sich
 * deren val_handle. Bei eingehenden Notifys werden die Daten in
 * diese Caches kopiert (mutex-gesichert) und per Event-Bus
 * signalisiert.
 * ============================================================ */

typedef struct {
    uint8_t  nav_status;       /* carplay_nav_status_val */
    uint8_t  maneuver;         /* carplay_maneuver_type */
    uint16_t maneuver_dist_m;
    uint8_t  media_state;      /* carplay_media_state */
    uint16_t media_position_s;
    uint16_t media_duration_s;
} carplay_inbound_t;

/* ============================================================
 * Shared internal state (defined in carplay_bridge.c)
 * ============================================================ */

extern SemaphoreHandle_t carplay_data_mutex;
extern carplay_inbound_t  carplay_inbound_cache;

/* Telemetry packer (defined in carplay_telemetry.c) */
void carplay_telemetry_build(carplay_telemetry_t *pkt,
                              const carplay_telemetry_input_t *in);

/* NVS pairing storage (defined in carplay_pairing.c) */
bool carplay_pairing_store(const uint8_t *addr, uint8_t addr_type);
bool carplay_pairing_load(carplay_peer_t *peer);
bool carplay_pairing_erase(void);

/* Client lifecycle (defined in carplay_client.c) */
void carplay_client_init(void);
void carplay_client_start(const carplay_peer_t *peer);
void carplay_client_stop(void);
carplay_client_state_t carplay_client_get_state(void);

/* Connection handle of the active CarPlay-display link (BLE_HS_CONN_HANDLE_NONE
 * if not connected). Used by the server-side notify path. */
uint16_t carplay_client_get_conn_handle(void);

/*
 * Forward a media-control command byte (carplay_media_cmd) to the peer
 * display's CHR_CP_MEDIA_CTRL (0x9104).  No-op when not connected or the
 * characteristic has not been discovered.  Closes gap C4 / FU-4.
 */
void carplay_client_media_ctrl(uint8_t cmd);

/* GATT access callback for the Bordcomputer-side service 0x9000
 * (defined in carplay_bridge.c, referenced by the service table). */
int carplay_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);

/* GAP event handler for the CarPlay client connection (carplay_client.c) */
int carplay_client_gap_event(struct ble_gap_event *event, void *arg);

/* Called by carplay_client.c when connection state changes */
void carplay_client_on_connected(void);
void carplay_client_on_disconnected(void);

/* Called by carplay_client.c when a notify is received */
void carplay_client_on_notify(uint16_t attr_handle,
                               const uint8_t *data, uint16_t len);

#endif /* _CARPLAY_INTERNAL_H_ */
