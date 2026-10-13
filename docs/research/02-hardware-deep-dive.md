# Deep Dive: Bordcomputer-Hardware, ESP32, Sensoren & Power-Management

> Stand: Oktober 2026 — Exhaustive Recherche zur kompletten Hardware-Auslegung.
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986–2000, rein mechanisch, keine ECU.
> Alle Preise sind Richtwerte (AliExpress / Amazon / DigiKey / Mouser, Stand 2026).

## Inhalt

1. [MCU-Plattform](#1-mcu-plattform)
2. [GPS-Module](#2-gps-module)
3. [Display](#3-display)
4. [IMU / Neigung / Beschleunigung](#4-imu--neigung--beschleunigung)
5. [Barometer / Höhenmesser](#5-barometer--höhenmesser)
6. [Power-Management](#6-power-management)
7. [SD-Karten-Logging](#7-sd-karten-logging)
8. [Bluetooth / Smartphone-Anbindung](#8-bluetooth--smartphone-anbindung)
9. [Relais-Steuerung](#9-relais-steuerung)
10. [Pin-Belegungs-Plan (gesamt)](#10-pin-belegungs-plan-gesamt)
11. [Stromverbrauchs-Schätzung](#11-stromverbrauchs-schätzung)

---

## 1. MCU-Plattform

### 1.1 Plattform-Vergleich

| MCU | GPIO | BT | BLE | SPI | I2C | ADC | Pulse-Cntr | Flash | RAM | Preis |
|---|---|---|---|---|---|---|---|---|---|---|
| ESP32-S3 | 45 | ✗ | 5.0 | ×4 | ×2 | 20ch (12-bit) | 4ch | 4–16MB | 512KB+PSRAM | 3–8€ |
| ESP32 (Classic) | 34 | Classic | 4.2 | ×4 | ×2 | 18ch (12-bit) | 8ch | 4–16MB | 520KB | 5–7€ |
| ESP32-C6 | 20+ | ✗ | 5.0+Thread | ×2 | ×1 | 8ch | 2ch | 4–8MB | 512KB | 4–6€ |
| RP2040 | 26 | ✗ | ✗ | ×2 | ×2 | 4ch (12-bit) | ✗ | ext. | 264KB | 4–5€ |
| Teensy 4.0 (IMXRT1062) | 40 | ✗ | ✗ | ×4 | ×4 | 14ch | 4ch (FlexPWM) | ext. | 1MB | 20–25€ |
| STM32F4 (Blue Pill) | 30+ | ✗ | ✗ | ×3 | ×3 | 10ch (12-bit) | Timer | 64–512KB | 20–64KB | 2–4€ |

### 1.2 Entscheidung: ESP32-S3-WROOM-1

Der ESP32-S3 ist die klare Wahl für dieses Projekt:

- **45 GPIO** — mehr als genug für alle geplanten Module
- **BLE 5.0** — für Smartphone-Anbindung, OTA-Updates
- **4× SPI** — Display + SD-Karte auf getrennten Bussen
- **2× I2C** — Sensoren (IMU, Baro) auf einem Bus
- **20 ADC-Kanäle** (2× 12-bit SAR) — Batterie-Monitor, Fahrzeug-Sensoren
- **4 Pulse-Counter-Kanäle** — Drehzahl, Tacho (Fahrzeug hat keine ECU, Signale kommen direkt vom Kabel)
- **USB-OTG** — Direktes Flashing, Logging am PC
- **Deep-Sleep: 7 µA** (bare chip), Wake-Sources: RTC Timer, GPIO 0-21, Touch, ULP

**Aber:** Der ESP32-S3 hat **kein Bluetooth Classic**. SPP (Serial Port Profile) für ELM327-Dongles geht nicht. Für dieses Projekt irrelevant, da die PD06 keine ECU/OBD hat — CAN/TWAI ist nur für optionale Erweiterung relevant, und die Transalp 600 ist rein mechanisch.

### 1.3 ESP32-S3 ADC-Kanäle & GPIO-Mux

Der ESP32-S3 hat zwei ADC-Blöcke:

- **ADC1**: 10 Kanäle, GPIO1–GPIO10
- **ADC2**: 10 Kanäle, GPIO11–GPIO20

**Kritisch: ADC2 wird von Wi-Fi/BLE arbitriert.** Sobald Bluetooth aktiv ist, liefern ADC2-Kanäle Müll oder `ESP_ERR_TIMEOUT`. Alle analogen Sensoren **müssen auf ADC1 (GPIO1–GPIO10)** liegen.

ADC-Attenuation und nutzbarer Bereich:

| Attenuation | Range | Error |
|---|---|---|
| 0 dB | 0–850 mV | ±5 mV |
| 2.5 dB | 0–1100 mV | ±6 mV |
| 6 dB | 0–1600 mV | ±10 mV |
| 12 dB | 0–2900 mV | ±50 mV |

Für 12V-Batteriemonitor: Spannungsteiler auf <2900 mV, ADC_ATTEN_DB_12.

Pulse-Counter (PCNT): 4 unabhängige Kanäle, bis 40 MHz Input. Perfekt für:
- Drehzahl (Zündimpulse, Reed-Kontakt am Vorderrad)
- Tacho (Reed-Schalter / Hall-Sensor am Vorderrad)

### 1.4 DevKitC-1 vs. Custom PCB

| Kriterium | ESP32-S3-DevKitC-1 | Custom PCB (WROOM-1 Modul) |
|---|---|---|
| Prototyp-Geschwindigkeit | Sofort einsatzbereit | 2–4 Wochen Layout+Bestückung |
| GPIO-Zugriff | Alle Pins auf Header | Volle Kontrolle über Pin-Wahl |
| USB | Native USB-JTAG/Serial | Eigenes USB-C Layout |
| Power | USB oder 5V-Pin | Eigener Power-Path |
| Größe | 70×52mm ( groß) | Min. 50×40mm möglich |
| Kosten | 5–8€ | 15–25€ (kleine Stückzahl) |
| I/O-Voltage | 3.3V | Konfigurierbar |

**Phase 1 (Prototyp):** DevKitC-1 auf Breadboard/Perfboard.
**Phase 2 (Production):** Custom PCB mit WROOM-1, eigenem Power-Management, SD-Slot, Sensor-Footprints.

### 1.5 PSRAM — braucht man es?

Der ESP32-S3-WROOM-1 ist in Varianten mit PSRAM erhältlich:

- **N8R2**: 8MB Flash + 2MB PSRAM (empfohlen)
- **N16R8**: 16MB Flash + 8MB octal PSRAM (Overkill)

PSRAM-Nutzung für dieses Projekt:
- **Display-Framebuffer**: 2.9" E-Paper = 296×128 = 4.744 Byte (1-bit). Passt locker ins SRAM. PSRAM nicht nötig.
- **GPS-Buffer**: NMEA-Sätze sind klein (<82 Byte). UBX-Binary bis ~1KB. Kein PSRAM nötig.
- **SD-Write-Buffer**: 4–32KB Ringbuffer. Passt ins SRAM.
- **BLE-GATT-Buffer**: Wenige KB.

**Fazit:** PSRAM nicht zwingend nötig, aber N8R2 kostet nur ~1€ mehr und gibt Reserven für spätere Features (z.B. Web-Server, größere Framebuffer bei TFT). **Empfehlung: N8R2.**

### 1.6 Deep-Sleep-Modus

| Modus | Stromverbrauch (bare chip) | Wake-Sources |
|---|---|---|
| Active | 90–240 mA | — |
| Modem-Sleep | ~50 mA | Auto, bei Wi-Fi/BLE |
| Light-Sleep | 240 µA (+140 µA PSRAM) | GPIO, Timer, BLE |
| Deep-Sleep | 7 µA (+RTC) | RTC GPIO (0–21), Timer, Touch, ULP |
| Power Off | 1 µA | — |

**Wichtig:** Die 7 µA gelten nur für den bare chip. Ein DevKit hat LDO, USB-Bridge und Power-LED, die zusammen 5–20 mA ziehen. Für echten Low-Power braucht es ein Custom-Board ohne diese Komponenten.

Deep-Sleep-Wake-Sources für den Bordcomputer:
- **RTC Timer**: Periodisches Aufwachen (z.B. alle 60s für GPS-Logging)
- **GPIO (RTC GPIO 0–21)**: Wake-on-Motion via IMU-Interrupt
- **ULP (Ultra-Low-Power Coprozessor)**: Kann ADC lesen und IMU-I2C pollsen während Deep-Sleep

Quellen:
- [ESP32-S3 Datasheet (PDF)](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)
- [ESP-IDF GPIO Reference (S3)](https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32s3/api-reference/peripherals/gpio.html)
- [ESP32 Chip Series Comparison](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32s3/hw-reference/chip-series-comparison.html)

---

## 2. GPS-Module

### 2.1 Modul-Vergleich

| Modul | GNSS | Update-Rate | Genauigkeit | Strom | TTFF Cold | Preis |
|---|---|---|---|---|---|---|
| u-blox NEO-M9N | 4 concurrent (GPS+GLO+GAL+BDS) | 25 Hz | 2.0 m CEP | 36 mA @3V (4GNSS) | 24 s | 25–35€ |
| u-blox NEO-M8U | 3 concurrent + DR | 10 Hz | 2.5 m CEP | 29 mA | 26 s | 30–45€ |
| u-blox NEO-M8N | 2 concurrent | 10 Hz | 2.5 m CEP | 29 mA | 26 s | 12–20€ |
| u-blox NEO-6M | GPS only | 5 Hz | 2.5 m CEP | 22 mA | 27 s | 3–6€ |
| ATGM336H | GPS+BDS | 1 Hz | 2.5 m CEP | 25 mA | 30 s | 2–4€ |
| Quectel L80 | GPS only | 1 Hz | 2.5 m CEP | 20 mA | 30 s | 5–8€ |

### 2.2 Entscheidung: u-blox NEO-M9N

Der NEO-M9N ist die beste Wahl für einen Motorrad-Bordcomputer:

- **4-Konstellation gleichzeitig** (GPS+GLONASS+BeiDou+Galileo) → 30+ Satelliten in Sicht, robust auch in Tälern/Wald
- **25 Hz Update-Rate** → Für 100 km/h = 1.1 m pro Update, genug Auflösung für Track-Logging
- **2.0 m CEP** → Real 0.8–1.5 m im Freien
- **Spoofing/Jamming-Detection** → Meldet Störungen an den Host
- **Pin-kompatibel** zu NEO-M8/M6 → Upgrade-Pfad
- **Power-Save-Mode**: 7 mA bei 1 Hz cyclic → perfekt für Battery-Backup-Logging
- **23 s Cold Start** → Akzeptabel nach Wochen ohne Strom

**Preis:** ~25–35€ (Achtung: Clones von AliExpress unter 15€ sind oft Fake. Original bei Mouser/DigiKey).

### 2.3 Dead Reckoning (NEO-M8U) — sinnvoll für Motorrad?

Der NEO-M8U hat einen integrierten 3D-IMU für Untethered Dead Reckoning (UDR). Vorteil: Positionierung in Tunneln, Tiefgaragen, Wald ohne GPS-Sicht.

**Aber für die Transalp:**
- Die Transalp fährt selten in Tunneln oder Tiefgaragen
- Der Bordcomputer hat bereits eine eigene IMU (siehe Abschnitt 4)
- UDR ist untethered = unabhängig von Fahrzeug-Sensoren, aber IMU-Drift bei Motorrad-Einsatz ist schwierig (Schräglage vs. Geradeausfahrt)
- Preis: 30–45€ statt 25–35€ für M9N

**Fazit:** Nicht wert. Die eigene IMU kann bei Bedarf für Dead-Reckoning mit GPS-Fusion verwendet werden. Der M9N ist besser und billiger.

### 2.4 UART-Konfiguration & Protokoll

Default-Konfiguration NEO-M9N:
- UART Baud: 38400 (konfigurierbar bis 921600)
- NMEA Output: GGA, RMC, GSV, GSA, VTG (1 Hz default)
- UBX-Protokoll: Binary, für Konfiguration und High-Rate-Output

Empfohlene Konfiguration für Bordcomputer:
```
Baud: 115200
NMEA: GGA + RMC + VTG (minimale Sätze, 5 Hz)
UBX: NAV-PVT (Position Velocity Time) bei 5–10 Hz
```

UBX NAV-PVT ist kompakter als NMEA (100 Byte vs. ~300 Byte für 3 NMEA-Sätze) und liefert alle Daten in einem Paket: Lat, Lon, Height, Speed, Heading, Sat-Anzahl, Accuracy.

### 2.5 Antennen-Wahl

| Antennentyp | Vorteil | Nachteil | Preis |
|---|---|---|---|
| Active Patch (25×25mm) | Beste Empfindlichkeit, Verstärker onboard | Braucht 3.3V Versorgung | 5–10€ |
| Passive Patch | Keine Stromversorgung nötig | Schwächer, Kabellänge kritisch | 2–4€ |
| Chip-Antenne (on-board) | Kompakt, keine Kabel | Schlechteste Empfindlichkeit | inkl. |
| Externe Magnetfuß-Antenne | Beste Platzierung, direkt auf Tank | Kabel, mechanische Befestigung | 8–15€ |

**Empfehlung:** Active Patch-Antenne mit Magnetfuß oder direkt auf Gehäuse. Das Modul hat VCC_RF Output (bis 200 mA) für die Antennenversorgung. Kabellänge <2m, sonst Signalverlust.

### 2.6 u-center Konfiguration

u-center ist u-blox' Konfigurationssoftware (Windows/macOS). Über USB oder UART verbunden:

- **Cold Start**: 24s (voller Almanach-Download)
- **Warm Start**: 2s (Almanach vorhanden, Ephemeris veraltet)
- **Hot Start**: 2s (Almanach+Ephemeris gültig)

Nach Konfiguration: Konfiguration in Flash speichern (UBX-CFG-CFG), dann autonom beim Start.

Quellen:
- [NEO-M9N Datasheet (PDF)](https://content.u-blox.com/sites/default/files/NEO-M9N-00B_DataSheet_UBX-19014285.pdf)
- [NEO-M9N Product Summary (PDF)](https://content.u-blox.com/sites/default/files/NEO-M9N_ProductSummary_UBX-19027207.pdf)
- [NEO-M8U Produktseite](https://www.u-blox.com/en/product/neo-m8u-module)
- [GPS Comparison: NEO-6M vs M8N vs M9N](https://zbotic.in/gps-accuracy-comparison-neo-6m-vs-neo-m8n-vs-neo-m9n/)

---

## 3. Display

### 3.1 Technologie-Vergleich für Motorrad-Einsatz

| Technologie | Sonnenlesbarkeit | Strom | Refresh | Größe | Preis |
|---|---|---|---|---|---|
| E-Paper (E-Ink) | Exzellent | ~0 mW (bistabil) | 0.3s (partial), 2s (full) | 1.54–4.2" | 8–25€ |
| TFT (ILI9341/ST7789) | Schlecht (Blendung) | 50–150 mA | 30 fps | 1.14–3.5" | 4–12€ |
| OLED (SSD1306/SH1106) | Gut (kontrastreich) | 20–50 mA | 60 fps | 0.96–1.3" | 2–6€ |

**Motorrad-spezifisch:** Sonnenlicht, Vibrationen, Regen, Handschuhe, Lesewinkel.

E-Paper gewinnt bei:
- **Sonnenlesbarkeit**: Reflexiv, kein Backlight nötig. So gut wie Papier.
- **Stromverbrauch**: Bistabil = 0 mW zwischen Updates. Nur beim Refresh ~26 mW.
- **Lesewinkel**: >170°

E-Paper verliert bei:
- **Refresh-Zeit**: 2s Full-Refresh (mit Screen-Flash), 0.3s Partial (ohne Flash)
- **Keine Farbe**: Nur schwarz/weiß (oder schwarz/weiß/rot bei teureren Panels)
- **Ghosting**: Bei Partial-Refresh bauen sich Reste auf, periodischer Full-Refresh nötig

**Entscheidung: E-Paper Waveshare 2.9" (296×128, SSD1680)**

Begründung: Der Bordcomputer braucht keine schnellen Animationen. Geschwindigkeit, Drehzahl, GPS-Track sind statische Zahlen/Layouts die sich alle 0.3s aktualisieren können. Sonnenlesbarkeit ist das wichtigste Kriterium am Motorrad. Low Power ist Bonus für Battery-Backup.

### 3.2 E-Paper Deep-Dive

**Treiber-ICs:**
- **SSD1680**: Waveshare 2.9" V2/V3, 296×128, monochrom. Häufigster Treiber.
- **UC8151D**: Good Display GDEW029T5D, 296×128, monochrom. Pin-kompatibel.
- Beide werden von GxEPD2 library unterstützt.

**Refresh-Modi:**
1. **Full Refresh** (~2 Sekunden): Kompletter Screen-Flash (schwarz→weiß→schwarz). Kein Ghosting. Lebensdauer: ~1.000.000 Full-Refreshes.
2. **Partial Refresh** (~0.3 Sekunden): Nur geänderte Region wird aktualisiert, kein Flash. Ghosting baut sich auf. Nach ~20–50 Partials einen Full-Refresh auslösen.
3. **Fast Refresh**: Spezielle SSD1680-Modi für schnellere Partials. Reduziert Lebensdauer schneller.

**Lebensdauer:** ~1.000.000 Full-Refreshes. Bei 10 Partials pro Sekunde + alle 30s ein Full = ~120 Full-Refreshes pro Stunde = 8.333 Stunden bis Lebensdauer-Grenze. Bei 2h Fahrt pro Tag = 11 Jahre. Ausreichend.

### 3.3 Größen-Vergleich

| Größe | Auflösung | Pixel | Aktive Fläche | Modul-Größe | Preis |
|---|---|---|---|---|---|
| 1.54" | 152×152 | 23.104 | 27×27 mm | 38×33 mm | 8–12€ |
| 2.13" | 250×122 | 30.500 | 48×23 mm | 65×30 mm | 10–15€ |
| 2.9" | 296×128 | 37.888 | 67×29 mm | 89×38 mm | 12–18€ |
| 4.2" | 400×300 | 120.000 | 84×63 mm | 107×67 mm | 18–25€ |

**Empfehlung: 2.9"** — Genug Platz für Geschwindigkeit (gross), Drehzahl, GPS-Info, Höhenmeter und Neigungsanzeige. Noch kompakt genug für eine Lenkerhalterung.

### 3.4 SPI-Pin-Belegung (ESP32-S3 → E-Paper)

E-Paper nutzt 4-Wire SPI (write-only, kein MISO):

```
E-Paper Pin     ESP32-S3 GPIO    Funktion
─────────────────────────────────────────────
VCC             3.3V              Power
GND             GND               Ground
DIN (MOSI)      GPIO11            SPI MOSI
CLK (SCK)       GPIO12            SPI SCK
CS              GPIO10            SPI Chip Select
DC              GPIO9             Data/Command
RST             GPIO14            Reset (active low)
BUSY            GPIO13            Busy-Status (input)
```

Verwendet SPI2 (FSPI) des ESP32-S3. SD-Karte geht auf separaten SPI-Bus (SPI3/HSPI).

GxEPD2 library:
```cpp
// 2.9" SSD1680, SPI2 (FSPI)
SPIClass fspi(FSPI);
fspi.begin(12, 13, 11, 10); // SCK, MISO(unused), MOSI, CS
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> display(
    GxEPD2_290_T94_V2(/*CS=*/10, /*DC=*/9, /*RST=*/14, /*BUSY=*/13)
);
```

Quellen:
- [Waveshare 2.9" E-Paper (SSD1680)](https://tinkered.ai/components/epaper-2in9)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [Waveshare Produktseite 2.9"](https://waveshare.com/2.9inch-e-paper-d.htm)

---

## 4. IMU / Neigung / Beschleunigung

### 4.1 Modul-Vergleich

| Modul | Achsen | Sensor-Fusion | Strom | Interface | Preis |
|---|---|---|---|---|---|
| MPU6050 | 6-DoF (Accel+Gyro) | Keine | 3.9 mA | I2C | 1–3€ |
| BMI270 | 6-DoF | On-chip step/gesture | 685 µA | I2C/SPI | 4–7€ |
| LSM6DS33 | 6-DoF | Keine | 0.9 mA | I2C/SPI | 4–6€ |
| BNO085/BNO086 | 9-DoF (Accel+Gyro+Mag) | Eingebaut (ARM Cortex M0+) | 8 mA | I2C/SPI/UART | 15–20€ |
| BMI088 | 6-DoF | Keine, vibrationsrobust | 5.15 mA | I2C/SPI | 6–10€ |

### 4.2 Entscheidung: BMI270

Der BMI270 ist der beste Kompromiss:

- **Ultra-Low-Power**: 685 µA bei voller ODR (6.4 kHz) — wichtig für Battery-Backup
- **Geringer Rausch**: Accel 160 µg/√Hz, Gyro 0.008 °/s/√Hz
- **2KB FIFO**: Sammelt Daten, reduziert I2C-Traffic und MCU-Wake-Ups
- **On-chip Interrupts**: Step-Counter, Activity, Any-Motion → Wake-on-Motion für Deep-Sleep
- **Automotive-Proven Gyro**: Bosch verbaut diesen in Produktions-Fahrzeugen

**Warum nicht BNO085?** Der BNO085 macht Sensor-Fusion on-chip ( quaternion output). Bequem, aber:
- 8 mA Stromverbrauch (10× mehr als BMI270)
- Proprietäre Fusion → weniger Kontrolle über Filterparameter
- Teurer (15–20€)
- Die Fusion ist für Schräglage am Motorrad nicht optimiert (Annahme: Device ist aufrecht)

**Warum nicht MPU6050?** Obsolete (seit 2013), hoher Stromverbrauch, hoher Rausch. Ein Wechsel auf BMI270 kostet 2€ mehr und ist in jeder Hinsicht besser.

### 4.3 Roll/Pitch für Neigungsanzeige

Die Neigungsanzeige (Schräglage) ist die wichtigste IMU-Funktion für den Bordcomputer:

**Roll** (Schräglage links/rechts) und **Pitch** (Nase hoch/runter) können aus Accelerometer und Gyroskop berechnet werden:

**Complementary Filter (einfach, robust):**
```c
// dt = Zeit seit letzter Messung
// accel_roll = atan2(accel_y, accel_z) * 180/PI
// gyro_roll += gyro_x * dt  // Integration der Gyro-Rate
// roll = 0.98 * (roll + gyro_x * dt) + 0.02 * accel_roll  // Complementary
```

Der Complementary Filter kombiniert:
- Gyro (kurzfristig präzise, driftet langfristig) → 98%
- Accel (langfristig stabil, kurzfristig verrauscht) → 2%

**Kalman Filter** ist genauer aber rechenintensiver. Für einen Bordcomputer mit ESP32-S3 (240 MHz dual core) ist ein Kalman Filter kein Problem, aber der Complementary Filter reicht völlig.

### 4.4 Crash-Erkennung & Wheelie-Detection

**Crash-Erkennung:**
- Plötzliche Verzögerung = Accel-X (Längsachse) > 2g (konfigurierbar)
- Trigger: |accel_x| > Threshold für >50ms
- Aktion: GPS-Position speichern, BLE-Notification ans Smartphone

**Wheelie-Detection:**
- Pitch-Rate (gyro_y) > 30°/s für >200ms = Frontlift
- Pitch > 15° = Wheelie confirmed
- Aktion: Log-Entry, evtl. LED-Notification

### 4.5 Vibrations-Filterung

Motorrad-Vibrationen sind das größte Problem für IMU-Messungen:

- **Motor-Vibration**: 50–200 Hz (Einzylinder), 100–400 Hz (V2 wie Transalp)
- **Straßen-Vibration**: 5–50 Hz (Unebenheiten)
- **Lenker-Vibration**: 10–30 Hz

**Hardware-Filter:**
- Mechanische Entkopplung: IMU-Modul auf Schaumstoff-Pad oder Silikontöpfchen
- BMI270 hat programmierbare Bandwidth-Filter: Accel 5–684 Hz, Gyro 11–751 Hz

**Software-Filter:**
- Low-Pass-Filter auf BMI270 einstellen: Accel BW = 40 Hz, Gyro BW = 50 Hz (schneidet Motor-Vibrationen ab)
- ZusätzlichMoving Average (N=5) auf Roll/Pitch-Werte im Code

**Kalibrierung:**
- Null-Offset bei stillstehendem Motorrad aufnehmen (50 Messungen mitteln)
- Offset im Flash speichern
- Bei jeder Fahrt-Start: 2 Sekunden Kalibrierung mit Meldung auf Display

### 4.6 Einbau-Orientierung

Der BMI270 muss im Gehäuse so ausgerichtet werden, dass:
- X-Achse = Längsrichtung (Fahrtrichtung)
- Y-Achse = Quer (links/rechts)
- Z-Achse = Vertikal (oben/unten)

Ist die Montage anders, muss in Software rotiert werden ( Achsen-Tausch + Vorzeichen).

Quellen:
- [BMI270 Datasheet (Mouser PDF)](https://www.mouser.com/pdfdocs/Product_flyer_BMI270.pdf)
- [BMI270 ESP Component](https://components.espressif.com/components/espp/bmi270/versions/1.0.22/readme?language=en)
- [Bosch BMI088 (vibrationsrobust)](https://www.bosch-sensortec.com/products/motion-sensors/imus/bmi088)
- [MPU6050 vs BNO055 Discussion](https://forum.arduino.cc/t/mpu6050-vs-bno055/673029)

---

## 5. Barometer / Höhenmesser

### 5.1 Modul-Vergleich

| Sensor | Druck-Range | Abs. Genauigkeit | Rel. Genauigkeit | Höhen-Auflösung | Strom @1Hz | FIFO | Preis |
|---|---|---|---|---|---|---|---|
| BMP280 | 300–1100 hPa | ±1 hPa | ±0.12 hPa | ~1 m | 2.7 µA | Nein | 1–2€ |
| BME280 | 300–1100 hPa | ±1 hPa | ±0.12 hPa | ~1 m | 3.6 µA | Nein | 2–3€ |
| BMP388 | 300–1250 hPa | ±0.5 hPa | ±0.08 hPa | ~0.5 m | 3.4 µA | 512 Byte | 3–9€ |
| BMP390 | 300–1250 hPa | ±0.5 hPa | ±0.03 hPa | ~0.25 m | 3.4 µA | 512 Byte | 4–10€ |
| MS5607 | 10–1200 mbar | ±1.5 hPa | ±0.2 hPa | ~1 m | 0.9 µA | Nein | 3–5€ |
| DPS310 | 300–1200 hPa | ±1 hPa | ±0.06 hPa | ~0.5 m | 1.7 µA | 32 samples | 3–5€ |

### 5.2 Entscheidung: BMP388

Der BMP388 ist die beste Wahl für Höhenmeter-Logging:

- **±0.08 hPa relative Genauigkeit** → ~0.5 m Höhenänderung detektierbar
- **512 Byte FIFO** → Sammelt bis zu 72 Samples ohne MCU-Intervention → wichtig für Deep-Sleep
- **200 Hz max ODR** → Schneller als nötig (1–10 Hz reicht)
- **3.4 µA bei 1 Hz** → Vernachlässigbar im Power-Budget
- **±0.5 hPa absolut** → Ausreichend für relative Höhenmessung

**Warum nicht BMP280?** Der BMP280 ist billiger (1–2€) aber:
- Kein FIFO → Jede Messung braucht MCU-Wake-Up
- ±1.0 m Höhen-Auflösung vs. ±0.5 m beim BMP388
- ±0.12 hPa rel. vs. ±0.08 hPa

Für 2€ mehr ist der BMP388 deutlich besser.

**Warum nicht BME280?** Luftfeuchtigkeit ist für einen Motorrad-Bordcomputer nicht relevant. BME280 kostet mehr bei gleicher Druck-Genauigkeit.

### 5.3 Höhenmeter-Logging

Die barometrische Höhenmessung berechnet Höhe aus Luftdruck:

```
h = 44330 * (1 - (P / P0)^(1/5.255))
```

Wobei P0 der Referenzdruck auf Meereshöhe ist (meist 1013.25 hPa).

**Relative Höhe** (wichtig für Touren-Logging):
- Beim Start: Aktuellen Druck als Referenz speichern
- Höhe = Differenz zum Referenzdruck → auf ±0.5 m genau
- Unabhängig von Wetter-Änderungen während der Fahrt (solange Referenz-Druck periodisch aktualisiert wird)

**Absolute Höhe** (für GPX-Tracks):
- GPS-Höhe ist ungenau (±10 m typisch)
- Barometrische Höhe ist genauer (±0.5 m), aber nur relativ
- Best Practice: GPS-Position + barometrische Höhe kombinieren

### 5.4 Temperatur-Kompensation

Der BMP388 hat eingebaute Temperatur-Kompensation. Die Koeffizienten sind werkskalibriert im OTP-ROM. Über die I2C-Schnittstelle kommen bereits kompensierte Werte.

TCO (Temperature Coefficient Offset): ±0.75 Pa/K → Bei 10°C Änderung: 7.5 Pa = ~0.6 m. Vernachlässigbar für Motorrad-Einsatz.

### 5.5 Wetter-Trend aus Luftdruck

Der Bordcomputer kann kurze Wetter-Vorhersagen aus dem Druck-Trend ableiten:

- Druck steigt (>1 hPa/3h): Wetter wird besser
- Druck fällt (>1 hPa/3h): Wetter wird schlechter
- Druck stabil: Keine Änderung

Das ist nicht hochpräzise, aber ein nützlicher Indikator auf dem Display.

### 5.6 I2C-Adresse & Verkabelung

BMP388 I2C-Adressen: 0x76 (SDO→GND) oder 0x77 (SDO→VCC).

Der BMI270 (IMU) ist auf 0x68 oder 0x69. Beide teilen sich den I2C-Bus:

```
I2C Bus (ESP32-S3):
  SDA → GPIO8   (I2C_NUM_0)
  SCL → GPIO9   (I2C_NUM_0)
  
  Adresse 0x68: BMI270 (IMU)
  Adresse 0x76: BMP388 (Baro)
```

Quellen:
- [BMP280 vs BME280 vs BMP388](https://zbotic.in/best-pressure-sensors-for-arduino-bmp280-vs-bme280-vs-bmp388)
- [BMP388 Adafruit](https://learn.adafruit.com/adafruit-bmp388-bmp390-bmp3xx/overview)
- [BMP388 Component Guide](https://componentindex.net/components/bmp388)
- [Sensor Comparison Hackaday](https://hackaday.io/project/86912-explog-exploration-logger/log/123294-choosing-the-right-sensors)

---

## 6. Power-Management

### 6.1 Bordspannung der PD06

Die Honda Transalp 600 (PD06) hat ein klassisches 12V-Bordnetz:

- **Lima-Output**: 13.5–14.4V bei laufendem Motor
- **Batterie-Ruhespannung**: 12.0–12.8V (Blei-Säure 12V)
- **Start-Spannungseinbruch**: Auf 9–10V beim Anlassen
- **Lastabwurf-Spikes**: Bis zu 40V Transienten bei induktiven Lasten (Relais, Hupe)

Der Bordcomputer muss mit all dem umgehen: 9V bis 15V Input-Range, Schutz gegen 40V-Spikes.

### 6.2 Power-Pfad Übersicht

```
12V Bordnetz ──[Sicherung 5A]──[TVS SMAJ15A]──[MP1584EN Buck]── 5V
                                                                      │
                          ┌───────────────────────────────────────────┘
                          │
              ┌───────────┴───────────┐
              │                       │
        HT7333 LDO            TP4056 + 18650
        5V → 3.3V             Backup-Akku
              │                       │
        3.3V Rail               MT3608 Boost
        (ESP32, Sensoren,        3.7V → 5V
         Display, GPS)                │
                                  5V Backup
```

### 6.3 DC/DC-Wandler: 12V → 5V (Buck)

| Modul | IC | Schaltfreq. | Max. Strom | Effizienz | Größe | Preis |
|---|---|---|---|---|---|---|
| MP1584EN-Modul | MP1584 (MPS) | 1.5 MHz | 3A | 90–92% | 22×15 mm | 1–2€ |
| LM2596-Modul | LM2596 (TI) | 150 kHz | 3A | 75–82% | 45×20 mm | 1.50–3€ |
| TPS5430-Modul | TPS5430 (TI) | 500 kHz | 3A | 88–91% | ~30×20 mm | 5–8€ |
| XL4015-Modul | XL4015 | 180 kHz | 5A | 85% | 40×20 mm | 2–4€ |

**Entscheidung: MP1584EN**

- Kleinste Fläche (22×15 mm) → passt ins Gehäuse
- Höchste Effizienz (90–92%) → weniger Wärme
- Hohe Schaltfrequenz → kleine Induktivitäten, wenig Ripple
- 4.5–28V Input → deckt 9–15V Bordnetz ab

**Kritisch:** Auto Load-Dumps (40V Spikes) überleben den MP1584 (max 28V) nicht. Schutz nötig:
- **TVS-Diode SMAJ15A** (15V standoff, 24.4V clamp) direkt am Input
- **5A Flachsicherung** (oder PTC 3A resettable) vor dem TVS
- Optional: 100µF Elko + 100nF Kerko am Input für Transienten

**Alternativ für Production:** TPS5430 (5.5–36V Input) überlebt Load-Dumps direkt. Teurer aber sicherer. Für Phase 2 (Custom PCB).

### 6.4 LDO: 5V → 3.3V

| LDO | Dropout | Strom | Rauschen | Quiescent | Preis |
|---|---|---|---|---|---|
| HT7333 | 90 mV | 250 mA | 30 µVrms | 4 µA | 0.30–0.50€ |
| AMS1117-3.3 | 1.1V | 1A | — | 5 mA | 0.05–0.15€ |
| MCP1700-3302E | 178 mV | 250 mA | — | 1.6 µA | 0.80–1.20€ |
| ME6211 | 100 mV | 500 mA | — | 5 µA | 0.30–0.60€ |

**Entscheidung: HT7333**

- Niedriger Rauschen → wichtig für ADC-Genauigkeit
- 250 mA reicht für ESP32-S3 + Sensoren (ESP32-S3 peak: ~500 mA für 20µs, HT7333 kann kurze Peaks mit einem 100µF Elko abfangen)
- 4 µA Quiescent → vernachlässigbar
- Preiswert

**Alternative für höhere Ströme:** ME6211 (500 mA) oder MCP1700-3302E.

### 6.5 Akku-Backup: 18650 + TP4056 + MT3608

Für Power-Loss-Backup (Daten retten, GPS-Track sichern,zeitgesteuertes Logging bei abgestelltem Motorrad):

**Komponenten:**

| Bauteil | Funktion | Preis |
|---|---|---|
| 18650 Li-Ion (NCR18650B, 3400 mAh) | Backup-Akku | 5–8€ |
| TP4056 Modul (mit DW01A Schutz) | Laden vom 5V-Rail | 0.30–0.60€ |
| MT3608 Boost-Modul | 3.7V → 5V | 0.50–1€ |
| Schottky-Diode (1N5822) | Power-Path ORing | 0.10€ |

**Schaltung (Power-Path-Management):**

```
5V (vom Buck) ──[1N5822 Schottky]──┬── 5V Rail → LDO → 3.3V
                                    │
TP4056 OUT+ ──[1N5822 Schottky]─────┘
                                    │
TP4056 B+/B- ←── 18650 Akku
TP4056 IN+/IN- ←── 5V (vom Buck, zum Laden)
MT3608 IN ←── TP4056 B+ (direkt vom Akku)
MT3608 OUT → 5V (zum Power-Path ORing)
```

**Wie es funktioniert:**
- Bei laufendem Motor: 5V vom Buck → versorgt alles + lädt den Akku über TP4056
- Bei Motor aus: Akku → MT3608 Boost → 5V → versorgt den Bordcomputer
- Schottky-Dioden verhindern Rückstrom (ORing)
- Spannungsdifferenz der Dioden: 0.3V (Schottky) → 4.7V am Rail. Die HT7333 LDO braucht min 3.4V → funktioniert.

**TP4056 Caveat:** Kein echtes Power-Path-Management! Wenn der Bordcomputer während des Ladens Strom zieht, kann der TP4056 die Ladung nicht korrekt terminieren (Ladestrom + Laststrom > C/10 Threshold). Lösung:
- Bei aktivem Bordcomputer (Motor läuft): TP4056 lädt, Last kommt vom Buck (über Diode) → kein Problem
- Bei Backup (Motor aus): TP4056 ist stromlos (5V fehlt), Last kommt vom Akku → kein Problem
- Problem nur: Wenn 5V vom Buck da ist, aber Last > 100mA zieht → TP4056 terminiert nie. Aber Last kommt ja über die Diode vom Buck, nicht vom TP4056-Output.

**Alternative:** IP5306-Modul (Charge + Boost + Power-Path in einem IC, ~2–4€). Eliminiert das TP4056/MT3608/Dioden-Setup, aber 5V Boost dann LDO = Doppelkonvertierung (etwas ineffizienter).

### 6.6 Battery Monitoring (ADC + Spannungsteiler)

Um den Akku-Ladestand zu überwachen:

```
18650 (+) ──[10kΩ]──┬── ADC1 GPIO1
                      │
                    [10kΩ]
                      │
GND ──────────────────┘
```

Spannungsteiler 1:2 → Bei 4.2V Akku: 2.1V am ADC. Bei 3.0V Akku: 1.5V am ADC.

ADC_ATTEN_DB_12 (0–2900 mV) abdecken. Messung:
```c
int raw = adc1_get_raw(ADC1_CHANNEL_0); // GPIO1
float voltage = raw * 2900.0 / 4095.0 * 2.0 / 1000.0; // Vbat
int percent = lithium_percent(voltage); // Lookup-Table
```

Li-Ion Entladekurve (nicht linear!):
- 4.2V = 100%
- 3.9V = ~70%
- 3.7V = ~50%
- 3.5V = ~20%
- 3.3V = ~5%
- 3.0V = 0% (cutoff)

### 6.7 Stromverbrauchs-Schätzung

Siehe [Abschnitt 11](#11-stromverbrauchs-schätzung) für die vollständige Tabelle.

---

## 7. SD-Karten-Logging

### 7.1 SPI-Mode vs. SDIO-Mode

Der ESP32-S3 unterstützt zwei Modi für SD-Karten:

| Modus | Bus-Width | Pins | Speed (typ.) | Speed (max.) |
|---|---|---|---|---|
| SPI | 4-wire (MOSI/MISO/CLK/CS) | 4 GPIO | 300–500 KB/s | ~1 MB/s |
| SDIO 1-bit | CMD/CLK/DAT0 | 3 GPIO | 5–10 MB/s | ~20 MB/s |
| SDIO 4-bit | CMD/CLK/DAT0-3 | 6 GPIO | 10–20 MB/s | ~40 MB/s |

**Für den Bordcomputer reicht SPI.** Die Telemetrie-Datenmenge ist klein:
- 10 Sensoren × 8 Byte × 10 Hz = 800 Byte/s = 2.9 MB/h
- GPS-Track (GPX): ~1 KB pro Minute = 60 KB/h

Bei SPI mit 400 KB/s schreibt das System die Daten in <1% der Zeit. Der Rest ist Deep-Sleep/Idle.

**Vorteil SPI:** Braucht nur 4 Pins, gut dokumentiert, funktioniert mit billigen MicroSD-Breakouts. SDIO braucht mehr Pins und hat Boot-Strap-Konflikte (GPIO12 beim ESP32 classic, beim S3 anders aber trotzdem beachten).

### 7.2 SD-Karten-Kompatibilität

| SD-Typ | Kapazität | Kompatibel? | Dateisystem |
|---|---|---|---|
| SDSC | ≤2 GB | Ja | FAT16/FAT32 |
| SDHC | 4–32 GB | Ja | FAT32 |
| SDXC | 64 GB – 2 TB | Ja, mit exFAT-Support | exFAT (FAT32 bei ≤32GB) |

**Empfehlung:** 8–16 GB SDHC, Class 10 (U1). Class 10 garantiert 10 MB/s min. Schreibrate. Viel mehr als SPI liefert. Keine High-End-Karte nötig — Marken-SD-Karte 16GB für ~5€ reicht.

**Wichtig:** Marken-Speicherkarte kaufen. No-Name-Karten haben oft falsche Kapazitätsangaben und hohe Ausfallraten. Eine Marken-SD-Karte Ultra 16GB oder eine Marken-SD-Karte EVO 16GB.

### 7.3 FATFS-Performance & Schreibstrategie

ESP-IDF FATFS-Performance hängt kritisch von der Buffer-Größe ab:

| Buffer-Größe | Schreibrate (SPI) | Bemerkung |
|---|---|---|
| 128 Byte (newlib default) | ~5 KB/s | Schlecht: jede fwrite() = 1 Sektor |
| 512 Byte (1 Sektor) | ~50 KB/s | OK für kleine Logs |
| 4 KB (Cluster-aligned) | ~300 KB/s | Gut |
| 32 KB (Cluster-aligned) | ~500 KB/s | Optimal für SPI |

**Kritisch:** Unaligned Writes (nicht Sektor-granular) triggern Read-Modify-Write im FATFS-Treiber. Eine 500-Byte fwrite() auf eine 512-Byte-Sektor-Grenze kostet 2× Sektor-Read + Sektor-Write statt 1× Write.

**Strategie: Ringbuffer → Batch-Write**

```c
#define LOG_BUFFER_SIZE 4096  // 1 Cluster oder 8 Sektoren
static uint8_t log_buffer[LOG_BUFFER_SIZE];
static size_t log_buffer_pos = 0;

void log_telemetry(const telemetry_t *data) {
    // Serialisiere ins Buffer
    size_t len = serialize(data, &log_buffer[log_buffer_pos]);
    log_buffer_pos += len;
    
    // Wenn Buffer voll: flush auf SD
    if (log_buffer_pos >= LOG_BUFFER_SIZE) {
        fwrite(log_buffer, 1, LOG_BUFFER_SIZE, log_file);
        log_buffer_pos = 0;
    }
}
```

**Flush-Strategie:**
- Alle 4KB (Cluster-Size) flushen
- Zusätzlich bei Timer (alle 10s) um Datenverlust bei Power-Loss zu minimieren
- Bei Power-Loss-Interrupt (GPIO oder Brownout): Letztes Flush retten

### 7.4 Dateiformate

**CSV (Telemetrie):**
```
timestamp,lat,lon,alt,speed,heading,roll,pitch,temp,vbat
1234567890,50.12345,11.23456,425.3,87.5,145.2,12.3,-2.1,18.4,3.92
```
Vorteile: Menschlich lesbar, Excel/Python direkt. Nachteile: Groß (ca. 100 Byte/Zeile).

**GPX (Track):**
```xml
<gpx>
  <trk>
    <trkseg>
      <trkpt lat="50.12345" lon="11.23456">
        <ele>425.3</ele>
        <time>2026-10-13T12:34:56Z</time>
      </trkpt>
    </trkseg>
  </trk>
</gpx>
```
Standard-Format für GPS-Tracks. Importierbar in jede Karten-App (Kurviger, OsmAnd, Geovisualisierungs-Software).

**BIN (Rohdaten):**
Binary-Format für maximale Dichte. Struct direkt schreiben. Etwa 40% kleiner als CSV.

**Empfehlung:** GPX für Tracks, CSV für Telemetrie. BIN optional für High-Rate-Logging.

### 7.5 Partition-Schema

SD-Karte Partitionierung:

```
Partition 1 (FAT32, 8 GB):  /tracks/    GPX-Dateien
                             /telemetry/ CSV-Dateien (nach Datum)
                             /config/    settings.json
                             /ota/       firmware.bin (OTA-Updates)
Partition 2 (optional, LittleFS, rest): /raw/  BIN-Logs
```

Einfacher: Eine FAT32-Partition mit Verzeichnisstruktur. Partitionierung nur nötig bei sehr großen Karten oder unterschiedlichen Dateisystemen.

Quellen:
- [ESP32 SD Card Optimization](https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html)
- [ESP-IDF SD/SDIO Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/sdmmc.html)
- [ESP-IDF SD Write Speed Issue](https://github.com/espressif/esp-idf/issues/6749)

---

## 8. Bluetooth / Smartphone-Anbindung

### 8.1 NimBLE vs. Bluedroid

| Eigenschaft | NimBLE | Bluedroid |
|---|---|---|
| BLE | Ja (BLE 5.0) | Ja (BLE 4.2/5.0) |
| BT Classic | Nein | Ja (SPP, A2DP, HFP) |
| RAM-Verbrauch | ~30–50 KB | ~50–100 KB |
| Flash-Verbrauch | ~80 KB | ~180 KB |
| API | NimBLE API + ESP wrapper | esp_ble_* API |
| Empfehlung für BLE-only | Ja (offiziell empfohlen) | — |
| Max Connections | Bis zu 9 (config) | Bis zu 7 |
| GATT-Server | Ja | Ja |
| GATT-Client | Ja | Ja |

**Entscheidung: NimBLE**

Der ESP32-S3 unterstützt nur BLE (kein BT Classic). NimBLE ist für BLE-only offiziell von Espressif empfohlen:
- 40–50% weniger RAM → mehr Heap für Application
- 55% weniger Flash → mehr Platz für Firmware
- Identische BLE-Features
- Support für BLE 5.0 Features (PHY 2M, Advertising Extensions)

In Arduino-ESP32 ab v2.0.4: `#define CONFIG_BT_NIMBLE_ENABLED 1` in `sdkconfig`.

### 8.2 GATT-Service-Design

Custom BLE-Profil für den Bordcomputer:

```
Service: "Transalp Bordcomputer" (UUID: 0001)
├── Characteristic: Speed (UUID: 0002) — Read/Notify
├── Characteristic: RPM (UUID: 0003) — Read/Notify
├── Characteristic: GPS Position (UUID: 0004) — Read/Notify
├── Characteristic: Altitude (UUID: 0005) — Read/Notify
├── Characteristic: Roll/Pitch (UUID: 0006) — Read/Notify
├── Characteristic: Temperature (UUID: 0007) — Read
├── Characteristic: Battery Level (UUID: 0008) — Read/Notify
├── Characteristic: Config (UUID: 0009) — Read/Write
├── Characteristic: Track Data (UUID: 000A) — Read (Long Read)
└── Characteristic: OTA Control (UUID: 000B) — Write
```

**Notify-Intervalle:**
- Speed/RPM: 500ms (oder bei Änderung >5%)
- GPS Position: 1s
- Roll/Pitch: 200ms (für Live-Anzeige)
- Battery: 60s
- Temperature: 60s

**Datenfluss Smartphone → Bordcomputer:**
- Config: Display-Helligkeit, Units (km/h vs mph), TZ-Offset
- NMEA-Stream (für GPS-Assist bei schlechtem Empfang)
- OTA-Firmware-Binärdaten (in Chunks)

**Datenfluss Bordcomputer → Smartphone:**
- Live-Telemetrie (Speed, RPM, Neigung, Höhe)
- Fertige GPX-Tracks (nach Fahrt-Ende)
- Crash-Alert (Push-Notification)
- Battery-Low-Warning

### 8.3 BLE-Profil: Custom vs. Standard

| Option | Vorteil | Nachteil |
|---|---|---|
| Custom-Profil | Voll flexible, nur was man braucht | Eigene App nötig |
| Cycling Speed/Cadence (CSC) | Standard-App-Support (Strava) | Nur Speed+Cadence, keine anderen Werte |
| Cycling Power Profile | Power+Cadence | Keine GPS/Neigung |
| Environmental Sensing | Standard für Temp/Druck | Keine Speed/Neigung |

**Empfehlung: Custom-Profil.** Es gibt keine Standard-BLE-Profile die alle Werte (Speed, RPM, GPS, Neigung, Höhe, Temp, Akku) abdecken. Die Custom-App ist ohnehin nötig für Track-Export, Config und OTA.

### 8.4 OTA-Update über BLE

OTA über BLE funktioniert in Chunks:

```
1. Smartphone → Bordcomputer: OTA Control Write "START" + fileSize
2. Bordcomputer: Flash erase, prepare partition
3. Smartphone → Bordcomputer: OTA Data Write (Chunk, 512 Byte default)
4. Bordcomputer: Write chunk to flash, ACK
5. Repeat 3-4 bis fileSize erreicht
6. Bordcomputer: Verify checksum, set boot partition, reboot
```

**Chunk-Größe:** 512 Byte ist ein guter Kompromiss. Größer = schneller, aber mehr RAM für Buffer. Bei 512 Byte und BLE 5.0 (2M PHY): ~100 KB/s → 2MB Firmware in ~20s.

**Flash-Partition-Layout (ESP32-S3, 8MB Flash):**
```
Partition    Offset    Size    Typ
nvs          0x9000    24 KB   data nvs
phy_init     0xF000    4 KB    data phy
ota_0        0x10000   1.5 MB  app ota_0
ota_1        0x280000  1.5 MB  app ota_1
spiffs       0x400000  2 MB    data spiffs
```

Zwei OTA-Slots (ota_0, ota_1) für A/B-Update. Beim Schreiben wird der inaktive Slot geflasht, dann gebootet. Schlägt der Boot fehl, fällt der Watchdog zurück auf den alten Slot.

Quellen:
- [ESP-IDF Bluetooth Overview](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-guides/ble/overview.html)
- [ESP-IDF BLE API Reference](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/bluetooth/index.html)
- [Arduino-ESP32 BLE Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE)

---

## 9. Relais-Steuerung

### 9.1 Übersicht

Der Bordcomputer steuert bis zu 4 Kanäle:
1. Heizgriffe (12V, ~3A max)
2. Zusatzscheinwerfer (12V LED, ~2A)
3. USB-Power (5V, ~2A)
4. 12V Zubehörsteckdose (12V, ~5A max)

### 9.2 Relais-Module

| Modul | Kanäle | Relais-Spannung | Trigger | Isolierung | Schaltleistung | Preis |
|---|---|---|---|---|---|---|
| 4-Kanal Relais-Modul (SRD-12V) | 4 | 12V DC | Active LOW | Optokoppler | 10A@250VAC / 10A@30VDC | 3–6€ |
| 4-Kanal Relais-Modul (SRD-5V) | 4 | 5V DC | Active LOW | Optokoppler | 10A@250VAC / 10A@30VDC | 2–5€ |
| 8-Kanal Relais-Modul | 8 | 12V DC | Active LOW | Optokoppler | 10A@250VAC / 10A@30VDC | 5–10€ |

**Wichtig:** Active-LOW bedeutet: GPIO LOW = Relais AN, GPIO HIGH = Relais AUS. Beim Boot muss GPIO also HIGH sein (oder Input mit Pull-Up), sonst klackern die Relais beim Start.

**Vermeide Strapping-Pins:** GPIO0, GPIO2, GPIO12, GPIO15 beim ESP32 (classic) bzw. GPIO0, GPIO3, GPIO45, GPIO46 beim S3. Boot-Time-State kann Relais kurz aktivieren.

### 9.3 MOSFET-Alternative: IRF520 / Logic-Level MOSFETs

Für rein DC-Lasten sind MOSFETs besser als Relais:
- Kein mechanischer Verschleiß
- Kein Klackern
- PWM-möglich (Heizgriffe stufenlos regeln)
- Schneller Schalten

| Modul | IC | Rds(on) | Vds max | Logic-Level | Preis |
|---|---|---|---|---|---|
| IRF520 Modul | IRF520 | 0.27 Ω | 100V | Ja (5V Gate) | 1–2€ |
| IRLZ44N Modul | IRLZ44N | 0.022 Ω | 55V | Ja (5V Gate) | 1.50–3€ |
| AO3400 (SMD) | AO3400 | 33 mΩ | 30V | Ja (3.3V Gate!) | 0.10€ |
| DMP4015SK3Q (P-Channel) | DMP4015 | 45 mΩ | 40V | Ja (3.3V) | 0.50€ |

**Problem mit IRF520 am ESP32-S3:** Der IRF520 ist kein echter Logic-Level MOSFET. Bei 3.3V Gate-Source ist er nicht voll durchgeschaltet (Rds(on) spec bei 10V Vgs). Für 3.3V-Logic braucht man:
- **IRLZ44N**: Spezifiziert bei Vgs=4V, funktioniert bei 3.3V gut genug für <5A
- **AO3400**: Voll spezifiziert bei Vgs=2.5V → perfekt für 3.3V, aber nur 30V/5A
- **Gate-Driver**: Wenn man IRF520 nutzen will, braucht man einen Transistor als Gate-Driver (3.3V → 12V Gate-Drive)

**Empfehlung für Heizgriffe (PWM-Steuerung):**
```
ESP32 GPIO (PWM) → TC4427 Gate-Driver → IRLZ44N MOSFET → Heizgriffe
```
Der TC4427 wandelt 3.3V PWM auf 12V Gate-Drive. Damit ist der IRLZ44N voll durchgeschaltet und kann 10A+ schalten. PWM-Frequenz: 500 Hz–1 kHz (überhörbar, geringe Switching-Verluste).

**Empfehlung für Schalt-Lasten (Licht, USB, Steckdose):**
- Relais-Modul (einfach, galvanisch getrennt, kein PWM nötig)
- Oder Logic-Level MOSFET (IRLZ44N direkt, wenn <5A)

### 9.4 Schaltungsbeispiel: Heizgriffe mit PWM

```
12V ──[5A Sicherung]──┬── Heizgriffe (+)
                        │
                        └── Drain (IRLZ44N)
                              Source ── GND
                              Gate ──[TC4427 Pin 6]
                              
ESP32 GPIO4 (LEDC PWM) ── TC4427 Pin 2 (IN A)
TC4027 Pin 3 (GND) ── GND
TC4427 Pin 6 (OUT A) ── Gate
TC4427 Pin 5 (VDD) ── 12V
TC4427 Pin 1 (VCC) ── 5V

Freilaufdiode (1N5408): Kathode an 12V, Anode an Drain (für induktive Last)
```

### 9.5 Vibrationssichere Verkabelung

- **Kabelquerschnitt:**
  - Steuerleitungen (GPIO): 0.14–0.25 mm² (AWG 26-24)
  - Last-Leitungen <5A: 0.75 mm² (AWG 18)
  - Last-Leitungen 5–10A: 1.5 mm² (AWG 16)
  - Last-Leitungen >10A: 2.5 mm² (AWG 14)

- **Steckverbinder:** MT115/Molex mit Rast, oder wasserdichte SuperSeal/AMP Superseal 1.5
- **Sicherung:** Nah an der Batterie (innerhalb 15cm). Flachstecksicherung oder glass tube 5×20
- **Kabelbinder:** Zusätzliche Sicherung gegen Vibration an fixen Punkten
- **Schutzschlauch:** Rippschlauch oder Schrumpfschlauch gegen Scheuern

### 9.6 Sicherungen

| Last | Strom | Sicherung | Kabel |
|---|---|---|---|
| Heizgriffe (paar) | ~3A | 5A | 0.75 mm² |
| Zusatzscheinwerfer LED | ~2A | 3A | 0.75 mm² |
| USB-Power | ~2A | 3A | 0.75 mm² |
| Zubehörsteckdose | ~5A | 7.5A | 1.0 mm² |
| Bordcomputer (gesamt) | ~0.5A | 1A | 0.25 mm² |

Sicherungstyp: Flachstecksicherung (ATO/ATC) im Sicherungskasten, oder Inline-Sicherungshalter direkt am Kabel.

### 9.7 Freilaufdiode für induktive Lasten

Bei Relais-Spulen und anderen induktiven Lasten: Freilaufdiode (Flyback Diode) parallel zur Last, um Spannungsspitzen beim Abschalten zu begrenzen.

- **1N4007** für kleine Lasten (1A, 1000V)
- **1N5408** für mittlere Lasten (3A, 1000V)
- **Schottky 1N5822** für schnelle Schaltung (3A, 40V, niedrere Durchlassspannung)

Kathode (Ring) zur +12V-Seite, Anode zur GND-Seite.

Quellen:
- [ShillehTek 2-Channel Relay Manual](https://shillehtek.com/blogs/shillehtek-product-manuals/2-channel-12v-relay-module-manual)
- [12V 8-Channel Relay Module Documentation](https://www.phippselectronics.com/support/12v-8-channel-relay-module-with-optocoupler-support-documentation/)
- [ESP32 Relay via PC817 + 2N7000](https://electronics.stackexchange.com/questions/719221/esp32-controls-a-12v-coil-relay-through-pc817-and-2n7000)
- [Motorcycle Heated Gear Wiring](https://ripsandrides.com/how-to-wire-heated-gear-to-motorcycle/)
- [Moto32 Hardware Guide](https://moto32.vercel.app/getting-started/installation)

---

## 10. Pin-Belegungs-Plan (gesamt)

### ESP32-S3-WROOM-1 (N8R2) — Vollständige Pin-Belegung

```
Funktion              GPIO     Bus/Interface    ADC    RTC    Bemerkung
═══════════════════════════════════════════════════════════════════════
─── SPI2 (FSPI) — E-Paper Display ───
EPD_MOSI              11       SPI2 MOSI               —      —
EPD_SCK               12       SPI2 SCK                —      —
EPD_CS                10       SPI2 CS                 —      —
EPD_DC                 9       GPIO (data/cmd)         —      Ja
EPD_RST               14       GPIO (reset)            —      Ja
EPD_BUSY              13       GPIO (input)            —      Ja

─── SPI3 (HSPI) — SD-Karte ───
SD_MOSI               35       SPI3 MOSI               —      —
SD_SCK                36       SPI3 SCK                —      —
SD_MISO               37       SPI3 MISO               —      —
SD_CS                 38       SPI3 CS                 —      —

─── I2C Bus 0 — Sensoren ───
I2C_SDA                8       I2C0 SDA                —      —
I2C_SCL                9       I2C0 SCL                —      —
  → BMI270 (0x68)
  → BMP388 (0x76)

─── UART — GPS ───
GPS_TX (→ MCU RX)     43       UART0 RX (default)      —      —
GPS_RX (← MCU TX)     44       UART0 TX (default)      —      —
  → NEO-M9N

─── ADC (ADC1, GPIO1–GPIO10) ───
VBAT_MONITOR           1       ADC1_CH0                Ja     Ja     Akku-Spannungsteiler
FUEL_LEVEL             2       ADC1_CH1                Ja     Ja     Tankgeber (analog)
TEMP_ENGINE            3       ADC1_CH2                Ja     Ja     Kühlmittel-Temp (NTC)

─── Pulse Counter (PCNT) ───
TACHO_PULSE            4       PCNT0                   —      Ja     Reed/Hall Vorderrad
RPM_PULSE              5       PCNT1                   —      Ja     Zündimpulse

─── Relais / MOSFET Steuerung ───
RELAY_HEAT_GRIP        6       GPIO (LEDC PWM)         —      —     Heizgriffe (PWM via TC4427)
RELAY_AUX_LIGHT        7       GPIO (Output)           —      —     Zusatzscheinwerfer
RELAY_USB_POWER       15       GPIO (Output)           —      —     USB-Power
RELAY_AUX_SOCKET     16       GPIO (Output)           —      —     12V Zubehör

─── Status / Debug ───
STATUS_LED            17       GPIO (Output)           —      —     LED Grün/Blau
BOOT_BTN               0       GPIO (Input)            —      Ja     Boot button (devkit)

─── USB (Native) ───
USB_DP                19       USB-OTG                 —      —
USB_DM                20       USB-OTG                 —      —

─── Intern (WROOM-1) ───
Flash/PSRAM           26–37    SPI0/1 (intern)         —      —     Nicht verwenden!
```

**Hinweis:** GPIO26–GPIO37 sind intern für Flash/PSRAM verbunden. Bei Octal-PSRAM (N8R2) zusätzlich GPIO33–37. **Niemals extern verwenden.**

**Strapping-Pins (nur beim Boot relevant):**
- GPIO0: Boot-Mode (LOW = Download-Mode)
- GPIO3: JTAG-Select
- GPIO45: VDDSPI Voltage
- GPIO46: Boot-Mode

Diese Pins können verwendet werden, aber der Zustand beim Boot muss bekannt sein. Für Relais-Steuerung: vermieden, um ungewolltes Schalten zu verhindern.

---

## 11. Stromverbrauchs-Schätzung

### 11.1 Aktiver Betrieb (Motor läuft, alle Module aktiv)

| Modul | Strom @3.3V | Strom @5V (via LDO) | Bemerkung |
|---|---|---|---|
| ESP32-S3 (Active, 240MHz, BLE) | 80 mA | 55 mA | Mit Wi-Fi aus, BLE an |
| E-Paper 2.9" (Refresh) | ~8 mA | ~6 mA | Nur beim Refresh, sonst ~0 |
| E-Paper 2.9" (Standby) | <0.1 mA | <0.1 mA | Bistabil |
| NEO-M9N GPS (4 GNSS, 10Hz) | 36 mA @3V | 25 mA | Tracking mode |
| BMI270 IMU (full ODR) | 0.7 mA | 0.5 mA | |
| BMP388 Baro (10 Hz) | 0.05 mA | 0.03 mA | |
| SD-Karte (write burst) | ~30 mA | ~20 mA | Nur beim Schreiben |
| HT7333 LDO Quiescent | 0.004 mA | — | |
| LED Status | 2 mA | 1.4 mA | |
| **Gesamt (aktiv)** | **~157 mA @3.3V** | **~108 mA @5V** | |

Über MP1584EN Buck (92% Effizienz): 108 mA @5V / 0.92 = 117 mA @5V Input = 585 mW. Bei 14V Bordnetz: ~42 mA. Die 12V-Batterie der Transalp (XTZ7S, 6Ah) hält das ewig — das ist vernachlässigbar.

### 11.2 Backup-Betrieb (Motor aus, Akku-only, Deep-Sleep)

| Modul | Strom @3.3V | Bemerkung |
|---|---|---|
| ESP32-S3 (Deep-Sleep, RTC only) | 7 µA | Bare chip. DevKit: ~5–20 mA |
| ESP32-S3 (Deep-Sleep + ULP) | ~50 µA | ULP pollt IMU/ADC |
| NEO-M9N (Power-Save, 1 Hz) | 7 mA | Optional für GPS-Tracking |
| BMI270 (Low-Power, Any-Motion) | 10 µA | Wake-on-Motion |
| BMP388 (1 Hz) | 3.4 µA | |
| MT3608 Quiescent | ~200 µA | Boost-Converter Idle |
| TP4056 Quiescent | ~5 µA | Nur bei anliegender 5V |
| **Gesamt (Deep-Sleep + GPS)** | **~7.3 mA** | Mit GPS-Tracking |
| **Gesamt (Deep-Sleep, kein GPS)** | **~0.3 mA** | Nur Wake-on-Motion |

### 11.3 Akku-Laufzeit (18650, 3400 mAh)

Effektive Kapazität bei 3.3V (via MT3608 Boost + LDO, ~85% gesamt):
```
3400 mAh × 3.7V × 0.85 / 3.3V = ~3240 mAh @3.3V
```

| Modus | Strom | Laufzeit |
|---|---|---|
| Voll aktiv (alle Module) | 157 mA | ~20 Stunden |
| Backup mit GPS-Tracking (1 Hz) | 7.3 mA | ~443 Stunden (~18 Tage) |
| Deep-Sleep nur Wake-on-Motion | 0.3 mA | ~10.800 Stunden (~450 Tage) |
| Deep-Sleep ohne alles | 0.01 mA | ~324.000 Stunden (~37 Jahre) |

**Realistische Szenarien:**
- **Tour 8h Fahrt**: 157 mA × 8h = 1.256 mAh = 39% einer 18650 → Batterie nicht nötig, Bordnetz versorgt
- **Parken 1 Woche mit GPS-Log (Diebstahlschutz)**: 7.3 mA × 168h = 1.226 mAh = 38% → Akku reicht
- **Parken 1 Monat nur Wake-on-Motion**: 0.3 mA × 720h = 216 mAh = 7% → Akku fast voll

Quellen:
- [TP4056 + MT3608 Power Path (smol project)](https://github.com/jphein/smol/blob/main/docs/power.md)
- [TP4056 + 18650 + Boost Guide](https://makerhub.my/tutorials/tp4056-18650-charging-safely)
- [Solar/18650 Battery Guide](https://makerhub.my/tutorials/solar-power-arduino-18650)
- [MP1584 vs LM2596 Comparison](https://industrialmonitordirect.com/blogs/knowledgebase/mp1584en-vs-lm2596-buck-converter-selection-for-esp32-led-control)
- [Buck Converter Selection Guide](https://electricalflux.com/mcu-general/arduino-buck-converter-voltage-compatibility-guide)
- [LM2596 Alternatives](https://hitop-tech.com/blogDetail/lm2596-alternative-5-best-pin-compatible-replacements/113.html)
