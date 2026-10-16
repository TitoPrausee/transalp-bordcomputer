# BLE GATT Interface Reference

> **Phase 4: API-Dokumentation**
> Transalp Bordcomputer — BLE GATT Interface
> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1, ESP-IDF v5.4, NimBLE BLE Stack

---

## Inhaltsverzeichnis

1. [Uebersicht](#1-uebersicht)
2. [GATT-Services](#2-gatt-services)
3. [Datenformate](#3-datenformate)
4. [Notification-Protokoll](#4-notification-protokoll)
5. [Write-Commands](#5-write-commands)
6. [OTA-Protokoll](#6-ota-protokoll)
7. [Beispiel-Code](#7-beispiel-code)
8. [Fehlercodes](#8-fehlercodes)

---

## 1. Uebersicht

### 1.1 BLE-Stack

Der Bordcomputer verwendet den NimBLE BLE-Stack auf einem ESP32-S3-WROOM-1
Mikrocontroller (Dual-Core 240 MHz, 16 MB Flash, 8 MB PSRAM). NimBLE ist ein
klassischer GATT-Server und exposiert 8 Custom-Services mit insgesamt 33
Characteristics sowie 2 Bluetooth-SIG-Standard-Services (Generic Access und
Device Information).

Die Companion-App fungiert als GATT-Client. Die Kommunikation ist
ausschliesslich ueber BLE (2.4 GHz); es gibt keine WLAN- oder
Ethernet-Schnittstelle im Normalbetrieb.

### 1.2 UUID-Schema

Alle Custom-Services und Characteristics verwenden einen projekt-spezifischen
128-bit Base-UUID:

```
6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
```

Die 16-bit-ID `XXXX` identifiziert den Service oder die Characteristic:
- Services: 0x1000-Schritte (0x1000 bis 0x8000)
- Characteristics: Service-ID + 0x01 bis Service-ID + 0x0F

Beispiel: Die Live-Telemetry-Characteristic hat die ID 0x5001 und ergibt die
UUID `6e405001-b5a3-f393-e0a9-e50e24dcca9e`.

Standard-Services verwenden Bluetooth-SIG-UUIDs:
- Generic Access: `00001800-0000-1000-8000-00805f9b34fb`
- Device Information: `0000180a-0000-1000-8000-00805f9b34fb`

### 1.3 Verbindungsaufbau

#### Pairing-Modus

Der Bordcomputer verwendet LE Secure Connections (Bluetooth 4.2+) mit
Numeric Comparison als Pairing-Methode. Beide Geraete zeigen eine
6-stellige Zahl an; der Nutzer bestaetigt die Uebereinstimmung.

Security-Manager-Konfiguration:

| Parameter | Wert | Beschreibung |
|---|---|---|
| IO-Capability | Display Yes/No | Numeric Comparison |
| MITM-Schutz | Aktiviert | Man-in-the-Middle-Schutz |
| LE Secure Connections | Aktiviert | ECDH P-256 |
| Bonding | Aktiviert | Keys persistent in NVS |
| Max Bonds | 4 | Hauptgeraet, Zweitgeraet, Werkstatt, Reserve |

Bonding-Keys werden im NVS (Non-Volatile Storage) des ESP32-S3 gespeichert
und ueberleben Stromverlust. Nach dem ersten Pairing ist kein erneuter
Pairing-Prozess noetig.

#### Advertising

Der Bordcomputer advertiset den Geraetenamen `Transalp-BC` und die
16-bit-kodierte Navigation-Service-UUID (0x1000) in den Advertising-Daten.
Die Scan-Response enthaelt den vollstaendigen Geraetenamen. Advertising ist
Connectable, Undirected, General Discoverable.

Nach einem Disconnect startet der Bordcomputer automatisch neu Advertising
(Auto-Reconnect).

#### Service-Discovery

Nach dem physischen Verbindungsaufbau fuehrt der Client GATT-Discovery durch:
alle Services, Characteristics und Descriptors werden enumeriert. Bei
bekanntem GATT-Layout kann dieser Schritt beschleunigt werden, indem nur
die bekannten 128-bit-UUIDs abgefragt werden und ein Service-Changed-Indication
geprueft wird.

### 1.4 MTU-Verhandlung und Connection-Parameter

#### MTU

Der Bordcomputer fordert nach jedem Connect eine MTU von 247 Bytes an
(`ble_att_set_preferred_mtu(247)` gefolgt von `ble_gattc_exchange_mtu`).
Die effektiv ausgehandelte MTU bestimmt die maximale Payload-Groesse pro
GATT-Operation: MTU - 3 Bytes ATT-Header = 244 Bytes Payload.

Bei Default-MTU (23 Bytes) ist die Payload auf 20 Bytes begrenzt. Fuer
alle Chunked-Transfers und Binary-Transfers wird MTU 247 vorausgesetzt.

#### Connection-Parameter

| Parameter | Wert | Beschreibung |
|---|---|---|
| Connection Interval | 15-30 ms | 12-24 Einheiten a 1.25 ms |
| Slave Latency | 0 | Keine Latenz-Schoenung |
| Supervision Timeout | 4 s | 400 Einheiten a 10 ms |
| Max Connections | 1 | Nur ein Geraet gleichzeitig |

Die Parameter werden vom Bordcomputer nach Connect gesetzt
(`ble_gap_update_params`). Der Client kann eigene Parameter fordern, aber
der Bordcomputer schlaegt diese Werte vor.

Connection-Parameter fuer optimierten Datendurchsatz:
- 15 ms Interval: maximale Reaktionszeit fuer Telemetrie (5 Hz) und
  Navigation-Updates (1-2 s)
- 0 Latency: jedes Connection-Event wird genutzt, da Telemetrie und
  Nav-Daten zeitkritisch sind
- 4 s Timeout: schnelle Erkennung von Verbindungsabbruch, aber kurz genug
  um kurzzeitige Stoerungen nicht als Abbruch zu werten

### 1.5 Authentifizierung: Admin-PIN

Nach erfolgreichem Pairing sind alle Read-Characteristics ohne weitere
Authentifizierung lesbar. Write-Characteristics fuer kritische Operationen
erfordern zusaetzlich eine Admin-PIN-Authentifizierung:

| Geschuetzte Operation | Characteristic |
|---|---|
| Konfiguration schreiben | CONFIG_WRITE (0x6002) |
| Konfiguration importieren | CONFIG_IMPORT (0x6004) |
| Bonding-Keys loeschen | BOND_MGMT (0x6005) |
| Self-Test ausloesen | SELF_TEST (0x7003) |
| OTA-Update steuern | OTA_CONTROL (0x8001) |

Die Admin-PIN wird ueber eine eigene Write-Characteristic (0x6006, geplant)
uebertragen, nachdem die Verbindung verschluesselt ist. Die PIN ist 4-stellig
(Standard: `0000`, aenderbar ueber Config-Service mit alter PIN). Eine
Admin-Session ist 5 Minuten gueltig; danach ist eine Re-Authentifizierung
erforderlich. Nach 3 fehlgeschlagenen Versuchen wird ein 60-Sekunden-Lockout
aktiviert.

PIN-Vergleich erfolgt constant-time zum Schutz gegen Timing-Angriffe.

### 1.6 Verbindungsablauf (ASCII-Diagramm)

```
  Client (App)                         Server (Bordcomputer)
  |                                    |
  |  BLE Scan                          |
  |  Filter: Service-UUID 0x1000       |
  |  <------- Advertising -------------|  Connectable, Undirected
  |                                    |
  |  -- CONNECT_REQ -----------------> |
  |                                    |
  |  <--- LL_CONNECT_RSP ------------- |
  |        (Connection established)    |
  |                                    |
  |  -- MTU Exchange (247) ----------> |
  |  <--- MTU Response (247) --------- |
  |                                    |
  |  -- Conn Param Update Req -------> |
  |  <-- Conn Param Update RSP ------- |
  |        (15-30ms, 0 latency, 4s)    |
  |                                    |
  |  -- Pairing Request --------------> |  (only if not bonded)
  |  <--- Pairing Response ------------ |
  |  <-- Numeric Comparison (6 digit) |
  |  -- Pairing Confirm --------------> |
  |                                    |
  |  -- Encryption Request -----------> |
  |  <--- Encryption Enabled ---------- |
  |                                    |
  |  -- GATT Discovery (Services) ---> |
  |  <--- Service/Char Handles -------- |
  |                                    |
  |  -- CCCD Write (Subscribe) ------> |  (per Notify-Characteristic)
  |  <--- CCCD Write Response -------- |
  |                                    |
  |  <=== Telemetry Notifications =====|  (5 Hz, 0x5001)
  |  <=== Nav Maneuver Notifications =|  (1-2s, 0x1001)
  |                                    |
  |  == Config Write (Admin-PIN) =====> |  (0x6002, with 0x6006 auth)
  |  <--- Config Notify (Result) ----- |
  |                                    |
  |  == Disconnect (optional) =======> |
  |  <--- Advertising (auto) --------- |  (restarts advertising)
  |                                    |
```

---

## 2. GATT-Services

### 2.1 Standard-Services

#### 2.1.1 Generic Access Service

- **UUID:** `00001800-0000-1000-8000-00805f9b34fb`
- **Typ:** Primary, Bluetooth SIG Standard

| Characteristic | UUID | Properties | Format |
|---|---|---|---|
| Device Name | 0x2A00 | Read | UTF-8 string: "Transalp-BC" |
| Appearance | 0x2A01 | Read | uint16: 0x0000 (Generic) |

#### 2.1.2 Device Information Service

- **UUID:** `0000180a-0000-1000-8000-00805f9b34fb`
- **Typ:** Primary, Bluetooth SIG Standard

| Characteristic | UUID | Properties | Format |
|---|---|---|---|
| Manufacturer Name | 0x2A29 | Read | UTF-8 string |
| Firmware Revision | 0x2A26 | Read | UTF-8 string: "x.y.z" |
| Hardware Revision | 0x2A27 | Read | UTF-8 string |
| Software Revision | 0x2A28 | Read | UTF-8 string: "x.y.z" |
| System ID | 0x2A23 | Read | 8 bytes (little-endian) |

Beispielwerte:
- Manufacturer Name: `DIY Bordcomputer`
- Firmware Revision: `1.0.0`
- Hardware Revision: `rev-c`

---

### 2.2 Navigation Service (0x1000)

- **UUID:** `6e401000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Uebertragung von Turn-by-Turn Navigationsdaten von der
  Smartphone-App an den Bordcomputer. Die App nutzt die native
  Navigations-Engine des Smartphones und extrahiert Maneuver-, Route- und
  Strasseninformationen, die der Bordcomputer auf dem Display anzeigt.

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e401001-... | Turn-by-Turn Maneuver | Read, Notify | 6-70 Bytes |
| 2 | 6e401002-... | Route Info | Read, Notify | 16 Bytes |
| 3 | 6e401003-... | Current Road | Read, Notify | 2-66 Bytes |
| 4 | 6e401004-... | Navigation Status | Read, Notify | 1 Byte |
| 5 | 6e401005-... | Nav Command | Write, WriteNR | 1 Byte |

#### 2.2.1 Turn-by-Turn Maneuver (0x1001)

Enthaelt das naechste Maneuver: Maneuver-Typ, Distanz zum Maneuver und
Strassenname der naechsten Strasse.

```
Byte 0:     Maneuver-Type (uint8)
            0=Straight, 1=SlightLeft, 2=Left, 3=SharpLeft,
            4=SlightRight, 5=Right, 6=SharpRight,
            7=UTurn, 8=Arrive, 9=Fork
Byte 1-4:   Distance (uint32, Meter, Little-Endian)
Byte 5:     Street-Name-Length (uint8, 0-64)
Byte 6+:    Street-Name (UTF-8, Street-Name-Length Bytes)
```

Maximalgroesse: 6 + 64 = 70 Bytes.

Beispiel: Linksabbieung in 250 m in "Hauptstrasse":
```
02  FA 00 00 00  0C  48 61 75 70 74 73 74 72 61 73 73 65
^^  ^^^^^^^^^^^^ ^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
|   distance=250  len=12  "Hauptstrasse"
Left
```

Update-Rate: alle 1-2 Sekunden oder sofort bei Maneuver-Aenderung.

#### 2.2.2 Route Info (0x1002)

```
Byte 0-3:   Total-Distance (uint32, Meter, Little-Endian)
Byte 4-7:   Remaining-Distance (uint32, Meter, Little-Endian)
Byte 8-11:  ETA (uint32, Unix-Timestamp, Little-Endian)
Byte 12-15: Remaining-Time (uint32, Sekunden, Little-Endian)
```

16 Bytes. Update-Rate: alle 10 Sekunden oder bei signifikanter Aenderung
(> 500 m).

Beispiel: Gesamtstrecke 125.000 m, Rest 48.200 m, ETA 1731424800,
Restzeit 3.600 s:
```
00 E8 01 00  00 BC BE 00  00 60 95 67  00 0E 00 00
```

#### 2.2.3 Current Road (0x1003)

```
Byte 0:     Speed-Limit (uint8, km/h, 0=unbekannt)
Byte 1:     Road-Name-Length (uint8, 0-64)
Byte 2+:    Road-Name (UTF-8, Road-Name-Length Bytes)
```

Maximalgroesse: 2 + 64 = 66 Bytes. Update: bei Strassenwechsel (event-basiert).

#### 2.2.4 Navigation Status (0x1004)

```
Byte 0:     Status (uint8)
            0=Inactive, 1=Active, 2=Rerouting,
            3=Arriving, 4=Offline
```

1 Byte. Update: bei Statuswechsel (event-basiert).

#### 2.2.5 Nav Command (0x1005, Write)

Bordcomputer -> App. Befehle vom Bordcomputer an die App.

```
Byte 0:     Command (uint8)
            0x01=CancelRoute, 0x02=RepeatManeuver, 0x03=RequestAlternative
```

1 Byte. Write Without Response fuer minimale Latenz.

---

### 2.3 Media Service (0x2000)

- **UUID:** `6e402000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Uebertragung von Now-Playing-Informationen (Artist, Titel,
  Position) vom Smartphone an den Bordcomputer und Steuerung der
  Medienwiedergabe vom Bordcomputer aus (Lenkertaster).

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e402001-... | Now Playing | Read, Notify | 9-139 Bytes |
| 2 | 6e402002-... | Media Control | Write, WriteNR | 1 Byte |
| 3 | 6e402003-... | Volume | Read, Notify, Write | 1 Byte |

#### 2.3.1 Now Playing (0x2001)

```
Byte 0:     Playback-State (uint8: 0=stopped, 1=playing, 2=paused)
Byte 1-4:   Duration (uint32, Sekunden, Little-Endian)
Byte 5-8:   Position (uint32, Sekunden, Little-Endian)
Byte 9:     Artist-Length (uint8, 0-64)
Byte 10+:   Artist (UTF-8)
Byte N:     Title-Length (uint8, 0-64)
Byte N+1+:  Title (UTF-8)
```

Maximalgroesse: 9 + 1 + 64 + 1 + 64 = 139 Bytes. Update: alle 5 Sekunden
oder bei Titelwechsel.

#### 2.3.2 Media Control (0x2002, Write)

Bordcomputer -> App.

```
Byte 0:     Command (uint8)
            0x01=PlayPause, 0x02=Next, 0x03=Prev,
            0x04=SeekFwd10s, 0x05=SeekBack10s
```

#### 2.3.3 Volume (0x2003)

Bidirektional: App notifiziert bei Aenderung, Bordcomputer kann schreiben.

```
Byte 0:     Volume (uint8, 0-100, Prozent)
```

---

### 2.4 Phone Service (0x3000)

- **UUID:** `6e403000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Anruf-Steuerung und Notification-Relay. Eingehende Anrufe und
  Smartphone-Notifications werden an den Bordcomputer weitergeleitet und
  auf dem Display angezeigt. Der Bordcomputer kann Anrufe annehmen oder
  ablehnen ueber Lenkertaster.

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e403001-... | Incoming Call | Notify | 2-66 Bytes |
| 2 | 6e403002-... | Call Status | Read, Notify | 1 Byte |
| 3 | 6e403003-... | Call Control | Write | 1 Byte |
| 4 | 6e403004-... | Notification | Notify | 3-201 Bytes |

#### 2.4.1 Incoming Call (0x3001)

```
Byte 0:     Call-Type (uint8: 0=voice, 1=video)
Byte 1:     Caller-ID-Length (uint8, 0-64)
Byte 2+:    Caller-ID (UTF-8, Telefonnummer oder Kontaktname)
```

Update: bei eingehendem Anruf (event-basiert).

#### 2.4.2 Call Status (0x3002)

```
Byte 0:     Status (uint8: 0=idle, 1=ringing, 2=active, 3=ended)
```

Update: bei jeder Statusaenderung.

#### 2.4.3 Call Control (0x3003, Write)

Bordcomputer -> App.

```
Byte 0:     Command (uint8: 0x01=Accept, 0x02=Reject, 0x03=HangUp)
```

#### 2.4.4 Notification (0x3004)

App -> Bordcomputer. Gefilterte Smartphone-Notifications.

```
Byte 0:     Priority (uint8: 0=critical, 1=high, 2=medium)
Byte 1:     App-Name-Length (uint8, 0-32)
Byte 2+:    App-Name (UTF-8)
Byte N:     Title-Length (uint8, 0-64)
Byte N+1+:  Title (UTF-8)
Byte M:     Body-Length (uint8, 0-100, auf 100 Zeichen gekuerzt)
Byte M+1+:  Body (UTF-8)
```

Maximalgroesse: 3 + 32 + 1 + 64 + 1 + 100 = 201 Bytes.

---

### 2.5 Weather Service (0x4000)

- **UUID:** `6e404000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Uebertragung von Wetterdaten (aktuelles Wetter, 6-Stunden-
  Vorhersage, Warnungen) vom Smartphone an den Bordcomputer. Die App ruft
  Wetterdaten per HTTP-API ab und sendet sie per BLE an den Bordcomputer.

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e404001-... | Current Weather | Read, Notify | 8 Bytes |
| 2 | 6e404002-... | Forecast | Read, Notify | 24 Bytes |
| 3 | 6e404003-... | Weather Alert | Notify | 4 Bytes |

#### 2.5.1 Current Weather (0x4001)

```
Byte 0-1:   Temperature (int16, Zehntelgrad Celsius, Little-Endian)
Byte 2:     Humidity (uint8, Prozent)
Byte 3:     Condition (uint8: 0=clear, 1=cloudy, 2=overcast,
            3=rain, 4=heavy_rain, 5=thunderstorm, 6=snow, 7=fog)
Byte 4:     Wind-Speed (uint8, km/h)
Byte 5-6:   Feels-Like (int16, Zehntelgrad Celsius, Little-Endian)
Byte 7:     Rain-Probability (uint8, Prozent, naechste 1h)
```

8 Bytes. Update: alle 15 Minuten.

Beispiel: 21.5 C, 65% Luftfeuchte, leicht bewoelkt, 12 km/h Wind,
gefuehlt 20.0 C, 30% Regenwahrscheinlichkeit:
```
 D7 00  41  01  0C  C8 00  1E
 ^^^^^  ^^  ^^  ^^  ^^^^^  ^^
 215    65  1   12  200    30
 (/10 = 21.5 C)
```

#### 2.5.2 Forecast (0x4002)

6-Stunden-Vorhersage als kompaktes Array:

```
6 Eintraege a 4 Bytes (insgesamt 24 Bytes):
  Byte 0: Hour-Offset (uint8: 0=now, 1=+1h, ..., 5=+5h)
  Byte 1: Condition (uint8, siehe Current Weather)
  Byte 2: Temp-Min (int8, Celsius)
  Byte 3: Temp-Max (int8, Celsius)
```

24 Bytes. Update: alle 30 Minuten oder bei signifikanter Aenderung.

#### 2.5.3 Weather Alert (0x4003)

```
Byte 0:     Alert-Type (uint8: 0=rain_approaching, 1=storm,
            2=frost, 3=heat, 4=wind)
Byte 1:     Severity (uint8: 0=info, 1=warning, 2=severe)
Byte 2-3:   Time-Until (uint16, Minuten, Little-Endian)
```

4 Bytes. Update: bei neuer Warnung (event-basiert).

---

### 2.6 Telemetry Service (0x5000)

- **UUID:** `6e405000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Uebertragung von Live-Fahrzeugdaten (Speed, RPM, Temperatur,
  Kraftstoff, Spannung, Gang) und GPS-Daten (Position, Hoehe, Kurs,
  Satelliten) vom Bordcomputer an die App. Ausserdem Track-Export
  (GPX-Datei-Transfer).

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e405001-... | Live Telemetry | Notify | 22 Bytes |
| 2 | 6e405002-... | Telemetry Snapshot | Read | ~60 Bytes |
| 3 | 6e405003-... | Track Export | Notify | Variabel |
| 4 | 6e405004-... | Track Export Control | Write | 1-5 Bytes |

#### 2.6.1 Live Telemetry (0x5001)

Kompaktes 22-Byte-Datenpaket mit allen Live-Werten. Wird mit 5 Hz
(alle 200 ms) vom Bordcomputer notifiziert.

```
Byte 0:     Speed (uint8, km/h)
Byte 1-2:   RPM (uint16, U/min, Little-Endian)
Byte 3:     Gear (int8, -1=neutral, 0=N/A, 1-6=Gang)
Byte 4:     Coolant-Temp (int8, Celsius)
Byte 5:     Ambient-Temp (int8, Celsius)
Byte 6:     Fuel (uint8, Prozent)
Byte 7:     Voltage (uint8, Zehntelvolt, z.B. 124 = 12.4 V)
Byte 8-11:  Latitude (int32, E-7, Little-Endian)
Byte 12-15: Longitude (int32, E-7, Little-Endian)
Byte 16-17: Altitude (int16, Meter, Little-Endian)
Byte 18-19: Heading (uint16, Zehntelgrad, Little-Endian)
Byte 20:    Satellites (uint8)
Byte 21:    HDOP (uint8, Zehntel, z.B. 14 = 1.4)
```

22 Bytes gesamt. Notify-Rate: 5 Hz.

Beispiel: 95 km/h, 4200 rpm, 3. Gang, 82 C Kuehlmittel, 18 C Umgebung,
75% Kraftstoff, 12.4 V, lat=50.9275000, lon=11.5862000, alt=320 m,
heading=145.0, 9 Satelliten, HDOP=1.4:
```
5F  68 10  03  52  12  4B  7C  00 00 FE 02 00 00
95  4200   3   82  18  75  124 lat=509275000
  48 58 00 00  40 01  6A 05  09  0E
  lon=115862000 hdg=320 1450  9   14
```

Hinweis: Latitude/Longitude sind in Einheiten von 1e-7 Grad gespeichert.
509275000 / 1e7 = 50.9275000 Grad.

#### 2.6.2 Telemetry Snapshot (0x5002)

Vollstaendiger Snapshot aller konfigurierten Sensoren. Read-on-Demand.
Die App liest diesen bei (Re-)Connect, um sofort einen vollstaendigen
Zustand zu haben. Umfasst zusaetzlich zu den Live-Werten: Tripmeter A/B,
Odometer, GPS-Fix-Qualitaet, Sensor-Health und System-Status.

System-Status-Felder (Teil des Snapshot):

```
Byte 0-3:   Uptime (uint32, Sekunden)
Byte 4-7:   Heap-Free (uint32, Bytes)
Byte 8-11:  Heap-Total (uint32, Bytes)
Byte 12:    Battery-Percent (uint8, Prozent)
Byte 13:    CPU-Temp (int8, Celsius)
Byte 14:    Task-Count (uint8)
Byte 15:    BLE-Connections (uint8)
Byte 16:    WiFi-RSSI (int8, dBm)
```

System-Status: 17 Bytes. Gesamter Snapshot ca. 60 Bytes.

#### 2.6.3 Track Export (0x5003)

GPX-Datei-Transfer ueber das Binary-Transfer-Protokoll (siehe Abschnitt 6).
Die App schreibt einen Befehl an 0x5004, der Bordcomputer beginnt, die
GPX-Datei chunked ueber Notify 0x5003 zu senden.

#### 2.6.4 Track Export Control (0x5004, Write)

App -> Bordcomputer.

```
Byte 0:     Command (uint8)
            0x01=StartExport, 0x02=CancelExport, 0x03=GetStatus
Byte 1-4:   Date (uint32, Unix-Timestamp) [nur bei 0x01]
```

---

### 2.7 Configuration Service (0x6000)

- **UUID:** `6e406000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Lesen und Schreiben der Geraetekonfiguration (Einstellungen,
  Kalibrierung, Schwellwerte). Konfiguration wird als JSON repraesentiert
  und ueber das Chunked-Transfer-Protokoll uebertragen. Ausserdem
  Backup/Restore als Binary-Blob und Bond-Management.

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e406001-... | Config Read | Read, Notify | Variabel (JSON) |
| 2 | 6e406002-... | Config Write | Write | Variabel (JSON) |
| 3 | 6e406003-... | Config Export | Notify | Variabel (Binary) |
| 4 | 6e406004-... | Config Import | Write | Variabel (Binary) |
| 5 | 6e406005-... | Bond Management | Write | 1 Byte |

#### 2.7.1 Config Read / Write (0x6001 / 0x6002)

Die Konfiguration wird als JSON repraesentiert (typischerweise 1-3 KB).
Wird ueber das Chunked-Transfer-Protokoll (Abschnitt 3.2) uebertragen.

Config-Write erfordert Admin-PIN-Authentifizierung.

Beispiel-JSON:
```json
{
  "device_name": "Transalp-BC",
  "wheel_circumference_mm": 1900,
  "rpm_pulses_per_rev": 2,
  "temp_offset_c": 0,
  "fuel_cal": 1.0,
  "rpm_redline": 8000,
  "battery_low_v": 11.5,
  "battery_shutdown_v": 10.5,
  "coolant_temp_crit": 115,
  "display_brightness": 80,
  "auto_dim": true,
  "log_level": 2,
  "final_drive_ratio": 2.933,
  "primary_drive_ratio": 2.733
}
```

#### 2.7.2 Config Export / Import (0x6003 / 0x6004)

Vollstaendige Konfiguration als Binary-Blob exportieren/importieren.
Verwendet das Binary-Transfer-Protokoll (Abschnitt 3.3). Umfasst die
komplette NVS-Config-Partition. Nuetzlich fuer Backup/Restore oder
Uebertragung auf einen anderen Bordcomputer.

Config-Import erfordert Admin-PIN-Authentifizierung.

#### 2.7.3 Bond Management (0x6005, Write)

Loescht alle gespeicherten Bonding-Keys auf dem Bordcomputer.

```
Byte 0:     Command (uint8: 0x00=DeleteAllBonds)
```

Nach dem Loeschen startet der Bordcomputer neu. Erfordert Admin-PIN-
Authentifizierung. Nur ein gebondetes Geraet kann dies ausloesen.

---

### 2.8 Diagnose Service (0x7000)

- **UUID:** `6e407000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Diagnose-Funktionen: Diagnostic Trouble Codes (DTCs) lesen,
  Sensor-Status abfragen, Self-Test ausloesen und System-Logs exportieren.

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e407001-... | DTC List | Read, Notify, Indicate | 6 * N Bytes |
| 2 | 6e407002-... | Sensor Status | Read, Notify | 5 Bytes |
| 3 | 6e407003-... | Self-Test | Write, Notify | 1-6 Bytes |
| 4 | 6e407004-... | Log Export | Notify | Variabel |
| 5 | 6e407005-... | Log Export Control | Write | 1-5 Bytes |

#### 2.8.1 DTC List (0x7001)

Diagnostic Trouble Codes als Array. Jeder Eintrag ist 6 Bytes:

```
Byte 0-1:   Code (uint16, Little-Endian)
Byte 2:     Severity (uint8: 0=info, 1=warning, 2=critical)
Byte 3-6:   Timestamp (uint32, Unix-Timestamp, Little-Endian)
```

6 Bytes pro Eintrag. Der Bordcomputer sendet ein Notify bei neuem DTC.
Das Indicate-Flag ermoeglicht zusaetzlich zuverlaessige Uebertragung
(bestaetigte Notifications).

DTC-Codes (Beispiele):

| Code | Severity | Beschreibung |
|---|---|---|
| 0x0001 | Warning | Coolant temperature high |
| 0x0002 | Critical | Coolant temperature critical |
| 0x0003 | Warning | Battery voltage low |
| 0x0004 | Critical | Battery voltage critical |
| 0x0005 | Info | GPS fix lost |
| 0x0006 | Warning | RPM sensor no signal |
| 0x0007 | Warning | Temperature sensor disconnected |
| 0x0008 | Warning | Fuel sensor error |
| 0x0009 | Warning | SD card not present |
| 0x000A | Warning | SD card full |
| 0x000B | Critical | SD card error |
| 0x000C | Info | BLE disconnected |
| 0x000D | Warning | Heap memory low |

#### 2.8.2 Sensor Status (0x7002)

Status aller Sensoren als 5-Byte-Paket:

```
Byte 0: GPS (uint8: 0=ok, 1=no_fix, 2=error)
Byte 1: RPM-Sensor (uint8: 0=ok, 1=no_signal, 2=error)
Byte 2: Temp-Sensor (uint8: 0=ok, 1=disconnected, 2=error)
Byte 3: Fuel-Sensor (uint8: 0=ok, 1=error)
Byte 4: SD-Card (uint8: 0=ok, 1=not_present, 2=full, 3=error)
```

5 Bytes. Update: bei Statusaenderung (event-basiert).

Beispiel: GPS ok, RPM no_signal, Temp ok, Fuel ok, SD ok:
```
00 01 00 00 00
```

#### 2.8.3 Self-Test (0x7003)

Write: App -> Bordcomputer (erfordert Admin-PIN).

```
Byte 0:     Mode (uint8: 0=Quick, 1=Full)
```

Notify: Bordcomputer -> App (Ergebnis).

```
Byte 0:     Result (uint8: 0=pass, 1=warn, 2=fail)
Byte 1+:    Detail-Bytes (ein Byte pro Sensor, gleiche Reihenfolge
            wie Sensor Status)
```

#### 2.8.4 Log Export (0x7004) und Log Export Control (0x7005)

System-Logs als Binary-Transfer ueber das Protokoll aus Abschnitt 3.3.
Die App triggert ueber 0x7005, der Bordcomputer sendet die Log-Datei
chunked ueber Notify 0x7004.

Log Export Control (0x7005, Write):
```
Byte 0:     Command (uint8: 0x01=StartExport, 0x02=Cancel, 0x03=GetStatus)
Byte 1-4:   Date (uint32, Unix-Timestamp) [nur bei 0x01]
```

---

### 2.9 OTA Update Service (0x8000)

- **UUID:** `6e408000-b5a3-f393-e0a9-e50e24dcca9e`
- **Typ:** Primary, Custom
- **Zweck:** Firmware-Update ueber BLE. Das Firmware-Image wird in Chunks
  uebertragen, per CRC16 pro Chunk und SHA-256 ueber das gesamte Image
  verifiziert. Dual-Partition-Layout mit automatischem Rollback.

| # | UUID | Name | Properties | Groesse |
|---|---|---|---|---|
| 1 | 6e408001-... | OTA Control | Write, Notify | 1-8 Bytes |
| 2 | 6e408002-... | OTA Data | WriteNR | Variabel |
| 3 | 6e408003-... | OTA Verify | Notify | 37-41 Bytes |

#### 2.9.1 OTA Control (0x8001)

Write: App -> Bordcomputer (erfordert Admin-PIN).

```
Byte 0:     Command (uint8)
            0x00=Start, 0x01=Cancel, 0x02=Verify, 0x03=Commit
```

Notify: Bordcomputer -> App (Status).

```
Byte 0:     State (uint8)
            0x00=Idle, 0x01=Receiving, 0x02=Verifying,
            0x03=ReadyToCommit, 0x04=Committed,
            0x80=Error
Byte 1:     Progress (uint8, 0-100, Prozent)
Byte 2+:    Error-Code (uint8, nur wenn State=0x80) [optional]
```

#### 2.9.2 OTA Data (0x8002)

Firmware-Image als Binary-Transfer. Write Without Response fuer
maximalen Throughput (kein ACK pro Paket). Siehe Abschnitt 6 fuer das
Fragmentierungs-Protokoll.

#### 2.9.3 OTA Verify (0x8003)

Nach Abschluss des Transfers sendet der Bordcomputer das
Verifikationsergebnis:

```
Byte 0:     Status (uint8: 0x00=ok, 0x01=hash_mismatch,
            0x02=too_large, 0x03=invalid_magic,
            0x04=signature_failed)
Byte 1-32:  SHA-256-Hash (32 Bytes, ueber empfangenes Image)
Byte 33-36: Image-Size (uint32, Bytes, Little-Endian)
Byte 37-40: Firmware-Version (4 Bytes ASCII: major.minor.patch + null)
```

Die App vergleicht den Hash mit dem erwarteten und sendet Commit (0x03
an 0x8001), wenn sie uebereinstimmen.

---

## 3. Datenformate

### 3.1 Endianness und Datentypen

Alle numerischen Werte werden in Little-Endian-Bytereihenfolge uebertragen
(niederwertigstes Byte zuerst). Ausnahme: UUIDs werden in der
Bluetooth-Standard-Notation als Big-Endian-Strings dargestellt.

| Datentyp | Groesse | Bereich | Bemerkung |
|---|---|---|---|
| uint8 | 1 Byte | 0..255 | Vorzeichenlos |
| int8 | 1 Byte | -128..127 | Zweierkomplement |
| uint16 | 2 Bytes | 0..65535 | Little-Endian |
| int16 | 2 Bytes | -32768..32767 | Little-Endian |
| uint32 | 4 Bytes | 0..4294967295 | Little-Endian |
| int32 | 4 Bytes | -2^31..2^31-1 | Little-Endian |
| float32 | 4 Bytes | IEEE 754 | Little-Endian |
| string | variabel | UTF-8 | Laengenpraefix (uint8) |

Strings werden immer mit einem uint8-Laengenpraefix gesendet, gefolgt von
den UTF-8-kodierten Bytes. Die maximale Stringlaenge ist 64 Bytes
(fuer Strassennamen, Artist, Titel) oder 100 Bytes (fuer Notification-Body).

### 3.2 Chunked-Transfer-Protokoll (20-500 Bytes)

Fuer Daten zwischen 20 und 500 Bytes wird ein einfaches
Chunked-Transfer-Protokoll verwendet. Dies betrifft die Config-Read/Write
Characteristics und kurze Telemetrie-Snapshots.

```
Byte 0:     Transfer-ID (uint8, fortlaufend)
Byte 1:     Flags (bit 0: more-flag, 1=more chunks follow, 0=last chunk)
Byte 2:     Sequence-Number (uint8, 0-basiert, wrap bei 255)
Byte 3-4:   Chunk-Length (uint16, Little-Endian, 0-244)
Byte 5+:    Payload (Chunk-Length Bytes)
```

Fehlerbehandlung: Bei fehlender Sequence-Number (Luecke) sendet der
Empfaenger eine Negative-Acknowledgement-Nachricht an die jeweilige
Control-Characteristic. Nach 3 fehlgeschlagenen Versuchen wird der
Transfer abgebrochen. Timeout: 5 Sekunden ohne Chunk = Abbruch.

### 3.3 Binary-Transfer-Protokoll (> 500 Bytes)

Fuer Daten ueber 500 Bytes (Track-Export, Log-Export, Firmware-Image,
Config-Binary) wird ein robustes Protokoll mit CRC, Retry und
Progress-Tracking verwendet.

#### Protokoll-Header (7 Bytes, im ersten Chunk)

```
Byte 0-1:   Magic (0xBC, 0x01 — Protokoll-Version 1)
Byte 2-5:   Total-Length (uint32, Little-Endian, Gesamtgroesse)
Byte 6:     Transfer-Type (0x01=Track, 0x02=Log, 0x03=Firmware, 0x04=Config)
```

#### Chunk-Format (jeder Chunk)

```
Byte 0-1:   Transfer-ID (uint16, identifiziert den Transfer)
Byte 2-3:   Sequence-Number (uint16, 0-basiert, wrap bei 65535)
Byte 4:     Flags (bit 0: more, bit 1: header_follows, bit 2: retransmit)
Byte 5-6:   Chunk-Length (uint16, 0-244)
Byte 7-8:   CRC-16 (CCITT, Polynom 0x1021, ueber Byte 0-6 + Payload)
Byte 9+:    Payload (Chunk-Length Bytes)
```

Maximalgroesse pro Chunk: 9 Bytes Header + 244 Bytes Payload = 253 Bytes.
Bei MTU 247: 9 + 238 = 247 Bytes pro Notification.

#### ACK-Format (Client -> Server)

```
Byte 0:     0xFF (ACK-Marker)
Byte 1-2:   Transfer-ID
Byte 3-4:   Last-Sequence (hoechste zusammenhaengend empfangene Seq-Num)
```

#### Completion-Notify

```
Byte 0:     0xFE (Completion-Marker)
Byte 1-2:   Transfer-ID
Byte 3-6:   CRC-32 (ueber alle reassemblierten Daten)
Byte 7-10:  Total-Bytes (uint32)
```

#### Ablauf

1. **Initiation:** Client schreibt Start-Befehl an die Control-Characteristic.
2. **Uebertragung:** Server sendet Chunks ohne ACK zu warten (Sliding-Window,
   Window-Size 8).
3. **Retransmission:** Bei Luecken sendet der Client ein ACK mit der letzten
   zusammenhaengenden Sequence-Number. Der Server sendet alle Chunks ab
   Last-Sequence+1 erneut (Flag bit 2 = retransmit).
4. **Completion:** Der Server sendet ein Final-Notify mit CRC-32.
5. **Abbruch:** Beide Seiten koennen durch Schreiben von 0x00 an die
   Control-Characteristic abbrechen.

---

## 4. Notification-Protokoll

### 4.1 Notify-faehige Characteristics

| Service | Characteristic | UUID (16-bit) | Notify | Indicate |
|---|---|---|---|---|
| Navigation | Maneuver | 0x1001 | Ja | Nein |
| Navigation | Route Info | 0x1002 | Ja | Nein |
| Navigation | Current Road | 0x1003 | Ja | Nein |
| Navigation | Nav Status | 0x1004 | Ja | Nein |
| Media | Now Playing | 0x2001 | Ja | Nein |
| Media | Volume | 0x2003 | Ja | Nein |
| Phone | Incoming Call | 0x3001 | Ja | Nein |
| Phone | Call Status | 0x3002 | Ja | Nein |
| Phone | Notification | 0x3004 | Ja | Nein |
| Weather | Current | 0x4001 | Ja | Nein |
| Weather | Forecast | 0x4002 | Ja | Nein |
| Weather | Alert | 0x4003 | Ja | Nein |
| Telemetry | Live | 0x5001 | Ja | Nein |
| Telemetry | Track Export | 0x5003 | Ja | Nein |
| Config | Config Read | 0x6001 | Ja | Nein |
| Config | Config Export | 0x6003 | Ja | Nein |
| Diagnose | DTC List | 0x7001 | Ja | Ja |
| Diagnose | Sensor Status | 0x7002 | Ja | Nein |
| Diagnose | Self-Test | 0x7003 | Ja | Nein |
| Diagnose | Log Export | 0x7004 | Ja | Nein |
| OTA | Control | 0x8001 | Ja | Nein |
| OTA | Verify | 0x8003 | Ja | Nein |

### 4.2 Update-Raten

| Characteristic | normale Rate | DEGRADING-Modus |
|---|---|---|
| Telemetry Live (0x5001) | 5 Hz (200 ms) | 1 Hz |
| Nav Maneuver (0x1001) | 1-2 s | 2-3 s |
| Nav Route (0x1002) | 10 s | gestoppt |
| Nav Road (0x1003) | event-basiert | event-basiert |
| Nav Status (0x1004) | event-basiert | event-basiert |
| Now Playing (0x2001) | 5 s | gestoppt |
| Volume (0x2003) | event-basiert | gestoppt |
| Incoming Call (0x3001) | event-basiert | event-basiert (P0) |
| Call Status (0x3002) | event-basiert | event-basiert (P0) |
| Weather Current (0x4001) | 15 min | gestoppt |
| Weather Forecast (0x4002) | 30 min | gestoppt |
| DTC List (0x7001) | event-basiert | event-basiert |
| Sensor Status (0x7002) | event-basiert | event-basiert |

### 4.3 Prioritaeten im DEGRADING-Modus

Bei schlechter Signalqualitaet (RSSI < -90 dBm oder Packet-Loss > 10%)
werden Notifications nach Prioritaet reduziert:

| Prioritaet | Daten | DEGRADING-Verhalten |
|---|---|---|
| P0 (Critical) | Incoming Call, Call Status | Unveraendert (sofort) |
| P1 (High) | Nav Maneuver | Reduziert auf 2-3 s |
| P2 (Medium) | Live Telemetry | Reduziert auf 1 Hz |
| P3 (Low) | Now Playing, Route, Weather | Gestoppt |
| P4 (Background) | Track/Log Export, OTA | Gestoppt |

### 4.4 CCCD-Subscription

Vor dem Empfang von Notifications muss der Client die jeweilige
Characteristic durch Schreiben des CCCD (Client Characteristic
Configuration Descriptor, Standard-UUID 0x2902) abonnieren:

```
CCCD-Wert:  0x0001 = Notifications aktivieren
            0x0002 = Indications aktivieren
            0x0000 = Deaktivieren
```

Der Bordcomputer trackt CCCD-Subscriptions pro Characteristic und sendet
Notifications nur an abonnierte Clients. Bei Disconnect werden alle
Subscriptions zurueckgesetzt.

### 4.5 Connection-Parameter fuer Notifications

Fuer zuverlaessige Notifications mit 5 Hz Telemetrie sind folgende
Connection-Parameter erforderlich:

- Connection Interval: 15-30 ms (12-24 Einheiten)
- Slave Latency: 0 (jedes Connection-Event nutzen)
- Supervision Timeout: 4 s (400 Einheiten)
- MTU: 247 Bytes

Bei 15 ms Interval und 1 Notification pro Connection-Event: 244 Bytes *
66.7 Events/s = ca. 16 kB/s. Praxis: ca. 8-16 kB/s fuer Notifications.

---

## 5. Write-Commands

### 5.1 Configuration-Writes

Konfiguration wird als JSON ueber das Chunked-Transfer-Protokoll an
CONFIG_WRITE (0x6002) geschrieben. Erfordert Admin-PIN-Authentifizierung.

Beispiel-Ablauf:
1. Admin-PIN an ADMIN_AUTH (0x6006) schreiben
2. JSON-Config in Chunks an 0x6002 schreiben (Chunked-Transfer)
3. Bordcomputer validiert und uebernimmt atomar
4. Resultat-Notify auf 0x6001 (Config Read)

Range-Checks fuer kritische Werte:
- `rpm_redline`: 1000..15000
- `battery_shutdown_v`: 9.0..12.0
- `coolant_temp_crit`: 100..150
- `wheel_circumference_mm`: 1000..3000

### 5.2 OBD-II Characteristics (geplant, ab v0.4.0 evaluiert)

Die folgenden Characteristics sind Teil der OBD-II-Integrations-Evaluierung
(siehe `docs/research/14-obd-ii-integration.md`). Sie erweitern den
bestehenden Telemetry-Service (0x5000) um OBD-spezifische Datenpunkte.

| Characteristic | UUID (16-bit) | Properties | Beschreibung |
|---|---|---|---|
| OBD_LIVE | 0x5005 | notify | 20-byte packed OBD live data (RPM, Speed, Coolant, Oil Temp, MAF) |
| OBD_PID_REQUEST | 0x5006 | write | PID-Anfrage vom Client (SAE J1979 PID-Code) |
| OBD_PID_RESPONSE | 0x5007 | read, notify | PID-Antwort mit variablem Payload |
| OBD_STATUS | 0x5008 | read, notify | Adapter-Status (connected, protocol, voltage) |

Die OBD-Characteristics unterstuetzen zwei Modi:
- Emulator: Bordcomputer emuliert OBD-PIDs aus eigenen Sensordaten
  (Mapping: RPM=0x0C, Speed=0x0D, Coolant=0x05, Oil Temp=0x5C, MAF=0x10)
- Reader: Bordcomputer verbindet sich per BLE-Central zu einem
  ELM327-kompatiblen OBD-Adapter und leitet die Daten weiter

### 5.3 Tachokalibrierung Config-Felder (geplant, ab v0.4.0 evaluiert)

Die Tachokalibrierung fuegt drei neue Config-Felder hinzu
(siehe `docs/research/16-tacho-kalibrierung.md`):

| Feld | Typ | Default | Bereich | Beschreibung |
|---|---|---|---|---|
| `wheel_circumference_mm` | uint16 | 2009 | 1000..3000 | Radumfang (130/80-17 Reifen) |
| `final_drive_ratio` | float | 2.933 | 1.0..10.0 | Uebersetzungsverhaeltnis Sekundaer (15/44 Zaehne) |
| `primary_drive_ratio` | float | 2.733 | 1.0..10.0 | Uebersetzungsverhaeltnis Primaer |

CONFIG_VERSION wird von 3 auf 4 erhoeht. Die Migration erfolgt automatisch
beim ersten Lesen mit Default-Werten fuer die neuen Felder.

### 5.4 Diagnose-Befehle

#### DTC loeschen

DTCs koennen ueber den Config-Service geloescht werden (Konfiguration
mit `dtc_clear`-Flag). Erfordert Admin-PIN.

#### Self-Test ausloesen

Write an SELF_TEST (0x7003):
```
Byte 0:     Mode (uint8: 0=Quick, 1=Full)
```

Erfordert Admin-PIN. Der Bordcomputer fuehrt den Self-Test durch und
sendet das Ergebnis als Notify auf 0x7003.

### 5.3 Kalibrierung

Sensor-Kalibrierung erfolgt ueber den Config-Service:
- `wheel_circumference_mm`: Radumfang fuer Speed/GPS-Korrektur
- `rpm_pulses_per_rev`: Impulse pro Umdrehung
- `temp_offset_c`: Temperatur-Offset
- `fuel_cal`: Kraftstoffsensor-Kalibrierungsfaktor
- `gyro_offset`: Gyroskop-Offset (IMU)
- `final_drive_ratio`: Sekundaer-Uebersetzung (ab v0.4.0, Default 2.933)
- `primary_drive_ratio`: Primaer-Uebersetzung (ab v0.4.0, Default 2.733)

Diese Werte werden als Teil der JSON-Konfiguration geschrieben und
erfordern Admin-PIN-Authentifizierung.

### 5.4 Admin-PIN-Authentifizierung

Geschuetzte Write-Operationen erfordern eine Admin-Session. Die
Authentifizierung erfolgt durch Schreiben der 4-stelligen PIN an die
ADMIN_AUTH-Characteristic (0x6006, geplant).

```
Byte 0-3:   PIN (4 Bytes ASCII, z.B. "0000")
```

Nach erfolgreicher Authentifizierung ist die Admin-Session 5 Minuten
gueltig. Nach 3 fehlgeschlagenen Versuchen wird ein 60-Sekunden-Lockout
aktiviert. PIN-Vergleich erfolgt constant-time.

Folgende Operationen erfordern Admin-PIN:

| Operation | Characteristic | UUID (16-bit) |
|---|---|---|
| Config Write | CONFIG_WRITE | 0x6002 |
| Config Import | CONFIG_IMPORT | 0x6004 |
| Bond Delete | BOND_MGMT | 0x6005 |
| Self-Test | SELF_TEST | 0x7003 |
| OTA Control | OTA_CONTROL | 0x8001 |

---

## 6. OTA-Protokoll

### 6.1 Uebersicht

Das OTA-Update uebertraegt ein Firmware-Image vom Smartphone an den
Bordcomputer. Das Image wird in die inaktive OTA-Partition geschrieben
(Dual-Partition-Layout). Die aktive Partition bleibt unveraendert bis
zum Commit. Bei Fehler wird automatisch auf die alte Partition
zurueckgerollt.

### 6.2 Paketierung und Fragmentierung

Das Firmware-Image wird in Chunks von max. 244 Bytes Payload zerlegt.
Jeder Chunk wird mit dem Binary-Transfer-Protokoll-Format (Abschnitt 3.3)
uebertragen:

```
9 Bytes Header + 244 Bytes Payload = 253 Bytes pro Write
```

Verwendet wird Write Without Response (OTA_DATA 0x8002) fuer maximalen
Throughput. Bei MTU 247 und 15 ms Connection-Interval:

- Theoretisch: 244 * 4 / 0.015s = ca. 65 kB/s
- Praxis: 20-40 kB/s

Ein 300 kB Firmware-Image dauert ca. 10-20 Sekunden.

### 6.3 Fortschritts-Tracking

Der Bordcomputer sendet nach jedem empfangenen Chunk (oder periodisch)
ein Status-Notify auf OTA_CONTROL (0x8001):

```
Byte 0:     State (0x01=Receiving)
Byte 1:     Progress (0-100, Prozent)
```

Die App zeigt einen Fortschrittsbalken basierend auf diesem Wert.

### 6.4 Fehlerbehandlung und Retry

- **CRC16 pro Chunk:** Jeder Chunk wird mit CRC16-CCITT geprueft. Bei
  CRC-Fehler wird der Chunk abgelehnt; die App kann ihn erneut senden.
- **Sliding-Window:** Der Bordcomputer sendet Chunks mit Window-Size 8
  ohne auf ACK zu warten. Die App sendet ACKs mit der letzten
  zusammenhaengend empfangenen Sequence-Number.
- **Retransmission:** Der Bordcomputer sendet alle Chunks ab
  Last-Sequence+1 erneut (Flag bit 2 = retransmit).
- **Timeout:** 5 Sekunden ohne Chunk-Empfang = Transfer-Abbruch.
- **Abbruch:** Beide Seiten koennen durch OTA_CONTROL (0x8001, Command=0x01)
  abbrechen.

### 6.5 Verifikation

Nach Abschluss des Transfers berechnet der Bordcomputer einen SHA-256-Hash
ueber die gesamte empfangene Partition und sendet das Ergebnis ueber
OTA_VERIFY (0x8003):

```
Byte 0:     Status (0x00=ok, 0x01=hash_mismatch, 0x02=too_large,
            0x03=invalid_magic, 0x04=signature_failed)
Byte 1-32:  SHA-256-Hash (32 Bytes)
Byte 33-36: Image-Size (uint32, Little-Endian)
```

Die App vergleicht den Hash mit dem erwarteten Hash des Firmware-Images.
Bei Uebereinstimmung sendet die App Commit (0x03 an 0x8001).

### 6.6 Commit und Boot

Nach dem Commit:
1. Der Bordcomputer prueft die RSA-3072-Signatur (wenn Secure Boot aktiviert)
2. Anti-Rollback-Counter wird geprueft
3. Die neue Partition wird als Boot-Partition gesetzt
4. Der Bordcomputer startet neu

Nach dem Boot:
1. Secure Boot V2 prueft die Signatur (wenn aktiviert)
2. Self-Test wird ausgefuehrt
3. Bei Erfolg: Firmware als gueltig markiert
4. Bei Fehlschlag: automatischer Rollback zur alten Partition
5. Boot-Loop-Erkennung: >3 Resets in 60s = Rollback

### 6.7 OTA-Status-Codes

| Code | State | Beschreibung |
|---|---|---|
| 0x00 | Idle | Bereit fuer OTA |
| 0x01 | Receiving | Empfaenge Firmware-Chunks |
| 0x02 | Verifying | Verifiziere Hash/Signatur |
| 0x03 | ReadyToCommit | Verifikation OK, wartet auf Commit |
| 0x04 | Committed | Update abgeschlossen, Reboot ausstehend |
| 0x80 | Error | Fehler aufgetreten (siehe Error-Code) |

OTA Error-Codes:

| Error-Code | Beschreibung |
|---|---|
| 0x01 | Hash mismatch |
| 0x02 | Image too large |
| 0x03 | Invalid magic number |
| 0x04 | Signature verification failed |
| 0x05 | Write error |
| 0x06 | Transfer timeout |
| 0x07 | Partition error |

---

## 7. Beispiel-Code

### 7.1 Verbindungsaufbau (TypeScript)

```typescript
import { BleManager } from 'react-native-ble-plx';
import { SERVICE, CHAR, DEVICE_NAME } from './gatt-uuids';

const manager = new BleManager();

async function connectAndSetup(): Promise<void> {
  // 1. Scan nach Bordcomputer
  const device = await manager.startDeviceScan(
    [SERVICE.navigation],
    null,
    (error, scannedDevice) => {
      if (error) { console.error(error); return; }
      if (scannedDevice.name === DEVICE_NAME) {
        manager.stopDeviceScan();
      }
    }
  );

  // 2. Verbinden
  await device.connect({ requestMtu: 247 });

  // 3. MTU und Connection-Parameter
  await device.requestMtu(247);
  // requestConnectionPriority(HIGH) — plattformspezifisch

  // 4. Service-Discovery
  await device.discoverAllServicesAndCharacteristics();

  // 5. CCCD-Subscriptions fuer wichtige Characteristics
  await device.setupNotification(
    CHAR.tlmLive,
    (data) => {
      const bytes = decodeBase64(data.value!);
      const telemetry = parseTelemetryLive(bytes);
      console.log('Speed:', telemetry.speed, 'km/h');
    }
  );

  await device.setupNotification(
    CHAR.navManeuver,
    (data) => {
      const bytes = decodeBase64(data.value!);
      const maneuver = parseNavManeuver(bytes);
      console.log('Turn:', maneuver.type, 'in', maneuver.distance, 'm');
    }
  );
}
```

### 7.2 Verbindungsaufbau (C, ESP-IDF)

```c
#include "nimble/ble.h"
#include "host/ble_hs.h"

static int gap_event(struct ble_gap_event *event, void *arg);

void ble_client_connect(void)
{
    uint8_t own_addr_type;
    ble_hs_id_infer_auto(0, &own_addr_type);

    /* Scan nach Advertising mit Service-UUID 0x1000 */
    struct ble_gap_disc_params disc_params = {
        .itvl = BLE_GAP_SCAN_FAST_INTERVAL_MIN,
        .window = BLE_GAP_SCAN_FAST_WINDOW,
        .filter_duplicates = 1,
        .passive = 0,
    };

    ble_gap_disc(own_addr_type, BLE_HS_FOREVER,
                 &disc_params, gap_event, NULL);
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        /* Pruefe Advertising-Daten auf Service-UUID 0x1000 */
        /* Bei Match: ble_gap_connect() */
        break;

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            uint16_t conn_handle = event->connect.conn_handle;
            /* MTU Exchange */
            ble_gattc_exchange_mtu(conn_handle);
            /* Connection-Parameter Update */
            struct ble_gap_upd_params params = {
                .itvl_min = 12,  /* 15 ms */
                .itvl_max = 24,  /* 30 ms */
                .latency = 0,
                .supervision_timeout = 400,  /* 4 s */
            };
            ble_gap_update_params(conn_handle, &params);
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Auto-Reconnect mit Backoff */
        break;
    }
    return 0;
}
```

### 7.3 Telemetrie abonnieren und empfangen (TypeScript)

```typescript
function parseTelemetryLive(bytes: Uint8Array): TelemetryLive {
  const dv = new DataView(bytes.buffer);
  return {
    speed: dv.getUint8(0),                    // km/h
    rpm: dv.getUint16(1, true),               // U/min (LE)
    gear: dv.getInt8(3),                      // -1=neutral
    coolantTemp: dv.getInt8(4),               // Celsius
    ambientTemp: dv.getInt8(5),               // Celsius
    fuel: dv.getUint8(6),                     // Prozent
    voltage: dv.getUint8(7) / 10,             // Zehntelvolt -> Volt
    latitude: dv.getInt32(8, true) / 1e7,     // E-7 -> Grad
    longitude: dv.getInt32(12, true) / 1e7,   // E-7 -> Grad
    altitude: dv.getInt16(16, true),          // Meter
    heading: dv.getUint16(18, true) / 10,     // Zehntelgrad -> Grad
    satellites: dv.getUint8(20),              // Anzahl
    hdop: dv.getUint8(21) / 10,               // Zehntel -> Wert
  };
}

// Subscribe
await device.setupNotification(
  CHAR.tlmLive,
  (data) => {
    const bytes = base64ToUint8Array(data.value!);
    if (bytes.length === 22) {
      const tlm = parseTelemetryLive(bytes);
      updateDashboard(tlm);
    }
  }
);
```

### 7.4 Konfiguration schreiben (TypeScript)

```typescript
async function writeConfig(device: Device, configJson: string): Promise<void> {
  // 1. Admin-PIN authentifizieren
  const pinBytes = new TextEncoder().encode('0000');
  await device.writeCharacteristicWithResponseForService(
    SERVICE.config,
    '6e406006-b5a3-f393-e0a9-e50e24dcca9e',  // ADMIN_AUTH (geplant)
    base64Encode(pinBytes)
  );

  // 2. Config als Chunked-Transfer an 0x6002 schreiben
  const configBytes = new TextEncoder().encode(configJson);
  const chunkSize = 244;  // MTU 247 - 3 ATT Header - 5 Chunk-Header
  const transferId = Date.now() & 0xFF;
  let seqNum = 0;

  for (let offset = 0; offset < configBytes.length; offset += chunkSize) {
    const chunkPayload = configBytes.subarray(offset, offset + chunkSize);
    const isLast = offset + chunkSize >= configBytes.length;

    const header = new Uint8Array(5);
    header[0] = transferId;
    header[1] = isLast ? 0 : 1;  // more-flag
    header[2] = seqNum++;
    header[3] = chunkPayload.length & 0xFF;
    header[4] = (chunkPayload.length >> 8) & 0xFF;

    const chunk = new Uint8Array([...header, ...chunkPayload]);
    await device.writeCharacteristicWithResponseForService(
      SERVICE.config,
      CHAR.configWrite,
      base64Encode(chunk)
    );
  }
}
```

### 7.5 OTA-Update initiieren (TypeScript)

```typescript
async function startOtaUpdate(
  device: Device,
  firmwareImage: Uint8Array,
  expectedHash: Uint8Array
): Promise<void> {
  // 1. Admin-PIN authentifizieren
  await authenticateAdmin(device, '0000');

  // 2. OTA-Start-Befehl
  await device.writeCharacteristicWithResponseForService(
    SERVICE.ota, CHAR.otaControl,
    base64Encode(new Uint8Array([0x00]))  // Start
  );

  // 3. Subscribe OTA-Control fuer Status-Updates
  await device.setupNotification(
    CHAR.otaControl,
    (data) => {
      const bytes = base64ToUint8Array(data.value!);
      const state = bytes[0];
      const progress = bytes[1];
      console.log(`OTA: state=${state}, progress=${progress}%`);
      if (state === 0x03) {  // ReadyToCommit
        commitOta(device);
      }
    }
  );

  // 4. Subscribe OTA-Verify fuer Hash
  await device.setupNotification(
    CHAR.otaVerify,
    (data) => {
      const bytes = base64ToUint8Array(data.value!);
      const status = bytes[0];
      const hash = bytes.subarray(1, 33);
      if (status === 0x00 && arraysEqual(hash, expectedHash)) {
        console.log('OTA verify OK, committing');
        commitOta(device);
      } else {
        console.error('OTA verify failed:', status);
      }
    }
  );

  // 5. Firmware in Chunks uebertragen (Write Without Response)
  const chunkSize = 238;  // 247 - 3 ATT - 6 Transfer-Header
  for (let offset = 0; offset < firmwareImage.length; offset += chunkSize) {
    const chunk = firmwareImage.subarray(offset, offset + chunkSize);
    const framed = frameBinaryChunk(chunk, offset / chunkSize);
    await device.writeCharacteristicWithoutResponseForService(
      SERVICE.ota, CHAR.otaData,
      base64Encode(framed)
    );
  }
}

async function commitOta(device: Device): Promise<void> {
  await device.writeCharacteristicWithResponseForService(
    SERVICE.ota, CHAR.otaControl,
    base64Encode(new Uint8Array([0x03]))  // Commit
  );
}
```

### 7.6 DTC auslesen und loeschen (TypeScript)

```typescript
function parseDtcList(bytes: Uint8Array): DtcEntry[] {
  const entries: DtcEntry[] = [];
  const dv = new DataView(bytes.buffer);
  for (let i = 0; i + 6 <= bytes.length; i += 6) {
    entries.push({
      code: dv.getUint16(i, true),
      severity: dv.getUint8(i + 2),
      timestamp: dv.getUint32(i + 3, true),
    });
  }
  return entries;
}

// DTC-Liste lesen
async function readDtcList(device: Device): Promise<DtcEntry[]> {
  const charRead = await device.readCharacteristicForService(
    SERVICE.diagnose, CHAR.dtcList
  );
  const bytes = base64ToUint8Array(charRead.value!);
  return parseDtcList(bytes);
}

// DTC-Liste abonnieren (Notify bei neuem DTC)
await device.setupNotification(
  CHAR.dtcList,
  (data) => {
    const bytes = base64ToUint8Array(data.value!);
    const dtcs = parseDtcList(bytes);
    dtcs.forEach(dtc => {
      console.log(`DTC ${dtc.code}: severity=${dtc.severity}`);
    });
  }
);

// Self-Test ausloesen (erfordert Admin-PIN)
async function triggerSelfTest(device: Device, mode: number): Promise<void> {
  await authenticateAdmin(device, '0000');
  await device.writeCharacteristicWithResponseForService(
    SERVICE.diagnose, CHAR.selfTest,
    base64Encode(new Uint8Array([mode]))  // 0=Quick, 1=Full
  );
}
```

### 7.7 Sensor-Status lesen (C)

```c
/* Sensor-Status lesen (0x7002, 5 Bytes) */
typedef struct __attribute__((packed)) {
    uint8_t gps;       /* 0=ok, 1=no_fix, 2=error */
    uint8_t rpm;       /* 0=ok, 1=no_signal, 2=error */
    uint8_t temp;      /* 0=ok, 1=disconnected, 2=error */
    uint8_t fuel;      /* 0=ok, 1=error */
    uint8_t sd_card;   /* 0=ok, 1=not_present, 2=full, 3=error */
} sensor_status_t;

static int read_sensor_status(uint16_t conn_handle,
                               sensor_status_t *status)
{
    /* GATT-Read auf 0x7002 */
    uint16_t val_handle = ble_val_handles[CHR_H_SENSOR_STATUS];
    if (val_handle == 0) return BLE_HS_ENOTCONN;

    /* ble_gattc_read wird asynchron verarbeitet */
    return ble_gattc_read(conn_handle, val_handle,
                          sensor_status_read_cb, status);
}

static int sensor_status_read_cb(uint16_t conn_handle,
                                  const struct ble_gatt_error *error,
                                  struct ble_gatt_attr *attr,
                                  void *arg)
{
    sensor_status_t *status = (sensor_status_t *)arg;
    if (error->status != 0 || attr->om == NULL) {
        return error->status;
    }

    uint16_t len = OS_MBUF_PKTLEN(attr->om);
    if (len < sizeof(sensor_status_t)) {
        return BLE_HS_EMSGSIZE;
    }

    ble_hs_mbuf_to_flat(attr->om, status, sizeof(*status), &len);
    printf("GPS=%u RPM=%u Temp=%u Fuel=%u SD=%u\n",
           status->gps, status->rpm, status->temp,
           status->fuel, status->sd_card);

    return 0;
}
```

---

## 8. Fehlercodes

### 8.1 BLE-Fehlercodes

Diese Fehlercodes werden vom BLE-Stack (NimBLE) zurueckgegeben und
entsprechend der Bluetooth Core Specification.

| Code | Name | Beschreibung |
|---|---|---|
| 0x00 | BLE_HS_ENOERROR | Kein Fehler |
| 0x01 | BLE_HS_EINVAL | Ungueltiger Parameter |
| 0x02 | BLE_HS_EMSGSIZE | Nachricht zu gross |
| 0x03 | BLE_HS_ENOENT | Eintrag nicht gefunden |
| 0x04 | BLE_HS_ENOTCONN | Keine Verbindung aktiv |
| 0x05 | BLE_HS_ENOTSUP | Operation nicht unterstuetzt |
| 0x06 | BLE_HS_EAPP | Anwendungsfehler |
| 0x07 | BLE_HS_EBADDATA | Ungueltige Daten |
| 0x08 | BLE_HS_ETIMEOUT | Timeout |
| 0x09 | BLE_HS_ENOMEM | Nicht genuegend Speicher |
| 0x0A | BLE_HS_EBUSY | Ressource belegt |
| 0x0B | BLE_HS_ECONTROLLER | Controller-Fehler |
| 0x0C | BLE_HS_EREJECT | Anfrage abgelehnt |
| 0x0D | BLE_HS_EUNKNOWN | Unbekannter Fehler |
| 0x0E | BLE_HS_EROLE | Falsche Rolle |
| 0x0F | BLE_HS_ETIMEOUT_HCI | HCI-Timeout |

### 8.2 Application-Level Error Codes

Diese Fehlercodes werden auf Anwendungsebene zurueckgegeben, typischerweise
als Teil eines Notify-Pakets (z.B. OTA-Control Status-Byte 0x80).

| Code | Name | Beschreibung |
|---|---|---|
| 0xFF | ERR_NOT_AUTHORIZED | Admin-PIN erforderlich oder abgelaufen |
| 0xFE | ERR_INVALID_LENGTH | Ungueltige Datenlaenge |
| 0xFD | ERR_INVALID_VALUE | Ungueltiger Wert (Range-Check fehlgeschlagen) |
| 0xFC | ERR_TRANSFER_FAILED | Transfer abgebrochen (CRC/Timeout) |
| 0xFB | ERR_NOT_READY | Geraet nicht bereit (z.B. OTA inaktiv) |
| 0xFA | ERR_BUSY | Operation laeuft bereits |

### 8.3 OTA-spezifische Fehlercodes

| Code | Name | Beschreibung |
|---|---|---|
| 0x01 | OTA_ERR_HASH_MISMATCH | SHA-256-Hash stimmt nicht ueberein |
| 0x02 | OTA_ERR_TOO_LARGE | Image zu gross fuer Partition |
| 0x03 | OTA_ERR_INVALID_MAGIC | Magic-Number im Image fehlt |
| 0x04 | OTA_ERR_VERIFY_SIG | RSA-Signaturpruefung fehlgeschlagen |
| 0x05 | OTA_ERR_WRITE | Schreibfehler in Flash |
| 0x06 | OTA_ERR_TIMEOUT | Transfer-Timeout |
| 0x07 | OTA_ERR_PARTITION | Partitionsfehler |

### 8.4 Timeout-Verhalten

| Operation | Timeout | Verhalten bei Timeout |
|---|---|---|
| Connect-Versuch | 3 s | Zurueck zu SCANNING mit Backoff |
| Chunked-Transfer | 5 s | Transfer-Abbruch, Ressourcen freigeben |
| Binary-Transfer | 5 s pro Chunk | Retransmission, dann Abbruch |
| Admin-Session | 5 min | Session abgelaufen, Re-Auth erforderlich |
| Self-Test | 30 s | Ergebnis mit `fail` zurueckgeben |
| OTA-Transfer | variabel | Abbruch bei 5s Inaktivitaet |
| Supervision | 4 s | Disconnect, Auto-Reconnect |

### 8.5 Disconnect-Handling

Bei Disconnect:

1. Alle CCCD-Subscriptions werden zurueckgesetzt (`s_cccd_telemetry = false`,
   `s_cccd_gps = false`, `s_cccd_dtc = false`, `s_cccd_sensor_status = false`)
2. Laufende Binary-Transfers werden abgebrochen und Ressourcen freigegeben
3. Admin-Session wird invalidiert
4. Der Bordcomputer startet automatisch Advertising neu
5. Die App beginnt Auto-Reconnect mit exponentiellem Backoff (1s, 2s, 4s,
   8s, max 30s)
6. Telemetrie-Logging auf SD-Karte wird unabhaengig von BLE fortgesetzt
7. Bei Reconnect durchlaeuft die App die Reconnect-Sequenz:
   MTU-Exchange -> Connection-Parameter -> Service-Discovery -> Encryption ->
   Subscribe P0 -> Read Snapshot -> Subscribe P1-P4

Disconnect-Gruende (BLE-HCI-Status-Codes):

| Code | Name | Beschreibung |
|---|---|---|
| 0x08 | Connection Timeout | Supervision-Timeout erreicht |
| 0x13 | Remote User Terminated | Gegenstelle hat getrennt |
| 0x16 | Connection Terminated by Local Host | Lokale Trennung |
| 0x22 | Connection Rejected | Verbindung abgelehnt |
| 0x3E | Connection Fail to Be Established | Verbindungsaufbau fehlgeschlagen |

---

## Anhang: Service- und Characteristic-Referenz

### Alle Custom-Services

| # | Service | 16-bit ID | UUID |
|---|---|---|---|
| 1 | Navigation | 0x1000 | 6e401000-b5a3-f393-e0a9-e50e24dcca9e |
| 2 | Media | 0x2000 | 6e402000-b5a3-f393-e0a9-e50e24dcca9e |
| 3 | Phone | 0x3000 | 6e403000-b5a3-f393-e0a9-e50e24dcca9e |
| 4 | Weather | 0x4000 | 6e404000-b5a3-f393-e0a9-e50e24dcca9e |
| 5 | Telemetry | 0x5000 | 6e405000-b5a3-f393-e0a9-e50e24dcca9e |
| 6 | Config | 0x6000 | 6e406000-b5a3-f393-e0a9-e50e24dcca9e |
| 7 | Diagnose | 0x7000 | 6e407000-b5a3-f393-e0a9-e50e24dcca9e |
| 8 | OTA | 0x8000 | 6e408000-b5a3-f393-e0a9-e50e24dcca9e |

### Alle Standard-Services

| Service | UUID |
|---|---|
| Generic Access | 00001800-0000-1000-8000-00805f9b34fb |
| Device Information | 0000180a-0000-1000-8000-00805f9b34fb |

### Anzahl Characteristics pro Service

| Service | Characteristics | Davon Notify | Davon Write |
|---|---|---|---|
| Navigation | 5 | 4 | 1 |
| Media | 3 | 2 | 2 |
| Phone | 4 | 3 | 1 |
| Weather | 3 | 3 | 0 |
| Telemetry | 4 | 2 | 1 |
| Config | 5 | 2 | 3 |
| Diagnose | 5 | 4 | 2 |
| OTA | 3 | 2 | 2 |
| **Total** | **33** (inkl. Standard: 35) | **22** | **12** |

---

## Anhang: Referenzen

**Interne Referenzen:**
- `firmware/components/ble/gatt_uuids.h` — GATT Service/Characteristic UUIDs
- `firmware/components/ble/include/ble.h` — BLE Interface
- `firmware/components/ble/ble_service.c` — NimBLE GATT-Server Implementierung
- `companion-app/src/core/ble/gatt-uuids.ts` — App-seitige UUID-Definitionen
- `companion-app/src/core/ble/BleProvider.ts` — BLE-Provider Interface
- `docs/ARCHITECTURE.md` — System-Architektur
- `docs/SECURITY-ANALYSIS.md` — Sicherheitskonzept (BLE Pairing, Auth)
- `docs/research/13-connectivity-app.md` — Connectivity-Konzept

**Spezifikationen:**
- Bluetooth Core Specification 4.2 (LE Secure Connections)
- Bluetooth Core Specification Supplement (GATT)
- CRC-16-CCITT: ITU-T V.41 (Polynom 0x1021)
- SHA-256: NIST FIPS 180-4
- RSA-3072: NIST FIPS 186-4

---

*Dieses Dokument ist die API-Referenz fuer das BLE GATT Interface des
Transalp Bordcomputers. Bei Aenderungen an den GATT-Services muss diese
Datei aktualisiert werden. Die UUIDs und Datenformate muessen mit den
Definitionen in `gatt_uuids.h` und `gatt-uuids.ts` uebereinstimmen.*
