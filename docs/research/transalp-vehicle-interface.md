# Honda XL750 Transalp — Fahrzeug-Schnittstellen & Bordcomputer-Recherche

> Recherchegrundlage für den eigenen Bordcomputer (ESP32-S3).
> Modell: Honda XL750 Transalp (2024+), 755cc Unicam parallel twin, ride-by-wire, 6-Gang, HSTC, 2-Kanal-ABS.
> Stand: Oktober 2026

---

## 1. Fahrzeugdaten abgreifen (Read)

### 1.1 CAN-Bus (primäre Schnittstelle)

Die XL750 Transalp (2024+) nutzt **CAN-Bus** alongside einer **Body Control Unit (BCU)**. Das bestätigt Honda offiziell:

> "To simplify and miniaturise the entire electrical system the Transalp uses a Controller Area Network (CAN) alongside a Body Control Unit (BCU)."
> — [Honda Press Release 23YM XL750 Transalp](https://hondanews.eu/eu/en/motorcycles/media/pressreleases/421070/23ym-honda-xl750-transalp)

Details:
- **2 CAN-Busse** + 1 K-Line (bestätigt durch ADV Rider Forum / Veridian Cruise Install)
- **BCU** sitzt am linken Rahmen unter dem Tank, verarbeitet Signale von ABS-Modulator, TFT-Display und Schaltern
- CAN-Protokoll: ISO 15765-4 (CAN 11-bit, 500 kbps) — Standard für Honda ab ~2008
- Diagnoseport: **6-poliger Euro-5-Stecker** unter der Sitzbank (nicht J1962!)

**Diagnoseport-Position**: Unter der Sitzbank, neben Batterie und Sicherungsbox. Bei der Transalp als "Data Link Connector" oder "DLC" bezeichnet. Roter oder schwarzer 6-poliger Stecker mit Schutzkappe.

> Quelle: Honda Owners Manual XL750, Transalp Forum, Veridian Cruise Install Guide

#### Bekannte CAN-IDs (Transalp-spezifisch)

⚠️ Es gibt **keine öffentliche, vollständige CAN-ID-Liste** für die XL750 Transalp. Reverse Engineering ist erforderlich.

Aus ähnlichen Honda Motorrädern (Africa Twin, NC750X, CBR) bekannte CAN-IDs als Startpunkt:

| CAN-ID | Datenbreite | Inhalt | Quelle/Anmerkung |
|--------|-------------|--------|-------------------|
| 0x0C0 | 8 Byte | RPM (Byte 2-3, Big Endian, /256) | Honda CRF300L Rally, Africa Twin |
| 0x0C1 | 8 Byte | Kühlmitteltemperatur (Byte 0, -40 Offset) | Honda CRF300L Rally |
| 0x0C2 | 8 Byte | Geschwindigkeit (Byte 0-1, /100) | Africa Twin |
| 0x0C4 | 8 Byte | Gangposition, Neutral, Seitenständer | Honda Motorrad-Community |
| 0x0D0 | 8 Byte | Lenkwinkel, Bremshebel | Africa Twin |
| 0x0D2 | 8 Byte | HSTC-Status, Riding Mode | Africa Twin |
| 0x0E0 | 8 Byte | OBD-II Diagnostic | Honda Standard |
| 0x7E0 | 8 Byte | OBD-II ECU Request | ISO 15765 |
| 0x7E8 | 8 Byte | OBD-II ECU Response | ISO 15765 |
| 0x7DF | 8 Byte | OBD-II Broadcast Request | ISO 15765 |

> Diese IDs sind **Startwerte aus der Honda-Community** und müssen mit einem CAN-Sniffer am Fahrzeug validiert werden. Die XL750 wird abweichende IDs haben, aber im ähnlichen Bereich.

**Reverse-Engineering-Ansatz:**
1. CAN-Sniffer (ESP32 + SN65HVD230) an Diagnoseport klemmen
2. Alle IDs aufzeichnen (Passiv, Motor an, Leerlauf)
3. Aktionen ausführen (Drehzahl erhöhen, Gang schalten, Blinker, Bremse) und Veränderungen beobachten
4. IDs mit veränderten Bytes identifizieren
5. Wertebereich → physikalische Größe ableiten

### 1.2 OBD-II über CAN (ISO 15765-4)

Die Transalp unterstützt OBD-II über CAN (Pins 6 und 14 am Diagnoseport). Standard-PIDs sind über CAN zugänglich.

**Bestätigte OBD-II PIDs** (validiert an Honda CRF300L Rally via ELM327, [Moto_OBD2_Discovery](https://github.com/omadon/Moto_OBD2_Discovery)):

| Mode | PID | Beschreibung | Skalierung | Verfügbar? |
|------|-----|-------------|------------|------------|
| 01 | 00 | Unterstützte PIDs [01-20] | Bitmap | Ja |
| 01 | 03 | Fuel System Status | Flags | Ja |
| 01 | 04 | Calculated Engine Load | % × 100/255 | Ja |
| 01 | 05 | Engine Coolant Temperature | °C (Wert - 40) | Ja |
| 01 | 06 | Short Term Fuel Trim Bank 1 | % | Ja |
| 01 | 07 | Long Term Fuel Trim Bank 1 | % | Ja |
| 01 | 0B | Intake Manifold Pressure | kPa | Ja |
| 01 | 0C | Engine RPM | (A×256+B) / 4 | Ja |
| 01 | 0D | Vehicle Speed | km/h | Ja |
| 01 | 0E | Timing Advance | ° (vor TDC) | Ja |
| 01 | 0F | Intake Air Temperature | °C (Wert - 40) | Ja |
| 01 | 11 | Throttle Position | % × 100/255 | Ja |
| 01 | 1F | Run Time Since Engine Start | Sekunden | Ja |
| 01 | 20 | Unterstützte PIDs [21-40] | Bitmap | Ja |
| 01 | 40 | Unterstützte PIDs [41-60] | Bitmap | Ja |
| 01 | 42 | Control Module Voltage | V (×0.001) | Ja |
| 01 | 46 | Ambient Air Temperature | °C (Wert - 40) | Ja |
| 01 | 5C | Engine Oil Temperature | °C (Wert - 40) | Nicht an CRF300L (bei Transalp testen) |
| 03 | — | Read DTCs | — | Ja |
| 09 | 02 | VIN | 17 Zeichen | Ja |
| 09 | 04 | Calibration ID | String | Ja |
| 09 | 06 | CVN | — | Ja |

> Die Transalp sollte mindestens diese PIDs unterstützen, da sie Euro-5-konform ist und OBD-II über CAN pflicht ist. VIN-Lesung ist bestätigt (Euro-5 Fordert das).

### 1.3 K-Line (Honda PGM-FI Diagnose — sekundär)

Die Transalp hat **zusätzlich zur CAN eine K-Line** am Diagnoseport. Honda nutzt ein **proprietäres Protokoll über die K-Line** (nicht Standard OBD-II), das ELM327 nicht versteht.

**Protokoll-Details (Honda Keihin ECU K-Line):**

- Baudrate: **10400 bps** (nicht-standard, FDTI-Adapter nötig für PC)
- Physical: ISO 9141-2 Hardware (L9637D Transceiver kompatibel)
- Logisch: Proprietäres Honda-Protokoll (KEIHIN-K-Line)

**Init-Sequenz:**
```
1. K-Line LOW für 70ms (TX Pin auf LOW ziehen)
2. K-Line HIGH für 120ms (TX Pin freigeben)
3. Baudrate auf 10400 setzen
4. Wakeup senden:    FE 04 72 8C  (keine Antwort erwartet)
5. Warten 200ms
6. Init senden:      72 05 00 F0 99
7. ECU antwortet:    02 04 00 FA  → Verbindung steht
```

**Datenabfrage (Tabellen-basiert):**
```
Anfrage-Format:  72 [Länge] 71 [Tabelle] [Checksum]
oder Teilabfrage: 72 [Länge] 72 [Tabelle] [StartByte] [Anzahl] [Checksum]

Checksum = 0x100 - (Summe aller Bytes mod 0x100)
```

**Bekannte Honda-Datentabellen (K-Line):**

| Tabelle | Inhalt | Skalierung |
|---------|--------|------------|
| 0x00 | ECU-ID / Hardware-Info | ASCII |
| 0x11 | RPM, TPS, ECT, IAT, MAP, Speed, Batt | Siehe unten |
| 0x16 | RPM (2 Byte, Big Endian), Speed | RPM = (Byte4×256+Byte5) |
| 0xD0 | Fehlercodes (DTC Bit-Flags) | Bitmask |
| 0xD1 | Switch-States: Neutral, Seitenständer, Engine ON/OFF | Bitmask |

**Datentabelle 0x11 — Werte und Skalierung** (aus [Gonzo's CTX-OBD Project](https://gonzos.net/projects/ctx-obd/), [Honda Data Tables PDF](https://gonzos.net/projects/wp-content/uploads/2015/09/Honda-data-tables.pdf)):

| Offset | Wert | Skalierung |
|--------|------|------------|
| Byte 0-1 | RPM | 0000h → dec |
| Byte 2 | TPS Voltage | dec × 5/256 |
| Byte 4 | TPS % | dec |
| Byte 6 | ECT Voltage | dec × 5/256 |
| Byte 8 | ECT °C | dec - 40 |
| Byte 10 | IAT Voltage | dec × 5/256 |
| Byte 12 | IAT °C | dec - 40 |
| Byte 14 | MAP Voltage | dec × 5/256 |
| Byte 16 | MAP kPa | dec |
| Byte 18 | Speed | dec (km/h) |
| Byte 20 | Fuel Injector | 0000h → dec |

> ⚠️ Die K-Line-Tabellen variieren zwischen Honda-Modellen und Baujahren. Die XL750 Transalp (2024+) hat voraussichtlich eine andere ECU als die dokumentierten CRF250L/CTX700. Die Tabellen müssen am Fahrzeug validiert werden.

**Keep-Alive**: Die ECU schläft nach ~3 Sekunden ohne Anfrage ein. Regelmäßiges Polling erforderlich.

### 1.4 Sensoren direkt anzapfen (ohne OBD/CAN)

Falls OBD/CAN nicht ausreicht oder als Fallback:

| Signal | Methode | Aufwand | Risiko |
|--------|---------|---------|--------|
| Tacho-Signal (Speed) | Spannungsteiler am Tacho-Sensor (Reed/Hall) | Mittel | Niedrig |
| Drehzahl (Zündung) | Induktiv am Zündkabel oder Hall-Sensor | Mittel | Niedrig |
| Ganganzeige | Analoge Spannung am Gangschalter (falls vorhanden) | Niedrig | Niedrig |
| Blinker-Status | Parallel zum Blinkerrelais (Optokoppler) | Niedrig | Niedrig |
| Seitenständer | Parallel zum Seitenständerschalter | Niedrig | Niedrig |
| Bremshebel | Parallel zum Bremslichtschalter | Niedrig | Niedrig |
| Kühlwassertemperatur | PT100/NTC am Kühlflaschensensor | Mittel | Niedrig |
| Lambdasonde | Spannung abgreifen (0.1–0.9V) | Mittel | ⚠️ Eingriff in ECU-Sensorik |

> Direkte Sensoranzapfung ist zuverlässiger als CAN (kein Protokoll-Dependency), aber invasiv und potenziell garantieverletzend. Empfehlung: CAN/OBD bevorzugen, Direktabgriff nur für Signale die über CAN nicht verfügbar sind.

### 1.5 Woolich Racing — ECU Flashing & Logging

[Woolich Racing Tuned (WRT)](https://www.woolichracing.com) bietet ECU-Flashing für die XL750 (2022–2024) an. Das zeigt:

- Die ECU ist **les- und schreibbar** (Bin File Definitions verfügbar)
- Zugriff über K-Line (nicht CAN) mit proprietärem Log Box K v3 Interface
- Verfügbare Parameter: IAP/TPS Fuel Maps, Ignition Timing, ETV Maps, Diagnostics (Read/Clear DTCs)
- ECU-Teilenummern: 38770-MLC-A01/C01/D01/J11/T01

> Woolich ist die beste Quelle für Reverse-Engineering der ECU-Datenstrukturen. Das WRT-Log-Format kann als Referenz für eigene CAN/K-Line-Dekodierung dienen.

---

## 2. Signale senden / Steuerung (Write)

⚠️ **Sicherheitswarnung**: Eingriffe in fahrzeugrelevante Systeme können lebensgefährlich sein. ABS, Traktionskontrolle und Ride-by-Wire dürfen **nicht** manipuliert werden.

### 2.1 Was sicher per CAN lesbar/schreibbar ist

| Funktion | Lesen | Schreiben | Risiko |
|----------|-------|-----------|--------|
| DTCs (Fehlercodes) | ✅ | ✅ (Clear) | Niedrig |
| RPM, Speed, Temp | ✅ | ❌ | — |
| Blinker | ✅ (Status) | ⚠️ Möglich, aber nicht empfohlen | Mittel |
| Hupe | ❌ | ⚠️ Nur über Relais | Niedrig |
| Licht (Fernlicht) | ✅ (Status) | ⚠️ Über BCU, nicht direkt | Mittel |
| TFT-Display-Anzeigen | ❌ | ⚠️ CAN-Injection möglich | ⚠️ Hoch |

### 2.2 Engine-Management-Eingriff

⚠️ **Nicht empfohlen.** Die Transalp hat Ride-by-Wire (Electronic Throttle Valve). Eingriffe können zu unkontrollierter Gasannahme führen.

- ECU-Flashing (Woolich WRT) ist möglich, aber offline und nicht während der Fahrt
- CAN-Injection in das ECU während der Fahrt ist technisch möglich, aber **extrem gefährlich**
- WRT kann Fuel Maps, Ignition Timing, ETV Maps verändern — aber nur im Stillstand

### 2.3 ABS und Traktionskontrolle (HSTC)

- **HSTC (Honda Selectable Torque Control)**: 3 Level + OFF, über CAN steuerbar (Riding-Mode abhängig)
- **ABS**: 2-Kanal, hinteres ABS abschaltbar
- Beide Systeme sind sicherheitskritisch. Lesen des Status ist OK, Schreiben/Manipulieren ist **tabu**
- Die Transalp kann HSTC und hinteres ABS über die Bordtasten ausschalten — das ist der einzige sichere Weg

### 2.4 CAN-Injection in BCU (Body Control Unit)

Die BCU verarbeitet Schalter, ABS, TFT-Display. Theoretisch können CAN-Nachrichten an die BCU gesendet werden um:
- Blinker zu aktivieren (falls BCU das steuert)
- Licht zu schalten
- TFT-Display mit eigenen Inhalten zu versorgen

⚠️ Das erfordert vollständige Kenntnis der CAN-ID-Belegung. Falsche CAN-Nachrichten können die BCU verwirren oder im schlimmsten Fall Fahrsysteme beeinträchtigen. Erst nach vollständigem CAN-Reverse-Engineering durchführen.

### 2.5 Was man NICHT tun sollte

- Keine CAN-Nachrichten an das ABS-Modulator senden
- Keine CAN-Nachrichten an die ECU schicken, die Motorsteuerung betreffen
- Keine Ride-by-Wire-Throttle-Befehle injecten
- Keine Manipulation der HSTC-Werte
- Keine Relais parallel zu sicherheitskritischen Schaltern schalten (Bremslicht, Seitenständer, Kill-Switch)

---

## 3. Hardware-Schnittstellen & Controller

### 3.1 ELM327 (Bluetooth OBD-II Adapter)

**Funktionsweise**: ELM327 ist ein OBD-II-Protokoll-Interpreter-Chip, der mit der ECU spricht und über Bluetooth/Wifi/Serial eine AT-Befehlsschnittstelle bietet.

| Eigenschaft | ELM327 BT Classic | ELM327 BLE | ELM327 WiFi |
|-------------|-------------------|-------------|-------------|
| ESP32-Kompatibilität | ✅ BT Classic (SerialBT) | ⚠️ BLE komplexer | ✅ WiFi Client |
| Library | ELMduino (Arduino) | ELMduino (BLE-Modus) | ELMo / ELMduino |
| Preis | ~5-15€ | ~15-30€ | ~10-20€ |
| Reichweite | ~10m | ~10m | ~15m |
| Latenz | Mittel | Mittel | Niedrig |
| Stromaufnahme | ~30-50mA | ~20-30mA | ~50-80mA |

**Billig-Clones vs. Original:**
- Billig-Clones (ab 5€): Verwenden oft PIC18F25K80 statt echtem ELM327-Chip. Protokoll meist OK, aber unzuverlässig bei schnellem Polling
- Original ELM327 (ELM Electronics): ~25€, stabiler, unterstützt alle Protokolle
- vLinker FS: Bekannt gute Qualität mit Honda Motorrädern (Transalp-Forum empfiehlt ihn für DMD2-App)

**ESP32 + ELM327 Anbindung:**
- BT Classic: `BluetoothSerial` → `ELMduino` (einfachster Weg)
- BLE: Komplexer, aber ESP32-S3 unterstützt BLE nativ
- WiFi: ESP32 als WiFi-Client, ELM327 als AP

**Library**: [ELMduino](https://github.com/PowerBroker2/ELMduino) — Non-Blocking, unterstützt RPM, Temp, Speed, DTCs und alle Standard-PIDs

⚠️ **Wichtig**: ELM327 spricht **Standard OBD-II**. Das proprietäre Honda K-Line-Protokoll (Tabellen 0x11 etc.) funktioniert **nicht** mit ELM327. Für Honda-spezifische Daten braucht man direkte K-Line-Anbindung.

### 3.2 MCP2515 (CAN-Controller Shield, SPI)

**Beschreibung**: Externer CAN-Controller über SPI. Der ESP32 hat zwar einen eingebauten CAN-Controller (SJA1000-kompatibel), braucht aber immer einen Transceiver.

| Eigenschaft | MCP2515 |
|-------------|---------|
| Schnittstelle | SPI |
| Spannung | 5V (Logik) oder 3.3V je nach Modul |
| CAN-Controller | MCP2515 (Stand Alone) |
| Transceiver | TJA1050 oder SN65HVD230 (auf Modul) |
| Bibliothek | arduino-CAN (sandeepmistry),esp32-can |
| Preis | ~3-8€ |
| Geschwindigkeit | Bis 1 Mbps |
| Filter | Hardware-Filter möglich |

⚠️ Die meisten MCP2515-Module laufen mit 5V und TJA1050-Transceiver. Bei 3.3V-ESP32 sind Level-Shifter nötig. Besser: Module mit SN65HVD230 direkt.

**SPI-Pin-Belegung (ESP32-S3):**
```
MCP2515    ESP32-S3
─────────  ─────────
VCC  → 3.3V (Modul mit 3.3V-Transceiver!)
GND  → GND
CS   → GPIO 7 (frei, siehe PINMAP)
SCK  → GPIO 12 (shared SPI)
MISO→ GPIO 13 (shared SPI)
MOSI→ GPIO 11 (shared SPI)
INT  → GPIO 3 (optional, für RX-Interrupt)
```

**Bibliotheken:**
- [sandeepmistry/arduino-CAN](https://github.com/sandeepmistry/arduino-CAN) — Arduino-kompatibel, ESP32-native CAN support
- [collin80/esp32_can](https://github.com/collin80/esp32_can) — ESP32-spezifisch, TWAI-Controller

### 3.3 SN65HVD230 (CAN-Transceiver, 3.3V)

**Empfohlene Lösung für ESP32-S3.** Der ESP32-S3 hat einen eingebauten CAN-Controller (TWAI). Man braucht nur den Transceiver.

| Eigenschaft | SN65HVD230 |
|-------------|-----------|
| Spannung | 3.3V (direkt, kein Level-Shifter!) |
| CAN-Controller | Nicht nötig — ESP32 hat SJA1000-kompatiblen TWAI |
| Schnittstelle | Direkt an GPIO |
| Preis | ~2-5€ |
| Geschwindigkeit | Bis 1 Mbps |
| Stromaufnahme | ~5mA aktiv, <10µA Standby |

**Pin-Belegung (ESP32-S3, TWAI):**
```
SN65HVD230   ESP32-S3
───────────  ─────────
3V3      → 3.3V
GND      → GND
CTX      → GPIO 5  (TWAI TX, konfigurierbar)
CRX      → GPIO 4  (TWAI RX, konfigurierbar)
CANH     → Diagnoseport CAN-High
CANL     → Diagnoseport CAN-Low
```

> Bibliothek: [sandeepmistry/arduino-CAN](https://github.com/sandeepmistry/arduino-CAN) — unterstützt ESP32 direkt, `CAN.setPins(rx, tx)` zum Konfigurieren der Pins.

### 3.4 L9637D (K-Line Transceiver)

Für direkte Honda K-Line-Kommunikation (proprietäres PGM-FI Protokoll).

| Eigenschaft | L9637D |
|-------------|--------|
| Spannung | 5V (Input), 3.3V-kompatibel (Logik) |
| Protokoll | ISO 9141-2 (K-Line) |
| Library | Custom (Honda-Protokoll nicht Standard) |
| Preis | ~2-4€ |
| Baudrate | 10400 bps (Honda) |

**Schaltung:**
```
L9637D       ESP32-S3
───────      ─────────
VCC  → 5V
GND  → GND
TX   → GPIO 17 (UART TX)
RX   → GPIO 18 (UART RX)
K-Line → Diagnoseport K-Line Pin
```

> Referenz: [andreibaw/Honda_K-Line_KWP2000](https://github.com/andreibaw/Honda_K-Line_KWP2000) — ESP32-S3-Zero + L9637D + 510Ω Pull-up + 100nF Cap

### 3.5 Diagnoseport-Adapter

Die Transalp hat einen **6-poligen Euro-5-Stecker**, keinen J1962. Adapterkabel sind nötig:

| Adapter | Preis | Quelle |
|---------|-------|--------|
| 6-pol → J1962 (OBD-II) | ~10-15€ | Amazon, AliExpress |
| 6-pol → 4-pol (K-Line direkt) | Selbst gebaut | — |
| 3-in-1 Multi-Brand Adapter | ~15€ | Amazon |

> Transalp-Forum bestätigt: Cheap 6-pin Euro-5 Adapter funktionieren mit ELM327 + Torque Pro einwandfrei.

### 3.6 MCU-Vergleich

| Eigenschaft | ESP32-S3 | ESP32-C6 | RP2040 (Pico) |
|-------------|----------|----------|---------------|
| Architektur | Xtensa LX7 Dual-Core | RISC-V Single + LP-Core | ARM Cortex-M0+ Dual |
| Takt | 240 MHz | 160 MHz | 125 MHz |
| Flash/PSRAM | 16MB / 8MB | 4MB extern | 2MB extern |
| WiFi | 802.11 b/g/n | WiFi 6 | Nur Pico W |
| Bluetooth | BLE 5 | BLE 5 + Thread/Zigbee | BT Classic (Pico W) |
| USB | Native OTG | Native | Native |
| CAN/TWAI | ✅ Eingebaut | ✅ Eingebaut | ❌ (PIO bit-bang) |
| SPI | 4× | 2× | 2× |
| I2C | 2× | 2× | 2× |
| ADC | 14ch 12-bit | 7ch 12-bit | 4ch 12-bit |
| Preis (DevKit) | ~10-15€ | ~3-5€ | ~4€ |
| Arduino-Support | ✅ Sehr gut | ✅ Gut | ✅ Gut |
| ESP-IDF | ✅ Nativ | ✅ Nativ | ❌ |
| FPU | Ja (Vector Ext.) | Ja | ❌ (Software) |
| Deep Sleep | ~10µA | ~7µA | ~1.3mA |

**Empfehlung für dieses Projekt: ESP32-S3** (wie bereits im Repo gewählt)

Gründe:
- CAN/TWAI eingebaut → nur Transceiver nötig
- Bluetooth BLE für Smartphone-Anbindung
- PSRAM für Display-Framebuffer
- Dual-Core für parallele Tasks (CAN-Polling + Display + Logging)
- Bestes Bibliotheks-Ökosystem

Der ESP32-C6 ist interessanter für reine IoT/Smart-Home-Anwendungen, nicht für Fahrzeugtelemetrie. RP2040 hat kein CAN und keine Wireless-Funktionen.

---

## 4. Software-Stack

### 4.1 ESP-IDF vs. Arduino Framework

| Aspekt | ESP-IDF | Arduino |
|--------|---------|---------|
| Kontrolle | Vollzugriff auf alle Hardware-Features | Abstrahiert, eingeschränkt |
| FreeRTOS | Nativ, konfigurierbar | Versteckt, einfacher |
| CAN/TWAI | `twai_driver_install()` API | arduino-CAN (wrapper) |
| Performance | Optimal (direkte Register) | Etwas Overhead |
| Bibliotheken | Weniger, dafür mächtiger | Riesiges Ökosystem |
| Lernkurve | Steiler | Flacher |
| Debugging | JTAG, GDB, Heap-Tracing | Serial.print |
| Build-System | CMake/IDF | PlatformIO/Arduino IDE |
| OTA | IDF OTA API | ArduinoOTA |

**Empfehlung: ESP-IDF** (wie bereits im Repo gewählt)

Gründe:
- FreeRTOS-Task-Modell mit definierter Priorisierung (siehe ARCHITECTURE.md)
- TWAI-Treiber nativ konfigurierbar (Filter, Listen-Only-Mode für Sniffer)
- Bessere Kontrolle über SPI-Bus-Sharing (Display + SD)
- Heap-Tracing und Debugging für Robustheit
- OTA-Update über BLE sauberer implementierbar

### 4.2 OBD-II Bibliotheken

| Bibliothek | Framework | Protokoll | Link |
|------------|-----------|-----------|------|
| ELMduino | Arduino | ELM327 über BT/WiFi | [PowerBroker2/ELMduino](https://github.com/PowerBroker2/ELMduino) |
| ELMo | Arduino | ELM327 über WiFi | [rudydelorenzo/ELMo](https://github.com/rudydelorenzo/ELMo) |
| ELMulator | Arduino | ELM327 Emulator (für Tests) | [jimwhitelaw/ELMulator](https://github.com/jimwhitelaw/ELMulator) |
| arduino-CAN | Arduino | CAN direkt (SN65HVD230) | [sandeepmistry/arduino-CAN](https://github.com/sandeepmistry/arduino-CAN) |
| esp32_can | Arduino | ESP32 TWAI direkt | [collin80/esp32_can](https://github.com/collin80/esp32_can) |
| Keyword-Protocol-2000 | Arduino | KWP2000 (Honda K-Line) | [aster94/Keyword-Protocol-2000](https://github.com/aster94/Keyword-Protocol-2000) |

**Für ESP-IDF gibt es keine fertige OBD-II-Library.** Die OBD-II-Implementierung muss selbst geschrieben werden, was bei der ESP-IDF-TWAI-API aber straightforward ist:

```c
// TWAI initialisieren (Listen-Only für Sniffer, Normal für OBD)
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, TWAI_MODE_NORMAL);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

twai_driver_install(&g_config, &t_config, &f_config);
twai_start();

// OBD-II Request senden (Mode 01, PID 0C = RPM)
twai_message_t message;
message.identifier = 0x7DF;
message.data_length_code = 8;
message.data[0] = 0x02;  // Number of bytes
message.data[1] = 0x01;  // Mode: Show current data
message.data[2] = 0x0C;  // PID: RPM
message.data[3] = 0x00;
message.data[4] = 0x00;
message.data[5] = 0x00;
message.data[6] = 0x00;
message.data[7] = 0x00;
twai_transmit(&message, pdMS_TO_TICKS(100));
```

### 4.3 CAN-Bibliotheken

| Bibliothek | Framework | Plattform | Link |
|------------|-----------|-----------|------|
| arduino-CAN | Arduino | ESP32, Teensy, STM32 | [sandeepmistry/arduino-CAN](https://github.com/sandeepmistry/arduino-CAN) |
| esp32_can | Arduino | ESP32 spezifisch | [collin80/esp32_can](https://github.com/collin80/esp32_can) |
| FlexCAN_T4 | Arduino | Teensy 4.x | [tonton81/FlexCAN_T4](https://github.com/tonton81/FlexCAN_T4) |
| MT07_CAN_Library | Arduino | Yamaha MT-07 spezifisch | [jackhat1/MT07_CAN_Library](https://github.com/jackhat1/MT07_CAN_Library) |
| ESP-IDF TWAI | ESP-IDF | ESP32 nativ | `driver/twai.h` |

> Für ESP-IDF: Der native TWAI-Treiber ist die beste Wahl. Für Arduino-Framework: arduino-CAN als dünnere Wrapper.

### 4.4 NMEA-Parsing (GPS)

| Library | Sprache | Framework | Link |
|---------|---------|-----------|------|
| minmea | C | ESP-IDF / Generic | [kosma/minmea](https://github.com/kosma/minmea) |
| TinyGPS++ | C++ | Arduino | [mikalhart/TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus) |
| Custom | C | ESP-IDF | Eigene Implementierung |

**Empfehlung: minmea** für ESP-IDF. Klein, schnell, C-Kompatibel, BSD-lizenziert. Unterstützt RMC, GGA, GLL, RMC, VTG, GSA, GSV, GLL.

> Das Repo nutzt bereits NEO-M9N. Die u-blox-spezifische UBX-Erweiterung kann über minmea nicht geparsed werden. Für UBX (Konfiguration, höhere Update-Rate) ist eine eigene Parser-Komponente nötig. Die Firmware-Struktur hat bereits einen GPS-Component.

### 4.5 Datenformate

| Format | Zweck | Library / Tool |
|--------|-------|---------------|
| GPX | GPS-Tracks (Standard) | XML-Writer selbst gebaut |
| CSV | Telemetrie-Logging | fwrite / fprintf |
| JSON | Konfiguration, BLE-Transfer | cJSON (ESP-IDF) |
| Binary | Kompakte Telemetrie auf SD | Custom struct layout |
| FIT | Garmin-Standard (TBD) | FIT SDK (C++) |

> Das Repo hat bereits `tools/export_track.py` — Python-Skript für Track-Export. CSV-Format für Roh-Logging ist ausreichend, GPX für GPS-Tracks.

---

## 5. Konkrete Transalp-Infos

### 5.1 Modellbestätigung

**Honda XL750 Transalp** (2024+)
- Motor: 755cc Unicam SOHC, flüssigkeitsgekühlt, Parallel-Twin mit 270° Kurbelwelle
- Leistung: 67.5 kW (91 PS) @ 9.500 rpm
- Drehmoment: 75 Nm @ 7.250 rpm
- EFI: PGM-FI, 46mm Drosselklappen, Ride-by-Wire
- Transmission: 6-Gang, Quickshifter, Assist/Slipper-Kupplung
- Bremsen: 310mm Doppel-Scheibe vorne (ABS), 256mm hinten (ABS abschaltbar)
- Fahrwerk: Diamond Steel Frame, Showa SFF-CA 43mm USD Gabel
- Räder: 21" vorne (90/90-21), 18" hinten (150/70-18), Speichen
- Gewicht: 208 kg (fahrfertig)
- Tank: 16.9 L
- Ride-By-Wire: Ja (Electronic Throttle Valve)
- HSTC: 3 Level + OFF
- Riding Modes: Sport, Standard, Rain, Gravel, User
- Display: 5" TFT-Panel
- Licht: LED
- Batterie: 12V 9.1Ah AGM
- USB: Type-C unter der Sozius-Sitzbank
- Connectivity: Honda RoadSync

> Quellen: [Honda UK Specs](https://www.honda.co.uk/motorcycles/range/adventure/xl750_transalp/specifications-and-price.html), [Honda News EU](https://hondanews.eu/eu/en/motorcycles/media/pressreleases/421070/23ym-honda-xl750-transalp), [Motorcyclist First Look](https://www.motorcyclistonline.com/news/honda-xl750-transalp-first-look-preview)

### 5.2 OBD/Diagnoseport-Position

- **Position**: Unter der Sitzbank, zwischen Batterie und Sicherungsbox
- **Stecker**: 6-polig (Euro-5-Standard, nicht J1962/OBD-II)
- **Kappe**: Honda 91771-MKK-H11 (Ersatzteil)
- **Pinbelegung** (Euro-5 6-polig, Standard für Honda/Suzuki/Yamaha Euro-5):
  - Pin 1: +12V (ungenwitched)
  - Pin 2: CAN-High
  - Pin 3: CAN-Low
  - Pin 4: K-Line (ISO 9141)
  - Pin 5: Signal Ground
  - Pin 6: Chassis Ground

> ⚠️ Die genaue Pinbelegung muss am Fahrzeug mit Multimeter verifiziert werden. Verschiedene Quellen zeigen leicht abweichende Belegungen. Der Adapter von 6-polig auf J1962 standardisiert das.

### 5.3 Communities & Foren

| Community | URL | Relevanz |
|-----------|-----|----------|
| Transalp Owners Forum | [transalpowners.com](https://www.transalpowners.com) | Transalp-spezifisch, OBD-Port-Threads |
| ADV Rider | [advrider.com](https://www.advrider.com) | Adventure-Bike-Community, XL750 Thread |
| Honda Motopub (Owners Manual) | [webom.hondamotopub.com](https://webom.hondamotopub.com/webom/HMEE/MLC250/html/GSP002001.html) | Offizielles Handbuch |
| PGMFI Forum | [forum.pgmfi.org](http://forum.pgmfi.org/) | Honda ECU Reverse Engineering |
| Arduino Forum (Bike OBD) | [forum.arduino.cc](https://forum.arduino.cc/t/bike-interface-obd/229761) | Honda K-Line Implementation |

### 5.4 Verwandte Projekte

| Projekt | Beschreibung | Link |
|---------|-------------|------|
| Honda Motorcycle Dash (sophienyaa) | ESP32 + L9637D + ILI9341 TFT, liest ECU-Daten, GPS, DHT22 | [sophienyaa/Honda-Motorcycle-Dash](https://github.com/sophienyaa/Honda-Motorcycle-Dash) |
| Honda Motorcycle ECU Tools (mickwheelz) | ESP32 + K-Line, CRF250L, scannt ECU-Tabellen | [mickwheelz/Honda-Motorcycle-ECU-Tools](https://github.com/mickwheelz/Honda-Motorcycle-ECU-Tools) |
| Honda_K-Line_KWP2000 (andreibaw) | ESP32-S3-Zero + L9637D + OLED, CBR600RR, Web-Interface | [andreibaw/Honda_K-Line_KWP2000](https://github.com/andreibaw/Honda_K-Line_KWP2000) |
| HondaECU (roccofz1) | Windows-Tool, K-Line ECU Flash/Read, viele Honda-Modelle | [roccofz1/HondaECU](https://github.com/roccofz1/HondaECU) |
| Honda Keihin KLine Protocol | Dokumentation der K-Line-Init und Tabellen | [AutotronicCommunity/Honda_Keihin_KLine_Protocol](https://github.com/AutotronicCommunity/Honda_Keihin_KLine_Protocol) |
| Moto_OBD2_Discovery | OBD-II PID-Discovery für Honda CRF300L | [omadon/Moto_OBD2_Discovery](https://github.com/omadon/Moto_OBD2_Discovery) |
| ESP32 Motorcycle Dash (f1n41) | Fork von Honda-Motorcycle-Dash, gleicher Ansatz | [f1n41/ESP32-Motorcycle-Dash](https://github.com/f1n41/ESP32-Motorcycle-Dash) |
| Woolich Racing Tuned | Kommerziell, ECU-Flashing für XL750 | [woolichracing.com](https://www.woolichracing.com) |
| Veridian Cruise | Cruise-Control für Transalp, nutzt CAN/Diagnoseport | [veridiancruise.com](https://veridiancruise.com/knowledge-base/install-honda-xl750-transalp) |
| Awesome Automotive CAN-ID | CAN-ID-Sammlung für verschiedene Hersteller | [iDoka/awesome-automotive-can-id](https://github.com/iDoka/awesome-automotive-can-id) |

### 5.5 Erkenntnisse für den Bordcomputer

**Erste Schritte am Fahrzeug:**
1. CAN-Sniffer (ESP32 + SN65HVD230) an Diagnoseport, alle IDs aufzeichnen
2. OBD-II PIDs über CAN testen (Mode 01, PIDs aus Tabelle 1.2)
3. K-Line Init ausprobieren und Tabellen scannen (siehe scan.ino von mickwheelz)
4. DTC-Abfrage testen (Mode 03)
5. VIN auslesen (Mode 09 PID 02)

**Architektur-Empfehlung:**
- **Primary**: CAN/TWAI direkt (SN65HVD230) für OBD-II-PIDs und CAN-Sniffing
- **Secondary**: L9637D K-Line für Honda-spezifische Daten (Tabellen 0x11, 0x16, 0xD1)
- **Fallback**: ELM327 BT als Zwischenlösung, falls CAN direkt zu komplex wird
- **E-Paper-Display**: ideal für Sonnenablesbarkeit am Motorrad (0mA statisch)
- **SD-Logging**: GPX für Tracks, CSV für Telemetrie mit Timestamps

**Hardware-Empfehlung:**
1. SN65HVD230 CAN-Transceiver (3.3V, direkt an ESP32-S3 TWAI)
2. L9637D K-Line Transceiver (für Honda PGM-FI)
3. 6-pol Euro-5 → breakout-Adapter (selbst gelötet oder Amazon)
4. Optional: ELM327 BT Classic als Backup/Referenz

---

## Quellen

### Offizielle Honda-Dokumente
- [Honda News EU — 23YM XL750 Transalp Features](https://hondanews.eu/eu/en/motorcycles/media/pressreleases/421070/23ym-honda-xl750-transalp)
- [Honda UK — Transalp Specifications](https://www.honda.co.uk/motorcycles/range/adventure/xl750_transalp/specifications-and-price.html)
- [Honda Owners Manual XL750](https://webom.hondamotopub.com/webom/HMEE/MLC250/html/GSP002001.html)
- [Honda OBD-II Pinout (PinoutGuide)](https://pinoutguide.com/CarElectronics/honda_obd_2_pinout.shtml)

### Community & Foren
- [Transalp Owners — OBD Port Adapter](https://www.transalpowners.com/threads/odb-port-adapter.1667/)
- [Transalp Owners — Diagnostic Port Splitter](https://www.transalpowners.com/threads/diagnostic-port-splitter.1581/)
- [ADV Rider — Veridian Cruise Transalp](https://www.advrider.com/f/threads/veridian-cruise-for-the-transalp-xl750.1687083)
- [Arduino Forum — Bike Interface OBD](https://forum.arduino.cc/t/bike-interface-obd/229761)

### GitHub-Projekte
- [sophienyaa/Honda-Motorcycle-Dash](https://github.com/sophienyaa/Honda-Motorcycle-Dash) — ESP32 Honda Dashboard
- [mickwheelz/Honda-Motorcycle-ECU-Tools](https://github.com/mickwheelz/Honda-Motorcycle-ECU-Tools) — K-Line Scanner
- [andreibaw/Honda_K-Line_KWP2000](https://github.com/andreibaw/Honda_K-Line_KWP2000) — ESP32-S3 K-Line Interface
- [roccofz1/HondaECU](https://github.com/roccofz1/HondaECU) — Honda ECU Reverse Engineering
- [AutotronicCommunity/Honda_Keihin_KLine_Protocol](https://github.com/AutotronicCommunity/Honda_Keihin_KLine_Protocol) — K-Line Protocol Docs
- [omadon/Moto_OBD2_Discovery](https://github.com/omadon/Moto_OBD2_Discovery) — OBD-II PID Discovery
- [PowerBroker2/ELMduino](https://github.com/PowerBroker2/ELMduino) — Arduino ELM327 Library
- [sandeepmistry/arduino-CAN](https://github.com/sandeepmistry/arduino-CAN) — Arduino CAN Library
- [aster94/Keyword-Protocol-2000](https://github.com/aster94/Keyword-Protocol-2000) — KWP2000 Library
- [kosma/minmea](https://github.com/kosma/minmea) — NMEA Parser
- [iDoka/awesome-automotive-can-id](https://github.com/iDoka/awesome-automotive-can-id) — CAN-ID Sammlung

### Hardware & Tools
- [Woolich Racing Tuned — XL750 ECU Flashing](https://www.woolichracing.com)
- [Veridian Cruise — XL750 Install](https://veridiancruise.com/knowledge-base/install-honda-xl750-transalp)
- [Gonzo's CTX-OBD Project](https://gonzos.net/projects/ctx-obd/)
- [Honda Data Tables PDF (Gonzo)](https://gonzos.net/projects/wp-content/uploads/2015/09/Honda-data-tables.pdf)
- [Honda DTC Codes PDF](https://downloads.laboratoryb.org/insight/other_manuals/HONDA_DTC.pdf)
