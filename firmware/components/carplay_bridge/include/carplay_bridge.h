#ifndef _CARPLAY_BRIDGE_H_
#define _CARPLAY_BRIDGE_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * carplay_bridge.h — CarPlay BLE-Bridge
 *
 * Koppelt den Bordcomputer mit einem externen 5 Zoll CarPlay-Display
 * (EKIY/Jiuyin). Der Bordcomputer fungiert gleichzeitig als:
 *
 *   - GATT-Server: pusht Telemetrie (RPM, Speed, Temp, Fuel, Gear)
 *     ans Display (Service 0x9000, CHR_CP_TLM_LIVE notify).
 *   - GATT-Client: verbindet sich zum Display und empfängt
 *     Navigations-Status und Musik-Metadaten (Service 0x9100).
 *
 * Das Bordcomputer-TFT bleibt fuer Fahrzeugdaten, das CarPlay-Display
 * wird als sekundaere Anzeige daneben montiert.
 *
 * Thread-safety: BLE-Callbacks laufen im NimBLE-Host-Task. Telemetrie-
 * und Inbound-Daten werden ueber Mutex-gesicherte Caches ausgetauscht.
 *
 * Siehe docs/CARPLAY-INTEGRATION.md.
 */

/* ============================================================
 * Lifecycle
 * ============================================================ */

/*
 * Registriert den Bordcomputer-seitigen CarPlay-Bridge GATT-Service
 * (0x9000) beim NimBLE-Stack. Muss waehrend ble_init() VOR dem Start
 * des NimBLE-Host-Tasks aufgerufen werden (vor nimble_port_freertos_init).
 * Wird von der BLE-Komponente aufgerufen, nicht direkt aus app_main.
 */
void carplay_bridge_register_gatt(void);

/*
 * Initialisiert die CarPlay-Bridge: erstellt Mutex, laedt gespeicherte
 * Pairing-Daten aus NVS, initialisiert den GATT-Client. Muss NACH
 * ble_init() aufgerufen werden.
 */
void carplay_bridge_init(void);

/*
 * CarPlay-Bridge Task: verarbeitet den Client-Verbindungszyklus und
 * pusht periodisch Telemetrie ans Display (5 Hz, wenn verbunden).
 */
void carplay_bridge_task(void *arg);

/* ============================================================
 * Connection state
 * ============================================================ */

/* True, wenn der Bordcomputer mit dem CarPlay-Display verbunden ist. */
bool carplay_bridge_is_connected(void);

/* Aktueller Verbindungsstatus (carplay_status_val). */
uint8_t carplay_bridge_get_status(void);

/* ============================================================
 * Pairing / Bonding (NVS)
 * ============================================================ */

/*
 * Speichert die BLE-Adresse des CarPlay-Displays in NVS. Nach dem
 * Speichern versucht der Client, automatisch zu verbinden.
 * Gibt true bei Erfolg zurueck.
 */
bool carplay_bridge_pair(const uint8_t *addr, uint8_t addr_type);

/*
 * Laedt die gespeicherte CarPlay-Display-Adresse aus NVS.
 * Gibt true zurueck, wenn ein gueltiger Eintrag existiert.
 */
bool carplay_bridge_get_paired(uint8_t *addr_out, uint8_t *addr_type_out);

/*
 * Loescht das gespeicherte CarPlay-Display-Pairing aus NVS und
 * trennt eine bestehende Client-Verbindung.
 */
void carplay_bridge_unpair(void);

/* ============================================================
 * Telemetry push (callable from any task)
 * ============================================================ */

/*
 * Pusht das aktuelle Telemetrie-Packet ans CarPlay-Display
 * (CHR_CP_TLM_LIVE notify), sofern verbunden und CCCD abonniert.
 */
void carplay_bridge_notify_telemetry(void);

/*
 * Aktualisiert den Telemetrie-Cache aus Sensordaten. Wird vom
 * Sensor-/Event-Loop-Task aufgerufen (analog zu ble_on_sensor_update).
 */
void carplay_bridge_on_sensor_update(const void *data, uint16_t len);

/* ============================================================
 * Inbound data from CarPlay display (read-only access)
 * ============================================================ */

/*
 * Kopiert die zuletzt empfangenen Navigations-/Musik-Daten vom
 * CarPlay-Display in out. Gibt true zurueck, wenn Daten vorliegen.
 */
bool carplay_bridge_get_inbound(void *out, uint16_t len);

#endif /* _CARPLAY_BRIDGE_H_ */
