# Developer Quick-Start Guide

> **Von 0 zu laufendem Build in unter 30 Minuten**
> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1, ESP-IDF v5.4, FreeRTOS SMP
> Companion-App: React Native (Expo)

---

## Inhaltsverzeichnis

1. [Voraussetzungen](#1-voraussetzungen)
2. [Repo Klonen & Setup](#2-repo-klonen--setup)
3. [Firmware Build](#3-firmware-build)
4. [Companion-App Build](#4-companion-app-build)
5. [Mockups Ansehen](#5-mockups-ansehen)
6. [Tests Ausfuehren](#6-tests-ausfuehren)
7. [Erste Schritte mit der Entwicklung](#7-erste-schritte-mit-der-entwicklung)
8. [Debugging](#8-debugging)
9. [CI/CD Pipeline verstehen](#9-cicd-pipeline-verstehen)
10. [Naechste Schritte](#10-naechste-schritte)

---

## 1. Voraussetzungen

| Tool | Version | Zweck |
|---|---|---|
| ESP-IDF | v5.4 (mit Toolchain) | Firmware-Build, Flash, Monitor |
| Python | 3.8+ | ESP-IDF-Skripte, Track-Export |
| Node.js | 18+ | Companion-App (Expo/Metro) |
| Git | 2.x | Repo klonen |
| GCC | any modern | Host-Unit-Tests (nur fuer Tests) |

Hardware (optional fuer ersten Build):
- ESP32-S3-DevKit (N16R8: 16 MB Flash, 8 MB PSRAM empfohlen)
- USB-Kabel fuer Flash + Serial-Monitor

ESP-IDF installieren (einmalig):

```bash
mkdir -p ~/esp && cd ~/esp
git clone -b v5.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
```

In jedem neuen Terminal die Umgebung aktivieren:

```bash
. ~/esp/esp-idf/export.sh
```

Wenn `idf.py --version` eine Versionsnummer ausgibt, ist das Setup
korrekt.

---

## 2. Repo Klonen & Setup

```bash
git clone https://github.com/TitoPrausee/transalp-bordcomputer.git
cd transalp-bordcomputer
```

Repo-Struktur Ueberblick:

```
transalp-bordcomputer/
├── firmware/                ESP-IDF Projekt (9 Komponenten + main)
│   ├── CMakeLists.txt       Root-CMake (project + EXTRA_COMPONENT_DIRS)
│   ├── partitions.csv       OTA-faehige Flash-Partition-Tabelle
│   ├── sdkconfig.defaults   Build-Defaults (PSRAM, NimBLE, FAT, OTA)
│   ├── main/                app_main(), event loop, board_config.h
│   ├── components/
│   │   ├── ble/             NimBLE GATT-Server (8 Services)
│   │   ├── diagnostics/     DTC, self-test, serial console
│   │   ├── display/         TFT + E-Paper Treiber, Renderer, Widgets
│   │   ├── gps/             NMEA-Parser, GPS-Task
│   │   ├── ota/             OTA-Update mit Dual-Partition-Rollback
│   │   ├── power/           Akku-Monitor, Sleep-Zustandsmaschine
│   │   ├── sensors/         IMU, RPM, Temp, Fuel, Gear
│   │   ├── storage/         SD-Karte, Logger, Config
│   │   └── utils/           Event-Bus, Filter
│   └── test/                Unity-Unit-Tests (Host-Build mit GCC)
├── companion-app/           React-Native-App (Expo)
│   └── src/
│       ├── components/      SpeedDisplay, RpmBar, NavArrow, UI
│       ├── core/ble/        BLE-Provider, GATT-UUIDs, Parsers
│       ├── core/state/      Zustand-Store, Config
│       ├── core/theme/      Farbpalette
│       ├── hooks/           useBleManager
│       ├── navigation/      TabNavigator
│       ├── screens/         Dashboard, Navigation, Diagnose, Tours, Settings
│       └── services/        BleManager
├── mockups/                 HTML/CSS-Mockups aller Screens
├── docs/                    Architektur, Firmware-Guide, API-Ref, CI/CD, ...
├── tools/                   export_track.py (CSV -> GPX)
└── .github/workflows/       CI/CD-Pipelines
```

---

## 3. Firmware Build

```bash
# ESP-IDF Umgebung aktivieren (falls noch nicht aktiv)
. ~/esp/esp-idf/export.sh

cd firmware
idf.py set-target esp32s3
idf.py menuconfig   # Optional: Board-Konfiguration anpassen
idf.py build
```

Erwartete Build-Zeit: 3-5 Minuten (erste Build mit vollstaendiger
Toolchain-Kompilierung). Folgende Builds dauern <30 Sekunden
(inkrementell via Ninja).

Erwartete Binary-Groesse: ~1.2-1.8 MB (Factory-Partition hat 3 MB
reserviert, OTA-Partitions jeweils 3 MB).

Build-Artefakte (in `firmware/build/`):

| Datei | Beschreibung |
|---|---|
| `transalp_bordcomputer.bin` | Haupt-Firmware-Image |
| `bootloader.bin` | Bootloader |
| `partition_table.bin` | Partition-Tabelle |
| `ota_data_initial.bin` | OTA-Boot-State (factory -> ota_0) |

Flashen (falls Hardware vorhanden):

```bash
# Port anpassen: macOS /dev/cu.SLAB_USBtoUART, Linux /dev/ttyUSB0
idf.py -p /dev/cu.SLAB_USBtoUART flash monitor
```

Der Serial-Monitor zeigt die Boot-Sequenz:

```
I (312) main: ========================================
I (312) main:   Transalp Bordcomputer v0.4.0-integration
I (312) main:   Build: Oct 13 2026 14:00:00
I (312) main:   ESP32-S3 dual-core, FreeRTOS
I (312) main: ========================================
I (318) main: NVS flash initialised
I (320) main: Init: Power management
I (325) main:   Battery: 4.12 V (95%)
I (330) main: Init: Storage (SD card + NVS config)
...
I (450) main: System ready.
```

Mit `Ctrl + ]` den Monitor verlassen.

---

## 4. Companion-App Build

```bash
cd companion-app
npm install
```

Das Projekt nutzt Expo SDK 52 mit React Native 0.76. Die
Abhaengigkeiten sind in `package.json` deklariert (React 18.3.1,
react-native-ble-plx fuer BLE, zustand fuer State-Management,
@react-navigation fuer Navigation).

Entwicklungsserver starten:

```bash
npm start
```

Expo oeffnet einen Development-Server. Scanne den QR-Code mit der
Expo-App auf dem Smartphone oder druecke `a` fuer Android-Emulator
bzw. `i` fuer iOS-Simulator.

TypeScript-Check:

```bash
npm run lint
# oder direkt:
npx tsc --noEmit
```

Build fuer Geraet (optional):

```bash
# Android APK
npm run android

# iOS (benoetigt macOS + Xcode)
npm run ios
```

Bekannte Issues:
- `react-native-ble-plx` benoetigt native Module. Im Expo-Prototyp
  wird der `MockBleProvider` verwendet (simuliert BLE-Daten). Fuer
  echte BLE-Kommunikation ist ein Development-Build noetig
  (`npx expo run:android`).
- Bei `npm install`-Fehlern: `node_modules` loeschen und
  `npm install --legacy-peer-deps` probieren.

---

## 5. Mockups Ansehen

HTML-Mockups fuer alle Screens liegen in `mockups/`:

```bash
open mockups/index.html
```

Verfuegbare Screens:

| Datei | Screen |
|---|---|
| `boot.html` | Boot-Screen mit Logo |
| `instrument.html` | Haupt-Instrumente (Speed, RPM, Gear) |
| `navigation.html` | Turn-by-Turn Navigation |
| `diagnose.html` | Diagnose- und DTC-Uebersicht |
| `settings.html` | Einstellungen |
| `split.html` | Split-View (Instrument + Navigation) |
| `tour-statistik.html` | Tour-Statistik |
| `warning.html` | Warnung-Overlay |

Die Mockups sind statisches HTML/CSS und zeigen das angestrebte
UI-Design. Sie dienen als Referenz fuer die Companion-App-Entwicklung.

---

## 6. Tests Ausfuehren

### Firmware Unit-Tests (Host-Build mit GCC)

Die Tests verwenden das Unity-Test-Framework und laufen auf dem
Host-PC. ESP-IDF-Abhaengigkeiten werden durch Mocks ersetzt.

```bash
cd firmware/test

# Unity klonen (falls nicht vorhanden)
git clone https://github.com/ThrowTheSwitch/Unity /tmp/unity

# CMake-basierter Build
mkdir build && cd build
cmake -DUNITY_DIR=/tmp/unity ..
make
ctest
```

Verfuegbare Tests:

| Test-Datei | Modul | Getestet |
|---|---|---|
| `test_nmea_parser.c` | gps/nmea_parser.c | GGA/RMC/VTG-Parsing, Checksum |
| `test_dtc.c` | diagnostics/dtc.c | DTC-Register, -Clear, -GetActive |
| `test_event_loop.c` | main/app_event_loop.c | Subscriber-Pattern, Dispatch |

Alternativ: direkter GCC-Aufruf (wie in der CI-Pipeline):

```bash
gcc -DTEST_HOST -I../components/gps/include -I../components/utils/include \
    -I../components/diagnostics/include -I./mocks -I/tmp/unity/src \
    test_nmea_parser.c /tmp/unity/src/unity.c -o test_nmea -lm -lpthread
./test_nmea
```

### Companion-App Tests

```bash
cd companion-app
npm test
```

Falls keine Test-Runner konfiguriert sind, mindestens den
TypeScript-Check ausfuehren:

```bash
npm run lint   # tsc --noEmit
```

---

## 7. Erste Schritte mit der Entwicklung

### 7.1 Schnellreferenz: Wo finde ich was?

| Ich will ... | Datei |
|---|---|
| Pin-Definitionen aendern | `firmware/main/board_config.h` |
| Task-Prioritaeten anpassen | `firmware/main/board_config.h` |
| Boot-Sequenz aendern | `firmware/main/main.c` (app_main) |
| Event-Subscribers hinzufuegen | `firmware/main/main.c` (register_subscribers) |
| NMEA-Parsing erweitern | `firmware/components/gps/nmea_parser.c` |
| GATT-Service hinzufuegen | `firmware/components/ble/ble_service.c` + `gatt_uuids.h` |
| UI-Widget zeichnen | `firmware/components/display/widgets.c` |
| Display-Layout aendern | `firmware/components/display/display.c` |
| Sensor hinzufuegen | `firmware/components/sensors/sensors.c` + `sensors.h` |
| Config-Werte lesen/schreiben | `firmware/components/storage/config.c` |
| App-State erweitern | `companion-app/src/core/state/store.ts` |
| BLE-UUID hinzufuegen | `companion-app/src/core/ble/gatt-uuids.ts` + firmware `gatt_uuids.h` |
| Screen hinzufuegen | `companion-app/src/screens/` + `navigation/TabNavigator.tsx` |

### 7.2 Event-Bus verstehen

Komponenten kommunizieren ueber zwei Mechanismen:

**1. Event-Bus (FreeRTOS EventGroups) -- State-Flags:**

```c
// firmware/components/utils/include/event_bus.h

// Event setzen (von jedem Task aus sicher):
event_bus_set(EVT_GPS_FIX);

// Auf Event warten (blockierend):
event_bus_wait(EVT_SDCARD_MOUNTED, 5000);

// Verfuegbare Events:
//   EVT_GPS_FIX, EVT_GPS_LOST_FIX, EVT_SENSOR_READY,
//   EVT_CRASH_DETECTED, EVT_BATTERY_LOW, EVT_BATTERY_CRITICAL,
//   EVT_SDCARD_MOUNTED, EVT_SDCARD_ERROR,
//   EVT_BLE_CONNECTED, EVT_BLE_DISCONNECTED,
//   EVT_DISPLAY_READY, EVT_SHUTDOWN,
//   EVT_OTA_START, EVT_OTA_DONE, ...
```

**2. App Event Loop -- Subscriber-Pattern (mit Payload):**

```c
// firmware/main/app_event_loop.h

// Callback definieren:
static void on_sensor_update(const app_event_t *ev, void *ctx)
{
    // ev->data enthaelt bis zu 128 Bytes Payload
    // ev->type == APP_EVENT_SENSOR_UPDATE
    renderer_invalidate(0, 0, TFT_WIDTH, TFT_HEIGHT / 2);
}

// Subscriber registrieren (in register_subscribers):
uint32_t mask = (1U << APP_EVENT_SENSOR_UPDATE);
app_event_subscribe(mask, on_sensor_update, NULL);

// Event dispatchen (von jedem Task aus):
app_event_dispatch(APP_EVENT_SENSOR_UPDATE, &sensor_data, sizeof(sensor_data_t));
```

### 7.3 Eigene Komponente hinzufuegen

```bash
mkdir -p firmware/components/my_component/include
```

Header anlegen (`firmware/components/my_component/include/my_component.h`):

```c
#ifndef _MY_COMPONENT_H_
#define _MY_COMPONENT_H_

void my_component_init(void);
int32_t my_component_get_value(void);

#endif
```

Implementierung (`firmware/components/my_component/my_component.c`):

```c
#include "my_component.h"
#include "esp_log.h"

static const char *TAG = "my_comp";

void my_component_init(void)
{
    ESP_LOGI(TAG, "Initialised");
}

int32_t my_component_get_value(void)
{
    return 42;
}
```

CMakeLists.txt (`firmware/components/my_component/CMakeLists.txt`):

```cmake
idf_component_register(
    SRCS "my_component.c"
    INCLUDE_DIRS "include"
    REQUIRES utils    # falls Event-Bus genutzt wird
)
```

In `main/CMakeLists.txt` hinzufuegen:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        gps
        display
        sensors
        storage
        ble
        power
        ota
        diagnostics
        my_component    # <-- hinzufuegen
)
```

In `main.c` nutzen:

```c
#include "my_component.h"
// ...
my_component_init();
```

### 7.4 Eigenes Widget erstellen

Widgets sind stateless Funktionen in
`firmware/components/display/widgets.c`, die in den Framebuffer
zeichnen. Die Renderer-API stellt Primitive bereit:

```c
// Neues Widget in widgets.c hinzufuegen:

void widgets_draw_battery(uint8_t percent,
                          uint16_t x, uint16_t y)
{
    /* Rahmen */
    renderer_fill_rect(x, y, 40, 16, COLOR_BG_PANEL);
    renderer_fill_rect(x + 40, y + 4, 4, 8, COLOR_BG_PANEL);

    /* Fuellung */
    uint16_t fill_w = (40 * percent) / 100;
    uint16_t color = percent > 50 ? COLOR_ACCENT_GREEN
                  : percent > 20 ? COLOR_ACCENT_AMBER
                  : COLOR_ACCENT_RED;
    renderer_fill_rect(x + 1, y + 1, fill_w, 14, color);
}
```

Deklaration in `display.h` ergaenzen:

```c
void widgets_draw_battery(uint8_t percent, uint16_t x, uint16_t y);
```

Aufruf in `display.c` (innerhalb von `display_task`):

```c
widgets_draw_battery(power_get_battery_pct(), 200, 5);
renderer_invalidate(200, 5, 44, 16);
```

### 7.5 BLE-Service erweitern

GATT-UUIDs sind zentral in `firmware/components/ble/gatt_uuids.h`
definiert und muessen mit der Companion-App
(`companion-app/src/core/ble/gatt-uuids.ts`) uebereinstimmen.

Neue Characteristic hinzufuegen:

```c
// 1. UUID in gatt_uuids.h definieren:
#define CHR_MY_DATA    0x5005   /* Service 0x5000 (Telemetry) + 0x05 */

// 2. In ble_service.c: Characteristic im GATT-Table registrieren
//    (ble_gatt_svc_def Array fuer den Telemetry-Service erweitern)

// 3. Read/Write-Callback implementieren:
static int gatt_svr_chr_my_data_access(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t data[] = { 0x01, 0x02, 0x03 };
        os_mbuf_append(ctxt->om, data, sizeof(data));
        return 0;
    }
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}
```

Companion-App-Seite:

```typescript
// companion-app/src/core/ble/gatt-uuids.ts
export const CHAR = {
  // ... bestehende Chars ...
  myData: uuid(0x5005),
} as const;
```

UUID-Format: Base `6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e`, wobei
`XXXX` die 16-bit-ID ist.

---

## 8. Debugging

### Serial-Console

```bash
idf.py -p /dev/cu.SLAB_USBtoUART monitor
```

Log-Level anpassen in `sdkconfig.defaults`:

```
CONFIG_LOG_DEFAULT_LEVEL_INFO=y     # Default
# Fuer mehr Details:
# CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
# CONFIG_LOG_DEFAULT_LEVEL_VERBOSE=y
```

Oder zur Laufzeit per `esp_log_level_set()`:

```c
esp_log_level_set("ble_svc", ESP_LOG_DEBUG);
esp_log_level_set("gps", ESP_LOG_VERBOSE);
```

### JTAG-Debugging (ESP32-S3)

```bash
idf.py -p /dev/cu.SLAB_USBtoUART openocd
# In anderem Terminal:
idf.py gdb
```

Benötigt einen JTAG-Adapter (z.B. ESP-PROG) oder den
USB-JTAG-Modus des ESP32-S3 (GPIO 39/40).

### Companion-App Debugging

React-Native-Debugger: Im Metro-Bundler-Menue `Debug JS Remotely`
aktivieren. Der Browser-DevTools-Inspector oeffnet sich.

Zustand-Store inspizieren (zustand):

```typescript
// In einer Screen-Komponente:
import { useAppStore } from '../core/state/store';
const telemetry = useAppStore((s) => s.telemetry);
console.log('telemetry:', telemetry);
```

### Common Issues

Siehe `docs/TROUBLESHOOTING.md` fuer ausfuehrliche Loesungen.

| Problem | Loesung |
|---|---|
| `idf.py: command not found` | `. ~/esp/esp-idf/export.sh` ausfuehren |
| Build schlaegt fehlt: `ESP_ERR_NVS` | `idf.py erase-flash`, dann neu flashen |
| BLE verbindet nicht | NimBLE-Config pruefen, Advertising-Logs lesen |
| SD-Karte nicht erkannt | SPI-Pins in `board_config.h` pruefen |
| Display bleibt weiss | Backlight-Pin (`TFT_BL_PIN`) pruefen |
| `npm install` fehlerhaft | `rm -rf node_modules && npm install` |

---

## 9. CI/CD Pipeline verstehen

Drei unabhaengige Pipelines in `.github/workflows/`:

| Workflow | Trigger (Pfad-Filter) | Stufen |
|---|---|---|
| `firmware-ci.yml` | `firmware/**` | Lint (clang-format, cppcheck) -> Build (ESP-IDF) -> Unit-Test (Unity/GCC) -> Artifact |
| `companion-app-ci.yml` | `companion-app/**` | Lint (tsc, eslint) -> Build (Metro) -> Artifact |
| `docs-ci.yml` | `docs/**` | markdown-lint, brand-check, link-check |

CI-Ergebnisse lesen:

```bash
gh pr checks          # CI-Status fuer aktuellen PR
gh run list           # Letzte Workflow-Runs auflisten
gh run view <run-id>  # Details + Logs eines Runs
```

### Markennamen-Blocklist

Die `docs-ci.yml` Pipeline enthaelt einen `brand-check`-Job, der alle
`.md`- und `.yml`-Dateien gegen eine Blocklist
(`.github/brand-blocklist.txt`) prueft. Bei einem Match schlaegt die
CI fehl.

```bash
# Lokal pruefen vor dem Commit:
while IFS= read -r pattern; do
    [ -z "$pattern" ] && continue
    grep -rinE "$pattern" docs/ && echo "VIOLATION: $pattern"
done < .github/brand-blocklist.txt
```

Die Blocklist enthaelt Hersteller- und Plattform-Markennamen, die in
diesem Open-Source-Projekt nicht erscheinen duerfen. Bei neuen
Abhaengigkeiten oder Tool-Erwaehnungen: Blocklist pruefen.

---

## 10. Naechste Schritte

| Thema | Dokument |
|---|---|
| System-Architektur, Task-Modell | `docs/ARCHITECTURE.md` |
| Firmware-Engineering (Build, Debug, OTA) | `docs/FIRMWARE-GUIDE.md` |
| BLE GATT-API (Services, Characteristics) | `docs/API-REFERENCE.md` |
| CI/CD-Pipeline (alle Stufen, Artifacts) | `docs/CI-CD-PIPELINE.md` |
| Hardware-Montage und Verkabelung | `docs/INSTALLATION-GUIDE.md` |
| Troubleshooting und FAQ | `docs/TROUBLESHOOTING.md` |
| Pin-Belegung ESP32-S3 | `docs/PINMAP.md` |
| Performance-Optimierung | `docs/PERFORMANCE-OPTIMIZATION.md` |
| Recherche (34k+ Woerter) | `docs/research/README.md` |

Beitraege: siehe `CONTRIBUTING.md` (falls vorhanden) oder ein
Issue im Repository oeffnen.
