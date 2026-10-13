# System-Architektur

> **Zentrale Referenz** fuer das Bordcomputer-System (Phase 3)
> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1 (N16R8: 16 MB Flash, 8 MB Octal PSRAM), Dual-Core 240 MHz
> Framework: ESP-IDF v5.4 mit FreeRTOS SMP
> Fahrzeug: Honda XL600V Transalp (PD06), Baujahr 1986-1999, rein mechanisch, keine ECU
> Companion-App: React Native (beide mobilen Betriebssysteme)

---

## Inhaltsverzeichnis

1. [System-Uebersicht](#1-system-uebersicht)
2. [Firmware-Architektur](#2-firmware-architektur)
3. [Komponenten-Details](#3-komponenten-details)
4. [Hardware-Schnittstellen](#4-hardware-schnittstellen)
5. [BLE-Protokoll](#5-ble-protokoll)
6. [Companion-App Architektur](#6-companion-app-architektur)
7. [Datenmodell](#7-datenmodell)
8. [Power-Management](#8-power-management)

---

## 1. System-Uebersicht

### 1.1 Blockdiagramm

Das Bordcomputer-System besteht aus drei Hauptebenzen: der Firmware (ESP32-S3),
der Hardware-Peripherie (Sensoren, Displays, Speicher) und der Companion-App
(Smartphone). Die Firmware ist das Zentrum: Sie akquiriert Sensordaten, rendert
die UI, loggt auf SD-Karte und kommuniziert per BLE mit der App.

```
                    +-----------------------------------------------------+
                    |               ESP32-S3 (MCU Core)                   |
                    |         FreeRTOS SMP / ESP-IDF v5.4                 |
                    |                                                     |
                    |   Core 1 (Echtzeit)         Core 0 (UI/Comms)     |
                    |   +----------------+        +----------------+      |
                    |   | sensor_task    |        | display_task   |      |
                    |   | 50 Hz          |  Queues| 30 FPS         |      |
                    |   | IMU, BMP280,   |------->| Framebuffer,   |      |
                    |   | ADC, PCNT      |        | Dirty-Rect     |      |
                    |   +----------------+        +-------+--------+      |
                    |   | gps_task       |                |               |
                    |   | 10 Hz          |-------+        |               |
                    |   | UART, NMEA     |       |        |               |
                    |   +----------------+       v        v               |
                    |   | (anim_task)    |   +----------------+           |
                    |   | 60 Hz          |   | logger_task    |           |
                    |   | Spring/Tween   |   | 5 Hz, SD-Karte |           |
                    |   +----------------+   +----------------+           |
                    |                         ^         ^                |
                    |                         |         |                |
                    |                   +------+   +--------+           |
                    |                   | ble_  |   | power_ |           |
                    |                   | task  |   | task   |           |
                    |                   | Event |   | 1 Hz   |           |
                    |                   +------+   +--------+           |
                    +----------|-----------------|----------|-----------+
                               | SPI2            | I2C      | UART1
                               |                 |          |
                    +----------v---+    +-------v---+  +---v--------+
                    | ILI9488 TFT |    | MPU6050   |  | GPS Modul  |
                    | 480x320     |    | BMP280    |  | (NMEA)     |
                    | RGB565      |    | I2C 400kHz|  | 10 Hz      |
                    +-------------+    +-----------+  +------------+
                               |
                    +----------v---+         +-------------------+
                    | MicroSD      |         | Companion-App     |
                    | SPI (shared) |  BLE    | React Native      |
                    | FAT32        |<------->| GATT-Client       |
                    +--------------+         +-------------------+
```

### 1.2 Komponenten-Uebersicht

| Komponente | Interface | Bus | Core | Beschreibung |
|---|---|---|---|---|
| MCU | ESP32-S3-WROOM-1 | Intern | 0+1 | Dual-Core 240 MHz, 16 MB Flash, 8 MB PSRAM |
| TFT-Display | ILI9488, 480x320, RGB565 | SPI2 (FSPI) | 0 | Hauptdisplay, 30 FPS, Dirty-Rect-Tracking |
| E-Paper-Display | 2.9" b/w, 296x128, 1-bit | SPI2 (FSPI) | 0 | Sekundaer-Display (optional), Partial-Refresh |
| IMU | MPU6050, 6-Achsen | I2C0 (400 kHz) | 1 | Roll/Pitch, Crash-Detection |
| Barometer | BMP280 | I2C0 (400 kHz) | 1 | Hoehe, Umgebungstemperatur |
| GPS-Modul | AT6558 / NEO-M9N, NMEA | UART1 (115200) | 1 | Position, Geschwindigkeit, Hoehe |
| MicroSD | FAT32, Class 10 | SPI2 (FSPI, shared) | 0 | Logging, Track-Export, Konfiguration |
| BLE | NimBLE, GATT-Server | Intern (Radio) | 0 | Smartphone-Kommunikation, 5 Services |
| Power | LiPo 18650, TP4056, MT3608, HT7333 | ADC + GPIO | 0 | Akku-ueberwachung, Sleep-Modi |
| Buzzer | Piezo | LEDC PWM | 0 | Akustische Signale |
| Status-LED | Gruene LED | GPIO | 0 | Heartbeat / BLE-Status |

### 1.3 Datenfluss

Der Datenfluss folgt einer strikten Pipeline: Sensoren akquirieren Rohdaten
(Core 1), Filter glatten die Daten, Queues uebergeben sie an Display und Logger
(Core 0), und BLE benachrichtigt die Companion-App.

```
GPS (UART1) ──► nmea_parse() ──► gps_data_queue ──┬──► display_task (Position, Speed)
  │                                                └──► logger_task (GPX-Track)
  │                                                     └──► ble_notify_gps (1 Hz)

IMU (I2C) ──► comp_filter() ──┐
BMP280 (I2C) ──► steinhart ──┤
ADC (coolant) ──► NTC ───────┤──► sensor_data_queue ──┬──► display_task (Neigung, Temp)
ADC (fuel) ──► lookup ───────┤                        ├──► logger_task (Telemetrie)
ADC (oil) ──► linear ────────┤                        └──► ble_notify_sensors (0.5 Hz)
ADC (vbat) ──► voltage ──────┤
PCNT (rpm) ──► median ───────┘

BLE ◄──────► Smartphone-App
  │           (Navigation, Media, Phone, Weather, OTA)
  └──► display_task (Nav-Maneuver, Notifications)
```

### 1.4 Dual-Core Aufgabenteilung

Die Kernphilosophie der Firmware ist die strikte Trennung von
Datenakquisition (Core 1, echtzeitkritisch) und Praesentation/Kommunikation
(Core 0, best-effort). Dadurch koennen Sensor-Sampling und Display-Rendering
unabhaengig voneinander laufen, ohne sich gegenseitig zu blockieren.

| Core | Verantwortung | Tasks | Begruendung |
|---|---|---|---|
| Core 1 (RT) | Echtzeit-Datenakquisition | sensor_task, gps_task | Deterministische Sampling-Raten, keine UI/Jitter |
| Core 0 (UI) | Praesentation und Kommunikation | display_task, anim_task, ble_task, logger_task, power_task | Best-effort, SPI/I2C getrennt vom RT-Pfad |

---

## 2. Firmware-Architektur

### 2.1 FreeRTOS Task-Uebersicht

Alle Tasks sind mit `xTaskCreatePinnedToCore` an einen festen Core gepinnt.
Die Prioritaeten sind so gewaehlt, dass echtzeitkritische Tasks (Sensoren,
GPS) hoeher priorisiert sind als UI- und Kommunikations-Tasks.

| Task | Core | Prio | Stack | Rate | Subsystem | Beschreibung |
|---|---|---|---|---|---|---|
| `sensor_task` | 1 | 5 | 4096 | 50 Hz | Pipeline | IMU, BMP280, ADC, PCNT abfragen, filtern, Queue |
| `gps_task` | 1 | 4 | 4096 | 10 Hz | Pipeline | NMEA ueber UART lesen, parsen, Queue |
| `display_task` | 0 | 2 | 8192 | 30 FPS | Rendering | Layout, Paint, Swap, Blit (Dirty-Rect) |
| `anim_task` | 0 | 4 | 2048 | 60 Hz | Animation | Tween/Spring-Engine, Timeline-Updates |
| `ble_task` | 0 | 3 | 6144 | Event | BLE | Anwendungsseitige BLE-Event-Behandlung |
| `logger_task` | 0 | 3 | 4096 | 5 Hz | Logging | Ringbuffer-Batch, SD-Schreiben |
| `power_task` | 0 | 6 | 2048 | 1 Hz | Power | Akku-ueberwachung, Sleep-Entscheidung |

Die `power_task` hat die hoechste Prioritaet (6), damit ein kritischer
Akku-Zustand sofort zu einem geordneten Shutdown fuehrt, unabhaengig davon,
was andere Tasks gerade tun.

### 2.2 Inter-Task Kommunikation

Die Tasks koppeln ueber drei FreeRTOS-Primitiv-Typen:

**Queues (Datenuebergabe):**

| Queue | Typ | Sender | Empfaenger | Groesse |
|---|---|---|---|---|
| `gps_data_queue` | `gps_data_t` | gps_task | display_task, logger_task | 4 Slots |
| `sensor_data_queue` | `sensor_data_t` | sensor_task | display_task, logger_task, ble_task | 4 Slots |

Beide Queues verwenden `xQueueOverwrite`: Wenn die Queue voll ist, wird der
aelteste Eintrag ueberschrieben. Das stellt sicher, dass immer die neuesten
Daten verfuegbar sind, auch wenn ein Empfaenger mal laenger braucht.

**EventGroups (Zustands-Flags):**

Der Event-Bus (`event_bus.h`) verwendet eine FreeRTOS EventGroup mit
14 definierten Bits:

| Event-Bit | Bedeutung | Ausloeser |
|---|---|---|
| `EVT_GPS_FIX` | GPS hat einen Fix | gps_task |
| `EVT_GPS_LOST_FIX` | GPS-Fix verloren | gps_task |
| `EVT_SENSOR_READY` | Sensoren initialisiert | sensor_task |
| `EVT_CRASH_DETECTED` | Crash erkannt (>3g) | sensor_task |
| `EVT_BATTERY_LOW` | Akku niedrig | power_task |
| `EVT_BATTERY_CRITICAL` | Akku kritisch (<3.2V) | power_task |
| `EVT_SDCARD_MOUNTED` | SD-Karte gemountet | sd_card_init |
| `EVT_SDCARD_ERROR` | SD-Karten-Fehler | logger_task |
| `EVT_BLE_CONNECTED` | BLE verbunden | ble_task |
| `EVT_BLE_DISCONNECTED` | BLE getrennt | ble_task |
| `EVT_DISPLAY_READY` | Display initialisiert | display_task |
| `EVT_SHUTDOWN` | Shutdown angefordert | power_task |
| `EVT_OTA_START` | OTA-Update gestartet | ble_task |
| `EVT_OTA_DONE` | OTA-Update abgeschlossen | ble_task |

**Mutexe (Ressourcen-Schutz):**

| Mutex | Schutzobjekt | Benutzer |
|---|---|---|
| `sensor_data_mutex` | `g_sensor_data` (gefilterte Sensordaten) | sensor_task, display_task, ble_task |
| `dtc_mutex` | DTC-Tabelle (`dtc_table`) | Alle Tasks, die DTCs melden |
| `spi_mutex` | Shared SPI2-Bus (Display + SD) | display_task, logger_task |
| `s_log_mutex` | Logger-Ringbuffer | logger_task |

### 2.3 Startup-Sequenz

Die Init-Reihenfolge aus `main.c` ist kritisch: Power zuerst (damit
Spannungsversorgung stabil ist), dann Buse (I2C, SPI, UART), dann
Peripherie (Sensoren, Display, Speicher), zuletzt BLE.

```
app_main()
  |
  1. power_init()        — ADC, Power-Control GPIOs
  2. sensors_init()      — I2C-Bus, MPU6050, BMP280, sensor_data_queue
  3. gps_init()          — UART1, gps_data_queue
  4. display_init()      — SPI-Bus, TFT/E-Paper Treiber, Renderer
  5. logger_init()       — Ringbuffer, SD-Karte, config_init
  6. ble_init()          — NimBLE Stack, GATT-Services, Advertising
  |
  7. Tasks starten:
     xTaskCreatePinnedToCore(gps_task,     "gps",     4096, NULL, 5, NULL, 1)
     xTaskCreatePinnedToCore(sensor_task,  "sensors", 4096, NULL, 4, NULL, 1)
     xTaskCreatePinnedToCore(display_task, "display", 6144, NULL, 2, NULL, 0)
     xTaskCreatePinnedToCore(logger_task,  "logger",  4096, NULL, 3, NULL, 0)
     xTaskCreatePinnedToCore(ble_task,     "ble",     4096, NULL, 3, NULL, 0)
     xTaskCreatePinnedToCore(power_task,   "power",   2048, NULL, 6, NULL, 0)
```

In der vollstaendigen Implementierung (siehe research/11) kommen hinzu:
`config_load()` (NVS), `event_bus_init()`, `run_self_tests()` und
`ota_check_on_boot()` (Rollback-Verifikation nach OTA-Update).

### 2.4 Watchdog-Konzept

Der ESP32-S3 hat zwei Watchdog-Ebenen:

1. **Task Watchdog (TWDT):** Ueberwacht kritische Tasks. Wenn ein Task
   nicht innerhalb seines Timeouts `esp_task_wdt_reset()` aufruft, wird
   ein Watchdog-Timeout ausgeloest und das System neu gestartet.

2. **Interrupt Watchdog (IWDT):** Ueberwacht, dass FreeRTOS-Ticks regulaer
   laufen. Schuetzt gegen ISR-Stoerungen.

Zusaetzlich gibt es anwendungsseitige Watchdogs:

- **BLE Connection-Watchdog:** Wenn 10 Sekunden kein BLE-Event empfangen
  wird, wird die Verbindung getrennt und neu aufgebaut.
- **Boot-Loop Detection:** Bei mehr als 3 Resets innerhalb von 60 Sekunden
  wird ein DTC (B0401) gesetzt und ggf. ein Rollback ausgeloest.

### 2.5 Speicher-Layout (Partition Table)

Die Flash-Partitionierung (`firmware/partitions.csv`) ist OTA-faehig
mit Dual-App-Partitionen:

```
| Name     | Type | SubType | Offset    | Size     | Beschreibung                    |
|----------|------|---------|-----------|----------|---------------------------------|
| nvs      | data | nvs     | 0x9000    | 0x6000   | Non-Volatile Storage (Config)   |
| phy_init | data | phy     | 0xF000    | 0x1000   | RF-Kalibrierung                  |
| factory  | app  | factory | 0x10000   | 0x300000 | Werkseinstellung (3 MB)          |
| ota_0    | app  | ota_0   | 0x310000  | 0x300000 | OTA-Slot 0 (3 MB)                |
| ota_1    | app  | ota_1   | 0x610000  | 0x300000 | OTA-Slot 1 (3 MB)                |
| otadata  | data | ota     | 0x910000  | 0x2000   | Boot-Selection fuer OTA          |
| storage  | data | fat     | 0x912000  | 0x6EE000 | FAT-FS (SD-Ersatz/Writable, ~7MB)|
```

Bei einem OTA-Update wird das Firmware-Image in die inaktive OTA-Partition
geschrieben. Beim Neustart bootet das System von der neuen Partition.
Wenn der Self-Test nach dem Boot fehlschlaegt, wird ein Rollback zur
vorherigen Partition durchgefuehrt.

**Speicher-Budget:**

| Komponente | Flash | RAM (intern) | RAM (PSRAM) |
|---|---|---|---|
| ESP-IDF + FreeRTOS | ~800 KB | ~80 KB | -- |
| NimBLE Stack | ~150 KB | ~20 KB | -- |
| Display-Treiber + Framebuffer | ~20 KB | ~32 KB (Bounce) | ~614 KB (2x FB) |
| Font-Data (2 Fonts, 4-bit AA) | ~120 KB | -- | -- |
| Sensor-Treiber | ~15 KB | ~4 KB | -- |
| BLE GATT Services | ~8 KB | ~2 KB | -- |
| OTA + Diagnose | ~12 KB | ~4 KB | -- |
| Logger (Buffer) | ~5 KB | ~4 KB | -- |
| Config-System | ~3 KB | ~0.5 KB | -- |
| **Gesamt** | **~1.4 MB** | **~200 KB** | **~614 KB** |

Bei 16 MB Flash und 8 MB PSRAM ist reichlich Platz. Die App-Partition ist
3 MB, von der ~1.4 MB verwendet werden.

---

## 3. Komponenten-Details

Die Firmware ist in 8 Komponenten gegliedert (firmware/components/).
Jede Komponente hat einen Header (oeffentliches Interface) und eine oder
mehrere Implementierungsdateien.

### 3.1 BLE-Komponente (`components/ble/`)

**Zweck:** NimBLE GATT-Server, Smartphone-Kommunikation, OTA-Transfer.

**Dateien:**
- `ble.h` — Oeffentliches Interface: `ble_init()`, `ble_task()`
- `ble_service.c` — NimBLE-Stack-Initialisierung, GATT-Service-Registrierung
- `ble_callbacks.c` — Read/Write/Notify-Handler fuer alle Characteristics
- `gatt_uuids.h` — UUID-Definitionen (muessen mit Companion-App uebereinstimmen)

**Oeffentliches Interface (`ble.h`):**
```c
void ble_init(void);       /* NimBLE initialisieren, Services registrieren */
void ble_task(void *arg);  /* Anwendungsseitige BLE-Event-Schleife */
```

**Abhaengigkeiten:** NimBLE-Stack (ESP-IDF), `gps_data_queue`, `sensor_data_queue`

**Task/Callback-Modell:**
- NimBLE laeuft intern in einem eigenen Host-Task (`nimble_port_freertos_init`).
- `ble_task` (Core 0, Prio 3) behandelt anwendungsseitige Events:
  Connect/Disconnect -> Event-Bus, neue GPS/Sensor-Daten -> Notify an App,
  OTA-Chunks -> `esp_ota_write`, Navigation-Daten -> an Display-Task.
- GATT-Access-Callbacks werden synchron von NimBLE aufgerufen.

**Konfiguration:**
- Device-Name: "Transalp-BC" (konfigurierbar via Config-Service)
- Security: LE Secure Connections, Just-Works (IO_CAP_NO_IO)
- Bonding: 1 (persistent, Keys im NVS)
- MTU: 247 (BLE 5.0 Maximum)
- Connection-Parameters: 7.5-20 ms Interval, 0 Latency, 2s Supervision Timeout
- Advertising: General Discoverable, Connectable Undirected, 20-40 ms Interval

### 3.2 Sensor-Komponente (`components/sensors/`)

**Zweck:** Alle Sensoren abfragen, filtern und in `sensor_data_queue` schreiben.

**Dateien:**
- `sensors.h` — Interface + `sensor_data_t` Struktur
- `sensors.c` — I2C-Bus-Init, `sensor_task` (50 Hz Polling-Loop)
- `imu.c` — MPU6050-Treiber (I2C), Complementary-Filter, Crash-Detection
- `rpm.c` — Drehzahl ueber Pulse-Counter (PCNT)
- `temperature.c` — Kuehlmitteltemperatur ueber NTC + ADC
- `fuel_level.c` — Kraftstoffstand ueber ADC + Kennlinie
- `gear_indicator.c` — Gang-Erkennung aus Speed/RPM-Ratio

**Oeffentliches Interface (`sensors.h`):**
```c
typedef struct {
    float temp_c;          /* BMP280 Umgebungstemperatur */
    float pressure_hpa;    /* BMP280 Luftdruck */
    float altitude_m;      /* BMP280 Hoehe */
    float roll_deg;        /* MPU6050 Rollwinkel */
    float pitch_deg;       /* MPU6050 Nickwinkel */
    float accel_g;         /* MPU6050 Gesamtbeschleunigung */
    bool  crash_detected;  /* MPU6050 >3g */
    uint32_t timestamp_ms; /* Zeitstempel */
} sensor_data_t;

void sensors_init(void);
void sensor_task(void *arg);
extern QueueHandle_t sensor_data_queue;
```

**Abhaengigkeiten:** `filters.h` (Complementary-Filter, Kalman, Moving-Average),
`board_config.h` (I2C-Pins, MPU6050-Adresse), I2C-Treiber, ADC-Treiber, PCNT-Treiber

**Task-Modell:**
- `sensor_task` laeuft auf Core 1 mit 50 Hz (20 ms Intervall).
- Pro Zyklus: I2C-Burst-Read (MPU6050 14 Bytes + BMP280), ADC-Sampling
  (16x Oversampling), PCNT-Auslesen, Filterung, Queue-Write.
- Der `xSemaphoreTake(sensor_data_mutex)` schuetzt den Zugriff auf
  `g_sensor_data` durch display_task und ble_task.

**Sensor-Pipeline (dreistufig):**

```
  +--------+     +----------+     +-----------+     +---------+
  | Sensor | --> | Raw      | --> | Filter    | --> | Display |
  | HW     |     | Read     |     | (Smooth)  |     | Driver  |
  +--------+     +----------+     +-----------+     +---------+
                    |                  |                |
                    v                  v                v
                 Queue            Animation         Dirty-Rect
```

Stufe 1 (Raw Read): I2C/ADC/PCNT auslesen, Rohwerte sammeln.
Stufe 2 (Filter): Moving-Average (RPM, Median), EMA (Temp, Fuel, Voltage),
Complementary-Filter (IMU Roll/Pitch), Steinhart-Hart (NTC).
Stufe 3 (Displayed): An Animation-Driver weitergeben, Queue an Logger/BLE.

### 3.3 GPS-Komponente (`components/gps/`)

**Zweck:** NMEA-Daten ueber UART lesen, parsen und in `gps_data_queue` schreiben.

**Dateien:**
- `gps.h` — Interface + `gps_data_t` Struktur
- `gps_task.c` — UART-Initialisierung, `gps_task` (10 Hz)
- `nmea_parser.c` — NMEA-Satz-Parser (GGA, RMC, VTG), minmea-Integration (TODO)

**Oeffentliches Interface (`gps.h`):**
```c
typedef struct {
    float lat;             /* Breitengrad (dezimal) */
    float lon;             /* Laengengrad (dezimal) */
    float speed_kmh;       /* Geschwindigkeit km/h */
    float course_deg;      /* Kurs (Heading) in Grad */
    float altitude_m;      /* Hoehe ueber NN */
    uint8_t satellites;    /* Anzahl Satelliten */
    bool  fix;             /* GPS-Fix aktiv? */
    uint32_t timestamp_ms; /* Zeitstempel */
} gps_data_t;

void gps_init(void);
void gps_task(void *arg);
extern QueueHandle_t gps_data_queue;
```

**Abhaengigkeiten:** UART-Treiber, `board_config.h` (UART-Pins, Baudrate),
`event_bus.h` (`EVT_GPS_FIX`), minmea-Library (geplant)

**Task-Modell:**
- `gps_task` laeuft auf Core 1.
- UART1 ist mit 38400 Baud (Default) konfiguriert, per UBX auf 115200 erhoebar.
- Der Task liest in einer Schleife UART-Daten (`uart_read_bytes`, 100 ms Timeout),
  splittet NMEA-Zeilen und parst sie einzeln.
- Bei einem Fix wird `EVT_GPS_FIX` im Event-Bus gesetzt.
- Neueste Daten werden per `xQueueOverwrite` in die Queue geschrieben.

**UART-Konfiguration:**
- UART1, RX=GPIO44, TX=GPIO43
- Baudrate: 38400 (Default), 115200 (per UBX konfigurierbar)
- 8 Datenbits, keine Paritaet, 1 Stop-Bit
- PPS-Pin: GPIO39 (1PPS Zeitsynchronisation, IRQ-faehig)

### 3.4 Display-Komponente (`components/display/`)

**Zweck:** Framebuffer-Verwaltung, Rendering-Pipeline, UI-Widgets, Display-Treiber.

**Dateien:**
- `display.h` — Interface
- `display.c` — `display_task` (30 FPS Render-Loop), pullt GPS/Sensor-Queues
- `renderer.c` — Framebuffer, Double-Buffering, Dirty-Rect-Tracking
- `tft_driver.c` — ILI9488 TFT-Treiber (SPI, 480x320, RGB565, DMA)
- `epaper_driver.c` — E-Paper-Treiber (SPI, 296x128, 1-bit, Partial-Refresh)
- `widgets.c` — UI-Widgets: Speed, RPM-Bar, Gear, Nav-Arrow, Altitude

**Oeffentliches Interface (`display.h`):**
```c
void display_init(void);
void display_task(void *arg);
```

**Abhaengigkeiten:** `board_config.h` (SPI-Pins, Aufloesungen), SPI-Treiber,
`gps_data_queue`, `sensor_data_queue`

**Task-Modell:**
- `display_task` laeuft auf Core 0 mit 30 FPS (33 ms Intervall).
- Pro Frame: Layout-Update -> Paint in Back-Buffer -> Swap -> Blit (Dirty-Rect).
- Wenn keine Dirty-Rects vorhanden sind, schlaeft der Task.

**Rendering-Pipeline (vierschichtig):**

```
  Widget Tree        Layout Engine       Paint Engine
  +----------+       +-------------+     +----------------+
  | Screen   |  -->  | Position    | --> | Draw Prims     |
  |  + RPM   |       | Size        |     | Rect/Circle/   |
  |  + Speed |       | Bounds      |     | Text/Line/Arc  |
  |  + Gear  |       +-------------+     +----------------+
  +----------+                                    |
                                                   v
  +----------+       +-------------+     +----------------+
  | SPI/DMA  | <--  | Compositor  | <-- | Dirty-Rect     |
  | Blit     |       | Merge Rects |     | Tracker        |
  +----------+       +-------------+     +----------------+
```

**Framebuffer im PSRAM:**
- Double-Buffer: Front (auf Display) und Back (wird gemalt), je ~300 KB.
- DMA kann nicht direkt von PSRAM lesen: Bounce-Buffer im internen SRAM
  (40 Zeilen = ~38 KB) als Zwischenspeicher.
- Dirty-Rect-Tracking: Maximal 16 Rechtecke, ueberlappende werden vereinigt.
- Bei einem typischen Dashboard-Update (4 Zahlenbereiche, je ~80x40 Pixel)
  sind nur 25.600 Bytes statt 307.200 Bytes zu uebertragen (Faktor 12 weniger).

### 3.5 Storage-Komponente (`components/storage/`)

**Zweck:** SD-Karte mounten, Telemetrie-Logging, Konfiguration in NVS/JSON.

**Dateien:**
- `storage.h` — Interface (SD, Logger, Config)
- `storage.c` — Modul-Entry-Point, ruft `sd_card_init()` und `config_init()`
- `sd_card.c` — MicroSD ueber SPI mounten (FATFS), SPI-Mutex erstellen
- `logger.c` — Ringbuffer + Batch-Write (5 Hz Polling, 64-Eintrag-Batch)
- `config.c` — NVS-BLOB fuer `device_config_t`, JSON-Export/Import

**Oeffentliches Interface (`storage.h`):**
```c
#define SD_MOUNT_POINT  "/sdcard"

void storage_init(void);
bool sd_card_mount(void);
bool sd_card_is_mounted(void);
void logger_init(void);
void logger_task(void *arg);
void config_init(void);
int config_load(void *cfg, uint16_t max_len);
int config_save(const void *cfg, uint16_t len);
```

**Abhaengigkeiten:** SPI-Treiber (shared mit Display, `spi_mutex`),
FATFS, NVS-Flash, `gps_data_queue`, `sensor_data_queue`

**Task-Modell:**
- `logger_task` laeuft auf Core 0 mit 5 Hz (200 ms Polling).
- Sammelt GPS- und Sensor-Daten im Ringbuffer (64 Eintraege).
- Schreibt Batch auf SD, wenn Buffer voll oder Timeout.
- SPI-Mutex wird beim Schreiben gehalten (shared mit Display).

**Log-Format (CSV):**
```
timestamp,lat,lon,speed_kmh,rpm,temp_c,alt_m,roll_deg,pitch_deg
```

**Dateinamen-Konvention:** `YYYYMMDD_HHMMSS.csv` im Verzeichnis `/sdcard/logs/`.
GPX-Tracks: `/sdcard/tracks/`.

### 3.6 Diagnostics-Komponente (`components/diagnostics/`)

**Zweck:** DTC-System (Diagnostic Trouble Codes), Self-Test beim Boot.

**Dateien:**
- `diagnostics.h` — Interface + `dtc_code_t` Enum + `dtc_entry_t` Struktur
- `diagnostics.c` — Modul-Entry-Point, ruft `dtc_init()`
- `dtc.c` — DTC-Tabelle (16 Eintraege), report/clear/get_active
- `selftest.c` — Boot-Self-Test (7 Hardware-Checks), OTA-Rollback-Verifikation

**Oeffentliches Interface (`diagnostics.h`):**
```c
typedef enum {
    DTC_NONE = 0,
    DTC_GPS_NO_FIX,
    DTC_GPS_UART_ERROR,
    DTC_IMU_NOT_RESPONDING,
    DTC_BARO_NOT_RESPONDING,
    DTC_SD_MOUNT_FAILED,
    DTC_SD_WRITE_FAILED,
    DTC_BATTERY_LOW,
    DTC_BATTERY_CRITICAL,
    DTC_BLE_INIT_FAILED,
    DTC_DISPLAY_INIT_FAILED,
    DTC_SPI_BUS_ERROR,
    DTC_OVERTEMPERATURE,
} dtc_code_t;

typedef struct {
    dtc_code_t code;
    uint32_t timestamp_ms;
    uint8_t count;      /* Wie oft aufgetreten */
    bool active;        /* Noch aktiv? */
} dtc_entry_t;

#define DTC_MAX_ENTRIES  16

void diagnostics_init(void);
void dtc_report(dtc_code_t code);
void dtc_clear(dtc_code_t code);
void dtc_clear_all(void);
uint8_t dtc_get_active(dtc_entry_t *entries, uint8_t max_entries);
bool selftest_run(void);
```

**Abhaengigkeiten:** `storage.h` (SD-Check), ESP-IDF OTA-API

**DTC-Kategorien (erweitert, siehe research/11):**

```
P0xxx  Powertrain   (Motor, Getriebe, Antrieb)
  P0101  RPM-Signal unplausibel (0-12000)
  P0201  Kuehlmitteltemperatur > 115C (Warnung)
  P0202  Kuehlmitteltemperatur > 120C (Kritisch)
  P0301  Oeldruck < 0.5 bar bei laufendem Motor
  P0401  Bordspannung < 11.5V
  P0501  Kraftstoffstand < 2L (Reserve)

S0xxx  Sensors (Sensor-Ausfaelle)
  S0101  I2C: IMU nicht erreichbar
  S0102  I2C: BMP280 nicht erreichbar
  S0201  GPS: kein Fix innerhalb 30s
  S0401  IMU: Crash erkannt (>3g)

C0xxx  Comms (Kommunikation)
  C0101  BLE: Advertising konnte nicht starten
  C0201  SD-Karte: Mount fehlgeschlagen
  C0202  SD-Karte: Schreibfehler

B0xxx  Body (System-Intern)
  B0101  OTA-Update fehlgeschlagen
  B0103  OTA-Rollback ausgeloest
  B0301  Watchdog-Timeout -> Neustart
  B0401  Boot-Loop erkannt (>3 Resets in 60s)
```

**Self-Test-Routine:**
Beim Boot werden 7 Hardware-Module geprueft (Best-Effort, kein Abbruch bei
Fehler). Bei OTA-Rollback-Schutz muss `selftest_run()` erfolgreich sein,
sonst wird `esp_ota_mark_app_invalid_rollback_and_reboot()` aufgerufen.

### 3.7 Power-Komponente (`components/power/`)

**Zweck:** Akku-ueberwachung, Sleep-Modi, Power-State-Machine.

**Dateien:**
- `power.h` — Interface + `power_state_t` Enum
- `power.c` — Modul-Entry-Point
- `power_manager.c` — ADC-Lese-Logik, State-Machine, Deep-Sleep

**Oeffentliches Interface (`power.h`):**
```c
typedef enum {
    POWER_STATE_ACTIVE,    /* Vollbetrieb, alle Tasks, Display an */
    POWER_STATE_IDLE,      /* Display aus, GPS+Logger an */
    POWER_STATE_SLEEP,     /* Nur RTC + Akku-Ueberwachung */
    POWER_STATE_SHUTDOWN,  /* Akku < 3.2V, sauberes SD-Flush + Power-Off */
} power_state_t;

void power_init(void);
void power_task(void *arg);
float power_get_battery_voltage(void);
power_state_t power_get_state(void);
```

**Abhaengigkeiten:** ADC-Treiber (ADC1_CH4), `event_bus.h`,
ESP-IDF Deep-Sleep-API

**Task-Modell:**
- `power_task` laeuft auf Core 0 mit 1 Hz (hoechste Prioritaet: 6).
- Liest Akkuspannung ueber ADC (Spannungsteiler 2:1, max 3.3V).
- Entscheidedt ueber State-Uebergaenge (siehe Abschnitt 8).

### 3.8 Utils-Komponente (`components/utils/`)

**Zweck:** Geteilte Utilities: Event-Bus, Signal-Filter, Makros.

**Dateien:**
- `event_bus.h` / `event_bus.c` — Pub/Sub ueber FreeRTOS EventGroups
- `filters.h` / `filters.c` — Kalman 1D, Complementary-Filter, Moving-Average
- `utils.h` — Makros (`ARRAY_LEN`, `CLAMP`, `DEG_TO_RAD`, `millis()`)

**Filter-API (`filters.h`):**
```c
/* Kalman 1D: fuer GPS-Speed-Glaettung */
typedef struct {
    float q, r, x, p, k;
} kalman1d_t;
void kalman1d_init(kalman1d_t *kf, float q, float r, float initial);
float kalman1d_update(kalman1d_t *kf, float measurement);

/* Complementary Filter: fuer IMU Roll/Pitch */
typedef struct {
    float alpha, angle, dt;
} complementary_filter_t;
void comp_filter_init(complementary_filter_t *cf, float alpha, float dt);
float comp_filter_update(complementary_filter_t *cf, float gyro_rate, float accel_angle);

/* Moving Average */
typedef struct {
    float *buf; uint8_t size, idx, count; float sum;
} moving_avg_t;
void moving_avg_init(moving_avg_t *ma, float *buffer, uint8_t size);
float moving_avg_update(moving_avg_t *ma, float value);
```

**Abhaengigkeiten:** FreeRTOS EventGroups, math.h

---

## 4. Hardware-Schnittstellen

### 4.1 Vollstaendige Pin-Map

Diese Tabelle ist die alleinige Referenz fuer alle Pin-Zuweisungen.
`firmware/main/board_config.h` muss dieser Tabelle entsprechen.

| GPIO | Funktion | Bus/Interface | ADC | RTC | Pull | IRQ | Zugewiesene Aufgabe |
|---|---|---|---|---|---|---|---|
| 0 | Input | — | — | Ja | PU | Ja | BOOT_BTN (Boot-Taster) |
| 1 | Analog Input | ADC1_CH0 | 12-bit | Ja | — | Nein | BATT_VOLTAGE (Bordspannung) |
| 2 | Analog Input | ADC1_CH1 | 12-bit | Ja | — | Nein | COOLANT_TEMP (NTC) |
| 3 | Digital Input | GPIO | ADC1_CH2 | Ja | PU | Ja | BTN_BACK (Taster Zurueck) |
| 4 | Analog Input | ADC1_CH3 | 12-bit | Ja | — | Nein | FUEL_LEVEL (Kraftstoffstand) |
| 5 | Analog Input | ADC1_CH4 | 12-bit | Ja | — | Nein | OIL_PRESSURE (Oeldruck) |
| 6 | Digital Input | GPIO | — | Ja | PU | Ja | BTN_MENU (Taster Menu) |
| 7 | Digital Input | GPIO | — | Ja | PU | Ja | BTN_SELECT (Taster Select) |
| 8 | Digital I/O | I2C0 SDA | ADC1_CH7 | — | PU | Nein | I2C_SDA (MPU6050+BMP280) |
| 9 | Digital Output | I2C0 SCL | ADC1_CH8 | — | PU | Nein | I2C_SCL (MPU6050+BMP280) |
| 10 | Digital Output | SPI2 CS | ADC1_CH9 | — | — | Nein | TFT_CS (ILI9488 Chip Select) |
| 11 | Digital Output | SPI2 MOSI | ADC2_CH0 | — | — | Nein | FSPI_MOSI (Shared Bus) |
| 12 | Digital Output | SPI2 SCK | ADC2_CH1 | — | — | Nein | FSPI_SCK (Shared Bus) |
| 13 | Digital Input | SPI2 MISO | ADC2_CH2 | — | — | Nein | FSPI_MISO (nur SD Lesen) |
| 14 | Digital Output | GPIO | — | Ja | — | Nein | TFT_DC (Data/Command) |
| 15 | Digital Input | PCNT0 | — | Ja | PU | Ja | RPM_PULSE (Drehzahl-Sensor) |
| 16 | Digital Input | PCNT1 | — | Ja | PU | Ja | TACHO_PULSE (Geschwindigkeit) |
| 17 | Digital Output | GPIO | — | Ja | — | Nein | TFT_RST (Reset) |
| 18 | Digital Output | LEDC PWM | — | Ja | — | Nein | TFT_BL (Backlight) |
| 19 | USB | USB-OTG D+ | — | — | — | — | USB_DP (USB-C Data+) |
| 20 | USB | USB-OTG D- | — | — | — | — | USB_DM (USB-C Data-) |
| 21 | Digital Output | GPIO | — | Ja | — | Nein | STATUS_LED (Heartbeat) |
| 22-25 | — | — | — | — | — | — | Nicht vorhanden auf ESP32-S3 |
| 26-32 | — | SPI0 (intern) | — | — | — | — | Interner Flash (nie extern) |
| 33-37 | — | SPI1 (intern) | — | — | — | — | Interner PSRAM (Octal, nie extern) |
| 38 | Digital Output | LEDC PWM | — | — | — | Nein | BUZZER (Piezo) |
| 39 | Digital Input | GPIO | — | Ja | PU | Ja | GPS_PPS (1PPS Zeitsync) |
| 40 | Digital Output | SPI2 CS | — | — | — | Nein | EPD_CS (E-Paper Chip Select) |
| 41 | Digital Output | GPIO | — | — | — | Nein | EPD_DC (Data/Command) |
| 42 | Digital Output | GPIO | — | — | — | Nein | EPD_RST (Reset) |
| 43 | Digital Output | UART1 TX | — | — | — | Nein | GPS_TX (MCU -> GPS) |
| 44 | Digital Input | UART1 RX | — | — | — | Nein | GPS_RX (GPS -> MCU) |
| 45 | — | — | — | — | — | — | Strapping-Pin (frei) |
| 46 | — | — | — | — | — | — | Strapping-Pin (frei) |
| 47 | Digital Output | SPI2 CS | — | — | — | Nein | SD_CS (MicroSD Chip Select) |
| 48 | Digital Input | GPIO | — | Ja | PU | Ja | MPU6050_INT (IMU Interrupt) |

**Strapping-Pins (Boot-relevant):**

| GPIO | Funktion beim Boot | Verwendung |
|---|---|---|
| 0 | Boot-Mode (LOW = Download) | BOOT_BTN (mit Pull-Up) |
| 3 | JTAG-Select (HIGH = USB) | BTN_BACK (Input, Pull-Up) |
| 45 | VDDSPI Voltage (3.3V) | Nicht verwendet |
| 46 | Boot-Mode | Nicht verwendet |

### 4.2 SPI-Bus: Shared Bus fuer Display + SD

Der SPI2 (FSPI) Bus wird von drei Geraeten geteilt: TFT-Display, E-Paper-Display
und MicroSD-Karte. Das Multiplexing erfolgt ueber separate Chip-Select-Leitungen
(active-LOW). Ein Mutex (`spi_mutex`) stellt sicher, dass nur ein Geraet
gleichzeitig auf den Bus zugreift.

```
                    ESP32-S3 FSPI2
                    +--+--+--+--+
                    |MO|MI|SK|  |
                    |11|13|12|  |
                    +-++-++-++--+
                     |  |  |
          +----------+--+--+----------+
          |             |             |
    +-----v----+  +----v----+  +-----v----+
    | TFT CS=10|  | EPD CS=40|  | SD CS=47 |
    | ILI9488  |  | 2.9" EPD |  | MicroSD  |
    | 480x320  |  | 296x128  |  | FAT32    |
    | 40-80MHz |  | 4 MHz    |  | 40 MHz   |
    +----------+  +----------+  +----------+
```

| Eigenschaft | Wert |
|---|---|
| Controller | FSPI2 (SPI2_HOST) |
| MOSI | GPIO11 |
| MISO | GPIO13 (nur SD Lese-Richtung, TFT/EPD sind Write-Only) |
| SCK | GPIO12 |
| Taktrate TFT | 40-80 MHz |
| Taktrate EPD | 4 MHz |
| Taktrate SD | 40 MHz |
| CS TFT | GPIO10 |
| CS EPD | GPIO40 |
| CS SD | GPIO47 |
| Mutex | `spi_mutex` (xSemaphoreCreateMutex) |

### 4.3 I2C-Bus: MPU6050 + BMP280

Der I2C0-Bus verbindet den ESP32-S3 mit dem IMU (MPU6050) und dem
Barometer (BMP280). Beide Sensoren teilen sich den Bus bei 400 kHz
(Fast Mode).

| Eigenschaft | Wert |
|---|---|
| Controller | I2C_NUM_0 |
| SDA | GPIO8 (mit internem Pull-Up) |
| SCL | GPIO9 (mit internem Pull-Up) |
| Taktrate | 400 kHz (Fast Mode) |
| MPU6050 Adresse | 0x68 (AD0=LOW) |
| BMP280 Adresse | 0x76 (oder 0x77, je Modul) |
| MPU6050 INT | GPIO48 (Bewegungs-/Crash-Erkennung) |

### 4.4 UART: GPS

| Eigenschaft | Wert |
|---|---|
| Controller | UART_NUM_1 |
| TX | GPIO43 (MCU -> GPS, Konfiguration) |
| RX | GPIO44 (GPS -> MCU, NMEA-Daten) |
| Baudrate | 38400 (Default), 115200 (per UBX) |
| PPS | GPIO39 (1PPS Zeitsynchronisation) |
| GPS-Modul | AT6558 / NEO-M9N kompatibel |
| Datenformat | NMEA 0183 (GGA, RMC, VTG) |

### 4.5 ADC: Fuel Level, Battery Voltage, Coolant, Oil

Alle analogen Sensoren nutzen ADC1 (ADC2 ist unzuverlaessig, wenn BLE aktiv ist).

| GPIO | ADC1-Kanal | Sensor | Messbereich | Attenuation | Auswertung |
|---|---|---|---|---|---|
| 1 | CH0 | Bordspannung | 0-15V -> 0-2.9V | 12 dB | Spannungsteiler, Generator-Status (>13.8V) |
| 2 | CH1 | Kuehlmitteltemp | 0-3.3V (NTC-Teiler) | 12 dB | NTC 10k B3950, Steinhart-Hart |
| 4 | CH3 | Kraftstoffstand | 0-3.3V (Teiler) | 12 dB | Kennlinien-Lookup, 6 Stuetzpunkte |
| 5 | CH4 | Oeldruck | 0.5-4.5V -> 0-2.9V | 12 dB | Linear: (V-0.5)/4.0*10 bar |

Alle ADC-Kanaele verwenden 16x Oversampling fuer Rauschunterdrueckung.

### 4.6 PCNT: RPM und Tacho Pulse Counter

| GPIO | PCNT-Kanal | Sensor | Signalquelle |
|---|---|---|---|
| 15 | PCNT0 | Drehzahl | Optokoppler (Zuendimpuls) |
| 16 | PCNT1 | Tacho | Reed-Kontakt (Vorderrad) |

Der Pulse-Counter zaehlt Impulse hardwaremaessig (keine CPU-Last).
Der `sensor_task` liest den Zaehler alle 20 ms (50 Hz), resettiert ihn
und berechnet RPM aus Pulsrate und Pulsen-pro-Umdrehung.

### 4.7 PWM: Backlight und Buzzer

| GPIO | LEDC-Kanal | Funktion | Frequenz | Beschreibung |
|---|---|---|---|---|
| 18 | LEDC | TFT Backlight | 5 kHz | Helligkeitsregelung 0-255 (PWM Duty) |
| 38 | LEDC | Buzzer | 2-4 kHz | Akustische Signale (Piezo) |

---

## 5. BLE-Protokoll

### 5.1 GATT-Service-Uebersicht

Das BLE-Protokoll verwendet einen projekt-spezifischen 128-bit Base-UUID:

```
6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
```

Die 16-bit-ID (`XXXX`) identifiziert den Service oder die Characteristic.
Services haben IDs im Bereich 0x1000-0x8000. Characteristics innerhalb eines
Services haben IDs im Bereich Service-ID + 0x01 bis Service-ID + 0x0F.

Standard-Services (Generic Access 0x1800, Device Information 0x180A)
verwenden Bluetooth-SIG-Standard-UUIDs und werden nicht in der Custom-Tabelle
gefuehrt.

Es gibt 8 Custom-Services:

| Service | 16-bit ID | UUID | Beschreibung |
|---|---|---|---|
| Navigation | 0x1000 | 6e401000-... | Turn-by-Turn, Route, Road, Status |
| Media | 0x2000 | 6e402000-... | Now Playing, Control, Volume |
| Phone | 0x3000 | 6e403000-... | Incoming Call, Call Status, Notification |
| Weather | 0x4000 | 6e404000-... | Current, Forecast, Alert |
| Telemetry | 0x5000 | 6e405000-... | Live Telemetry, Snapshot, Track Export |
| Config | 0x6000 | 6e406000-... | Read/Write, Export/Import, Bond Management |
| Diagnose | 0x7000 | 6e407000-... | DTC List, Sensor Status, Self-Test, Log Export |
| OTA | 0x8000 | 6e408000-... | OTA Control, Data, Verify |

### 5.2 Characteristic-Tabelle

#### Navigation Service (0x1000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x1001 | 6e401001-... | Read, Notify | var (max 70B) | NAV_MANEUVER: type(1), distance(4), street(var) |
| 0x1002 | 6e401002-... | Read, Notify | 16B | NAV_ROUTE: total(4), remaining(4), eta(4), time(4) |
| 0x1003 | 6e401003-... | Read, Notify | var (max 66B) | NAV_ROAD: speed_limit(1), road_name(var) |
| 0x1004 | 6e401004-... | Read, Notify | 1B | NAV_STATUS: 0=inactive..4=offline |
| 0x1005 | 6e401005-... | Write, WriteNoResp | 1B | NAV_CMD: 0x01=cancel, 0x02=repeat, 0x03=alt |

#### Media Service (0x2000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x2001 | 6e402001-... | Read, Notify | var (max 139B) | MEDIA_NOWPLAYING: state, duration, pos, artist, title |
| 0x2002 | 6e402002-... | Write, WriteNoResp | 1B | MEDIA_CMD: 0x01=play_pause..0x05=seek_back |
| 0x2003 | 6e402003-... | Read, Notify, Write | 1B | Volume (0-100) |

#### Phone Service (0x3000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x3001 | 6e403001-... | Notify | var (max 70B) | PHONE_CALL: caller_id(64B), call_type(1) |
| 0x3002 | 6e403002-... | Read, Notify | 1B | Call Status: 0=idle..3=ended |
| 0x3003 | 6e403003-... | Write | 1B | PHONE_CMD: 0x01=accept, 0x02=reject, 0x03=hangup |
| 0x3004 | 6e403004-... | Notify | var (max 201B) | PHONE_NOTIF: priority, app, title, body |

#### Weather Service (0x4000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x4001 | 6e404001-... | Read, Notify | 8B | WX_CURRENT: temp(i16), humidity(u8), condition(u8), wind(u8), feels(i16), rain(u8) |
| 0x4002 | 6e404002-... | Read, Notify | 24B | WX_FORECAST: 6 Eintraege a 4B (hour, condition, temp_min, temp_max) |
| 0x4003 | 6e404003-... | Notify | 4B | WX_ALERT: type(u8), severity(u8), time_until(u16) |

#### Telemetry Service (0x5000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x5001 | 6e405001-... | Notify | 28B | TLM_LIVE: speed, rpm, gear, coolant, ambient, fuel, voltage, lat, lon, alt, heading, sats, hdop |
| 0x5002 | 6e405002-... | Read | ~60B | TLM_SNAPSHOT: Vollstaendiger Sensor-Snapshot |
| 0x5003 | 6e405003-... | Notify (Write-Trigger) | var | Track Export (Binary Transfer) |
| 0x5004 | 6e405004-... | Write | var | TLM_TRACK_CMD: 0x01=start, 0x02=cancel, 0x03=status |

#### Config Service (0x6000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x6001 | 6e406001-... | Read, Notify | var | Config Read (JSON, Chunked Transfer) |
| 0x6002 | 6e406002-... | Write | var | Config Write (JSON, Chunked Transfer) |
| 0x6003 | 6e406003-... | Notify (Binary) | var | Config Export (Binary Transfer) |
| 0x6004 | 6e406004-... | Write (Binary) | var | Config Import (Binary Transfer) |
| 0x6005 | 6e406005-... | Write | 1B | Bond Management: 0x00=delete all bonds |

#### Diagnose Service (0x7000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x7001 | 6e407001-... | Read, Notify | var | DTC List (6B pro Eintrag: code, severity, timestamp) |
| 0x7002 | 6e407002-... | Read, Notify | 5B | Sensor Status: GPS, RPM, Temp, Fuel, SD |
| 0x7003 | 6e407003-... | Write, Notify | var | Self-Test: Write 0x01=start, Notify=result |
| 0x7004 | 6e407004-... | Notify (Binary) | var | Log Export (Binary Transfer) |
| 0x7005 | 6e407005-... | Write | var | Log Export Control |

#### OTA Service (0x8000)

| Char ID | UUID | Properties | Size | Payload |
|---|---|---|---|---|
| 0x8001 | 6e408001-... | Write, Notify | var | OTA Control: 0x01=begin, 0x02=commit, 0x03=abort |
| 0x8002 | 6e408002-... | Write Without Response | var | OTA Data (Firmware-Image, Binary Transfer) |
| 0x8003 | 6e408003-... | Notify | 37B | OTA Verify: SHA256(32), size(4), status(1) |

### 5.3 Notify-Raten und Trigger

| Characteristic | Rate | Trigger | Payload |
|---|---|---|---|
| NAV_MANEUVER (0x1001) | 1-2 s | Timer / Maneuver-Aenderung | type, distance, street (max 70B) |
| NAV_ROUTE (0x1002) | 10 s | Timer / >500m Aenderung | total, remaining, eta, time (16B) |
| NAV_ROAD (0x1003) | Event | Strassenwechsel | speed_limit, road_name (max 66B) |
| NAV_STATUS (0x1004) | Event | Statuswechsel | status (1B) |
| MEDIA_NOWPLAYING (0x2001) | 5 s | Timer / Titelwechsel | state, duration, pos, artist, title (max 139B) |
| PHONE_CALL (0x3001) | Event | Eingehender Anruf | caller_id, call_type (max 70B) |
| PHONE_NOTIF (0x3004) | Event | Neue Notification | priority, app, title, body (max 201B) |
| WX_CURRENT (0x4001) | 15 min | Timer | temp, humidity, condition, wind, feels, rain (8B) |
| WX_FORECAST (0x4002) | 30 min | Timer / Aenderung | 6x (hour, condition, temp_min, temp_max) (24B) |
| WX_ALERT (0x4003) | Event | Neue Warnung | type, severity, time_until (4B) |
| TLM_LIVE (0x5001) | 4 Hz (250 ms) | Timer | 28B (alle Live-Werte) |
| OTA_STATUS (via 0x8001) | Event | On-Chunk | progress, error (4B) |
| DTC List (0x7001) | Event | On-Change | code, severity, timestamp (6B/Eintrag) |

Im DEGRADING-Zustand (RSSI < -90 dBm) werden die Raten reduziert:

| Prioritaet | Daten | DEGRADING-Frequenz |
|---|---|---|
| P0 (Critical) | Incoming Call, Call Status | Sofort (event) |
| P1 (High) | Nav Maneuver | Alle 2-3 s (von 1-2s) |
| P2 (Medium) | Live Telemetry | Alle 1 s (von 250 ms) |
| P3 (Low) | Now Playing, Route, Weather | Gestoppt |
| P4 (Background) | Track/Log Export, OTA | Gestoppt |

### 5.4 Pairing / Security

- **Pairing-Methode:** LE Secure Connections (Bluetooth 4.2+), ECDH P-256
- **IO Capability:** No Input/No Output -> Just-Works (im Prototyp),
  Numeric Comparison (im Final-Design, 6-stellige Zahl auf Bordcomputer-Display)
- **Bonding:** Persistent, Keys im NVS gespeichert (ueberleben Stromausfall)
- **Max Bonds:** 3 (Haupttelefon, Zweitgeraet/Werkstatt, Reserve)
- **Encryption:** AES-128-CCM nach erfolgreichem Pairing
- **MITM-Schutz:** LE Secure Connections bietet Schutz auch ohne Passkey

### 5.5 Abstimmung mit Companion-App UUIDs

Die UUIDs muessen auf beiden Seiten (Firmware + App) identisch sein.
In der Companion-App sind sie in `src/core/ble/gatt-uuids.ts` definiert:

```typescript
const BASE_UUID_PREFIX = '6e40';
const BASE_UUID_SUFFIX = '-b5a3-f393-e0a9-e50e24dcca9e';

function uuid(id16: number): string {
  const hex = id16.toString(16).padStart(4, '0');
  return `${BASE_UUID_PREFIX}${hex}${BASE_UUID_SUFFIX}`;
}

export const SERVICE = {
  navigation: uuid(0x1000),
  media:      uuid(0x2000),
  phone:      uuid(0x3000),
  weather:    uuid(0x4000),
  telemetry:  uuid(0x5000),
  config:     uuid(0x6000),
  diagnose:   uuid(0x7000),
  ota:        uuid(0x8000),
} as const;
```

In der Firmware sind sie in `firmware/components/ble/gatt_uuids.h` als
16-bit-Makros definiert (Telemetry 0xA001, Config 0xA002, Navigation 0xA003,
OTA 0xA004). Bei der finalen Implementierung muessen diese auf die
128-bit-Base-UUID-Schemata angepasst werden, damit beide Seiten
uebereinstimmen.

### 5.6 Datenuebertragungs-Protokoll

Je nach Datenmenge werden drei Uebertragungsmodi verwendet:

**Klein (< 20 Bytes):** Direkter GATT-Write/Notify, kein Framing, kein CRC.
Der BLE-Link-Layer gewaehrleistet Integritaet und Reihenfolge.
Beispiele: Volume (1B), Media-CMD (1B), Call-CMD (1B).

**Mittel (20-500 Bytes):** Chunked-Write mit Transfer-ID, Sequence-Number,
More-Flag. Timeout 5 s pro Transfer-ID. Bei Luecke: Negative-ACK, 3 Retries.
Beispiel: Config-JSON (1200 Bytes = 5 Chunks a 244 Bytes).

**Gross (> 500 Bytes):** Binary-Transfer-Protokoll mit 7-Byte Header,
9-Byte Chunk-Header (Transfer-ID, Seq, Flags, Length, CRC16), Sliding-Window
(Window-Size 8), ACKs, Retransmission, Final-Notify mit CRC32.
Verwendet fuer: Track-Export, Log-Export, Firmware-OTA, Config-Binary.

**Bandbreiten-Schaetzung (MTU 247, 15 ms Interval):**

| Szenario | Bandbreite | Use-Case |
|---|---|---|
| MTU 247, 15 ms, Write NoResp | 20-40 kB/s | Firmware-OTA, Track-Export |
| MTU 247, 30 ms, Notify | 8-16 kB/s | Log-Export, Config |
| MTU 185, 30 ms, Notify | 5-10 kB/s | Standard-Betrieb |
| MTU 23 (Default), 30 ms | 1-2 kB/s | Worst-Case |

Ein 300 kB Firmware-Update dauert bei 30 kB/s ca. 10 Sekunden.

---

## 6. Companion-App Architektur

### 6.1 App-Stack

Die Companion-App ist eine React-Native-Applikation (eine Codebasis fuer
beide mobilen Betriebssysteme).

| Komponente | Wahl | Begruendung |
|---|---|---|
| Framework | React Native | Eine Codebasis fuer iOS + Android |
| BLE-Bibliothek | react-native-ble-plx | Vollstaendige GATT-API, Connection-Parameter-Kontrolle |
| State-Management | Zustand | Minimal, performant, kein Boilerplate |
| Navigation | React Navigation | Standard fuer RN, Stack/Tab-Navigation |
| Persistenz | SQLite (react-native-quick-sqlite) | Telemetrie-Cache, Tour-Historie, offline-faehig |
| Maps | react-native-maps | Native Karten-API des jeweiligen OS |
| Wetter | OpenWeatherMap API | Kostenlos bis 1000 Calls/Tag |
| Charts | react-native-chart-kit | Diagramme fuer Telemetrie-Historie |
| Icons | react-native-vector-icons | Material/Feather-Icons |

### 6.2 Architektur-Diagramm

Die App ist in drei Schichten gegliedert: UI-Layer, Data-Layer, BLE-Layer.

```
+=====================================================================+
|                        Companion-App                                 |
+=====================================================================+
|                                                                      |
|  +------------------+  +------------------+  +------------------+   |
|  |   UI-Layer       |  |   Data-Layer     |  |   BLE-Layer      |   |
|  |                  |  |                  |  |                  |   |
|  |  Dashboard       |  |  Telemetry Cache |  |  Connection Mgr  |   |
|  |  Navigation      |  |  Tour Store      |  |  GATT Operations |   |
|  |  Diagnose        |  |  Config Model    |  |  Transfer Proto  |   |
|  |  Settings        |  |  Nav State Cache |  |  Reconnect Logic|   |
|  |  Tours           |  |  Weather Cache   |  |  MTU/Param Mgr  |   |
|  |  (OTA)           |  |  Media State     |  |                  |   |
|  |                  |  |                  |  |  Zustand Store   |   |
|  |  Zustand Store   |  |  Zustand Store   |  |  (BLE-State)     |   |
|  |  (App-State)     |  |  (App-State)     |  |                  |   |
|  +--------+---------+  +--------+---------+  +--------+---------+   |
|           |    Zustand (global state bus)    |                     |
|           +---------------------+------------+                     |
|                                 |                                   |
|                     +-----------------------+                       |
|                     |   Native Bridge       |                       |
|                     +-----------------------+                       |
|                                 |                                   |
|                    +------------+------------+                      |
|                    | BLE (OS)   | Maps (OS) |                      |
|                    +------------+------------+                      |
+======================================================================+
                              | BLE GATT
                              v
                    +------------------+
                    |  ESP32-S3        |
                    +------------------+
```

### 6.3 Screen-Uebersicht

Die App hat 5 Haupt-Screens (Bottom-Tab-Navigation) plus OTA-Screen:

**Dashboard Screen:**
- Live-Telemetrie vom Bordcomputer (TLM_LIVE, 4 Hz Notify)
- Grosses Speed-Display (Ziffern, zentral)
- RPM als Balken/Tacho
- Gang-Anzeige
- Temperatur (Kuehlmittel + Umgebung)
- Kraftstoffstand mit Reichweite
- Spannung
- Verbindungssymbol (BLE-Status)
- Mini-Diagramm: Speed-Verlauf 5 Minuten (Telemetry Cache)
- Mini-Wetter-Widget

**Navigation Screen:**
- Vollbild-Karte mit Route (native Karten-API)
- Naechstes Maneuver als Overlay (Pfeil + Distanz + Strassenname)
- ETA und Reststrecke
- Die App ist Vermittler, kein eigenes Navi: Sie nutzt die
  Navigations-Engine des Betriebssystems und extrahiert Nav-Daten

**Diagnose Screen:**
- DTC-Liste (Code, Beschreibung, Severity, Timestamp)
- Sensor-Status als Status-Grid
- Self-Test-Button (startet Self-Test, zeigt Ergebnis)
- Log-Export-Button
- Firmware-Version

**Settings Screen:**
- Display-Einstellungen (Helligkeit, Theme, Auto-Dim)
- Sensor-Kalibrierung (Radumfang, RPM-Pulses, Temp-Offset)
- BLE-Einstellungen (Device-Name, Bond-Management)
- Logging-Einstellungen (Log-Level, Log-Format, Interval)
- Config-Export/Import (Backup/Restore)

**Tours Screen:**
- Liste aller aufgezeichneten Touren
- Datum, Strecke, Dauer, max/avg Speed
- Tap auf Tour -> Detail mit Karte (GPX), Hoehenprofil, Speed-Profil
- Export als GPX-Datei

**OTA Screen:**
- Aktuelle Firmware-Version
- Firmware-Datei auswaehlen (Datei-Picker)
- Transfer mit Fortschrittsbalken
- SHA-256 Verifikation
- Commit-Button
- Rollback-Info

### 6.4 State Management

Die App verwendet Zustand als globalen State-Store. Der Store verwaltet:

- `bleState`: ConnectionState, deviceName, deviceId, rssi, mtu, isEncrypted
- `telemetry`: Live-Telemetrie (TLM_LIVE, 4 Hz)
- `config`: BordcomputerConfig (via Config-Read/Write)
- `navManeuver`, `navRoute`, `navRoad`, `navStatus`: Navigation
- `sensorStatus`: Sensor-Status (5 Bytes)
- `dtcList`: DTC-Eintraege
- `selfTestResult`: Self-Test-Ergebnis
- `weather`: WeatherCurrent
- `tourSummary[]`: Tour-Historie

### 6.5 BLE Connection State Machine

Die BLE-Verbindung durchlaeuft eine State-Machine mit 10 Zustaenden:

```
UNBONDED -> BONDED -> DISCONNECTED -> SCANNING -> CONNECTING ->
DISCOVERING -> AUTHENTICATING -> CONNECTED -> DEGRADING -> RECONNECTING
```

**Zustaende:**

| Zustand | Beschreibung |
|---|---|
| UNBONDED | Keine gespeicherten Pairing-Keys. Advertising aktiv. |
| BONDED | Keys gespeichert. Direkter Connect ohne erneutes Pairing. |
| DISCONNECTED | Keine Verbindung. Advertising laeuft. 5s Timeout ->节能-Advertising. |
| SCANNING | App sucht aktiv per BLE-Scan. Filtert nach Service-UUID. |
| CONNECTING | LL-Connect-Request gesendet. 50-200 ms, 3s Timeout. |
| DISCOVERING | Physische Verbindung steht. GATT-Discovery laeuft. |
| AUTHENTICATING | Verschluesselte Verbindung mit Bonding-Key (LTK). |
| CONNECTED | Vollstaendig etabliert. Alle Services operativ. |
| DEGRADING | Verbindung steht, aber RSSI < -90 dBm. Reduzierte Datenrate. |
| RECONNECTING | Verbindung verloren. Auto-Reconnect mit Backoff. |

**Auto-Reconnect:**
- Exponentieller Backoff: 1s, 2s, 4s, 8s, 16s, max 30s
- Nach 5 fehlgeschlagenen Versuchen: 30s Intervall (Akku sparen)
- Vollstaendig transparent fuer den Nutzer (kein "Verbinden"-Button)

**Connection-Parameters:**

| Parameter | Wert | Begruendung |
|---|---|---|
| Connection Interval | 15-30 ms | Schnelle Nav-Updates, Telemetrie alle 250 ms |
| Slave Latency | 0 | Keine Latenz-Schoenung, jedes Event muss genutzt werden |
| Supervision Timeout | 5 s | Schneller Disconnect bei echtem Abbruch |

### 6.6 Offline/Cache Konzept

Bei Verbindungsabbruch degradieren Smartphone-abhaengige Features kontrolliert:

| Feature | Bei Abbruch | Wiederherstellung |
|---|---|---|
| Navigation | "Navigation Offline", letzte Route 30s gecacht | Bei Reconnect: neue Nav-Daten |
| Media | "Media Disconnected", Widget nach 10s ausblenden | Bei Reconnect: Now-Playing abonnieren |
| Phone | Notification-Relay deaktivieren | Bei Reconnect: Filter reaktivieren |
| Wetter | Letzte Daten 15 Min gueltig, danach "Weather Offline" | Bei Reconnect: Cache aktualisieren |
| Telemetry | SD-Logging laeuft weiter, Live-Stream stoppt | Bei Reconnect: Ring-Buffer (60s) abrufbar |

**Reconnect-Sequenz:**
1. MTU-Exchange (247)
2. Connection-Parameter (HIGH priority)
3. Service-Discovery (nur bei Service-Changed, sonst cached)
4. Encryption aktivieren
5. Subscribe P0: Incoming Call (0x3001), Call Status (0x3002)
6. Subscribe P1: Nav Maneuver (0x1001), Nav Status (0x1004)
7. Read Telemetry Snapshot (0x5002)
8. Subscribe P2: Live Telemetry (0x5001)
9. Read Config (0x6001)
10. Subscribe P3: Now Playing (0x2001), Weather (0x4001/0x4002)
11. Subscribe P4 (falls aktive Transfers): Track/Log Export

Der gesamte Reconnect dauert typischerweise 500-1000 ms.

---

## 7. Datenmodell

### 7.1 Zentrale Datenstrukturen

**sensor_data_t (Firmware, sensors.h):**

```c
typedef struct {
    float temp_c;          /* BMP280 Umgebungstemperatur [C] */
    float pressure_hpa;    /* BMP280 Luftdruck [hPa] */
    float altitude_m;      /* BMP280 Hoehe [m] */
    float roll_deg;        /* MPU6050 Rollwinkel [deg] */
    float pitch_deg;       /* MPU6050 Nickwinkel [deg] */
    float accel_g;         /* MPU6050 Gesamtbeschleunigung [g] */
    bool  crash_detected;  /* MPU6050 >3g */
    uint32_t timestamp_ms; /* Zeitstempel seit Boot [ms] */
} sensor_data_t;
```

In der vollstaendigen Implementierung (research/11) ist `sensor_data_t`
erweitert um alle Fahrzeugdaten:

```c
typedef struct {
    /* Powertrain */
    float rpm;              /* Drehzahl [U/min] */
    float speed_kmh;        /* Geschwindigkeit [km/h] */
    int8_t gear;            /* Gang (0=Leerlauf, 1-5) */
    bool  neutral;          /* Leerlaufschalter */

    /* Thermals */
    float coolant_temp_c;   /* Kuehlmitteltemperatur [C] */
    float oil_pressure_bar; /* Oeldruck [bar] */
    float ambient_temp_c;   /* Umgebungstemperatur [C] */

    /* Fuel */
    float fuel_liters;      /* Kraftstoffstand [L] */
    float fuel_percent;     /* Kraftstoff [%] */

    /* Orientation */
    float roll_deg;         /* Seitenneigung [deg] */
    float pitch_deg;        /* Laengsneigung [deg] */

    /* GPS */
    float lat, lon;         /* Position [dezimal] */
    float gps_speed_kmh;    /* GPS-Geschwindigkeit [km/h] */
    float altitude_m;       /* Hoehe [m] */
    uint8_t satellites;     /* Satellitenanzahl */
    float hdop;             /* Horizontale Praezision */
    uint8_t gps_fix;        /* 0=none, 1=2D, 2=3D, 3=DGPS */

    /* Power */
    float battery_v;        /* Bordspannung [V] */
    bool  generator_ok;     /* Generator laeuft (>13.8V) */

    uint32_t timestamp;     /* Zeitstempel [ms] */
} sensor_data_t;
```

**gps_data_t (Firmware, gps.h):**

```c
typedef struct {
    float lat;             /* Breitengrad [dezimal] */
    float lon;             /* Laengengrad [dezimal] */
    float speed_kmh;       /* Geschwindigkeit [km/h] */
    float course_deg;      /* Kurs/Heading [deg] */
    float altitude_m;      /* Hoehe ueber NN [m] */
    uint8_t satellites;    /* Anzahl Satelliten */
    bool  fix;             /* GPS-Fix aktiv? */
    uint32_t timestamp_ms; /* Zeitstempel [ms] */
} gps_data_t;
```

**TelemetryLive (Companion-App, gatt-uuids.ts):**

```typescript
interface TelemetryLive {
  speed: number;        // km/h (uint8)
  rpm: number;         // U/min (uint16)
  gear: number;        // int8 (-1=neutral)
  coolantTemp: number; // Celsius (int8)
  ambientTemp: number; // Celsius (int8)
  fuel: number;        // Prozent (uint8)
  voltage: number;     // Zehntelvolt (uint8)
  latitude: number;    // E-7 (int32)
  longitude: number;   // E-7 (int32)
  altitude: number;    // Meter (int16)
  heading: number;     // Zehntelgrad (uint16)
  satellites: number;  // (uint8)
  hdop: number;        // Zehntel (uint8)
}
// Gesamtgroesse: 28 Bytes, Notify-Rate: 4 Hz (250 ms)
```

**dtc_entry_t (Firmware, diagnostics.h):**

```c
typedef struct {
    dtc_code_t code;       /* DTC-Code (Enum) */
    uint32_t timestamp_ms; /* Zeitstempel [ms] */
    uint8_t count;         /* Wie oft aufgetreten */
    bool active;           /* Noch aktiv? */
} dtc_entry_t;
```

In der BLE-Uebertragung (DIAG_DTC, 0x7001) wird jeder Eintrag als
6 Bytes kodiert: code(2), severity(1), timestamp(4).

### 7.2 Konfigurations-Struktur

Die Konfiguration wird in zwei Speichern gehalten:

**NVS (Binary BLOB, `device_config_t`):**

```c
typedef struct __attribute__((packed)) {
    uint16_t version;        /* Schema-Version fuer Migrationen */

    /* Kalibrierung */
    int16_t  gyro_offset[3];
    int16_t  accel_offset[3];
    uint32_t bmp_sea_level_pa;    /* QNH-Referenzdruck */
    uint16_t adc_vref_mv;
    float    fuel_cal_liters[6];  /* 6 Stuetzpunkte */
    uint16_t fuel_cal_adc[6];

    /* Schwellen */
    uint16_t rpm_redline;         /* 8000 */
    uint16_t rpm_shift_light;     /* 7000 */
    float    battery_low_v;       /* 11.5 */
    float    battery_shutdown_v;  /* 10.5 */
    float    oil_pressure_warn;   /* 0.5 bar */
    float    oil_pressure_high;   /* 6.0 bar */
    float    coolant_temp_warn;    /* 115 C */
    float    coolant_temp_crit;   /* 120 C */

    /* Logging */
    uint8_t  log_interval_s;      /* 1-60 */
    uint8_t  log_format;          /* 0=CSV, 1=Binary, 2=Both */

    /* GPS */
    uint8_t  gps_rate_hz;         /* 1, 5, 10 */

    /* Display */
    uint8_t  display_mode;        /* 0=Tour, 1=Track, 2=Nav, 3=Config */
    uint8_t  display_invert;
    uint8_t  display_brightness;   /* 0-255 (Backlight-PWM) */

    /* BLE */
    char     device_name[16];

    /* Animation */
    uint8_t  anim_enabled;
    uint8_t  rpm_needle_spring;   /* 0=Linear, 1=Spring-Physics */
} device_config_t;
```

**Config-Modell (Companion-App, config.ts):**

```typescript
interface BordcomputerConfig {
  brightness: number;   // 0-100 %
  theme: 'dark' | 'light';
  autoDim: boolean;
  gpsRate: number;      // Hz
  rpmRedline: number;   // U/min
  shiftLight: number;   // U/min
  bmpQnh: number;       // hPa
  deviceName: string;
  pairedDevices: number;
  logFormat: 'csv' | 'binary' | 'csv_binary';
  logInterval: number;  // Sekunden
}
```

Config-Aenderungen ueber BLE werden sofort angewendet (Live-Update),
ohne Reboot. Config-Read/Write uebertragen JSON per Chunked-Transfer.

**Migration:** Bei NVS-Versionsaenderung wird `config_migrate()` aufgerufen:
alte Werte bleiben erhalten, neue Felder bekommen Defaults.

### 7.3 DTC-Format und -Speicherung

DTCs werden in einer RAM-Tabelle (16 Eintraege) gehalten.
Bei kritischen Fehlern wird zusaetzlich auf SD-Karte geloggt
(`/sdcard/logs/dtc.log`):

```
[timestamp_ms] CODE [severity] description
```

Bei Persistenz in NVS (geplant): DTC-Tabelle als BLOB, ueberlebt Reboot.
Per BLE wird die DTC-Liste ueber den Diagnose-Service (0x7001) uebertragen.

### 7.4 Tour-Log Format

Telemetrie wird als CSV auf SD-Karte geloggt:

```
timestamp,lat,lon,speed_kmh,rpm,temp_c,alt_m,roll_deg,pitch_deg
```

Dateinamen: `/sdcard/logs/YYYYMMDD_HHMMSS.csv`

GPX-Track-Export: `/sdcard/tracks/YYYYMMDD_HHMMSS.gpx`

Ein GPX-Track-Point ist ca. 80-120 Bytes. Bei 1 Hz GPS und 24h Tour
(~86.400 Punkte) sind das ca. 8.6 MB.

---

## 8. Power-Management

### 8.1 Stromversorgungskette

```
LiPo 18650 (3.7V, 3400mAh)
  |
  +---> TP4056 (Laden via USB-C)
  |
  +---> MT3608 Boost (3.7V -> 5V)
  |       |
  |       +---> TFT-Display (Backlight)
  |       +---> E-Paper (Refresh)
  |       +---> HT7333 LDO (5V -> 3.3V)
  |               |
  |               +---> ESP32-S3 (Core + Radio)
  |               +---> MPU6050
  |               +---> BMP280
  |               +---> GPS-Modul
  |               +---> MicroSD
  |
  +---> Spannungsteiler ---> ADC (Akku-Ueberwachung)
```

| Komponente | Strom (aktiv) | Strom (sleep) |
|---|---|---|
| ESP32-S3 | ~80 mA | ~10 uA (Deep Sleep) |
| GPS-Modul | ~25 mA | ~5 mA (Backup) |
| TFT-Display | ~50 mA (Backlight) | ~0 mA (aus) |
| E-Paper | ~2 mA (Refresh) | ~0 mA (statisch) |
| MPU6050 | ~3.5 mA | ~10 uA |
| BMP280 | ~3.6 uA | ~0.3 uA |
| MicroSD | ~30 mA (Write) | ~200 uA |
| MT3608 Quiescent | ~0.3 mA | -- |
| **Summe aktiv** | **~190 mA** | |
| **Summe sleep** | | **~0.4 mA** |

Bei 3400 mAh Akku: ~18 Stunden aktiv, mehrere Wochen im Sleep-Modus.

### 8.2 Power-States

| Zustand | Display | GPS | Logger | BLE | Sensoren | Beschreibung |
|---|---|---|---|---|---|---|
| ACTIVE | An | An | An | An | An | Vollbetrieb, alle Tasks |
| IDLE | Aus | An | An | An | An | Track-Aufzeichnung ohne Display |
| SLEEP | Aus | Aus | Aus | Advertising | RTC only | Nur Akku-Ueberwachung |
| SHUTDOWN | Aus | Aus | Flush | Aus | Aus | Akku < 3.2V, SD-Flush, Power-Off |

### 8.3 State-Uebergaenge

```
ACTIVE --[60s keine Display-Interaktion + kein BT]--> IDLE
IDLE   --[5min keine Bewegung (IMU < Schwellwert)]--> SLEEP
SLEEP  --[IMU-Interrupt oder BT-Verbindungsanfrage]--> ACTIVE
SLEEP  --[Zuendung AN (GPIO)]--> ACTIVE
*      --[Akku < 3.2V]--> SHUTDOWN (sofort, alle Zustaende)
```

### 8.4 Wakeup-Quellen

| Quelle | Trigger | Verwendung |
|---|---|---|
| GPIO (Zuendung) | Spannung an Zündungsleitung | Motor an -> ACTIVE |
| IMU-Interrupt | Bewegung > Schwellwert | Fahrzeug bewegt -> ACTIVE |
| BLE | Verbindungsanfrage der App | App verbindet -> ACTIVE |
| Timer | RTC-Timer | Periodische GPS-Log-Rolle (z.B. alle 10 Min) |

### 8.5 Backup-Akku (18650)

Der LiPo 18650 (3400 mAh) ermoeglicht GPS-Logging bei ausgeschaltetem Motor:

- Im SLEEP-Modus ist nur die RTC aktiv (~0.4 mA).
- Bei periodischem Wakeup (alle 10 Min) wird GPS kurz eingeschaltet,
  Position geloggt, dann wieder SLEEP.
- Dadurch kann der Bordcomputer die Position des Motorrads ueber Wochen
  verfolgen, wenn der Motor abgestellt ist.
- Die Akku-ueberwachung erfolgt ueber ADC1_CH4 (GPIO4) mit Spannungsteiler 2:1.
- Generator-Status: Bordspannung > 13.8V -> Generator laeuft.

### 8.6 OTA-Update

Das OTA-Update uebertraegt ein Firmware-Image ueber BLE in 4 KB Chunks:

```
Companion-App -> BLE OTA Service -> Flash Write -> SHA256 Verify -> Boot
```

1. **Begin:** App schreibt 0x01 an OTA-Control (0x8001), mit Gesamtgroesse.
2. **Transfer:** Firmware-Image in 4 KB Chunks ueber OTA-Data (0x8002),
   Write Without Response fuer maximalen Throughput.
3. **Verify:** Bordcomputer berechnet SHA256 des geflashten Images und
   sendet es ueber OTA-Verify (0x8003). App vergleicht mit erwartetem Hash.
4. **Commit:** App schreibt 0x02 an OTA-Control. Bordcomputer setzt
   Boot-Partition und startet neu.
5. **Rollback:** Beim Boot prueft das System, ob die Firmware neu ist
   (`ESP_OTA_IMG_PENDING_VERIFY`). Self-Test wird durchgefuehrt.
   Bei Erfolg: `esp_ota_mark_app_valid_cancel_rollback()`.
   Bei Fehlschlag: Rollback zur vorherigen Partition.

**Rollback-Schutz:** Eine neue Firmware wird nicht sofort als gueltig markiert.
Sie muss sich beim naechsten Boot bewaehren (Self-Test, Watchdog, kein
Boot-Loop). Bei Boot-Loop (>3 Resets in 60s) wird automatisch zur
vorherigen Partition zurueckgekehrt.

---

## Anhang: Datei-Struktur

```
transalp-bordcomputer/
|-- docs/
|   |-- ARCHITECTURE.md        (diese Datei)
|   |-- HARDWARE.md            (Bauteile, Stromversorgung)
|   |-- PINMAP.md              (Pin-Belegung, alleinige Referenz)
|   |-- ROADMAP.md             (Projekt-Roadmap)
|   |-- research/              (Forschungs-Dokumente)
|       |-- 01-sensoren-fahrzeugdaten.md
|       |-- 02-hardware-deep-dive.md
|       |-- 03-software-stack.md
|       |-- 04-pd06-technische-daten.md
|       |-- 05-gehaeuse-einbau-emv.md
|       |-- 06-feature-roadmap.md
|       |-- 07-display-navigation-smartphone.md
|       |-- 08-feature-realization-matrix.md
|       |-- 09-master-plan.md
|       |-- 10-ui-ux-design.md
|       |-- 11-firmware-architecture.md
|       |-- 12-diagnose-support.md
|       |-- 13-connectivity-app.md
|
|-- firmware/
|   |-- CMakeLists.txt          (Root-CMake, EXTRA_COMPONENT_DIRS=components)
|   |-- partitions.csv          (OTA-faehige Partition Table)
|   |-- sdkconfig.defaults      (Build-Konfiguration)
|   |-- main/
|   |   |-- main.c              (app_main: Init + Task-Erstellung)
|   |   |-- board_config.h      (Pin-Definitionen, Task-Konstanten)
|   |   |-- CMakeLists.txt
|   |-- components/
|       |-- ble/                (BLE: NimBLE GATT-Server)
|       |   |-- ble.h, ble.c
|       |   |-- ble_service.c, ble_callbacks.c
|       |   |-- gatt_uuids.h
|       |-- sensors/            (Sensoren: IMU, RPM, Temp, Fuel, Gear)
|       |   |-- sensors.h, sensors.c
|       |   |-- imu.c, rpm.c, temperature.c
|       |   |-- fuel_level.c, gear_indicator.c
|       |-- gps/                (GPS: UART, NMEA-Parser)
|       |   |-- gps.h, gps_task.c
|       |   |-- nmea_parser.c
|       |-- display/            (Display: Renderer, TFT, EPD, Widgets)
|       |   |-- display.h, display.c
|       |   |-- renderer.c, tft_driver.c
|       |   |-- epaper_driver.c, widgets.c
|       |-- storage/            (Storage: SD, Logger, Config)
|       |   |-- storage.h, storage.c
|       |   |-- sd_card.c, logger.c, config.c
|       |-- diagnostics/        (Diagnose: DTC, Self-Test)
|       |   |-- diagnostics.h, diagnostics.c
|       |   |-- dtc.c, selftest.c
|       |-- power/               (Power: Akku, Sleep)
|       |   |-- power.h, power.c
|       |   |-- power_manager.c
|       |-- utils/               (Utils: Event-Bus, Filter)
|           |-- event_bus.h, event_bus.c
|           |-- filters.h, filters.c
|           |-- utils.h
|
|-- companion-app/              (React Native App, Prototyp)
    |-- src/
    |   |-- App.tsx             (Root: BLE-Manager, Navigator)
    |   |-- navigation/
    |   |   |-- TabNavigator.tsx (5 Tabs: Dashboard, Nav, Diagnose, Settings, Tours)
    |   |-- screens/
    |   |   |-- DashboardScreen.tsx
    |   |   |-- NavigationScreen.tsx
    |   |   |-- DiagnoseScreen.tsx
    |   |   |-- SettingsScreen.tsx
    |   |   |-- ToursScreen.tsx
    |   |-- components/
    |   |   |-- UI.tsx, SpeedDisplay.tsx
    |   |   |-- RpmBar.tsx, NavArrow.tsx
    |   |-- core/
    |   |   |-- ble/
    |   |   |   |-- gatt-uuids.ts     (UUID-Definitionen, synchron zu Firmware)
    |   |   |   |-- connection-state.ts (BLE State-Machine)
    |   |   |   |-- BleProvider.ts     (Provider-Interface)
    |   |   |   |-- MockBleProvider.ts  (Mock fuer Entwicklung)
    |   |   |-- state/
    |   |   |   |-- store.ts    (Zustand globaler Store)
    |   |   |   |-- config.ts   (Config-Modell)
    |   |   |-- theme/
    |   |       |-- colors.ts   (Farbpalette, Design-Tokens)
    |   |-- hooks/
    |       |-- useBleManager.ts (BLE-Lebenszyklus, Callback-Wiring)
```

---

*Diese Datei ist die zentrale Architektur-Referenz. Bei Aenderungen an
der Architektur muss diese Datei aktualisiert werden. Pin-Aenderungen
muessen auch in `docs/PINMAP.md` und `firmware/main/board_config.h`
nachgezogen werden.*
