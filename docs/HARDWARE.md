# Hardware

## Bauteile

### Core
- **ESP32-S3-DevKitC-1** (ESP32-S3-WROOM-1, 16MB Flash, 8MB PSRAM)
- WLAN 802.11 b/g/n, Bluetooth 5 (LE), USB-OTG
- 45 GPIO, Dual-Core 240MHz, Ultra-Low-Power-RTC

### GPS
- **u-blox NEO-M9N** (Breakout-Board, z.B. SparkFun oder Waveshare)
- 10Hz Update-Rate, 72-Kanal, -167dBm Tracking
- Konfigurierbar via UBX-Protocol (UART, 38400 baud)
- PPS-Pin für genaue Zeitsynchronisation
- Antenne: Active Patch Antenna 25x25mm, 3m Kabel (Magnetfuß)

### Display
- **Waveshare 2.9" E-Paper (296x128,黑白)**
- SPI-Anschluss, Partial-Refresh für Live-Daten
- Sonnenlesbar, ~0mA im statischen Zustand
- Alternative: **1.54" E-Paper** falls Baugröße kritisch

### Sensoren
- **MPU6050** (Gyro + Accel, I2C)
  - Neigung/Rollwinkel, Crash-Detection, Bewegungs-Sleep-Trigger
- **BMP280** (Temp + Baro, I2C)
  - Höhe, Höhenmeter, Umgebungstemperatur
  - I2C-Adresse: 0x76 (oder 0x77, je Modul)

### Daten-Logging
- **MicroSD-Slot (SPI)** mit Level-Shifter (SD 3.3V, ESP32 3.3V direkt)
- Class 10, 8-32GB, FAT32
- Braucht dedizierten CS-Pin + Pull-Up

### Power
- **LiPo 18650 (3400mAh, 3.7V)** im Wechselgehäuse
- **TP4056** Lade-Controller (USB-C Eingang)
- **MT3608 Boost** 3.7V → 5V (für Display/OBD)
- **HT7333 LDO** 5V → 3.3V (ESP32 + Sensoren)
- Akku-Überwachung: Spannungsteiler an ADC-Pin

### OBD-II
- **ELM327 Bluetooth** (fertiges Modul, pairing mit ESP32 BT-Classic)
- OBD-II Adapterkabel (J1962 → 4-polig, Transalp hat OBD unten)
- Alternativ: Direkt K-Line (ISO 9141) für nackte Fahrzeugdaten

### Gehäuse
- 3D-gedruckt (PETG oder ABS), wasserresistent (IP54)
- Siebdruck-Front für Display-Ausschnitt
- Montage: Lenkerhalter (Ø 22mm) oder Crossbar
- Kabeldurchführungen: PG7 Verschraubungen

## Spannungsversorgung

```
LiPo 3.7V ──┬──► TP4056 (Laden via USB-C)
            │
            ├──► MT3608 Boost → 5V ──┬──► E-Paper
            │                        ├──► ELM327 (falls direkt)
            │                        └──► HT7333 LDO → 3.3V ──┬──► ESP32-S3
            │                                                  ├──► MPU6050
            │                                                  ├──► BMP280
            │                                                  ├──► NEO-M9N
            │                                                  └──► SD-Karte
            │
            └──► Spannungsteiler ──► ADC (Akku-Überwachung)
```

## Stromabschätzung

| Komponente | Strom (aktiv) | Strom (sleep) |
|---|---|---|
| ESP32-S3 | ~80mA | ~10µA (Deep Sleep) |
| NEO-M9N | ~25mA | ~5mA (Backup) |
| E-Paper 2.9" | ~2mA (Refresh) | ~0mA (statisch) |
| MPU6050 | ~3.5mA | ~10µA |
| BMP280 | ~3.6µA | ~0.3µA |
| SD-Karte | ~30mA (Write) | ~200µA |
| MT3608 Quiescent | ~0.3mA | — |
| **Summe aktiv** | **~140mA** | |
| **Summe sleep (GPS off)** | | **~0.4mA** |

→ 3400mAh Akku: ~24h aktiv, ~Wochen im Sleep-Modus.

## Werkzeug / Diagnose

- Logik-Analyzer (Saleae kompat.) an UART und I2C für Debug
- U-Center (macOS via Windows-VM oder u-center2 für macOS) für GPS-Konfig
- ESP-IDF Monitor für Flash/Debug
