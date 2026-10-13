# Transalp Bordcomputer

Custom Bordcomputer für die Honda Transalp — DIY-Projekt von Tito.

## Überblick

Eigenbau eines kompakten Bordcomputers für Motorrad-Touren (insb. Transalp).
Basierend auf einem ESP32-S3 als Core-Modul, erweiterbar um Sensoren, GPS und Display.

## Features (geplant)

- **GPS-Tracking**: Positionsdaten, Geschwindigkeit, Strecke
- **Display**: E-Paper oder TFT für sonnenlesbare Anzeige
- **Sensoren**: Temperatur, Luftdruck, Neigung, Beschleunigung
- **Daten-Logging**: SD-Karte für Touren-Aufzeichnung
- **Bluetooth**: Smartphone-Anbindung für Navigation/Telefonie
- **Power-Management**: Akku-Überwachung, automatische Schlafmodi
- **OBD-II**: Fahrzeugdaten auslesen (Drehzahl, Temp, Fehlercodes)
- **Wetter**: Live-Wetterdaten via BT-Internet vom Smartphone

## Hardware (geplant)

| Komponente | Modul | Bemerkung |
|---|---|---|
| MCU | ESP32-S3 DevKit | WiFi+BT, genug GPIO, low power |
| GPS | u-blox NEO-M9N | 10Hz, hochpräzise |
| Display | Waveshare 2.9" E-Paper | sonnenlesbar, low power |
| IMU | MPU6050 | Neigung/Beschleunigung |
| Temp/Druck | BMP280 | Umgebung, Höhenmeter |
| SD | MicroSD-Modul (SPI) | Daten-Logging |
| Power | LiPo + TP4056 + Boost | 3.3V/5V Versorgung |
| OBD-II | ELM327 BT oder K-Line | Fahrzeugdaten |

## Software-Architektur (geplant)

- **FreeRTOS** als Scheduler (kommt mit ESP-IDF)
- Modulare Tasks: GPS, Display, Sensoren, Logging, BLE
- Header-only Config für Pin-Mapping und Feature-Toggles
- OTA-Update über Bluetooth

## Repository-Struktur

```
transalp-bordcomputer/
├── README.md
├── docs/
│   ├── ARCHITECTURE.md      # Software-Architektur, Task-Modell
│   ├── HARDWARE.md          # Schaltplan, Pin-Mapping, Gehäuse
│   └── PINMAP.md            # GPIO-Belegung
├── firmware/
│   ├── main/                # ESP-IDF main app
│   ├── components/          # Wiederverwendbare Komponenten
│   │   ├── gps/             # GPS-Treiber + NMEA-Parsing
│   │   ├── display/         # E-Paper Treiber + UI
│   │   ├── sensors/         # IMU, BMP280
│   │   ├── logger/          # SD-Karten Logging
│   │   ├── ble/             # Bluetooth LE Kommunikation
│   │   ├── power/           # Power Management
│   │   └── obd/             # OBD-II Interface
│   ├── CMakeLists.txt
│   └── sdkconfig.defaults
├── hardware/
│   ├── kicad/               # KiCad Schaltpläne + PCB
│   └── case/               # Gehäuse-Design (STL/FreeCAD)
├── tools/
│   └── export_track.py      # GPX-Export aus SD-Logs
└── LICENSE
```

## Status

🚧 Planungsphase — Hardware-Teile bestellt, Firmware-Struktur steht.

## License

MIT
