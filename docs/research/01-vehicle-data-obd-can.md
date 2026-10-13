# Recherche: OBD-II, CAN-Bus & Fahrzeugdaten (Honda XL750 Transalp)

> Stand: 13.10.2026  
> Status: Recherche abgeschlossen, ready for implementation  
> Sicherheitshinweise mit ⚠️ markiert

---

## 1. Zusammenfassung

Die Honda XL750 Transalp (2023+, Euro 5) hat **zwei CAN-Busse** und einen **6-Pin ISO 19689 Diag-Port** unter der Sitzbank. Der externe Diag-Port bietet CAN (OBD-II-kompatibel) und K-Line. Über CAN-OBD-II lassen sich Drehzahl und Geschwindigkeit auslesen, aber **keine Gang- oder Bremsinformationen**. Der interne CAN-Bus hat mehr Daten (Gang, Radgeschwindigkeiten, RPM), ist aber ohne Stecker-Hack nicht zugänglich.

| Interface | Protokoll | Daten | Zugriff |
|---|---|---|---|
| Diag-Port (6-Pin, extern) | CAN 500 kbps, 11-bit (ISO 15765-4) | RPM, Geschwindigkeit, DTCs, Kühlmitteltemp (Standard PIDs) | Plug & Play, ELM327-kompatibel |
| Diag-Port (6-Pin, extern) | K-Line (Honda KWP2000-Variante) | Volle PGM-FI-Tabellen (RPM, TPS, ECT, IAT, MAP, Batt, Speed, Gang, Kickstand) | Honda-spezifisch, kein Standard-OBD |
| Interner CAN-Bus | CAN (BCU-Bus) | Radgeschwindigkeiten, Gang, RPM, Bremsinfo | ⚠️ Kabel anzapfen nötig |

---

## 2. OBD-II

### 2.1 Protokoll

Die XL750 Transalp ist Euro 5 (seit 2020 Pflicht für neue Typgenehmigungen). Euro 5 Motorräder nutzen den **ISO 19689 6-Pin Diag-Connector** und kommunizieren über:

- **ISO 15765-4 (CAN 11-bit, 500 kbps)** — Standard OBD-II über CAN, primär für emissionsrelevante Daten
- **K-Line (ISO 14230-4 / KWP2000)** — Honda-spezifische Variante für volle ECU-Tabellen

Honda Motorräder nutzen **nicht** das Standard ISO 9141-2 K-Line Protokoll. Honda hat eine eigene KWP2000-Variante mit proprietärer Initialisierung und eigenem Checksum-Verfahren.

Quellen:
- ISO 19689 Standard: https://www.iso.org/standard/46045.html (Diag-Port Definition)
- AiM InfoTech Euro 5 Dokument: https://www.aim-sportline.com/download/ecu/stock/Bike_euro5_std_eng_100.pdf
- AdvRider Forum (Veridian Cruise): https://advrider.com/f/threads/veridian-cruise-for-the-transalp-xl750.1687083

### 2.2 OBD-II Stecker-Position

Der **rote 6-Pin JST-Connector** befindet sich **unter der Sitzbank**, zwischen Batterie und Sicherungskasten. Er hat eine schwarze Schutzkappe.

Bei der XL750 Transalp ist der Stecker laut Veridian-Installationsanleitung "between battery and fuse box" unter der Sitzbank.

Quellen:
- Veridian Cruise Install Guide: https://veridiancruise.com/knowledge-base/install-honda-xl750-transalp
- YouTube (Heated Grips Install, zeigt Diag-Port): https://www.youtube.com/watch?v=Xgvi2gAxDE8

### 2.3 6-Pin Diag-Port Pinout (ISO 19689)

| Pin | Funktion | Kabelfarbe (typisch Honda) |
|---|---|---|
| 1 | — (nicht belegt) | — |
| 2 | CAN High (CAN+) | Weiß |
| 3 | Ground (GND) | Grün |
| 4 | Battery Voltage (12V) | Schwarz/Weiß |
| 5 | CAN Low (CAN-) | Blau |
| 6 | K-Line | Orange/Weiß |

Quelle: AiM InfoTech Euro 5 Standard Protocol Dokument

### 2.4 Standard OBD-II PIDs (Mode 01, CAN-basiert)

Über den CAN-OBD-II-Port sind folgende **Standard-SAE-J1979-PIDs** erreichbar (getestet mit ELM327 + Torque Pro an Euro-5-Hondas):

| PID (hex) | Beschreibung | Formel | Datenbytes |
|---|---|---|---|
| 0x0C | Motordrehzahl (RPM) | (256A+B)/4 | 2 |
| 0x05 | Kühlmitteltemperatur (ECT) | A-40 | 1 |
| 0x0D | Fahrzeuggeschwindigkeit | A | 1 |
| 0x0F | Ansauglufttemperatur (IAT) | A-40 | 1 |
| 0x11 | Drosselklappenposition (TPS) | 100A/255 | 1 |
| 0x42 | Bordspannung | 256A/1000+B/10000 | 2 |
| 0x46 | Außentemperatur | A-40 | 1 |
| 0x33 | Lambda-Sonde Bank 1 | Siehe J1979 | 2 |

**Funktioniert nicht über Standard-OBD-II:**
- Gangposition (herstellerspezifisch)
- Brems-Status (nicht über OBD)
- Radgeschwindigkeiten (nur interner CAN)
- Kickstand-Status

Quellen:
- Wikipedia OBD-II PIDs: https://en.wikipedia.org/wiki/OBD-II_PIDs
- SimmaSoftware ISO 15765-4 Guide: https://simmasoftware.com/iso-15765-4-code-the-complete-guide-to-obd-II-over-can/
- TransalpOwners Forum (ELM327 + Torque Pro bestätigt): https://www.transalpowners.com/threads/odb-port-adapter.1667/

### 2.5 DTC-Auslese über CAN-OBD-II

| Mode | Funktion | CAN-ID Request | CAN-ID Response |
|---|---|---|---|
| 03 | Stored DTCs lesen | 0x7DF | 0x7E8 |
| 04 | DTCs löschen | 0x7DF | 0x7E8 |
| 07 | Pending DTCs | 0x7DF | 0x7E8 |
| 09 PID 02 | VIN auslesen | 0x7DF (Multi-Frame) | 0x7E8 |
| 01 PID 00 | Unterstützte PIDs abfragen | 0x7DF | 0x7E8 |

CAN-OBD-II Format:
- Request: `0x7DF` (Broadcast), 8 Bytes: `[Anzahl Bytes, Mode, PID, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC]`
- Response: `0x7E8` (ECU), 8 Bytes: `[Anzahl Bytes, Mode+0x40, PID, Daten..., 0xCC...]`

Beispiel RPM-Request:
```
TX: 7DF#02 01 0C CC CC CC CC CC
RX: 7E8#04 41 0C 1A F0 CC CC CC   → RPM = (0x1A*256 + 0xF0)/4 = 1728 RPM
```

### 2.6 Herstellerspezifische PIDs (Honda Mode 22)

Honda nutzt **Mode 22** (enhanced PIDs) für erweiterte Daten. Diese sind nicht standardisiert und müssen pro Modell reverse-engineered werden. Bekannte Honda Mode 22 PIDs (aus dem Automobil-Bereich):

| Mode+PID | Beschreibung | Formel |
|---|---|---|
| 22 2201 | Getriebeöl-Temperatur (ATF) | A*(9/5)-40 |
| 22 2202 | Motoröltemperatur | A-40 |

Für die XL750 Transalp sind diese PIDs **nicht dokumentiert** und müssen durch Sniffing ermittelt werden.

⚠️ Mode 22 PIDs sind modellspezifisch und können zwischen Baujahren variieren. Ungetestete PIDs können fehlerhafte Werte liefern.

---

## 3. CAN-Bus

### 3.1 CAN-Bus-Architektur der XL750 Transalp

Honda bestätigt offiziell (Pressemitteilung 23YM):

> "To simplify and miniaturize the entire electrical system the Transalp uses a Controller Area Network (CAN) alongside a Body Control Unit (BCU). The BCU is sited on the left-hand frame rail, under the fuel tank and collectively processes control signals from the ABS modulator, TFT screen and switch gear."

**Zwei CAN-Busse:**

1. **Externer/Diagnostic CAN** — Über den 6-Pin Diag-Port zugänglich. Bietet OBD-II-Protokoll (ISO 15765-4). Liefert RPM und Geschwindigkeit.
2. **Interner CAN (BCU-Bus)** — Verbindet ECU, BCU, ABS-Modulator, TFT-Display, Schalter. Liefert Radgeschwindigkeiten, Gang, RPM, Brems-Info. Kein freier Stecker — Kabel muss angezapft werden.

CAN-Spezifikation: **CAN 2.0A (11-bit Standard), 500 kbps** (Euro 5 Standard für OBD-II über CAN).

Quellen:
- Honda Pressemitteilung: https://hondanews.eu/eu/en/motorcycles/media/pressreleases/421070/23ym-honda-xl750-transalp
- AdvRider (2 CAN-Busse bestätigt): https://advrider.com/f/threads/veridian-cruise-for-the-transalp-xl750.1687083

### 3.2 Bekannte CAN-IDs für Honda Motorräder

Es gibt **keine öffentliche DBC-Datei** für Honda Motorräder (weder XL750 noch andere Honda-Bikes). OpenDBC (commaai) enthält nur Honda-Automobile (Civic, Accord, CRV etc.), nicht Motorräder.

Die folgenden CAN-IDs sind aus Community-Reverse-Engineering von Honda Motorrädern und verwandten Plattformen bekannt:

#### KTM 790/890 Duke (zum Vergleich, aus RaceChrono-Forum)

Diese IDs stammen von einer KTM, nicht von Honda. Sie zeigen aber das typische Muster für Euro-5-Motorräder mit ähnlicher ECU-Architektur. Die XL750 wird ähnliche, aber nicht identische IDs haben.

| CAN-ID | Beschreibung | Byte-Layout | Skalierung |
|---|---|---|---|
| 0x120 | RPM + Modus | data[0:2] = RPM | (data[0]<<8 \| data[1]) |
| 0x129 | Gang | data[0] | direkt |
| 0x12A | TPS | data[0] | /2.55 = % |
| 0x12B | Radgeschw + IMU | data[0:2]=V_rear, data[2:4]=V_front, data[4]=Lean, data[5]=Pitch | /10 für km/h, -128 für deg |
| 0x290 | Bremsdruck + ABS | data[0:2]=Front, data[2:4]=Rear | /10 für bar |
| 0x450 | Traktionskontrolle | data[0] | >0 = aktiv |
| 0x540 | Temperaturen | data[0] | -40 für °C |

Quelle: RaceChrono Forum (ESP32 CAN-Bus DIY): https://racechrono.com/forum/d/2712-simply-diy-esp32-can-bus

#### Honda Civic (Auto, aus OpenDBC, zum Vergleich)

Die OpenDBC Honda-Auto-Dateien enthalten folgende relevante Messages:

| CAN-ID | Message | Signale |
|---|---|---|
| 0x191 (401) | GEARBOX | GEAR_SHIFTER, GEAR2 |
| 0x1A3 (419) | GEARBOX_ALT | GEAR |
| 0x1B0 (432) | STANDSTILL | WHEELS_MOVING, BRAKE_ERROR |
| 0x1BE (446) | BRAKE_MODULE | BRAKE_PRESSED |
| 0x1C8 (456) | ACC_CONTROL | ACCEL_COMMAND, STANDSTILL |
| 0x158 (344) | STEERING_CONTROL | STEER_TORQUE |
| 0x1A0 (416) | WHEEL_SPEEDS | FL, FR, RL, RR |

Quelle: OpenDBC: https://github.com/commaai/opendbc

⚠️ Diese IDs gelten für Honda-Autos, nicht für die XL750. Sie dienen als Referenz für das erwartete Datenmodell.

### 3.3 Reverse-Engineering der XL750 CAN-IDs

Da keine DBC-Datei für die XL750 existiert, müssen die CAN-IDs selbst ermittelt werden. Vorgehen:

1. **CAN-Sniffer am Diag-Port** (CAN H/L, 500 kbps) — liefert OBD-II-Daten und eventuell Broadcast-Messages
2. **CAN-Sniffer am internen Bus** (Kabel anzapfen) — liefert alle BCU-Messages
3. **Methode**: Bestimmte Aktion ausführen (Motor hochdrehen, Gang wechseln, bremsen) und die CAN-IDs mit sich ändernden Werten korrelieren

Tools dafür:
- **cabana** (comma.ai) — Web-Tool zum Visualisieren und Decodieren von CAN-Logs: https://community.comma.ai/cabana/
- **CANdevStudio** — CAN-Simulator und Analyzer: https://github.com/GENIVI/CANdevStudio
- **SavvyCAN** — Open Source CAN-Analyzer
- **python-can** — Python-Bibliothek für CAN-Kommunikation

Community-Quellen für CAN-IDs:
- awesome-automotive-can-id: https://github.com/iDoka/awesome-automotive-can-id (keine Honda-Motorräder, aber Referenz)
- Africa Twin Forum (ESP32 CAN-Sniff): https://africatwinforum.com/threads/got-rid-of-the-notorious-warning-screen.50216 — jemand hat mit ESP32 die CRF1100-Dash-Messages gesnifft

### 3.4 BCU (Body Control Unit)

Die BCU sitzt an der linken Hauptrahmenschiene unter dem Tank. Sie verarbeitet:
- ABS-Modulator-Signale
- TFT-Display-Daten
- Schalter-Signale (Lenker)
- Beleuchtung (LED)
- Turn-Signal-Canceller (nutzt Radgeschwindigkeitsdifferenz)

Die BCU ist vermutlich über den internen CAN-Bus mit der ECU verbunden. Für den Bordcomputer ist der **Diag-Port-CAN** der einfachste und sicherste Zugang.

---

## 4. K-Line / KWP2000 (Honda PGM-FI)

### 4.1 Honda K-Line Protokoll

Honda Motorräder nutzen ein proprietäres K-Line-Protokoll, das auf KWP2000 (ISO 14230) basiert, aber nicht standardkonform ist. Unterschiede zu Standard-OBD:

- **Baudrate**: 10400 baud (half-duplex)
- **Keine Standard-ISO-9141-Initialisierung** — Honda hat eine eigene Wakeup-Sequenz
- **Eigener Checksum-Algorithmus**: `Checksum = 0x100 - (Summe aller Bytes mod 0x100)`
- **Eigene Request/Response-Struktur**: Tabellen-basiert (nicht PID-basiert)

### 4.2 K-Line Initialisierung

```
1. K-Line auf Low ziehen für 70ms
2. K-Line auf High für 120ms
3. Wakeup senden: FE 04 FF FF (keine Response erwartet)
4. 200ms warten
5. Initialisierung senden: 72 05 00 F0 99
6. ECU antwortet: 02 04 00 FA
```

Danach kann man Requests senden. Die Verbindung muss alle ~3 Sekunden am Leben gehalten werden (Keep-Alive), sonst geht die ECU schlafen.

### 4.3 Honda ECU Daten-Tabellen

Die ECU liefert Daten über Tabellen-Nummern. Request-Format:

```
72 05 71 <TABLE_ID> <CHECKSUM>
```

| Table ID | Inhalt |
|---|---|
| 0x00 / 0x60 | Basis-Daten (RPM, TPS, ECT, IAT, MAP, Batt, Speed) |
| 0x10 / 0x61 | Erweiterte Sensordaten (gleicher Inhalt, andere Adressierung) |
| 0x11 / 0xD1 | Status (Gang, Neutral, Kickstand, Engine-Status) |

#### Table 0x10 — Sensordaten (22 Bytes Response)

| Offset | Größe | Beschreibung | Formel |
|---|---|---|---|
| 0 | 1 | Sender (0x02 = ECU) | — |
| 1 | 1 | Length | — |
| 2 | 1 | Cmd (0x71 = Read Table) | — |
| 3 | 1 | Table ID (0x10) | — |
| 4-5 | 2 | **RPM** | (Hi<<8 \| Lo) — direkt in RPM |
| 6 | 1 | **TPS Voltage** | Wert * 5/256 |
| 7 | 1 | **TPS %** | Direkt (0-121) |
| 8 | 1 | **ECT Voltage** | Wert * 5/256 |
| 9 | 1 | **ECT °C** | Wert - 40 |
| 10 | 1 | **IAT Voltage** | Wert * 5/256 |
| 11 | 1 | **IAT °C** | Wert - 40 |
| 12 | 1 | **MAP Voltage** | Wert * 5/256 |
| 13 | 1 | **MAP kPa** | Direkt |
| 14-15 | 2 | unbekannt | — |
| 16 | 1 | **Batterie Voltage** | Wert / 10 |
| 17 | 1 | **Geschwindigkeit km/h** | Direkt |
| 18-19 | 2 | mögliche Kraftstoffdaten | — |
| 20 | 1 | unbekannt | — |
| 21 | 1 | Checksum | 0x100 - Sum mod 0x100 |

#### Table 0xD1 — Status (10 Bytes Response)

| Offset | Beschreibung | Werte |
|---|---|---|
| 4 | Gang-Status | 0x01=Neutral/Kupplung, 0x00=im Gang, 0x03=Seitenständer draußen |
| 8 | Motor-Status | 0x01=läuft, 0x00=aus |

Quellen:
- andreibaw/Honda_K-Line_KWP2000: https://github.com/andreibaw/Honda_K-Line_KWP2000
- atoid/Dash (Honda ECU Tables): https://github.com/atoid/Dash
- gonzos.net/projects/ctx-obd: https://gonzos.net/projects/ctx-obd/
- Honda Data Tables PDF: https://gonzos.net/projects/wp-content/uploads/2015/09/Honda-data-tables.pdf
- HondaECU (GitHub): https://github.com/AlexKovalevich/HondaECU

### 4.4 K-Line Hardware

Für die K-Line-Kommunikation wird ein **einfacher Optokoppler-Adapter** benötigt:

```
K-Line → Optokoppler 1 Input
12V    → 1kΩ Widerstand → Optokoppler 1 LED
K-Line → Optokoppler 2
GND    → Optokoppler 2 Output
```

Verwendbare Optokoppler: PC817, 817C (Standard, billig)

Alternativ: **FTDI-basierter USB-Serial-Adapter** mit K-Line-Pegelwandler. Der HondaECU-GitHub-Repo zeigt eine Schaltung mit Optokopplern für galvanische Trennung.

⚠️ Die K-Line-Pegel liegen bei 12V. Direkter Anschluss an ESP32 (3.3V) **zerstört den Pin**. Immer Optokoppler oder Pegelwandler verwenden.

### 4.5 Diag-Port 4-Pin (ältere Hondas)

Ältere Honda Motorräder (vor Euro 5) haben einen **roten 4-Pin Sumitomo HM090 Stecker**:

| Pin | Funktion | Farbe |
|---|---|---|
| 1 | 12V | Schwarz/Weiß |
| 2 | GND | Grün |
| 3 | K-Line | Orange/Weiß |
| 4 | Reset (SCS) | Braun |

Die XL750 Transalp hat den 6-Pin ISO 19689 Connector, nicht den 4-Pin. Aber die K-Line-Logik ist bei beiden gleich.

---

## 5. CAN-Transceiver für ESP32

### 5.1 Vergleich

| Feature | MCP2515 (SPI) | SN65HVD230 | ESP32-S3 TWAI (intern) |
|---|---|---|---|
| Typ | Externer CAN-Controller + Transceiver | Nur Transceiver (PHY) | Nur Controller (MAC) |
| Spannung | 5V (mit TJA1050) | 3.3V | 3.3V |
| Interface | SPI | Direkte GPIO (TX/RX) | Intern (TWAI Peripheral) |
| CAN-FD | Nein (MCP2515FD ja) | Nein | Nein (nur Classic CAN) |
| Kosten | ~4€ (Modul) | ~2€ (Breakout) | 0€ (integriert) |
| Bibliothek | mcp_can (Arduino), ESP-IDF driver | ESP-IDF TWAI driver | ESP-IDF TWAI driver |
| GPIO-Bedarf | 4 Pins (MOSI, MISO, SCK, CS) + optional INT | 2 Pins (TX, RX) | 2 Pins (TX, RX) |
| Bus-Off Recovery | Software | Über TWAI | Hardware-supportet |

### 5.2 Empfehlung: ESP32-S3 TWAI + SN65HVD230

Für den Transalp-Bordcomputer ist diese Kombination die beste Wahl:

1. **ESP32-S3 hat integrierten TWAI-Controller** (CAN 2.0, 11-bit und 29-bit)
2. **SN65HVD230 ist 3.3V-nativ** — kein Level-Shifter nötig
3. **Minimaler GPIO-Bedarf**: Nur 2 Pins (TX + RX) für CAN
4. **Kein SPI-Overhead** — TWAI ist hardware-basiert
5. **Listen-Only-Mode verfügbar** — sicher für passives Sniffing

Schaltung:

```
ESP32-S3 GPIO 5 (TWAI TX) ──→ SN65HVD230 Pin 1 (D)
ESP32-S3 GPIO 4 (TWAI RX) ←── SN65HVD230 Pin 4 (R)
3.3V ─────────────────────────→ SN65HVD230 Pin 3 (VCC)
GND ──────────────────────────→ SN65HVD230 Pin 2 (GND)
SN65HVD230 Pin 6 (CANL) ──────→ Diag-Port Pin 5 (CAN Low)
SN65HVD230 Pin 7 (CANH) ──────→ Diag-Port Pin 2 (CAN High)
SN65HVD230 Pin 8 (RS) ────────→ GND (oder 10kΩ nach GND für Normal-Mode)
```

⚠️ **Bus-Terminierung**: Der CAN-Bus braucht zwei 120Ω Terminierungswiderstände, einen an jedem Ende. Der Diag-Port hat wahrscheinlich schon einen (ECU-seitig). Wenn ein SN65HVD230-Modul mit onboard-Terminator verwendet wird, prüfen ob ein zweiter am anderen Bus-Ende existiert. Bei nur zwei Knoten reicht ein einzelner 120Ω oft aus.

### 5.3 ESP-IDF TWAI Beispiel-Code

```c
#include "driver/twai.h"

// TWAI Konfiguration für 500 kbps, Listen-Only
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    GPIO_NUM_5,    // TX
    GPIO_NUM_4,    // RX
    TWAI_MODE_LISTEN_ONLY  // nur lesen, nicht senden
);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

twai_driver_install(&g_config, &t_config, &f_config);
twai_start();

// Message empfangen
twai_message_t message;
while (twai_receive(&message, portMAX_DELAY) == ESP_OK) {
    // message.identifier enthält die CAN-ID
    // message.data[0..message.data_length_code-1] enthält die Daten
    printf("CAN ID: 0x%03X  Data: ", message.identifier);
    for (int i = 0; i < message.data_length_code; i++) {
        printf("%02X ", message.data[i]);
    }
    printf("\n");
}
```

Quellen:
- ESP32 TWAI + SN65HVD230 Komponente: https://github.com/JonasAtelier/esp-sn65hvd230-can
- CAN Bus for Makers (ESP32 TWAI Guide): https://1c3d.com/howtos/can-bus-for-makers-esp32-twai-mcp2515
- ESP-IDF CAN Multi-Backend: https://github.com/idf-can-bus/can-multibackend-idf
- ElectricalFlux MCP2515 vs SN65HVD230: https://electricalflux.com/mcu-projects/can-bus-arduino-mcp2515-wiring-code

### 5.4 Alternative: MCP2515 (falls TWAI belegt)

Wenn die TWAI-Pins bereits für andere Funktionen belegt sind, kann der MCP2515 über SPI nachgerüstet werden. Der MCP2515 benötigt einen separaten Transceiver (MCP2551 für 5V oder TJA1050). Bei 5V-Betrieb sind Level-Shifter für die SPI-Anbindung an den 3.3V ESP32 nötig.

---

## 6. Implementierungs-Strategie für den Bordcomputer

### 6.1 Phase 1: OBD-II über CAN (Standard)

Einfachster Weg, schnell implementierbar:

- ESP32-S3 TWAI + SN65HVD230
- 6-Pin auf Dupont-Adapterkabel (oder fertiges 6-Pin-auf-OBD2-Kabel kaufen)
- Standard Mode 01 PIDs über CAN-ID 0x7DF
- Drehzahl, Geschwindigkeit, Kühlmitteltemp, IAT, TPS, Bordspannung

Vorteile: Standardisiert, ELM327-kompatibel, keine proprietäre Honda-Logik
Nachteile: Kein Gang, kein Brems-Status, keine Radgeschwindigkeiten

### 6.2 Phase 2: K-Line PGM-FI Tabellen

Zusätzlich zum CAN-OBD-II die K-Line anzapfen:

- Optokoppler-Schaltung zwischen K-Line (Pin 6) und ESP32 UART
- Honda-Initialisierungssequenz implementieren
- Tabellen 0x10 und 0xD1 abfragen
- Liefert: RPM, TPS, ECT, IAT, MAP, Batt, Speed, Gang, Kickstand

Vorteile: Gang-Position, Kickstand-Status, MAP (Saugrohrdruck)
Nachteile: Proprietäres Protokoll, komplexere Implementierung, 10.4 kbaud ist langsam

### 6.3 Phase 3: Interner CAN-Bus (optional)

Wenn mehr Daten gebraucht werden (Radgeschwindigkeiten, Brems-Info, Lean-Angle vom IMU):

- ⚠️ Erfordert Anzapfen des internen CAN-Bus-Kabels unter dem Tank
- Kann-Arbitrierung: Nur Listen-Only, niemals Messages auf den internen Bus senden
- CAN-IDs müssen gesnifft und reverse-engineered werden
- Höheres Risiko (Eingriff ins Fahrzeug-Elektronik-System)

⚠️ **Sicherheitswarnung**: Der interne CAN-Bus ist sicherheitskritisch. Er verbindet ABS, BCU und ECU. Ein fehlerhaft gesendeter CAN-Frame kann ABS oder BCU stören. **Nur Listen-Only-Mode verwenden.** Niemals CAN-Frames aktiv auf den internen Bus senden.

---

## 7. Adapter-Kabel

### 7.1 6-Pin ISO 19689 auf OBD2 16-Pin

Fertige Adapterkabel sind verfügbar (Amazon, eBay, ~10-15€):

- Suchbegriff: "Euro 5 6 pin motorcycle OBD2 adapter"
- Bekannte Marken: Autel, FXTUL, SuperOBD, OBD-AUS
- Kompatibel mit ELM327 Bluetooth/WiFi-Adaptern und vLinker FS

Diese Adapter unterstützen CAN-Bus und K-Line-Verbindung.

### 7.2 Eigenbau 6-Pin auf Dupont

Für den direkten ESP32-Anschluss ohne OBD2-Umweg:

| 6-Pin (Bike) | Funktion | ESP32-S3 |
|---|---|---|
| 2 | CAN High | SN65HVD230 CANH (Pin 7) |
| 3 | GND | GND |
| 4 | 12V | Step-Down → 5V/3.3V |
| 5 | CAN Low | SN65HVD230 CANL (Pin 6) |
| 6 | K-Line | Optokoppler-Schaltung |

⚠️ Pin 4 liefert 12V Batteriespannung. Direkt an ESP32 = zerstört. Step-Down-Converter (z.B. LM2596 oder MP1584) auf 5V, dann ESP32-Bordregler auf 3.3V.

---

## 8. Quellenverzeichnis

### XL750 Transalp spezifisch
- Honda Pressemitteilung XL750 Transalp (CAN + BCU bestätigt): https://hondanews.eu/eu/en/motorcycles/media/pressreleases/421070/23ym-honda-xl750-transalp
- Honda Global XL750 Technology: https://global.honda/en/tech/XL750_TRANSALP/
- AdvRider Veridian Cruise Thread (2 CAN-Busse, Diag-Port Info): https://advrider.com/f/threads/veridian-cruise-for-the-transalp-xl750.1687083
- Veridian Cruise Install Guide (Diag-Port Position): https://veridiancruise.com/knowledge-base/install-honda-xl750-transalp
- TransalpOwners OBD Port Adapter (ELM327 + Torque Pro bestätigt): https://www.transalpowners.com/threads/odb-port-adapter.1667/
- YouTube Diag-Port Position: https://www.youtube.com/watch?v=Xgvi2gAxDE8
- HEX ezCAN für Honda (CAN-Bus Accessory Manager): https://www.hexinnovate.com/hex-ezcan-for-honda/

### OBD-II Standard
- Wikipedia OBD-II PIDs: https://en.wikipedia.org/wiki/OBD-II_PIDs
- SimmaSoftware ISO 15765-4 Guide: https://simmasoftware.com/iso-15765-4-code-the-complete-guide-to-obd-II-over-can/
- CSS Electronics OBD2 PID Overview: https://www.csselectronics.com/pages/obd2-pid-table-on-board-diagnostics-j1979
- ISO 19689 (Motorcycle Diag Connector): https://www.iso.org/standard/46045.html
- AiM InfoTech Euro 5 Protocol (Pinout): https://www.aim-sportline.com/download/ecu/stock/Bike_euro5_std_eng_100.pdf

### Honda K-Line / PGM-FI
- andreibaw/Honda_K-Line_KWP2000 (ESP32 K-Line Interface): https://github.com/andreibaw/Honda_K-Line_KWP2000
- atoid/Dash (Honda ECU Tables 0x11, 0xD1): https://github.com/atoid/Dash
- atoid/DashBle (BLE ECU Interface): https://github.com/atoid/dashble
- AlexKovalevich/HondaECU (Diagnostic + Flashing Tool): https://github.com/AlexKovalevich/HondaECU
- gonzos.net/projects/ctx-obd (K-Line Anleitung): https://gonzos.net/projects/ctx-obd/
- Honda Data Tables PDF: https://gonzos.net/projects/wp-content/uploads/2015/09/Honda-data-tables.pdf
- AiM Honda CBR PGM-Fi (HDS Protocol): https://aimsportsystems.com.au/download/ecu/stock/honda/HondaCBR-RR_600_2003-2005+CBR-RR1000_2004-2015_100_eng.pdf

### CAN-Bus Reverse Engineering
- OpenDBC (comma.ai): https://github.com/commaai/opendbc
- awesome-automotive-can-id: https://github.com/iDoka/awesome-automotive-can-id
- CSS Electronics CAN Bus Sniffer Guide: https://www.csselectronics.com/pages/can-bus-sniffer-reverse-engineering
- RaceChrono ESP32 CAN DIY (KTM CAN-IDs): https://racechrono.com/forum/d/2712-simply-diy-esp32-can-bus
- Africa Twin Forum ESP32 CAN-Sniff: https://africatwinforum.com/threads/got-rid-of-the-notorious-warning-screen.50216
- cabana (CAN Analyzer): https://community.comma.ai/cabana/
- CANdevStudio: https://github.com/GENIVI/CANdevStudio

### ESP32 CAN-Transceiver
- ESP32 SN65HVD230 CAN Component: https://github.com/JonasAtelier/esp-sn65hvd230-can
- CAN Bus for Makers Guide: https://1c3d.com/howtos/can-bus-for-makers-esp32-twai-mcp2515
- ESP-IDF CAN Multi-Backend: https://github.com/idf-can-bus/can-multibackend-idf
- ElectricalFlux MCP2515 vs SN65HVD230: https://electricalflux.com/mcu-projects/can-bus-arduino-mcp2515-wiring-code
- Honda OBD DTC Codes: https://obdcode.org/honda

### Kaggle Dataset
- Honda Africa Twin CRF1000L OBD Data: https://www.kaggle.com/datasets/nikitasshvetsov/honda-africa-twin-crf1000l-obd-data
