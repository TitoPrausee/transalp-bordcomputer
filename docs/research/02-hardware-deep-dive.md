# Deep Dive: Bordcomputer-Hardware — ESP32-S3, GPS, Display, Sensoren, Power-Management

> Stand: Oktober 2026 — Recherche-Grundlage für die Hardware-Auslegung.
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986–2000, rein mechanisch, keine ECU.

## 0. Zusammenfassung

Der Bordcomputer für die Transalp 600 PD06 basiert auf dem **ESP32-S3-WROOM-1** als
Zentraleinheit. Die PD06 hat keine ECU, keinen CAN-Bus, keine OBD-Schnittstelle —
alle Fahrdaten werden direkt über analoge Sensoren, Pickups und Spannungsteiler
erfasst. Dieses Dokument deckt die komplette Hardware-Auslegung ab: MCU-Plattform,
GPS, Display, IMU, Barometer, Power-Management, SD-Logging, Bluetooth und Relais-Steuerung.

Die Architektur ist zweistufig ausgelegt:
1. **Prototyp-Phase**: ESP32-S3-DevKitC-1 auf Breadboard/Perfboard mit Breakout-Modulen
2. **Production-Phase**: Custom PCB mit ESP32-S3-WROOM-1, integrierten Sensoren, Power-Management

---

## 1. MCU-Plattform: ESP32-S3

### 1.1 Plattform-Vergleich

| MCU | Takt | RAM | Flash | BLE | BT-Classic | GPIO | ADC | Preis (EUR) | Fazit |
|---|---|---|---|---|---|---|---|---|---|
| ESP32-S3 | 240 MHz | 512 KB + PSRAM | 4-16 MB | BLE 5.0 | Nein | 45 | 20 (ADC1+ADC2) | 4-6 | **Gewinner** |
| ESP32-C6 | 160 MHz | 512 KB | 4 MB | BLE 5.3 + Zigbee | Nein | 30 | 8 | 4-5 | Gut, aber weniger ADC/Pins |
| ESP32 (Classic) | 240 MHz | 520 KB | 4-16 MB | BLE 4.2 | Ja (SPP) | 34 | 18 | 5-7 | BT-Classic ist Vorteil, aber veraltet |
| RP2040 | 133 MHz | 264 KB | extern | Nein (extern) | Nein | 26 | 4 | 4-5 | Kein BT, zu wenig ADC |
| Teensy 4.0 | 600 MHz | 1 MB | 2 MB | Nein | Nein | 40 | 14 | 20-25 | Überpowered, kein BT, teuer |
| STM32F4 | 168 MHz | 192 KB | 1 MB | Nein (extern) | Nein | 50+ | 16 | 8-12 | Enterprise, aber komplexes Toolchain |

**Entscheidung: ESP32-S3-WROOM-1.** Gründe:
- Ausreichend GPIO (45 nutzbare, davon 22 über USB-OTG belegbar -> 22+ frei)
- BLE 5.0 mit Long Range und Mesh-Support
- Dual-Core 240 MHz fuer Sensor-Sampling + UI-Rendering parallel
- Integrierter USB-Serial-JTAG (kein separater CH340/CP2102 noetig)
- ESP-IDF als professionelles SDK, FreeRTOS integriert
- PSRAM-Option fuer Display-Framebuffer und GPS-Puffer
- Grosse Community, Arduino + ESP-IDF Support

Der ESP32-Classic (ESP32-WROOM-32) haette BT-Classic mit SPP-Profil — das waere
fuer einfache serielle Bluetooth-Verbindungen interessant. Aber BLE 5.0 am S3
ist moderner, hat hoeheren Throughput (2M PHY) und NimBLE als schlanken Stack.
SPP-Emulation ueber BLE GATT ist problemlos moeglich.

### 1.2 ESP32-S3 GPIO-Ressourcen

#### ADC-Kanaele
Der ESP32-S3 hat **zwei ADC-Einheiten**:
- ADC1: 7 Kanaele (GPIO1-GPIO10, bei bestimmten Konfigurationen)
- ADC2: 13 Kanaele (GPIO11-GPIO20)
- Gesamt: 20 ADC-Kanaele bei 12-bit Aufloesung (0-3.3V)

**Einschraenkung**: ADC2 wird teilweise von Wi-Fi belegt. Bei aktivem WLAN sind nicht
alle ADC2-Kanaele nutzbar. Da der Bordcomputer primaer BLE nutzt (nicht WLAN), bleibt
ADC2 groesstenteils verfuegbar. Fuer sicherheitsrelevante Sensoren (Kraftstoff,
Temperatur) nur ADC1 verwenden.

12-bit ADC -> 4096 Stufen bei 0-3.3V -> Aufloesung ~0.8mV. Fuer Kraftstoffpegel
(0-12V ueber Spannungsteiler auf 0-3.3V) reicht das aus.

#### Pulse-Counter / PCNT
Der ESP32-S3 hat **4 Pulse-Counter-Einheiten** (PCNT), jeweils mit 2 Kanaelen = 8 Kanaele
gesamt. Perfekt fuer:
- Drehzahl-Pickup (Reedschalter/Hall-Sensor am Zuendkabel)
- Tacho (Hall-Sensor am Vorderrad)
- Radgeschwindigkeit hinten (falls ergaenzt)

Jede PCNT-Einheit zaehlt bis 32767, dann Overflow-Interrupt. Fuer 10.000 RPM bei
4-Zylinder = 40.000 Impulse/s — das schafft der PCNT problemlos (Maximal 40 MHz Input).

#### GPIO-Matrix / GPIOMUX
Der ESP32-S3 hat eine flexible GPIO-Matrix — fast jeder Pin kann fast jede Funktion
uebernehmen. Ausnahmen:
- **GPIO26-GPIO32**: verbunden mit internem Flash (nicht herausgefuehrt, nicht nutzbar)
- **GPIO33-GPIO37**: verbunden mit PSRAM bei WROOM-1 mit PSRAM-Variante
- **GPIO19/GPIO20**: USB D-/D+ (nur nutzbar wenn USB-OTG nicht aktiv)
- **GPIO0**: Boot-Strap-Pin (must be HIGH at boot, avoid as input with pull-down)

**Frei nutzbare GPIOs bei ESP32-S3-WROOM-1 (mit PSRAM)**: ~22 GPIOs.

#### I2C, SPI, UART
- **I2C**: 2 Busse (I2C0 + I2C1), beliebige Pin-Zuweisung. Alle Sensoren (IMU, Baro) an I2C0.
- **SPI**: 4 Busse (SPI0-SPI3). SPI0/SPI1 fuer internen Flash/PSRAM reserviert.
  SPI2 (FSPI) fuer Display, SPI3 fuer SD-Karte.
- **UART**: 3 UART-Interfaces. UART0 fuer USB-Serial-JTAG/Debug, UART1 fuer GPS, UART2 frei.

### 1.3 DevKitC-1 vs. Custom PCB

| Kriterium | DevKitC-1 | Custom PCB (WROOM-1) |
|---|---|---|
| Entwicklungszeit | Sofort einsatzbereit | 2-4 Wochen Design + Bestueckung |
| Groesze | 69.7x52.4 mm | Beliebig (z.B. 50x50 mm) |
| GPIO-Zugang | Pin-Header | Direkte Leiterbahnen |
| PSRAM | Optional (N16R8-Variante) | Nach Wahl |
| USB | USB-C onboard | USB-C eigens designed |
| Power | USB-only, kein Power-Management | Integriertes Power-Management |
| Kosten (Prototyp) | ~6 EUR | ~15-25 EUR (PCB + Bestueckung) |
| Robustheit | Steckverbindungen, lose Kabel | Geloetet, vibrationsfest |

**Strategie**: Phase 1 mit DevKitC-1 (N16R8 = 16MB Flash + 8MB PSRAM) auf
Perfboard entwickeln. Phase 2: Custom PCB mit integriertem Power-Management,
SD-Slot, Sensor-Footprints und Relais-Treiber.

### 1.4 PSRAM: Braucht man es?

**Ja, unbedingt.** Die Variante **ESP32-S3-WROOM-1-N16R8** (16MB Flash, 8MB PSRAM)
ist die empfohlene Variante. Gruende:
- **Display-Framebuffer**: 2.9" E-Paper 296x128 = 4.744 Bytes (1-bit). Bei 4-bit Graustufen
  = 9.488 Bytes. Im internen RAM (512 KB) machbar, aber PSRAM entlastet.
- **GPS-NMEA-Puffer**: NMEA-Saetze koennen lang sein. Mit PSRAM kein Risiko von Buffer-Overflows.
- **SD-Logging-Ringbuffer**: Batch-Write reduziert SD-Verschleiss. 64KB Ringbuffer im PSRAM.
- **Font-Rendering**: Bei TFT-Displays mit Farbtiefe braucht ein Font-Cache erheblich Platz.

PSRAM-Zugriff ist langsamer als internes RAM (~40 MHz QSPI vs. On-Chip SRAM), aber fuer
nicht-zeitkritische Daten (Framebuffer, Puffer) vollkommen ausreichend.

Kostenunterschied: N16R2 (2MB PSRAM) vs. N16R8 (8MB PSRAM) ~0.50 EUR. N16R8 nehmen.

### 1.5 Deep-Sleep und Low-Power

| Modus | Stromverbrauch | Wake-Sources | RAM |
|---|---|---|---|
| Active | ~80-240 mA | — | Alle |
| Modem Sleep | ~3-20 mA | Timer, GPIO | Alle |
| Light Sleep | ~130 µA | GPIO, Timer, UART | Alle (erhalten) |
| Deep Sleep | ~10-44 µA | GPIO (EXT0/EXT1), Timer, ULP | Nur RTC-Memory |
| Hibernation | ~5 µA | RTC-GPIO, Timer | RTC-Memory |

**Wake-Strategie fuer Motorrad**:
- Zuendung AN -> ESP32 active ueber Power-Good-Signal vom DC/DC-Converter
- Zuendung AUS -> ESP32 bekommt Interrupt (GPIO) vom Zuendungs-Signal -> schreibt
  letzten Stand auf SD, geht in Deep Sleep
- Deep Sleep -> Wake Sources: ULP (Ultra-Low-Power Coprozessor) ueberwacht IMU-Interrupt
  (Bewegungserkennung = Diebstahlwarnung) oder Timer (alle 30 Min GPS-Fix fuer Track-Logging)

**Wichtig**: Der ESP32-S3 hat einen **ULP-RISC-V-Coprozessor**, der im Deep Sleep laeuft
und GPIO/I2C/SARADC ueberwachen kann. Der ULP kann den IMU-Interrupt abfangen und den
Hauptprozessor wecken. Das ist die Basis fuer die Diebstahlsicherung.

Quellen:
- [ESP32-S3 Datasheet (Espressif)](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ESP32-S3-DevKitC-1 Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html)

ESP32-S3-WROOM-1 Preise: 4.50-6.00 EUR (Mouser, Digi-Key, LCSC). DevKitC-1: 6-8 EUR.

---
