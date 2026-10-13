# Recherche: Hardware-Module, Controller & Software-Stack

> Stand: Oktober 2026 — Recherche-Grundlage für die finale Hardware-Auswahl.
> Alle Preise sind Richtwerte (AliExpress / Amazon / DigiKey, Stand 2026).

## 0. Kritischer Befund vorab

Das bestehende `HARDWARE.md` plant **ELM327 Bluetooth (pairing mit ESP32 BT-Classic)**.
Das funktioniert nicht: Der **ESP32-S3 hat kein Bluetooth Classic**, nur BLE 5.0.
Die originale ELM327-Spezifikation benötigt Serial Port Profile (SPP), das ausschließlich
über Bluetooth Classic läuft. Es gibt drei Lösungswege:

1. **BLE-ELM327-Dongle** (z.B. Vgate iCar Pro BLE, OBDLink CX) + ESP32-S3 über BLE
2. **Direkter CAN-Zugriff** über TWAI + Transceiver (kein ELM327 nötig)
3. **WiFi-ELM327-Dongle** (z.B. Kiwi 4 WiFi) + ESP32-S3 über WLAN

Empfehlung: **Weg 2 (TWAI + SN65HVD230)** — direkter CAN-Zugriff ohne ELM327-Umweg,
niedrigere Latenz, weniger Störanfälligkeit, kein zusätzlicher Dongle.

## 1. ELM327 — Varianten & Kompatibilität

### Varianten-Übersicht

| Variante | Transport | ESP32-S3 kompatibel? | Bemerkung |
|---|---|---|---|
| ELM327 v1.5 BT Classic (SPP) | BT Classic SPP | Nein (kein BT Classic auf S3) | Standard-Dongle, funktioniert nur mit ESP32 (classic), nicht S3 |
| ELM327 v2.1 BT Classic | BT Classic SPP | Nein | Häufig Clones, instabil auf CAN |
| ELM327 BLE (z.B. Vgate iCar Pro BLE) | BLE 4.0+ | Ja, mit BLE-Bibliothek | Braucht BLE-Serial-Wrapper, s. `obd-ble-serial` |
| ELM327 WiFi (z.B. Kiwi 4 WiFi) | WLAN TCP | Ja, über WiFi | Höherer Stromverbrauch am OBD-Port |
| ELM327 USB (v2.2 original) | USB | Ja, über USB-OTG | Nur für Bench-Testing |
| OBDLink CX (STN1110) | BLE | Ja, nativ BLE | Bessere Performance als ELM327-Clones |

### Original vs. Clone

- **Original ELM327 v1.5+** (ELM Electronics): volle Protokollunterstützung (CAN, KWP2000, ISO 9141, J1850), firmware-updatebar
- **Billig-Clones** ($5-15 AliExpress): meist CAN-only, fehlende KWP2000/UDS-Unterstützung, instabil bei Kaltstart, keine Firmware-Updates
- **STN1110/STN1170** (OBDLink): ELM327-kompatibel aber schneller, bessere Protokollabdeckung, firmware-updatebar
- Für Motorrad relevant: viele ältere Motorräder (vor 2008) nutzen K-Line (ISO 9141), nicht CAN — Clone ohne KWP2000-Support fällt hier durch

### Für das Projekt

Wenn ELM327-Weg: **OBDLink CX** (BLE, ~$40-50) als bester Kompromiss.
Aber: **TWAI + Transceiver ist die bessere Route** für ein fest verbautes Bordcomputer.
ELM327 nur sinnvoll für Bench-Testing oder temporäre Diagnose am OBD-Port.

### BLE-Serial für ELM327 auf ESP32-S3

GitHub: `vdvornichenko/obd-ble-serial`
- Wrappt BLE-Verbindung als Serial-Interface für ELMduino
- Nutzt Service UUID `FFF0`, RX `FFF1` (Notify), TX `FFF2` (Write)
- Funktioniert mit OBDLink CX und meisten chinesischen BLE-Dongles

## 2. CAN-Anbindung — TWAI, MCP2515 & Transceiver

### 2.1 ESP32-S3 TWAI (eingebauter CAN-Controller)

Der ESP32-S3 hat **1 TWAI-Controller** (Two-Wire Automotive Interface), das ist Espressifs
Implementierung von CAN 2.0B (ISO 11898-1).

- Unterstützt Standard (11-bit) und Extended (29-bit) Identifier
- Bis 1 Mbit/s
- 1 TX-Mailbox, 1 RX-Buffer + 64-Frame Acceptance Filter
- Register-Modell SJA1000-kompatibel
- **KEIN integrierter Transceiver** — braucht zwingend einen externen
- TX/RX über GPIO-Matrix auf beliebige Pins routingbar
- Nicht kompatibel mit CAN FD (interpretiert FD-Frames als Fehler)

Quelle: ESP-IDF v5.4.4 Docs — `esp-idf/en/v5.4.4/esp32s3/api-reference/peripherals/twai.html`

### 2.2 SN65HVD230 — CAN-Transceiver (empfohlen)

Der **SN65HVD230** von Texas Instruments ist der kanonische Transceiver für den ESP32-S3:

| Eigenschaft | Wert |
|---|---|
| Versorgung | 3.3V (nicht 5V!) |
| Standard | ISO 11898-2 High-Speed CAN |
| Datenrate | bis 1 Mbps |
| Standby-Strom | 370 µA |
| ESD-Schutz | ±16 kV (HBM) auf Bus-Pins |
| Bus-Spannungsbereich | -2V bis +7V |
| Modi | High-Speed (RS→GND), Slope-Control (RS über Widerstand), Standby (RS→VCC) |

**Vorteile gegenüber TJA1050/MCP2551:**
- 3.3V direkt, kein Level-Shifter nötig (TJA1050 braucht 5V + Level-Shift)
- Pin-kompatibel mit VP230 (Clone-Alternative)
- Niedriger Standby-Strom für Sleep-Modus

**Pin-Belegung SN65HVD230 Modul (CJMCU-230):**

| Modul-Pin | Funktion | ESP32-S3 |
|---|---|---|
| 3V3 | VCC | 3.3V |
| GND | Ground | GND |
| CTX | TXD (Transceiver-Input) | GPIO 5 (TWAI TX) |
| CRX | RXD (Transceiver-Output) | GPIO 4 (TWAI RX) |
| CANH | CAN High | OBD-II Pin 6 |
| CANL | CAN Low | OBD-II Pin 14 |

Wichtig: 120Ω Terminierung an beiden Bus-Enden nicht vergessen.
Viele Breakout-Module haben einen Lötjumper für den 120Ω-Widerstand.

### 2.3 MCP2515 — externer CAN-Controller über SPI

Der MCP2515 ist ein eigenständiger CAN-Controller, der über SPI angesteuert wird.
Auf dem ESP32-S3 **nur nötig, wenn man eine zweite CAN-Schnittstelle braucht**
(z.B. getrennter High-Speed-CAN und Low-Speed-CAN).

| Eigenschaft | Wert |
|---|---|
| Interface | SPI (bis 10 MHz) |
| CAN-Standard | 2.0B (11/29-bit) |
| Datenrate | bis 1 Mbps |
| Versorgung | 5V (meist auf Modul), 3.3V SPI-Pins kompatibel |
| Buffer | 3 TX-Buffer, 2 RX-Buffer |

**Module:** Standard "MCP2515 CAN Bus Shield" (mit TJA1050 oder MCP2551 Transceiver)
- Achtung: viele Module haben 5V-Transceiver → Level-Shifter für ESP32 SPI-Pins
- Besser: Module mit SN65HVD230-Transceiver (direkt 3.3V kompatibel)

**SPI-Pin-Belegung (Vorschlag für zweiten Bus):**

| Signal | ESP32-S3 GPIO |
|---|---|
| SPI SCK | GPIO 12 |
| SPI MISO | GPIO 13 |
| SPI MOSI | GPIO 11 |
| MCP2515 CS | GPIO 7 (reserve) |
| MCP2515 INT | GPIO 6 (reserve) |

**Bibliotheken:**
- `pierremolinaro/acan2515` — ACAN2515, MIT-Lizenz, Arduino-kompatibel, beste Wahl
- `autowp/arduino-mcp2515` — alternative Bibliothek, ebenfalls SPI
- `Microver-Electronics/mcp2515-esp32-idf` — ESP-IDF native, wenn ohne Arduino-Core

### 2.4 TJA1050 — 5V CAN-Transceiver

- 5V-Versorgung, nicht direkt ESP32-S3 kompatibel
- Braucht Level-Shifter auf TX-Leitung
- Älter, höherer Stromverbrauch
- **Nicht empfohlen** — SN65HVD230 ist die bessere Wahl

### 2.5 Fazit CAN

| Ansatz | Komponenten | Kosten | Empfehlung |
|---|---|---|---|
| TWAI + SN65HVD230 | 1 Modul ($2-3) | ++ | **Erste Wahl** — direkt, 3.3V, minimal |
| TWAI + TJA1050 | 1 Modul + Level-Shifter | + | Nur wenn SN65HVD230 nicht verfügbar |
| MCP2515 + Transceiver | 1 Shield ($5-8) | + | Nur für zweiten CAN-Bus |
| ELM327 BT | Dongle ($10-40) | - | Nur für Bench-Testing |

## 3. K-Line Adapter (ISO 9141 / ISO 14230)

Für ältere Motorräder (vor ~2008) ohne CAN-Bus läuft die Diagnose über K-Line.
K-Line ist halbduplex UART bei 10400 Baud, 0/12V Pegel.

### 3.1 Transistor-basierte Schaltung (Low-Cost)

Die einfachste Lösung: ein NPN-Transistor (z.B. BC547) in Open-Collector-Schaltung
mit Pull-Up auf 12V. TX-Invertierung im ESP32 UART einstellen (ESP32-S3 unterstützt das).

Referenz: `muki01/OBD2_K-line_Reader` auf GitHub — enthält Schaltpläne für 3.3V und 5V MCUs.
- R6 = 3.3kΩ für 3.3V-MCUs (wie ESP32-S3)
- R6 = 5.3kΩ für 5V-MCUs (Arduino Nano)
- Kompatibel mit ISO 9141 und ISO 14230 (KWP slow/fast init)

### 3.2 Dedizierte K-Line ICs

Für production-grade Qualität:
- **L9637D** — K-Line Transceiver, automotive-grade
- **MCZ33290** — ISO 9141 K-Line Interface
- **Si9241** — K-Line Driver

Diese ICs haben eingebaute Level-Shifting und Schutzschaltungen.
Empfohlen für dauerhaften Einsatz am Motorrad.

### 3.3 Init-Sequenz

Slow-Init: 200ms LOW, 400ms HIGH/LOW/HIGH/LOW, 227ms HIGH auf K-Line.
Auto antwortet mit 3 Bytes (z.B. `0x55 0xEF 0x8F`).
Fast-Init: K-Line 25ms LOW + 25ms HIGH, dann sofort KWP2000-Request.

Achtung: 5-Baud-Init (slow) am besten über GPIO, nicht UART — die meisten
UARTs können nicht sauber 5 Baud erzeugen. ESP32-S3 kann GPIO-Timing per Software.

### 3.4 MAX3232

MAX3232 ist kein K-Line-IC sondern ein RS-232-Transceiver (±12V).
Für K-Line nicht direkt geeignet, wird aber in einigen Hobby-Schaltungen
als Pegelwandler zweckentfremdet. Nicht empfohlen — besser Transistor oder L9637D.

## 4. GPS-Module — Vergleich

| Modul | Chip | Konstellationen | max. Rate | Genauigkeit | Schnittstelle | Preis |
|---|---|---|---|---|---|---|
| NEO-M9N | u-blox M9 | GPS+GLO+GAL+BDS (4 simultan) | 25 Hz | ~1.5 m CEP | UART/SPI/I2C/USB | $28-42 |
| NEO-M8N | u-blox M8 | GPS+GLO+BDS (3 simultan) | 10 Hz | ~2.5 m CEP | UART/I2C | $18-25 |
| NEO-6M | u-blox 6 | GPS only | 5 Hz (1 Hz default) | ~2.5 m CEP | UART | $3-8 |
| ATGM336H | AT6558 | GPS+BDS+GLO+GAL+QZSS | 10 Hz | ~1.5 m CEP | UART/I2C/SPI | $2-5 |
| HGLRC M100 | u-blox M10 | GPS+GAL+BDS+GLO | 10 Hz | ~1.5 m CEP | UART/I2C | $15-20 |

### Bewertung für Bordcomputer

**NEO-M9N (empfohlen):**
- 4 Konstellationen simultan → mehr Satelliten, bessere Abdeckung bei Bäumen/Gebäuden
- 25 Hz Update-Rate → bei 100 km/h alle 1.1m ein Update
- Konfigurierbar über UBX-Protokoll (u-center)
- SparkFun-Breakout mit Qwiic (I2C) verfügbar
- Etwas teurer, aber der Performance-Sprung rechtfertigt es

**ATGM336H (Budget-Alternative):**
- Überraschend gut: 4 Konstellationen, 1.5m CEP, 10 Hz
- Preis unschlagbar (~$3)
- NMEA 0183 Output, 9600 Baud → direkt mit TinyGPS++ parsebar
- Kein UBX-Protokoll → Konfiguration über CASIC (chinesisch dokumentiert)
- u-center funktioniert nicht

**NEO-6M (nicht empfohlen):**
- Veraltet, GPS-only, 1 Hz default
- Viele Clones auf dem Markt mit recycelten Chips
- Nur für Lernzwecke

### Konfiguration NEO-M9N

- UART Baudrate: 38400 (default), auf 115200 hochsetzen für 10Hz+ NMEA-Output
- UBX-Config über u-center oder `SparkFun_u-blox_GNSS_Arduino_Library`
- PPS-Pin: 1 Puls pro Sekunde, für Zeitsynchronisation des Loggers
- Antenne: Active Patch 25x25mm mit Magnetfuß, 3m Kabel, IPEX/U.FL Connector

## 5. E-Paper Displays

### 5.1 Größen-Vergleich

| Display | Auflösung | Treiber-IC | Partial-Refresh | Preis | Bemerkung |
|---|---|---|---|---|---|
| Waveshare 1.54" b/w | 200x200 | SSD1681 | Ja | $8-12 | Kompakt, gut ablesbar |
| Waveshare 2.9" b/w | 128x296 | SSD1680/SSD1608 | Ja | $12-18 | Breit, gut für Tacho-Layout |
| Waveshare 2.9" b/w V2 | 128x296 | GDEM029T94 | Ja (eingeschränkt) | $13-19 | Neuere Variante |
| Waveshare 2.9" b/w 168x384 | 168x384 | SSD1685 | Ja | $15-20 | Höhere Auflösung |
| Waveshare 1.54" b/w/r | 200x200 | SSD1681 | Nein (full only) | $12-16 | 3-Farben, langsamer |
| Waveshare 2.9" b/w/y | 128x296 | SSD1680 | Nein | $14-18 | Schwarz/Gelb, 3-Farben |

### 5.2 Bewertung für Motorrad-Bordcomputer

**Waveshare 2.9" b/w (empfohlen):**
- 128x296 Pixel, querformat möglich (296x128)
- Partial-Refresh in ~0.3s für Live-Daten (Geschwindigkeit, Drehzahl)
- Full-Refresh in ~3s für Layout-Wechsel
- Sonnenlesbar, ~0mA statisch
- SPI-Anschluss, 3.3V direkt kompatibel mit ESP32-S3
- GxEPD2 unterstützt alle gängigen Panel-Varianten

**Waveshare 1.54" b/w (Alternative):**
- 200x200 Pixel, quadratisch
- Partial-Refresh in ~0.3s
- Kompakter, passt in kleineres Gehäuse
- Weniger Platz für Daten →.Layout muss kompakter sein

### 5.3 Panel-Varianten erkennen

Waveshare liefert keine zuverlässige Panel-Bezeichnung auf den Modulen.
GxEPD2 hat eine Auswahlliste (`GxEPD2_display_selection_new_style.h`),
man muss anhand des Flex-Connector-Aufdrucks raten.
Wichtig: "V2" auf der Waveshare-Verpackung bedeutet nicht zwingend ein anderes Panel.

### 5.4 GxEPD2 Bibliothek

GitHub: `ZinggJM/GxEPD2`
- Unterstützt alle Waveshare- und Good-Display-Panels
- Basiert auf Adafruit_GFX
- Partial-Refresh für schnelle Updates
- Multi-Display-Support (verschiedene CS-Pins pro Display)
- Getestet mit ESP32, ESP32-S3, ESP8266, STM32, AVR

**Wiring (Standard ESP32-S3, shared SPI mit SD-Karte):**

| E-Paper Pin | ESP32-S3 GPIO | Bemerkung |
|---|---|---|
| BUSY | GPIO 47 | IRQ, Refresh-Status |
| RST | GPIO 21 | Reset |
| DC | GPIO 14 | Data/Command |
| CS | GPIO 10 | Chip Select (Display) |
| CLK | GPIO 12 | SPI SCK (shared) |
| DIN | GPIO 11 | SPI MOSI (shared) |
| GND | GND | |
| 3.3V | 3.3V | |

## 6. Power-Management

### 6.1 Bestehende Planung (HARDWARE.md)

Die aktuelle Planung: LiPo 18650 → TP4056 (Laden) → MT3608 Boost → 5V → HT7333 LDO → 3.3V

### 6.2 Probleme der aktuellen Planung

- **HT7333**: max. 250mA — ESP32-S3 braucht beim WiFi/BLE-Peak bis zu 500mA → Brownout-Gefahr
- **MT3608**: Effizienz fällt unter 80% bei Lasten <50mA → Verschwendung bei Low-Power
- **Doppelkonvertierung**: Boost 3.7→5V, dann LDO 5→3.3V = zwei Verluste statt einer Stufe
- **Kein Power-Path-Management**: TP4056 kann nicht gleichzeitig laden und Last versorgen (sauber)

### 6.3 Alternative Bauteile

| Bauteil | Typ | Input | Output | Effizienz | Max. Strom | Preis | Bemerkung |
|---|---|---|---|---|---|---|---|
| TP4056 | LiPo-Lader | USB 5V | 4.2V | — | 1A | $0.30 | Standard, kein Power-Path |
| IP5306 | Lader+Boost+Path | USB 5V | 5V | ~90% | 2A | $2-4 | All-in-one, auto-off bei Light-Load (Problem!) |
| MT3608 | Boost | 2-24V | 5-28V | 80-93% | 2A | $0.50 | Effizienz fällt bei <50mA |
| MP1584 | Buck | 4.5-28V | 0.8-20V | ~92% | 3A | $0.80 | Kompakt, effizient |
| HT7333 | LDO 3.3V | bis 6V | 3.3V | ~50% (lineaer) | 250mA | $0.20 | Zu schwach für ESP32-S3 Peaks |
| AP2112K-3.3 | LDO 3.3V | bis 6V | 3.3V | ~55% | 600mA | $0.30 | Besser als HT7333, genug für ESP32-S3 |
| ME6211 | LDO 3.3V | bis 6V | 3.3V | ~55% | 500mA | $0.25 | Auf vielen SuperMini-Boards |
| TPS63000 | Buck-Boost | 1.8-5.5V | 1.2-5.5V adj. | bis 96% | 1.8A | $3-5 | Beste Lösung, teuer |
| TPS61022 | Boost | 0.5-5.5V | 5V | bis 95% | 2A | $2-3 | Sehr effizient bei Low-Voltage |

### 6.4 Empfohlene Power-Architektur

**Option A: LiPo-Betrieb (Bordakku, unabhängig vom Motorrad)**
```
LiPo 3.7V ──┬── TP4056 (Laden via USB-C)
            │
            ├── TPS63000 Buck-Boost → 3.3V direkt ──┬── ESP32-S3
            │                                        ├── MPU6050
            │                                        ├── BMP280
            │                                        ├── NEO-M9N
            │                                        ├── SD-Karte
            │                                        └── SN65HVD230
            │
            └── Spannungsteiler → ADC (Akku-Überwachung)
```
- TPS63000: 1 Stufe statt 2, 96% Effizienz, buck-boost für Entladung unter 3.3V
- Direkt 3.3V, kein 5V-Zwischenschritt nötig
- E-Paper braucht 3.3V, SN65HVD230 braucht 3.3V → alles auf einer Schiene

**Option B: Motorrad-Bordnetz (12V)**
```
12V Batterie ── MP1584 Buck → 3.3V direkt ──┬── ESP32-S3 (+ alle 3.3V Module)
                                             └── E-Paper
```
- Motorrad liefert 12V bei laufendem Motor (13.5-14.4V)
- MP1584 als effizienter Step-Down auf 3.3V
- LiPo als UPS/Puffer über TP4056 + Diode-OR
- Bei Motor aus: automatische Umschaltung auf LiPo

### 6.5 IP5306 Hinweis

Der IP5306 kombiniert Lader, Boost und Power-Path in einem Chip.
Problem: viele IP5306-Module schalten bei Light-Load (<45mA) automatisch ab.
Das ist für ESP32-Deep-Sleep tödlich. Nur Module ohne Auto-Off verwenden,
oder den EN-Pin per ESP32-GPIO kontrollieren.

## 7. MCU-Vergleich

### 7.1 Übersicht

| Eigenschaft | ESP32-S3 | ESP32-C6 | RP2040 (Pico) | Teensy 4.0 |
|---|---|---|---|---|
| CPU | Dual Xtensa LX7 | RISC-V (HP+LP) | Dual ARM Cortex-M0+ | ARM Cortex-M7 |
| Takt | 240 MHz | 160 MHz | 133 MHz | 600 MHz |
| CoreMark | ~1182 (dual) | ~497 | ~550 (dual) | ~3000+ |
| SRAM | 512 KB | 512 KB | 264 KB | 1 MB |
| PSRAM | bis 8 MB (octal) | Nein | Extern möglich | Extern möglich |
| Flash | bis 16 MB | 4 MB | 2 MB | 2 MB |
| WiFi | 4 (2.4GHz) | 6 (2.4GHz) | Nein (Pico W: ja) | Nein |
| BLE | 5.0 | 5.3 | Nein (Pico W: ja) | Nein |
| BT Classic | Nein | Nein | Nein | Nein |
| USB | OTG FS | Serial/JTAG | Device | Device/Host |
| CAN/TWAI | 1x TWAI | 1x TWAI | Nein | 3x FlexCAN |
| GPIO | 45 | 30 | 26 | 40 |
| Vector/SIMD | Ja | Nein | Nein | Ja (FPU) |
| Thread/Zigbee | Nein | Ja (802.15.4) | Nein | Nein |
| ADC | 2x 12-bit | 1x 12-bit | 4x 12-bit | 2x 12-bit (slow) |
| Preis | $3-5 | $2-3 | $4 | $20+ |

### 7.2 Bewertung für das Projekt

**ESP32-S3 (empfohlen):**
- Bereits im Projekt gewählt, alle Pins gemappt
- 1 TWAI-Controller direkt verfügbar → CAN ohne externe Bauteile (außer Transceiver)
- BLE 5.0 für Smartphone-Anbindung
- Genug GPIO (45) für alle Module
- PSRAM für Display-Buffer und SD-Puffer
- SIMD für eventuelle DSP-Aufgaben (FFT für Vibrationsanalyse)

**ESP32-C6:**
- WiFi 6 + Thread/Zigbee interessant für Smart-Home, nicht für Bordcomputer
- Nur 1 Core (HP) → schlechter für parallele Tasks (CAN + Display + GPS)
- Kein PSRAM → Display-Buffer problematisch bei großen E-Paper
- Vorteil: LP-Core für Sensor-Polling im Sleep
- Fazit: nicht geeignet, S3 ist die bessere Wahl

**RP2040 (Pico):**
- Kein WiFi/BLE nativ (Pico W hat WiFi+BLE, aber nur single-core)
- Kein CAN/TWAI → braucht MCP2515 über SPI (extra Hardware)
- Keine Analog-Peripherals (ADC schlecht)
- Vorteil: sehr billig, gut dokumentiert
- Fazit: nicht geeignet, zu viele Bauteile nötig

**Teensy 4.0:**
- Mit Abstand die meiste Rechenleistung (600MHz M7)
- 3x FlexCAN → bester CAN-Support
- Aber: kein WiFi/BLE, teuer ($20+), geschlossenes Ökosystem
- Fazit: überdimensioniert, BLE müsste extern ergänzt werden

### 7.3 Kann ein ESP32-S3 CAN + BLE + GPS + Display gleichzeitig?

**Ja, mit Einschränkungen:**
- TWAI ist hardware-basiert → minimal CPU-Last für CAN
- BLE (NimBLE) läuft im Hintergrund, ~30-50 KB Heap
- GPS über UART → Interrupt-gesteuert, minimal CPU
- E-Paper über SPI → CPU nur beim Refresh aktiv (partial ~0.3s)
- SD-Karte über SPI → CPU nur beim Schreiben
- I2C-Sensoren → periodic Poll, minimal

Kritisch: WiFi aktivieren. WiFi+BLE gleichzeitig braucht mehr Heap und
erhöht den Peak-Stromverbrauch. Wenn WiFi nicht gebraucht wird: WiFi deaktivieren.
Wifi nur für OTA-Updates aktivieren, sonst aus.

**FreeRTOS-Task-Modell (empfohlen):**

| Task | Core | Priority | Stack | Bemerkung |
|---|---|---|---|---|
| TWAI RX | 1 | High | 2KB | CAN-Frame-Queue, interruptgesteuert |
| OBD/PID Processing | 1 | Medium | 4KB | Decode PIDs, Fehlercodes |
| GPS NMEA Parse | 0 | Medium | 2KB | UART → Position/Speed |
| Display Update | 0 | Low | 8KB | Partial-Refresh, nicht-blockierend |
| SD Logger | 1 | Low | 4KB | Write-Queue → SD im Batch |
| BLE | 0 | Medium | 6KB | NimBLE Stack |
| Sensors I2C | 0 | Low | 2KB | MPU6050 + BMP280 poll |
| Watchdog/Power | 1 | High | 2KB | Akku-Überwachung, Sleep-Mgmt |

## 8. Software-Stack

### 8.1 ESP-IDF vs. Arduino Framework

| Eigenschaft | ESP-IDF | Arduino Core |
|---|---|---|
| Sprache | C (native) | C++ (Arduino API) |
| FreeRTOS | Vollzugang, beide Cores | Eingeschränkt (loop() auf Core 1) |
| TWAI-Treiber | Native `driver/twai.h` API | Über Wrapper oder direkt |
| Binary-Größe | Kleiner | Größer (Arduino-Core Overhead) |
| Lernkurve | Steiler | Flacher |
| Bibliotheken | ESP-IDF Components | Arduino Library Manager (größeres Ökosystem) |
| Build-System | CMake/idf.py | Arduino IDE / PlatformIO |
| PSRAM-Zugriff | Direkt konfigurierbar | Über menuconfig |
| Debugging | JTAG, GDB, OpenOCD | Serial.print (besser mit PlatformIO) |
| Performance | Optimierte Kompilierung | Etwas Overhead |
| Updates | Schneller für neue Chips | Hinterher |

**Empfehlung: Arduino Core mit PlatformIO**
- Das Projekt nutzt bereits viele Arduino-Bibliotheken (GxEPD2, TinyGPS++, ELMduino)
- PlatformIO gibt CMake-ähnliche Build-Kontrolle ohne ESP-IDF-Komplexität
- FreeRTOS ist auch unter Arduino verfügbar (`freertos/FreeRTOS.h`)
- TWAI kann direkt über ESP-IDF-APIs angesprochen werden, auch im Arduino-Core
- Bei Performance-Problemen kann man kritische Tasks auf ESP-IDF-Ebene optimieren

### 8.2 OBD-II Bibliotheken

| Bibliothek | GitHub | Ansatz | TWAI-Support | Bemerkung |
|---|---|---|---|---|
| ELMduino | `PowerBroker2/ELMduino` | ELM327 (BT/WiFi/USB) | Indirekt (über ELM327) | Standard für ELM327, gut dokumentiert |
| MagnusThome/esp32_obd2 | `MagnusThome/esp32_obd2` | Direkt über ESP32 TWAI | Ja (S3, C6, etc.) | Basiert auf collin80/esp32_can, aktiv gepflegt |
| OBD2_CAN_Bus_Library | `muki01/OBD2_CAN_Bus_Library` | CAN direkt | Ja | ELM327 + TWAI, ESP32 |
| Freematics OBD | `freematics/freematics_library` | Eigenes OBD-II | Ja | Kommerzielles Ökosystem, gut dokumentiert |

**Empfehlung: `MagnusThome/esp32_obd2`**
- Direkter CAN-Zugang über TWAI, kein ELM327-Dongle nötig
- Läuft auf ESP32-S3 (nutzt `collin80/esp32_can` als Backend)
- Standard OBD-II PIDs (Mode 01) + universelle CAN-Send/Receive
- Aktiv gepflegt (Stand 2026)

Für ELM327-Bench-Testing: ELMduino als Zweitspur behalten.

### 8.3 CAN Bibliotheken

| Bibliothek | GitHub | Ansatz | Platform |
|---|---|---|---|
| esp32_can | `collin80/esp32_can` | ESP32 TWAI native | ESP32/S3/C6 |
| ACAN2515 | `pierremolinaro/acan2515` | MCP2515 über SPI | Alle Arduino-Boards |
| arduino-mcp2515 | `autowp/arduino-mcp2515` | MCP2515 über SPI | Alle Arduino-Boards |
| mcp2515-esp32-idf | `Microver-Electronics/mcp2515-esp32-idf` | MCP2515 über SPI (ESP-IDF) | ESP32 (IDF) |

**Empfehlung:**
- TWAI: `collin80/esp32_can` — der Standard für ESP32-S3 CAN
- MCP2515 (falls zweiter Bus): `pierremolinaro/acan2515` — am besten gepflegt

### 8.4 NMEA Parsing

| Bibliothek | GitHub | Sprache | Floating-Point | RAM-Nutzung | Bemerkung |
|---|---|---|---|---|---|
| minmea | `kosma/minmea` | C99 (pure C) | Optional (fixed-point) | Minimal (kein malloc) | ISO C99, test suite, statische Analyse |
| TinyGPS++ | `mikalhart/TinyGPSPlus` | C++ (Arduino) | Ja | Moderat | Standard für Arduino, einfach zu nutzen |
| NeoGPS | `SlashDevin/NeoGPS` | C++ (Arduino) | Konfigurierbar | Konfigurierbar | Am flexibelsten, konfigurierbare Sentences |

**Empfehlung: TinyGPS++**
- Gute Balance zwischen Einfachheit und Funktionalität
- Standard in der Arduino-Community, gut dokumentiert
- Extrahiert Position, Speed, Course, Altitude, Date/Time
- Funktioniert direkt mit NEO-M9N NMEA-Output (9600 oder 38400 Baud)
- Für ESP-IDF-native Builds: minmea (pure C, kein malloc)

### 8.5 E-Paper Bibliotheken

| Bibliothek | GitHub | Bemerkung |
|---|---|---|
| GxEPD2 | `ZinggJM/GxEPD2` | Standard, alle Panels, Adafruit_GFX-basiert, aktiv gepflegt |
| Adafruit EPD | `adafruit/Adafruit_EPD` | Adafruit-eigene Library, weniger Panel-Support |
| Waveshare e-Paper | Waveshare-Beispiele | Board-spezifisch, begrenzt |

**Empfehlung: GxEPD2**
- Supports Waveshare 2.9" (alle Varianten), 1.54", und viele mehr
- Partial-Refresh für schnelle Live-Daten
- Multi-Display-Support
- Aktive Community, regelmäßige Updates

### 8.6 SD-Logging

| Bibliothek | Ansatz | Bemerkung |
|---|---|---|
| esp_vfs_fat (ESP-IDF) | Native VFS + FATFS | Beste Performance, direkter ESP-IDF |
| Arduino SD (`SD.h`) | Arduino Wrapper über SPI | Einfach, aber langsam |
| SdFat (`billgreiman/SdFat`) | Eigenständige FAT-Bibliothek | Schneller als Arduino SD, exFAT-Support |

**Empfehlung: SdFat**
- Schnelleres Schreiben als Arduino SD (wichtig bei 10Hz GPS-Logging)
- exFAT-Support für große Dateien
- Kompatibel mit PlatformIO
- Falls zu komplex: Arduino SD als Fallback

### 8.7 BLE: NimBLE vs. Bluedroid

| Eigenschaft | NimBLE | Bluedroid |
|---|---|---|
| BT Classic | Nein | Ja (nur ESP32 classic, nicht S3) |
| Heap-Bedarf | Geringer (~30-50 KB) | Höher (~70-100 KB) |
| Flash-Bedarf | Geringer | Höher |
| BLE 5.0 | Ja | Ja |
| BLE Mesh | Ja | Ja |
| API | NimBLE API + ESP-NimBLE Wrapper | ESP-BLE API (`esp_` prefix) |
| Wartung | Apache Mynewt Project | Aus Android Bluedroid portiert |
| Code-Qualität | Sauberer, modern | Legacy, komplex |

**Empfehlung: NimBLE**
- ESP32-S3 unterstützt kein BT Classic → Bluedroid's einziger Vorteil (Classic) fällt weg
- NimBLE braucht deutlich weniger Heap → mehr RAM für Display-Buffer und SD
- ESP-IDF unterstützt NimBLE nativ auf S3
- API etwas anders als Bluedroid, aber besser dokumentiert
- NimBLE ist der Standard für ESP32-S3 BLE-Projekte

Quelle: ESP-IDF v5.5 Docs — `esp-idf/en/v5.5/esp32/api-guides/ble/overview.html`

## 9. Bestehende Open-Source Bordcomputer-Projekte

### 9.1 Übersicht

| Projekt | GitHub | MCU | Ansatz | Bemerkung |
|---|---|---|---|---|
| AEZEL | `ijlaal1610/AEZEL` | ESP32-S3 | VCU + Dashboard | Smart Motorcycle Platform, modular, OTA, BLE |
| ESP32-Motorcycle-Dash | `f1n41/ESP32-Motorcycle-Dash` | ESP32 | ECU + GPS + Display | OBD-Diagnostic-Port, GPS, Temp/Humidity |
| digital-dashboard | `Sneupi/digital-dashboard` | ESP32 | CAN-Bus Dashboard | Formula SAE, SN65HVD230, TFT 5" SSD1963 |
| esp32-obd2-logger | `roypeter/esp32-obd2-logger` | ESP32 | OBD2 CAN Logger | TJA1050, OLED, SD, WiFi-Dashboard |
| RejsaCAN-ESP32 | `MagnusThome/RejsaCAN-ESP32` | ESP32/S3 | CAN-Bridge | SN65HVD230 onboard, 12V, auto-shutdown |
| ESP-DASH | `ayushsharma82/ESP-DASH` | ESP32/RP2040 | Web-Dashboard | On-device Web-Dashboard, nicht physisch |
| Freematics | `freematics.com` | ESP32 | OBD+GPS+Logger | Kommerziell, komplett Ökosystem |

### 9.2 Lernpunkte aus diesen Projekten

**AEZEL (smart motorcycle platform):**
- ESP32-S3 als MCU (gleiche Wahl wie unseres)
- Modulare VCU-Architektur (Vehicle Control Unit)
- OTA-Updates über BLE
- Ride Analytics + Smartphone-Integration
- → Architektur-Ansatz als Vorbild für modulare Tasks

**ESP32-Motorcycle-Dash (f1n41):**
- Verbindet sich über Diagnose-Port mit der ECU
- GPS-Modul + Temperatur/Feuchtigkeitssensor
- → Ähnliche Feature-Set wie unseres, wertvoll für OBD-Integration

**digital-dashboard (Sneupi):**
- ESP32 + SN65HVD230 CAN-Transceiver (genau unsere geplante Konfiguration!)
- TFT 5" SSD1963 Display (nicht E-Paper)
- CAN-Bus-Daten direkt interpretiert, nicht über OBD-II PIDs
- Pinout: CAN_RX=GPIO18, CAN_TX=GPIO19 (klassischer ESP32, nicht S3)
- → CAN-Transceiver-Bestätigung, aber TFT statt E-Paper

**RejsaCAN-ESP32 (MagnusThome):**
- ESP32/S3 mit onboard SN65HVD230
- 12V-Power direkt aus dem Bordnetz (5-15V Input)
- Auto-Shutdown gegen Batterie-Entleerung
- 3x5cm mini-Board
- → Hardware-Design als Vorbild für Power-Management und CAN-Integration

**esp32-obd2-logger (roypeter):**
- ESP32 + TJA1050 CAN-Transceiver (5V, braucht Level-Shift)
- SD-Card-Logging, WiFi-Web-Dashboard zum CSV-Download
- OLED für Live-Werte
- → SD-Logging und WiFi-Dashboard als Feature-Ideen

**Freematics:**
- Komplettes kommerzielles Ökosystem für Fahrzeug-Telemetrie
- Arduino-kompatibel, eigene OBD-II-Bibliothek
- GPS, Bewegungssensor, Cellular
- → Bibliothek als Referenz für OBD-PID-Implementierung

### 9.3 Was wir adaptieren können

1. **CAN-Hardware**: SN65HVD230 + ESP32-S3 TWAI (bestätigt durch Sneupi und RejsaCAN)
2. **Power-Management**: 12V direkt aus Bordnetz + Auto-Shutdown (RejsaCAN)
3. **OBD-II-PID-Decode**: MagnusThome/esp32_obd2 Bibliothek
4. **SD-Logging**: Write-Queue + Batch-Write für Performance (esp32-obd2-logger)
5. **Modulare Task-Architektur**: AEZEL's VCU-Ansatz
6. **BLE-Smartphone-Integration**: AEZEL's OTA-Ansatz

## 10. Empfohlener Hardware-Stack (Zusammenfassung)

| Komponente | Modul | Preis | Quelle |
|---|---|---|---|
| MCU | ESP32-S3-DevKitC-1 (N16R8: 16MB Flash, 8MB PSRAM) | $5-8 | Espressif |
| CAN-Transceiver | SN65HVD230 (CJMCU-230 Breakout) | $2-3 | TI / AliExpress |
| GPS | u-blox NEO-M9N (SparkFun Breakout) | $28-42 | SparkFun |
| GPS (Budget) | ATGM336H (Alternative) | $2-5 | AliExpress |
| Display | Waveshare 2.9" E-Paper b/w (128x296) | $12-18 | Waveshare |
| IMU | MPU6050 | $1-2 | AliExpress |
| Baro/Temp | BMP280 | $1-2 | AliExpress |
| SD-Card | MicroSD Slot (SPI) + 32GB Class 10 | $2-5 | beliebig |
| Power (LiPo) | TPS63000 Buck-Boost | $3-5 | TI |
| Power (12V) | MP1584 Buck | $0.80 | AliExpress |
| LiPo-Lader | TP4056 (USB-C) | $0.30 | AliExpress |
| LiPo | 18650 3400mAh | $3-5 | beliebig |
| K-Line (optional) | L9637D oder Transistor-Schaltung | $0.50-2 | DigiKey |

**Gesamtkosten Hardware (ohne Gehäuse): ~$55-95**

## 11. Empfohlener Software-Stack (Zusammenfassung)

| Komponente | Bibliothek | Quelle |
|---|---|---|
| Framework | Arduino Core + PlatformIO | platformio.org |
| CAN-Treiber | esp32_can (collin80) | github.com/collin80/esp32_can |
| OBD-II | esp32_obd2 (MagnusThome) | github.com/MagnusThome/esp32_obd2 |
| GPS-NMEA | TinyGPS++ | github.com/mikalhart/TinyGPSPlus |
| E-Paper | GxEPD2 | github.com/ZinggJM/GxEPD2 |
| SD-Logging | SdFat | github.com/billgreiman/SdFat |
| BLE | NimBLE (ESP-NimBLE) | ESP-IDF native |
| RTOS | FreeRTOS | ESP-IDF native |
| IMU | Adafruit MPU6050 | github.com/adafruit/Adafruit_MPU6050 |
| Baro | Adafruit BMP280 | github.com/adafruit/Adafruit_BMP280_Library |
| OBD-Bench | ELMduino (für ELM327-Testing) | github.com/PowerBroker2/ELMduino |

## 12. Korrektur-Bedarf für bestehende Doku

Die folgende Änderungen sollten in `docs/HARDWARE.md` und `docs/PINMAP.md` nachgezogen werden:

1. **HARDWARE.md — OBD-II Sektion**:
   - "ELM327 Bluetooth (pairing mit ESP32 BT-Classic)" → funktioniert nicht auf S3
   - Neu: SN65HVD230 CAN-Transceiver + TWAI (direkt)
   - ELM327 BLE als optionale Diagnose-Schnittstelle

2. **HARDWARE.md — Power-Sektion**:
   - HT7333 → AP2112K-3.3 oder TPS63000 (250mA reicht nicht)
   - MT3608 + HT7333 Doppelkonvertierung → TPS63000 Buck-Boost
   - Optional: MP1584 für 12V-Bordnetz

3. **PINMAP.md — CAN/TWAI**:
   - TWAI TX → GPIO 5 (statt OBD-II UART auf GPIO 18)
   - TWAI RX → GPIO 4 (statt OBD-II UART auf GPIO 17)
   - SN65HVD230 CANH → OBD-II Pin 6
   - SN65HVD230 CANL → OBD-II Pin 14
   - ADC-Pin prüfen: GPIO 4 war für ADC gemappt → Conflict mit TWAI RX!
   - ADC verschieben auf GPIO 3 oder GPIO 15

4. **PINMAP.md — ADC-Konflikt**:
   - GPIO 4 aktuell: Akku-Spannung (ADC)
   - GPIO 4 neu: TWAI RX
   - Lösung: ADC auf GPIO 15 verlegen ( ADC1, frei)

## 13. Links

### Offizielle Doku
- ESP-IDF TWAI (S3): https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/peripherals/twai.html
- ESP-IDF BLE Overview: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-guides/ble/overview.html
- SN65HVD230 Datasheet: https://www.ti.com/product/SN65HVD230

### GitHub Repos
- esp32_can: https://github.com/collin80/esp32_can
- esp32_obd2: https://github.com/MagnusThome/esp32_obd2
- ACAN2515: https://github.com/pierremolinaro/acan2515
- ELMduino: https://github.com/PowerBroker2/ELMduino
- GxEPD2: https://github.com/ZinggJM/GxEPD2
- TinyGPS++: https://github.com/mikalhart/TinyGPSPlus
- minmea: https://github.com/kosma/minmea
- SdFat: https://github.com/billgreiman/SdFat
- obd-ble-serial: https://github.com/vdvornichenko/obd-ble-serial
- OBD2_K-line_Reader: https://github.com/muki01/OBD2_K-line_Reader
- AEZEL: https://github.com/ijlaal1610/AEZEL
- ESP32-Motorcycle-Dash: https://github.com/f1n41/ESP32-Motorcycle-Dash
- digital-dashboard: https://github.com/Sneupi/digital-dashboard
- RejsaCAN-ESP32: https://github.com/MagnusThome/RejsaCAN-ESP32
- esp32-obd2-logger: https://github.com/roypeter/esp32-obd2-logger
- Freematics: https://freematics.com
