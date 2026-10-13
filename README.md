# Transalp Bordcomputer

Custom Bordcomputer für die Honda Transalp 600 (PD06, Baujahr 1986–2000) — DIY-Projekt von Tito.

## Überblick

Eigenbau eines kompakten Bordcomputers für Motorrad-Touren (Honda Transalp 600 PD06).
Die PD06 ist eine rein mechanische Maschine (kein CAN-Bus, kein OBD-II, keine ECU im modernen Sinn).
Bordcomputer basiert auf ESP32-S3 als Core-Modul, erweiterbar um Sensoren, GPS und Display.
Fahrzeugdaten werden über direkte Sensor-Anzapfung und separate Sensoren erfasst.

## Features (geplant)

- **GPS-Tracking**: Positionsdaten, Geschwindigkeit, Strecke
- **Display**: E-Paper oder TFT für sonnenlesbare Anzeige
- **Sensoren**: Temperatur, Luftdruck, Neigung, Beschleunigung
- **Daten-Logging**: SD-Karte für Touren-Aufzeichnung
- **Bluetooth**: Smartphone-Anbindung für Navigation/Telefonie
- **Power-Management**: Akku-Überwachung, automatische Schlafmodi
- **Fahrzeugdaten (PD06-spezifisch)**: Direkte Sensor-Anzapfung — Tachosignal, Drehzahl (Zündung), Temperatur, Ganganzeige, Kraftstoffstand
- **Zusätzliche Sensoren**: Öldruck, Umwelttemperatur, Neigung (Wheelie-Detection)
- **Steuerung**: Relais-basiert — Zusatzlicht, Heizgriffe, USB-Ladung
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
| Tachosignal | Optokoppler/Induktiv | Tacho-Kabel anzapfen |
| Drehzahl | Induktiver Sensor am Zündkabel | oder Hall-Sensor |
| Ganganzeige | Mikroschalter/Gabellichtschranke | Schaltwalze |
| Kraftstoffstand | Widerstandssensor im Tank | Spannungsteiler |
| Temp Motor | KTY81/PT1000 im Kühlschlauch | oder Thermistor |
| Öldruck | Drucksensor 0-10bar | 0.5-4.5V Output |
| Steuerung | Relais-Modul 4fach | Licht, Heizgriffe, etc. |

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
├── components/          # Wiederverwendbare Komponenten
│   ├── gps/             # GPS-Treiber + NMEA-Parsing
│   ├── display/         # E-Paper Treiber + UI
│   ├── sensors/         # IMU, BMP280
│   ├── vehicle/         # PD06-Sensoren: Drehzahl, Tacho, Gang, Temp, Kraftstoff
│   ├── logger/          # SD-Karten Logging
│   ├── ble/             # Bluetooth LE Kommunikation
│   ├── power/           # Power Management
│   └── control/         # Relais-Steuerung (Licht, Heizgriffe, etc.)
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

🚧 Planungsphase — Recherche abgeschlossen (33.500 Wörter in `docs/research/`), Hardware-Teile werden spezifiziert.

## License

MIT
