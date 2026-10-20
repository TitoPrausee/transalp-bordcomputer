# CarPlay-Integration: BLE-Bridge Firmware

> CarPlay-Integration fuer den Transalp Bordcomputer ueber ein externes
> 5 Zoll CarPlay-Display (EKIY/Jiuyin, Online-Marktplatz, ca. 65-90 EUR).
> Das Bordcomputer-TFT bleibt fuer Fahrzeugdaten, das CarPlay-Display
> wird als sekundaere Anzeige daneben montiert.
>
> Komponente: `firmware/components/carplay_bridge/`
> Status: Implementiert, Unit-Tests gruen
> Stand: Oktober 2026

---

## Inhaltsverzeichnis

1. [Uebersicht](#1-uebersicht)
2. [Architektur](#2-architektur)
3. [Hardware](#3-hardware)
4. [BLE GATT Service (Bordcomputer, 0x9000)](#4-ble-gatt-service-bordcomputer-0x9000)
5. [BLE GATT Client (Display, 0x9100)](#5-ble-gatt-client-display-0x9100)
6. [Telemetry-Paket-Format](#6-telemetry-paket-format)
7. [Pairing / NVS-Speicherung](#7-pairing--nvs-speicherung)
8. [Event-Bus-Integration](#8-event-bus-integration)
9. [NimBLE-Konfiguration](#9-nimble-konfiguration)
10. [Wiring / Lifecycle](#10-wiring--lifecycle)
11. [Unit-Tests](#11-unit-tests)
12. [Datei-Uebersicht](#12-datei-uebersicht)

---

## 1. Uebersicht

Der Bordcomputer (ESP32-S3) koppelt sich ueber BLE mit einem externen
CarPlay-Display. Auf derselben BLE-Verbindung fungiert der Bordcomputer
gleichzeitig als:

- **GATT-Server** (Service 0x9000): pusht Telemetrie-Daten
  (RPM, Speed, Temp, Fuel, Gear) ans Display via Notify.
- **GATT-Client** (Central): verbindet sich zum Display, entdeckt
  dessen Service 0x9100 und abonniert Notify-Characteristics fuer
  Navigations-Status und Musik-Metadaten.

Das Bordcomputer-TFT zeigt weiterhin Fahrzeugdaten an. Das CarPlay-Display
wird als sekundaere Anzeige daneben montiert und zeigt CarPlay
(Karten, Musik, Telefon) sowie die vom Bordcomputer gepushte Telemetrie.

---

## 2. Architektur

```
  +-------------------+          BLE GATT          +-------------------+
  |   Bordcomputer    | <========================> |  CarPlay-Display  |
  |   (ESP32-S3)      |    Service 0x9000 (Server)  |  (EKIY/Jiuyin)    |
  |                   |    Service 0x9100 (Client)   |                   |
  +-------------------+                             +-------------------+
  |  carplay_bridge   |                             |  CarPlay-App      |
  |  - Telemetry TX   |  --> RPM, Speed, Gear, ...  |  (Karten-App,     |
  |  - Nav/Media RX   |  <-- Nav-Status, Musik      |   Spotify, ...)   |
  |  - NVS Pairing    |                             |                   |
  |  - Reconnect      |                             |                   |
  +-------------------+                             +-------------------+
         |
  |  Event-Bus       |
  |  EVT_CARPLAY_*   |
  +-------------------+
```

### Rollen auf der BLE-Verbindung

| Rolle        | Service   | Richtung  | Daten                              |
|--------------|-----------|-----------|------------------------------------|
| GATT-Server  | 0x9000    | TX (Notify)| Telemetrie, Status, Pairing       |
| GATT-Client  | 0x9100    | RX (Notify)| Nav-Status, Musik-Metadaten       |

Der Bordcomputer initiiert die Verbindung als Central (GATT-Client).
Auf derselben Verbindung stellt er gleichzeitig seinen GATT-Server
(Service 0x9000) bereit. Das ist mit NimBLE durch die Aktivierung
von Peripheral + Central Role moeglich.

---

## 3. Hardware

### CarPlay-Display

| Eigenschaft       | Wert                                      |
|-------------------|-------------------------------------------|
| Display           | 5 Zoll TFT, 800x480                       |
| CarPlay           | CarPlay (wireless via BLE + WiFi)   |
| Hersteller         | EKIY / Jiuyin (Online-Marktplatz)                |
| Preis              | ca. 65-90 EUR                             |
| Anbindung          | BLE 5.0 (GATT Server + Client)            |
| Stromversorgung    | 12V DC (Motorrad-Bordnetz)                |

### Montage

Das CarPlay-Display wird neben dem Bordcomputer-TFT am Lenker oder
der Cockpit-Verkleidung montiert. Es wird separat mit 12V versorgt.
Die BLE-Verbindung zum Bordcomputer ist kabellos.

---

## 4. BLE GATT Service (Bordcomputer, 0x9000)

Der Bordcomputer stellt den Service `0x9000` (CarPlay-Bridge) bereit.
UUID-Base: `6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e`.

### Characteristics

| UUID 16-bit | Name             | Eigenschaften     | Beschreibung                          |
|-------------|------------------|-------------------|---------------------------------------|
| 0x9001      | CHR_CP_TLM_LIVE  | Notify            | 12-byte Telemetrie-Live-Paket (5 Hz) |
| 0x9002      | CHR_CP_TLM_SNAPSHOT | Read           | Vollstaendige Telemetrie (On-Demand) |
| 0x9003      | CHR_CP_STATUS    | Read + Notify     | CarPlay-Verbindungsstatus (0-4)      |
| 0x9004      | CHR_CP_PAIRING   | Write             | Display-Adresse speichern (7 Byte)   |
| 0x9005      | CHR_CP_BOND_MGMT | Write             | 0x00 = Pairing loeschen               |

### Status-Werte (CHR_CP_STATUS)

| Wert | Bedeutung     |
|------|---------------|
| 0    | Disconnected  |
| 1    | Connecting    |
| 2    | Connected     |
| 3    | Degrading     |
| 4    | Error         |

### Pairing-Protokoll (CHR_CP_PAIRING)

Schreibe 7 Bytes (Little-Endian):

```
Byte 0-5: BLE-Adresse (6 Bytes, NimBLE-Konvention)
Byte 6:   Adress-Typ (0=public, 1=random)
```

Nach dem Schreiben speichert der Bordcomputer die Adresse in NVS
und startet automatisch den GATT-Client-Verbindungsaufbau.

---

## 5. BLE GATT Client (Display, 0x9100)

Der Bordcomputer verbindet sich als GATT-Client zum CarPlay-Display
und entdeckt dessen Service `0x9100`.

### Discovery

Nach dem `BLE_GAP_EVENT_CONNECT` startet der Client eine
`ble_gattc_disc_all_chrs`-Suche ueber den vollen Handle-Bereich
(1-0xFFFF). Die Discovery-Callback-Funktion matcht die Ziel-UUIDs
fuer Nav-Status und Musik-Metadaten und merkt sich deren Value-Handles.

### CCCD-Abonnement

Nach Abschluss der Discovery schreibt der Client 0x0001 (LE) in die
CCCD-Deskriptoren der gefundenen Characteristics, um Notifications
zu aktivieren. Die CCCD wird konventionell bei `val_handle + 1`
erwartet.

### Notify-Empfang

Eingehende Notifications werden im `BLE_GAP_EVENT_NOTIFY_RX`-Handler
entgegengenommen, via `ble_hs_mbuf_to_flat` in einen Linearpuffer
kopiert und an `carplay_client_on_notify()` weitergereicht.

### Reconnect-Backoff

Bei Verbindungsabbruch versucht der Client automatisch einen
Reconnect mit exponentiellem Backoff:

| Versuch | Delay  |
|---------|--------|
| 1       | 1 s    |
| 2       | 2 s    |
| 3       | 4 s    |
| 4       | 8 s    |
| 5       | 16 s   |
| 6+      | 30 s   |

Der Reconnect wird von `carplay_client_poll()` getrieben, das
periodisch aus dem CarPlay-Bridge-Task aufgerufen wird.

---

## 6. Telemetry-Paket-Format

### CHR_CP_TLM_LIVE (0x9001) — 12 Bytes, Little-Endian, packed

| Offset | Feld            | Typ     | Einheit      | Bereich          |
|--------|-----------------|---------|--------------|------------------|
| 0      | speed_kmh       | uint8   | km/h         | 0-255            |
| 1-2    | rpm             | uint16  | U/min        | 0-65535          |
| 3      | gear            | int8    | Gang         | -1=neutral, 1-6  |
| 4      | coolant_temp_c  | int8    | Celsius      | -128..127        |
| 5      | fuel_percent    | uint8   | %            | 0-100            |
| 6-7    | voltage_tenth   | uint16  | Zehntelvolt  | 0-6553           |
| 8-9    | ambient_temp_c  | int16   | Celsius      | -32768..32767    |
| 10     | fuel_low        | uint8   | Flag         | 0/1              |
| 11     | reserved        | uint8   | -            | 0                |

### Konvertierungsregeln (carplay_telemetry_build)

- **Speed**: float km/h -> uint8, clamp 0-255, runden
- **RPM**: direkte Uebernahme als uint16
- **Gear**: 0 (neutral) -> -1 im Wire-Format; sonst direkte Uebernahme
- **Coolant**: float Celsius -> int8, clamp -128..127
- **Fuel**: (fuel_level_l / tank_capacity_l) * 100, clamp 0-100, runden
- **Voltage**: float Volt -> uint16 Zehntelvolt (V * 10), clamp 0-6553
- **Ambient**: float Celsius -> int16
- **Fuel-low**: 1 wenn fuel_level_l <= fuel_reserve_l (und reserve > 0)

---

## 7. Pairing / NVS-Speicherung

Die BLE-Adresse des gekoppelten CarPlay-Displays wird in NVS
gespeichert, damit der Bordcomputer beim Start automatisch
wieder verbinden kann.

| NVS-Namespace | Schluessel   | Inhalt                          |
|---------------|--------------|---------------------------------|
| `carplay`     | `peer_addr`  | 6-Byte BLE-Adresse (blob)       |
| `carplay`     | `peer_type`  | Adress-Typ (uint8: 0=public, 1=random) |

### API

| Funktion                      | Beschreibung                          |
|-------------------------------|---------------------------------------|
| `carplay_pairing_store()`     | Adresse + Typ in NVS speichern        |
| `carplay_pairing_load()`      | Adresse + Typ aus NVS laden           |
| `carplay_pairing_erase()`     | Pairing-Eintrag loeschen              |

Beim `carplay_bridge_init()` wird automatisch ein gespeichertes
Pairing geladen und der Verbindungsaufbau gestartet.

---

## 8. Event-Bus-Integration

Die CarPlay-Bridge signalisiert Verbindungswechsel ueber den
FreeRTOS-Event-Bus (`event_bus.h`):

| Event                    | Bit      | Ausloeser                          |
|--------------------------|----------|------------------------------------|
| `EVT_CARPLAY_CONNECTED`  | (1 << 17)| CarPlay-Display verbunden          |
| `EVT_CARPLAY_DISCONNECTED` | (1 << 18) | CarPlay-Display getrennt         |

Der Event-Loop (`app_event_loop.c`) kann diese Events abonnieren,
um z.B. die Display-Anzeige anzupassen oder Telemetrie-Push zu
aktivieren/deaktivieren.

---

## 9. NimBLE-Konfiguration

Damit der Bordcomputer gleichzeitig als GATT-Server (Peripheral) und
GATT-Client (Central) arbeiten kann, muessen in `sdkconfig.defaults`
folgende Optionen gesetzt sein:

```ini
# Bluetooth — NimBLE
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y    # GATT-Server (Companion-App)
CONFIG_BT_NIMBLE_ROLE_BROADCASTER=y
CONFIG_BT_NIMBLE_ROLE_CENTRAL=y       # GATT-Client (CarPlay-Display)  [NEU]
CONFIG_BT_NIMBLE_MAX_CONNECTIONS=2    # 1x Companion + 1x CarPlay      [NEU]
CONFIG_BT_NIMBLE_NVS_PERSIST=y
CONFIG_BT_NIMBLE_MAX_BONDS=4
CONFIG_BT_NIMBLE_MAX_CCCDS=8
CONFIG_BT_NIMBLE_50_FEATURES_SUPPORTED=y
CONFIG_BT_NIMBLE_EXT_ADV=n
```

Die zwei benoetigten Verbindungen:
1. Companion-App (Peripheral-Seite, Bordcomputer als Server)
2. CarPlay-Display (Central-Seite, Bordcomputer als Client)

---

## 10. Wiring / Lifecycle

### Startup-Sequenz

```
app_main()
  |
  +-> init_step_ble()
  |     +-> ble_init()                          # NimBLE-Stack starten
  |     |     +-> ble_gatts_add_svcs()          # BLE-Services registrieren
  |     |     +-> carplay_bridge_register_gatt() # CarPlay-Service 0x9000 [NEU]
  |     |     +-> nimble_port_freertos_init()   # Host-Task starten
  |     +-> carplay_bridge_init()               # Client init + NVS laden [NEU]
  |
  +-> create_tasks()
        +-> xTaskCreate(carplay_bridge_task)    # Telemetry-Push + Reconnect [NEU]
```

### carplay_bridge_task

Der Task laeuft auf Core 0 (UI/Comms) mit Prioritaet 3 und 4 KB Stack.
Er macht zwei Dinge im 50 ms Zyklus:

1. **Reconnect-Poll**: Wenn der Client im RECONNECTING-Zustand ist
   und der Backoff-Timer abgelaufen ist, wird ein neuer
   Verbindungsversuch gestartet.
2. **Telemetry-Push**: Wenn verbunden und CCCD abonniert, wird das
   12-byte Telemetrie-Paket mit 5 Hz (alle 200 ms) via Notify gepusht.

### Task-Konfiguration (board_config.h)

```c
#define TASK_STACK_CARPLAY   4096
#define TASK_PRIO_CARPLAY    3
```

---

## 11. Unit-Tests

### test_carplay_bridge.c

Testet den Telemetry-Packer (`carplay_telemetry.c`) als reine
Funktion ohne BLE-/ESP-IDF-Abhaengigkeiten. Laeuft auf dem Host-PC
mit GCC + Unity.

| Test                              | Beschreibung                              |
|-----------------------------------|-------------------------------------------|
| test_telemetry_packed_size        | 12-byte packed size                       |
| test_telemetry_field_offsets       | Byte-Offsets matchen Wire-Format          |
| test_telemetry_build_normal_values | Werte korrekt konvertiert (LE)            |
| test_telemetry_speed_clamping      | Speed clamp 0-255 + Rundung               |
| test_telemetry_gear_neutral_mapping | gear=0 -> -1                             |
| test_telemetry_fuel_calculation    | Fuel-Prozent + fuel_low Flag              |
| test_telemetry_no_reserve_no_low_flag | reserve=0 -> fuel_low immer 0         |
| test_telemetry_voltage_conversion  | Volt -> Zehntelvolt + clamp              |
| test_telemetry_coolant_clamping    | Coolant temp clamp int8                   |
| test_telemetry_ambient_temp        | Ambient temp int16                        |
| test_telemetry_build_null_safety   | NULL-Pointer keinen Crash                 |
| test_telemetry_zero_tank_capacity  | tank_capacity=0 kein div-by-zero          |
| test_telemetry_full_byte_layout    | Komplettes Byte-Array verifiziert         |
| test_telemetry_zeroed_input        | Null-Eingabe -> Null-Packet (gear=-1)    |

Ausfuehrung:

```bash
cd firmware
gcc -Icomponents/carplay_bridge/include -Icomponents/carplay_bridge \
    -Icomponents/utils/include -Itest/mocks -I/tmp/unity/src \
    -DTEST_HOST -DUNITY_INCLUDE_DOUBLE -std=c11 -Wall -Wextra \
    test/test_carplay_bridge.c /tmp/unity/src/unity.c \
    -o test_carplay_bridge -lm -lpthread
./test_carplay_bridge
```

---

## 12. Datei-Uebersicht

```
firmware/components/carplay_bridge/
  CMakeLists.txt          # ESP-IDF component registration
  carplay_uuids.h         # GATT Service/Characteristic UUIDs + Enums
  carplay_internal.h      # Internal types (telemetry, peer, client state)
  include/
    carplay_bridge.h      # Public API
  carplay_bridge.c        # GATT-Server + Lifecycle + Event-Bus
  carplay_client.c        # GATT-Client (Discovery, Notify-RX, Reconnect)
  carplay_telemetry.c     # Telemetry Packet Packer (pure, testable)
  carplay_pairing.c       # NVS Pairing Storage

firmware/test/
  test_carplay_bridge.c   # Unit-Tests (14 Tests, Unity + GCC)
```

### Modul-Abhaengigkeiten

```
carplay_bridge
  REQUIRES: utils (event_bus.h)
  NimBLE headers via ESP-IDF bt component
```

### Integration in bestehende Module

| Modul     | Aenderung                                               |
|-----------|---------------------------------------------------------|
| ble/      | `ble_init()` ruft `carplay_bridge_register_gatt()` auf  |
| ble/      | CMakeLists.txt: REQUIRES += carplay_bridge              |
| main/     | `init_step_ble()` ruft `carplay_bridge_init()` auf      |
| main/     | `create_tasks()` startet `carplay_bridge_task`          |
| main/     | CMakeLists.txt: REQUIRES += carplay_bridge              |
| main/     | board_config.h: TASK_STACK_CARPLAY, TASK_PRIO_CARPLAY   |
| utils/    | event_bus.h: EVT_CARPLAY_CONNECTED, EVT_CARPLAY_DISCONNECTED |
| sdkconfig | CONFIG_BT_NIMBLE_ROLE_CENTRAL=y, MAX_CONNECTIONS=2     |
