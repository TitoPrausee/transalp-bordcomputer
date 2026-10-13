# Architektur

## System-Übersicht

```
                    ┌─────────────────────────────┐
                    │       ESP32-S3 (Core)        │
                    │    FreeRTOS / ESP-IDF v5.x   │
                    └──────┬──────────┬───────────┘
                           │          │
          ┌────────────────┼──────────┼────────────────┐
          │               │          │                  │
    ┌─────┴─────┐  ┌──────┴──────┐ ┌─┴────────┐ ┌──────┴──────┐
    │  GPS Task │  │ Sensor Task │ │Disp Task │ │ Logger Task │
    │  (UART)   │  │  (I2C)      │ │(SPI)     │ │  (SPI/SD)   │
    └─────┬─────┘  └──────┬──────┘ └─┬────────┘ └──────┬──────┘
          │               │          │                  │
    NEO-M9N          MPU6050+BMP280  E-Paper         MicroSD
```

## Task-Modell (FreeRTOS)

Alle Tasks laufen auf dem ESP32-S3 Dual-Core. Prioritäten sind Vorschläge.

| Task | Core | Prio | Stack | Rate | Beschreibung |
|---|---|---|---|---|---|
| `gps_task` | 1 | 5 | 4096 | 10 Hz | Liest NMEA über UART, parst Position/Geschwindigkeit |
| `sensor_task` | 1 | 4 | 4096 | 50 Hz | IMU+Baro über I2C, gleitender Mittelwert |
| `display_task` | 0 | 2 | 6144 | 1 Hz | Rendert UI auf E-Paper (Partial-Refresh) |
| `logger_task` | 0 | 3 | 4096 | 5 Hz | Schreibt GPS+Sensordaten auf SD (Ringbuffer) |
| `ble_task` | 0 | 3 | 4096 | Event | BLE-GATT für Smartphone-Kommunikation |
| `power_task` | 0 | 6 | 2048 | 1 Hz | Akku-Überwachung, Sleep-Entscheidung |
| `obd_task` | 1 | 2 | 4096 | 2 Hz | OBD-II Polling ( wenn ELM327 verbunden) |

## Inter-Task-Kommunikation

- **Queues**: GPS → Display, GPS → Logger, Sensors → Display/Logger
- **EventGroups**: Power-States (WAKE / SLEEP / SHUTDOWN)
- **Semaphore**: SPI-Bus (Display + SD teilen sich VSPI)
- **RingBuffer**: Roh-IMU-Daten für Crash-Detection (letzten 5s)

## Datenfluss

```
GPS ──► gps_data_queue ──► Display (Position, Speed)
  │                    └──► Logger (GPX-Track)
  │
IMU ──► sensor_data_queue ──► Display (Neigung)
  │                        └──► Logger (Telemetrie)
  │
BMP280 ─► sensor_data_queue ──► Display (Temp, Höhe)
  │                          └──► Logger
  │
OBD-II ──► obd_data_queue ──► Display (Drehzahl, Temperatur)
  │                       └──► Logger
  │
BLE ◄──► Smartphone (Navigation, Wetter, Phone-Notifications)
  │
Power ──► System-State (Voting: Bewegung? Spannung? Timer?)
```

## Speicher-Layout

- **NVS**: Konfiguration, Kalibrierung, letzte Position
- **SD-Karte**: `/tracks/` (GPX), `/logs/` (CSV-Telemetrie), `/config/` (Einstellungen)
- **PSRAM** (falls vorhanden): Display-Framebuffer, Tile-Cache

## Power-Management

Zustände:
1. **ACTIVE**: Vollbetrieb, alle Tasks, Display an
2. **IDLE**: Display aus, GPS+Logger an (nur Track-Aufzeichnung)
3. **SLEEP**: Nur RTC + Akku-Überwachung, aufwecken durch Bewegung (IMU-Interrupt) oder Vibration
4. **SHUTDOWN**: Akku < 3.2V, sauberes SD-Flush + Power-Off

Übergänge:
- ACTIVE → IDLE: 60s keine Display-Interaktion + keine BT-Verbindung
- IDLE → SLEEP: 5min keine Bewegung (IMU < Schwellwert)
- SLEEP → ACTIVE: IMU-Interrupt oder BT-Verbindungsanfrage
- *→ SHUTDOWN: Akku < 3.2V (sofort, alle Zustände)

## OTA-Update

Über BLE: Smartphone sendet Firmware-Image in 4KB-Chunks → ESP32 schreibt ins OTA-Partition → Boot-Flag setzen → Neustart.
Rollback-Schutz: Bei Boot-Loop nach Update → automatischer Rollback zur vorherigen Partition.
