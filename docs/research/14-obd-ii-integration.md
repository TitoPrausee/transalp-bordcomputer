# OBD-II Integration: ELM327-Adapter, PID-Emulation und Diagnose-Support

> Stand: 16.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-2000, rein mechanisch, keine ECU
> Plattform: ESP32-S3-WROOM-1 (N8R2: 8MB Flash, 2MB PSRAM), ESP-IDF v5.4, FreeRTOS SMP
> Vorherige Dokumente: 01-sensoren, 02-hardware, 03-software-stack, 04-pd06-daten, 05-gehaeuse-emv, 06-feature-roadmap, 07-display-navigation, 08-feature-matrix, 09-master-plan, 10-ui-ux, 11-firmware-arch, 12-diagnose-support, 13-connectivity-app

## 0. Einleitung

Die Honda Transalp 600 (PD06) hat keine ECU, keinen CAN-Bus und keine OBD-II-
Schnittstelle. Jeder Sensorwert, den der Bordcomputer anzeigt, stammt aus
eigener Sensor-Anzapfung. Das bedeutet: OBD-II im klassischen Sinn (Daten
von der Fahrzeug-ECU lesen) ist auf der PD06 nicht moeglich.

Dieses Dokument evaluiert zwei Richtungen der OBD-II-Integration:

1. **OBD-II-Emulator (Ausgangsrichtung):** Der Bordcomputer nimmt seine eigenen
   Sensordaten (RPM, Kuehlmitteltemperatur, Oeldruck, etc.) und macht sie als
   OBD-II-PIDs verfuegbar. Ein Standard-Diagnosegeraet (ELM327-Adapter) kann
   diese Daten per BLE auslesen. Dies ist der "Emulator"-Aspekt.

2. **OBD-II-Reader (Eingangsrichtung):** Der Bordcomputer (oder die Companion-
   App) verbindet sich per BLE mit einem ELM327-Adapter, der an der OBD-II-
   Buchse eines anderen Fahrzeugs steckt. Dies ist relevant fuer Nutzer, die
   den Bordcomputer an mehreren Motorraedern nutzen (PD06 + ein neueres Bike
   mit OBD-II).

Beide Richtungen werden Architektur, Datenfluss, API-Design, Risiken und
Aufwandsschaetzung enthalten. Die Empfehlung am Ende priorisiert die
Implementierung.

---

## 1. Grundlagen: ELM327 und OBD-II PIDs

### 1.1 ELM327 AT-Befehlssatz

Der ELM327 ist ein OBD-II-zu-UART-Interpreter. Er wandelt ASCII-Kommandos
im AT-Stil in CAN/K-Line-Frames um. Die Kommunikation ist seriell:
Jedes Kommando endet mit CR (0x0D), die Antwort endet mit dem `>` Prompt.

Kernbefehle fuer die Initialisierung:

```
ATZ\r          Reset, Antwort: "ELM327 v1.5" (oder aehnlich)
ATE0\r         Echo aus
ATL0\r         Linefeed aus
ATS0\r         Leerzeichen in Antwort unterdruecken
ATH0\r         Header ausblenden
ATSP0\r        Protokoll automatisch erkennen
```

Nach der Initialisierung werden PIDs als `Mode + PID` gesendet:

```
010C\r         Mode 01 PID 0C = Motordrehzahl
010D\r         Mode 01 PID 0D = Fahrzeuggeschwindigkeit
0105\r         Mode 01 PID 05 = Kuehlmitteltemperatur
```

Die Antwort folgt dem Schema `41 <PID> <Datenbytes> >`:

```
Sende:  010C\r
Antwort: 41 0C 1A F8 >
        -> (0x1A * 256 + 0xF8) / 4 = 1726 RPM
```

### 1.2 SAE J1979 PID-Referenz (Mode 01)

Mode 01 (Show Current Data) ist der wichtigste Service fuer Live-Daten.
Jede PID hat eine feste Datenlaenge und eine Decodier-Formel.

| PID (hex) | Parameter | Bytes | Formel | Einheit |
|---|---|---|---|---|
| 0x04 | Engine Load | 1 | A * 100 / 255 | % |
| 0x05 | Kuehlmitteltemperatur | 1 | A - 40 | C |
| 0x0B | Saugrohrdruck (MAP) | 1 | A | kPa |
| 0x0C | Motordrehzahl | 2 | (256*A + B) / 4 | rpm |
| 0x0D | Fahrzeuggeschwindigkeit | 1 | A | km/h |
| 0x0F | Ansauglufttemperatur | 1 | A - 40 | C |
| 0x10 | Luftmasse (MAF) | 2 | (256*A + B) / 100 | g/s |
| 0x11 | Drosselklappenposition | 1 | A * 100 / 255 | % |
| 0x1F | Laufzeit seit Start | 2 | 256*A + B | s |
| 0x2F | Kraftstoffstand | 1 | A * 100 / 255 | % |
| 0x42 | Steuergeraetespannung | 2 | (256*A + B) / 1000 | V |
| 0x5C | Oeltemperatur | 1 | A - 40 | C |

PID 0x00 ist ein 4-Byte-Bitfeld, das anzeigt, welche PIDs 0x01-0x20
unterstuetzt werden. PID 0x20 zeigt PIDs 0x21-0x40, usw. Dies ist der
Universal-Init/Ping: Jedes OBD-II-faehige Steuergeraet muss PID 0x00
unterstuetzen.

### 1.3 Erweiterte PIDs fuer Motorrad-Diagnose

Die im Issue geforderten zusaetzlichen Parameter:

| Parameter | PID | Formel | Bemerkung |
|---|---|---|---|
| Oeltemperatur | 0x5C | A - 40 | Standard PID, Mode 01 |
| Oeldruck | -- | -- | KEIN Standard-OBD-II-PID! |
| Luftmasse (MAF) | 0x10 | (256A + B) / 100 | Standard PID, Mode 01 |
| Lambda Zyl. 1-4 | 0x14-0x1B | siehe Formel | O2-Sensor-PIDs |

**Oeldruck** ist kein Standard-OBD-II-PID. SAE J1979 definiert keinen
PID fuer Oeldruck. Bei Fahrzeugen mit OelDrucksensor wird dieser Wert
oft ueber herstellerspezifische PIDs (Mode 22) oder ueber einen
analogen Eingang am ELM327-Adapter (falls unterstuetzt) ausgelesen.

Fuer den Bordcomputer bedeutet das: Der Oeldruck muss ueber den
eigenen analogen Sensor erfasst werden (siehe docs/research/01-sensoren-
fahrzeugdaten.md Abschnitt Oeldruck). In der OBD-II-Emulation kann er
als Custom-PID angeboten werden (Mode 01, PID 0x80+ oder Mode 22).

**Lambda-Sonden** werden ueber PID 0x14 bis 0x1B ausgegeben (bis zu
8 Sonden). Jede PID liefert 2 Bytes: Byte A = Spannung (A * 0.005 V),
Byte B = Short-Term-Fuel-Trim ((B - 128) * 100 / 128 %).

Die PD06 hat keine Lambda-Sonden (Vergaser, nicht Einspritzung).
In der Emulation koennen diese PIDs mit festen Werten oder
Berechnungen aus dem Kraftstoffverbrauch belegt werden.

---

## 2. Architektur

### 2.1 Uebersicht: Zwei Betriebsmodi

```
                +------------------------------------------+
                |          Bordcomputer (ESP32-S3)         |
                |                                          |
                |  +-------------+    +-------------+      |
                |  | OBD-Emu     |    | OBD-Reader  |     |
                |  | (Ausgang)   |    | (Eingang)   |     |
                |  |             |    |             |     |
                |  | Sensor-Daten|   | ELM327-BLE  |     |
                |  | -> PID-MAP  |    | Client      |     |
                |  +------+------+    +------+------+     |
                |         |                |              |
                |         v                v              |
                |  +-------------+  +-------------+      |
                |  | GATT: OBD   |  | GATT: OBD   |      |
                |  | (neue Char) |  | (neue Char) |      |
                |  +-------------+  +-------------+      |
                |         |                |              |
                +---------|----------------|-------------+
                          |                |
                    BLE   |          BLE   |
                          v                v
              +------------------+  +------------------+
              | Companion-App    |  | ELM327-BLE       |
              | OBD-Live Screen  |  | Adapter          |
              | (Display)        |  | (an OBD-Buchse   |
              |                  |  |  eines anderen   |
              |                  |  |  Fahrzeugs)      |
              +------------------+  +------------------+
```

### 2.2 OBD-II-Emulator (Ausgangsrichtung)

Der Bordcomputer nimmt seine vorhandenen Sensordaten und mappt sie auf
OBD-II-PIDs. Die Mapping-Tabelle:

| Bordcomputer-Sensor | OBD-II PID | Formel (Encode) |
|---|---|---|
| RPM (PCNT, GPIO16) | 0x0C | rpm = sensor_rpm; raw = rpm * 4; A = raw >> 8; B = raw & 0xFF |
| Geschwindigkeit (GPS) | 0x0D | A = (uint8_t) speed_kmh |
| Kuehlmitteltemp (DS18B20) | 0x05 | A = (uint8_t)(temp_c + 40) |
| Bordspannung (ADC) | 0x42 | raw = voltage_mV; A = raw >> 8; B = raw & 0xFF |
| Kraftstoffstand (ADC) | 0x2F | A = (uint8_t)(fuel_pct * 2.55) |
| Oeltemperatur (NTC, geplant) | 0x5C | A = (uint8_t)(oil_temp_c + 40) |
| Oeldruck (analog, geplant) | Custom PID 0x80 | A = (uint8_t)(oil_pressure_bar * 10) |

Der Emulator lauscht auf eingehende OBD-II-Kommandos ueber BLE und
antwortet mit den codierten PID-Daten. Die Companion-App oder ein
externes Diagnosegeraet kann diese Daten wie von einem echten
Fahrzeug abfragen.

### 2.3 OBD-II-Reader (Eingangsrichtung)

Der Bordcomputer verbindet sich als BLE-Central mit einem ELM327-BLE-
Adapter. Der Adapter ist physisch an der OBD-II-Buchse eines Fahrzeugs
angeschlossen. Der Bordcomputer sendet AT-Kommandos an den Adapter
und wertet die Antworten aus.

Fuer die PD06 ist dieser Modus nicht relevant (keine OBD-II-Buchse).
Er ist aber relevant fuer Nutzer, die den Bordcomputer als universelles
Diagnosegeraet an mehreren Fahrzeugen einsetzen.

### 2.4 BLE-GATT-Integration

Der Bordcomputer ist ein GATT-Server mit 8 Custom-Services (0x1000 bis
0x8000, siehe docs/API-REFERENCE.md und firmware/components/ble/gatt_uuids.h).
Die Service-IDs sind in 0x1000-Schritten organisiert. Der naechste freie
Service-ID-Bereich waere 0x9000.

Da OBD-II-Daten eng mit der Telemetrie verwandt sind, wird empfohlen,
statt eines neuen Services zusaetzliche Characteristics im bestehenden
Telemetry-Service (0x5000) zu definieren:

| Characteristic | UUID (16-bit) | Properties | Beschreibung |
|---|---|---|---|
| OBD_LIVE | 0x5005 | notify | 20-byte packed OBD live data |
| OBD_PID_REQUEST | 0x5006 | write | PID-Anfrage vom Client |
| OBD_PID_RESPONSE | 0x5007 | read+notify | PID-Antwort (variable Laenge) |
| OBD_STATUS | 0x5008 | read+notify | Adapter-Status (connected, protocol, voltage) |

**Konflikt-Hinweis:** Einige ELM327-BLE-Adapter verwenden den Nordic
UART Service (NUS) mit der Base-UUID `6e40xxxx-b5a3-f393-e0a9-e50e24dcca9e`.
Dies ist dieselbe Base-UUID, die der Bordcomputer fuer seine Custom-Services
verwendet. Wenn der Bordcomputer als BLE-Central (Reader-Modus) nach
ELM327-Adaptern scannt, muss er nach dem NUS-Service-UUID 0x0001 filtern.
Da der Bordcomputer selbst GATT-Server mit Services im Bereich 0x1000-0x8000
ist, gibt es keinen UUID-Konflikt in der eigenen GATT-Tabelle. Der Konflikt
waere nur beim Scannen nach externen Adaptern relevant.

Alternative: Einige Adapter verwenden den Custom-Service `FFF0` mit
den Characteristics `FFF1` (notify) und `FFF2` (write). Der Reader muss
beide UUID-Schemata unterstuetzen.

---

## 3. Datenfluss

### 3.1 Emulator-Modus: PID-Anfrage und Antwort

```
Diagnosegeraet / Companion-App
    |
    | BLE Write auf CHR_OBD_PID_REQUEST (0x5006)
    | Payload: [mode, pid]  z.B. [0x01, 0x0C] fuer RPM
    v
+-----------------------------------+
| Bordcomputer: OBD-Emulator-Task   |
|                                   |
| 1. PID aus Payload extrahieren    |
| 2. Mapping-Tabelle konsultieren   |
| 3. Sensor-Wert abrufen            |
|    (sensor_data_queue oder GPS)   |
| 4. Wert nach SAE J1979 codieren   |
| 5. Response-Payload zusammen-     |
|    stellen: [0x41, pid, A, B...]  |
|                                   |
| 6. Notify auf CHR_OBD_PID_RESPONSE|
|    (0x5007) mit Response-Payload  |
+-----------------------------------+
    |
    v
Diagnosegeraet / Companion-App
    empfängt Notify, decodiert PID
```

### 3.2 Reader-Modus: ELM327-Kommunikation

```
Bordcomputer (BLE-Central)
    |
    | BLE Connect zu ELM327-Adapter
    | Subscribe auf Notify-Char (FFF1 oder NUS-TX)
    | Write auf Write-Char (FFF2 oder NUS-RX)
    |
    | Send: ATZ\r
    | Warte auf ">" Prompt
    | Send: ATE0\r
    | Send: ATSP0\r
    | Send: 0100\r  (PID-Unterstützung abfragen)
    |
    v
+-----------------------------------+
| ELM327-BLE-Adapter                |
| (verbunden mit OBD-II-Buchse)     |
|                                   |
| Uebersetzt AT-Kommandos in CAN/   |
| K-Line-Frames, fragt ECU ab,      |
| gibt ASCII-Hex-Antwort zurueck    |
+-----------------------------------+
    |
    v
Fahrzeug-ECU (sofern vorhanden)
    antwortet mit PID-Daten
```

### 3.3 Polling-Strategie

OBD-II-PIDs muessen einzeln angefragt werden (oder als Batch bis zu 6 PIDs).
Die effiziente Strategie ist ein rotierender Poll-Cycle:

```
Cycle (1 Sekunde):
  1. PID 0x0C (RPM)         - alle 100ms (10 Hz, Echtzeit-kritisch)
  2. PID 0x0D (Speed)       - alle 100ms (10 Hz)
  3. PID 0x05 (Coolant)     - alle 1000ms (1 Hz, traegt)
  4. PID 0x5C (Oil Temp)    - alle 1000ms (1 Hz)
  5. PID 0x10 (MAF)         - alle 200ms (5 Hz)
  6. PID 0x42 (Voltage)     - alle 1000ms (1 Hz)
  7. PID 0x2F (Fuel)        - alle 2000ms (0.5 Hz)
```

Die Poll-Raten sind an die physikalische Aenderungsrate der Werte
angepasst. RPM und Speed aendern sich schnell, Temperaturen langsam.

Im Emulator-Modus entfaellt das Polling: Der Bordcomputer pushed
die Werte per Notify (CHR_OBD_LIVE, 0x5005) in einem 20-Byte-Packed-
Frame mit 10 Hz. Der Client abonniert die Characteristic und erhaelt
automatisch Updates.

### 3.4 OBD_LIVE Packed Frame Format (Characteristic 0x5005)

```
Byte  0: RPM_H        (RPM / 4 >> 8)
Byte  1: RPM_L        (RPM / 4 & 0xFF)
Byte  2: Speed        (km/h, uint8)
Byte  3: Coolant_Temp (C + 40, uint8)
Byte  4: Oil_Temp     (C + 40, uint8, oder 0xFF = nicht verfuegbar)
Byte  5: Oil_Pressure (bar * 10, uint8, oder 0xFF = nicht verfuegbar)
Byte  6: MAF_H        (MAF * 100 >> 8)
Byte  7: MAF_L        (MAF * 100 & 0xFF)
Byte  8: Voltage_H    (mV >> 8)
Byte  9: Voltage_L    (mV & 0xFF)
Byte 10: Fuel_Level   (0-100 %, uint8)
Byte 11: Engine_Load  (0-100 %, uint8)
Byte 12: Intake_Air_T (C + 40, uint8)
Byte 13: Throttle_Pos (0-100 %, uint8)
Byte 14-15: Lambda_1  (Voltage * 200, uint16)
Byte 16-17: Lambda_2  (Voltage * 200, uint16)
Byte 18-19: Reserved  (0x00)
```

---

## 4. API-Design

### 4.1 Firmware: OBD-Emulator-Modul

Neue Datei: `firmware/components/obd/obd_emulator.c` + `include/obd.h`

```c
/* obd.h - OBD-II Emulator und Reader */

#ifndef _OBD_H_
#define _OBD_H_

#include <stdint.h>
#include <stdbool.h>
#include "sensors.h"
#include "gps.h"

/* OBD-Service Characteristic IDs (im Telemetry-Service 0x5000) */
#define CHR_OBD_LIVE         0x5005
#define CHR_OBD_PID_REQUEST  0x5006
#define CHR_OBD_PID_RESPONSE 0x5007
#define CHR_OBD_STATUS       0x5008

/* OBD-Modi (SAE J1979) */
#define OBD_MODE_CURRENT_DATA    0x01
#define OBD_MODE_FREEZE_FRAME    0x02
#define OBD_MODE_READ_DTC        0x03
#define OBD_MODE_CLEAR_DTC       0x04
#define OBD_MODE_VEHICLE_INFO    0x09

/* OBD-Status */
typedef enum {
    OBD_MODE_DISABLED = 0,
    OBD_MODE_EMULATOR,
    OBD_MODE_READER,
} obd_mode_t;

typedef struct {
    obd_mode_t mode;
    bool adapter_connected;
    char adapter_version[16];
    uint8_t protocol;       /* ATSP-Wert */
    float adapter_voltage;   /* Spannung am OBD-Port */
} obd_status_t;

/* OBD-Live-Daten (fuer packed frame) */
typedef struct {
    uint16_t rpm;
    uint8_t speed_kmh;
    uint8_t coolant_temp_c;
    uint8_t oil_temp_c;       /* 0xFF = N/A */
    uint8_t oil_pressure;     /* 0xFF = N/A, sonst bar*10 */
    uint16_t maf;             /* g/s * 100 */
    uint16_t voltage_mv;
    uint8_t fuel_level_pct;
    uint8_t engine_load_pct;
    uint8_t intake_air_temp_c;
    uint8_t throttle_pos_pct;
    uint16_t lambda_1_mv;     /* mV, 0 = N/A */
    uint16_t lambda_2_mv;
} obd_live_t;

/* API */

/* Emulator: PID-Anfrage verarbeiten und Response erzeugen */
int obd_emu_handle_request(const uint8_t *request, uint8_t len,
                           uint8_t *response, uint8_t *resp_len);

/* Emulator: Live-Frame fuer Notify packen */
int obd_emu_pack_live(const sensor_data_t *sens, const gps_data_t *gps,
                      uint8_t *buf, uint8_t buf_len);

/* Reader: ELM327-Adapter verbinden und initialisieren */
int obd_reader_connect(const char *adapter_addr);
int obd_reader_init_elm(void);
int obd_reader_query_pid(uint8_t mode, uint8_t pid,
                         uint8_t *data, uint8_t *data_len);
void obd_reader_disconnect(void);

/* Status */
const obd_status_t *obd_get_status(void);

#endif /* _OBD_H_ */
```

### 4.2 PID-Mapping-Tabelle (Emulator)

```c
/* Interne Mapping-Tabelle: PID -> Sensor-Quelle */
typedef struct {
    uint8_t pid;
    uint8_t data_bytes;
    /* Encoder-Funktion: nimmt sensor_data_t + gps_data_t,
     * schreibt rohe OBD-II-Bytes in buf, gibt Laenge zurueck */
    uint8_t (*encode)(const sensor_data_t *s, const gps_data_t *g,
                      uint8_t *buf);
} obd_pid_map_t;

static const obd_pid_map_t s_pid_map[] = {
    { 0x04, 1, encode_engine_load },    /* aus RPM + MAP-Schaetzung */
    { 0x05, 1, encode_coolant_temp },   /* aus DS18B20 / NTC */
    { 0x0C, 2, encode_rpm },            /* aus PCNT-GPIO16 */
    { 0x0D, 1, encode_speed },          /* aus GPS speed_kmh */
    { 0x0F, 1, encode_intake_temp },    /* aus BMP280 Die-Temp */
    { 0x10, 2, encode_maf },            /* geschaetzt aus RPM + MAP */
    { 0x11, 1, encode_throttle },       /* aus ADC (sofern verkabelt) */
    { 0x2F, 1, encode_fuel_level },     /* aus fuel_level_read_liters() */
    { 0x42, 2, encode_voltage },        /* aus BATT_ADC */
    { 0x5C, 1, encode_oil_temp },       /* aus zusaetzlichem NTC (geplant) */
    { 0x80, 1, encode_oil_pressure },   /* Custom-PID fuer Oeldruck */
    { 0x00, 4, encode_supported_pids },  /* Bitfeld der obigen PIDs */
};
```

### 4.3 Companion-App: OBD-Live-Screen

Neue Datei: `companion-app/src/screens/ObdScreen.tsx`

Der Screen zeigt die OBD-Live-Daten in einer scrollbaren Liste mit
Gauges und numerischen Werten. Layout:

```
+-----------------------------------+
|  OBD-II Live Data                 |
|                                   |
|  [RPM: 4250]    [Speed: 87 km/h] |
|                                   |
|  Coolant:  92 C    Oil Temp: 88 C |
|  Oil Press: 3.2 bar                |
|  MAF: 14.5 g/s     Voltage: 13.8V |
|  Fuel: 68%         Load: 42%      |
|  Lambda 1: 0.95V   Lambda 2: ---  |
|                                   |
|  [Connect ELM327]  [Mode: Emu]   |
+-----------------------------------+
```

Die Datenquelle ist CHR_OBD_LIVE (0x5005) per Notify. Bei 10 Hz
Update-Rate wird ein Moving-Average-Filter (Window 3) fuer die
Anzeige geglaettet.

---

## 5. Risiken

### 5.1 Technische Risiken

| Risiko | Wahrscheinlichkeit | Auswirkung | Mitigation |
|---|---|---|---|
| ELM327-BLE-Adapter inkompatibel (abweichende UUIDs) | hoch | mittel | Multi-UUID-Discovery (FFF0 + NUS + Heuristik) |
| OBD-II-Protokoll nicht erkannt (altes Fahrzeug) | mittel | hoch | Fallback auf ATSP0 Auto-Detect, Timeout 30s |
| BLE-Bandbreite reicht nicht fuer 10 Hz OBD-Live | niedrig | mittel | Adaptive Rate: 5 Hz fallback, nur RPM+Speed priorisiert |
| Custom-PID 0x80 (Oeldruck) von Standard-Tools ignoriert | hoch | niedrig | Dokumentation der Custom-PIDs, Companion-App als primaerer Client |
| Reader-Modus blockiert BLE-Stack (GATT-Server stoert Central) | mittel | hoch | Zeitmultiplex: Reader-Polling nur in Idle-Phasen, nicht waehrend Navigation |
| ELM327-Klon sendet fehlerhafte Daten (Billig-Adapter) | hoch | mittel | Plausibilitaetspruefung: RPM 0-16000, Temp -40..215, Speed 0..255 |

### 5.2 Architektur-Risiken

| Risiko | Auswirkung | Mitigation |
|---|---|---|
| Emulator beansprucht zusaetzlichen Flash und RAM | gering | PID-Map ist ca. 200 Bytes, Task-Stack 2KB. Negligible. |
| Zwei BLE-Rollen (Server + Central) gleichzeitig | ESP32-S3 unterstuetzt dies, aber mit Einschraenkungen | Reader-Modus als opt-in, nicht gleichzeitig mit Navigation aktiv |
| OBD-II-Daten sind nicht fuer die PD06 relevant (keine ECU) | Feature hat begrenzten Nutzen auf dem Prim%C3%A4rfahrzeug | Emulator als Hauptmodus, Reader als zusaetzliches Feature fuer Multi-Bike-Nutzer |

### 5.3 Datenschutz und Lizenz

Das Repo steht unter PolyForm Noncommercial 1.0.0. OBD-II-Daten sind
Fahrzeugdaten, keine personenbezogenen Daten. Die DSGVO ist nicht
direkt betroffen, solange keine Touren mit GPS-Daten an Dritte
uebertragen werden (das ist Cloud-Sync, siehe Dokument 15).

Die SAE J1979-Spezifikation ist urheberrechtlich geschuetzt. Die
PID-Codes und Formeln sind jedoch weitgehend veroeffentlicht und
in zahlreichen Open-Source-Projekten implementiert. Die Verwendung
der PID-Codes und Decodier-Formeln in der Dokumentation und im Code
ist unbedenklich.

---

## 6. Aufwandsschaetzung

### 6.1 Emulator-Modus (Ausgangsrichtung)

| Aufgabe | Aufwand | Abhaengigkeit |
|---|---|---|
| PID-Mapping-Tabelle und Encoder-Funktionen | 4h | sensor_data_t Struktur |
| OBD-Emulator-Task (Request-Handler) | 3h | BLE-GATT-Callback-Infrastruktur |
| Neue GATT-Characteristics (0x5005-0x5008) | 2h | gatt_uuids.h, ble_service.c |
| OBD_LIVE packed frame Encoder | 2h | obd_live_t Struktur |
| Companion-App: ObdScreen.tsx | 6h | BleProvider, binary-parsers.ts |
| Companion-App: OBD binary parser | 2h | obd_live_t Format |
| Unit-Tests: PID Encoder/Decoder | 3h | test framework |
| **Zwischensumme Emulator** | **22h** | |

### 6.2 Reader-Modus (Eingangsrichtung)

| Aufgabe | Aufwand | Abhaengigkeit |
|---|---|---|
| BLE-Central-Scan + Connect zu ELM327 | 4h | NimBLE Central API |
| AT-Kommando-Stack (Senden/Prompt-Parse) | 4h | BLE Write/Notify |
| Multi-UUID-Discovery (FFF0, NUS, Heuristik) | 3h | BLE-Service-Enumeration |
| PID-Query + Response-Parser | 3h | SAE J1979 Formeln |
| Polling-Task mit adaptiven Raten | 3h | FreeRTOS Task |
| Adapter-Status-Reporting | 1h | CHR_OBD_STATUS |
| Plausibilitaetspruefung / Fehlerbehandlung | 2h | Wertebereiche |
| Unit-Tests: ELM327-Parser | 3h | Mock-Responses |
| **Zwischensumme Reader** | **23h** | |

### 6.3 Gesamt

| Bereich | Aufwand |
|---|---|
| Emulator-Modus | 22h |
| Reader-Modus | 23h |
| Doku + Integrationstests | 5h |
| **Gesamt** | **50h** |

**Empfehlung:** Phase 1 nur Emulator-Modus (22h). Der Reader-Modus ist
optional und wird nur implementiert, wenn es eine klar formulierte
Anforderung fuer Multi-Fahrzeug-Nutzung gibt. Der Emulator-Modus
liefert den direkten Mehrwert: Werkstatt kann Standard-Diagnosegeraete
an den Bordcomputer anbinden.

---

## 7. Implementierungs-Plan

### Phase 1: Emulator (MVP)

1. `firmware/components/obd/` Verzeichnis anlegen
2. `obd.h` und `obd_emulator.c` implementieren
3. PID-Mapping-Tabelle mit 8 Standard-PIDs
4. GATT-Characteristics 0x5005-0x5008 in `gatt_uuids.h` definieren
5. BLE-Callbacks in `ble_service.c` erweitern
6. OBD-Emulator-Task (Prio 2, Core 0) starten
7. Companion-App: `ObdScreen.tsx` und binary parser
8. TabNavigator um OBD-Tab erweitern
9. Unit-Tests fuer PID-Encoder

### Phase 2: Reader (optional)

1. BLE-Central-Modus aktivieren (NimBLE)
2. ELM327-Discovery und Verbindung
3. AT-Kommando-Stack
4. PID-Query-Rotation
5. Fehlerbehandlung und Fallback

---

## 8. Referenzen

- SAE J1979: E/E Diagnostic Test Modes (PID-Definitionen)
- SAE J1962: OBD-II物理连接器
- ELM327 Datasheet (ELM Electronics): AT-Befehlssatz
- ISO 15765-4: CAN-Protokoll fuer OBD-II
- ISO 9141-2: K-Line-Diagnose (aeltere Fahrzeuge)
- Firmware: `firmware/components/ble/gatt_uuids.h` (GATT-Service-Definitionen)
- Firmware: `firmware/components/sensors/include/sensors.h` (sensor_data_t)
- Firmware: `firmware/main/board_config.h` (OBD-UART-Pins bereits definiert)
- Doku: `docs/research/12-diagnose-support.md` (bestehendes DTC-System)
- Doku: `docs/API-REFERENCE.md` (BLE-GATT-Interface-Referenz)
- Doku: `docs/research/01-sensoren-fahrzeugdaten.md` (Oeldruck-Sensor)

---

*Dieses Dokument evaluiert die OBD-II-Integration fuer den Transalp Bordcomputer.
Die Empfehlung ist ein stufenweiser Ausbau: Phase 1 Emulator (22h), Phase 2 Reader (23h, optional).*
