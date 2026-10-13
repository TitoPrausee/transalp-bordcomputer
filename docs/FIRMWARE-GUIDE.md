# Firmware-Engineering Guide

> **Comprehensive Build, Flash, Debug, Test, and Release Guide**
> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1 (N16R8: 16 MB Flash, 8 MB Octal PSRAM)
> Framework: ESP-IDF v5.4 mit FreeRTOS SMP

---

## Inhaltsverzeichnis

1. [Build-System](#1-build-system)
2. [Partition Table](#2-partition-table)
3. [Flash-Layout](#3-flash-layout)
4. [Build-Pipeline & CI/CD](#4-build-pipeline--cicd)
5. [Debugging & Profiling](#5-debugging--profiling)
6. [Coding-Standards](#6-coding-standards)
7. [Testing](#7-testing)
8. [Release-Management](#8-release-management)
9. [OTA-Update System](#9-ota-update-system)
10. [Anhang: Checklisten](#10-anhang-checklisten)

---

## 1. Build-System

### 1.1 ESP-IDF v5.x Setup und Konfiguration

Das Bordcomputer-Projekt basiert auf ESP-IDF v5.4, dem offiziellen
Entwicklungs-Framework fuer die ESP32-Serie. ESP-IDF liefert FreeRTOS
im SMP-Modus (Symmetric Multi-Processing), Treiber fuer alle Peripherie
und die Build-Infrastruktur auf Basis von CMake und Ninja.

#### Installation

ESP-IDF wird als Git-Repository geklont und mit einem Setup-Skript
konfiguriert. Das Setup installiert den Cross-Compiler (xtensa-esp32s3-elf-gcc),
CMake, Ninja und die Python-Abhaengigkeiten.

```bash
# ESP-IDF klonen (v5.4 Stable)
mkdir -p ~/esp && cd ~/esp
git clone -b v5.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3

# Umgebung aktivieren (in jedem neuen Terminal)
. ~/esp/esp-idf/export.sh
```

Nach dem `export.sh` stehen folgende Tools zur Verfuegung:

- `idf.py` — Hauptkommando (build, flash, monitor, menuconfig)
- `xtensa-esp32s3-elf-gcc` — Cross-Compiler
- `ninja` — Build-Backend
- `cmake` — Build-System-Meta-Generator

#### Projekt-Struktur

```
transalp-bordcomputer/
├── firmware/
│   ├── CMakeLists.txt          # Root CMake (project())
│   ├── sdkconfig.defaults      # Default-Konfiguration
│   ├── partitions.csv          # Flash-Partition-Table
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── main.c              # app_main() Entry-Point
│   │   └── board_config.h      # Pin-Definitionen
│   └── components/
│       ├── ble/                # BLE GATT-Server (NimBLE)
│       ├── display/            # Rendering-Engine, ILI9488
│       ├── gps/                # GPS-Parser, NMEA
│       ├── ota/                # OTA-Update-System
│       ├── power/               # Power-Management
│       ├── sensors/            # IMU, BMP280, ADC, PCNT
│       ├── storage/            # SD-Karte, Logger, Config
│       └── utils/              # Event-Bus, Filter
├── docs/
├── companion-app/
└── .github/
    └── workflows/
```

#### Konfiguration mit menuconfig

ESP-IDF verwendet Kconfig fuer die Build-Konfiguration. Die Datei
`sdkconfig.defaults` enthaelt die projekt-spezifischen Defaults, die
bei `idf.py menuconfig` als Startwerte dienen.

```bash
cd firmware
idf.py menuconfig
```

Wichtige Konfigurationsbereiche:

- **Component config → ESP32S3-Specific** — CPU-Frequenz (240 MHz),
  Flash-Size (16 MB), PSRAM-Mode (Octal, 80 MHz)
- **Component config → FreeRTOS** — Tick-Rate (1000 Hz), SMP-Konfiguration
- **Component config → Bluetooth → NimBLE** — BLE-Stack-Konfiguration
- **Component config → FAT Filesystem** — SD-Karten-Support
- **Component config → mbedTLS** — RSA und SHA fuer OTA-Verifikation
- **Security features** — Secure Boot V2, Flash Encryption

### 1.2 CMakeLists.txt Struktur fuer alle Komponenten

Jede Komponente hat ein eigenes `CMakeLists.txt`, das mit
`idf_component_register()` bei ESP-IDF registriert wird.

#### Root CMakeLists.txt

```cmake
# firmware/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(transalp_bordcomputer)
```

Die Variable `EXTRA_COMPONENT_DIRS` erweitert den Standard-Suchpfad
fuer Komponenten. Standardmaessig sucht ESP-IDF in `components/` und
`managed_components/`. Durch die Setzung werden unsere eigenen
Komponenten im Verzeichnis `firmware/components/` gefunden.

#### Komponenten CMakeLists.txt (Beispiel OTA)

```cmake
# firmware/components/ota/CMakeLists.txt
idf_component_register(
    SRCS "ota_update.c" "ota_secure_boot.c"
    INCLUDE_DIRS "include"
    REQUIRES utils
)
```

Felder:

- `SRCS` — Quelldateien der Komponente
- `INCLUDE_DIRS` — Verzeichnisse mit oeffentlichen Headern
- `REQUIRES` — Abhaengigkeiten zu anderen Komponenten (fuer
  Include-Pfade und Link-Reihenfolge)
- `PRIV_REQUIRES` — Private Abhaengigkeiten (nur fuer Implementierung,
  nicht fuer Header sichtbar)

#### Main CMakeLists.txt

```cmake
# firmware/main/CMakeLists.txt
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        gps
        display
        sensors
        logger
        ble
        power
        ota
)
```

Die `REQUIRES` in `main/` deklarieren, welche Komponenten das
Hauptprogramm direkt verwendet. Die Komponenten selbst deklarieren
ihre Abhaengigkeiten untereinander, sodass der Build-Graph
topologisch sortiert wird.

### 1.3 Kconfig-Optionen

Neben den ESP-IDF-internen Kconfig-Optionen koennen Komponenten eigene
Optionen definieren. Dies ermoeglicht Board-Varianten und
Feature-Toggles ohne Code-Aenderungen.

#### Kconfig fuer Board-Varianten

```kconfig
# firmware/components/Kconfig
menu "Transalp Bordcomputer"

config BOARD_VARIANT
    string "Board-Variante"
    default "pd06"
    help
        PD06 = Honda XL600V Transalp (1986-1999)

config BC_DISPLAY_TYPE
    choice
        prompt "Display-Typ"
        default BC_DISPLAY_ILI9488
        help
            Auswahl des Hauptdisplays.

    config BC_DISPLAY_ILI9488
        bool "ILI9488 (3.5\" TFT, 480x320)"
    config BC_DISPLAY_ST7789
        bool "ST7789 (1.3\" TFT, 240x240)"
    config BC_DISPLAY_EPAPER
        bool "E-Paper (2.9\" b/w, 296x128)"
    endchoice

config BC_FEATURE_OTA
    bool "OTA-Update-System aktivieren"
    default y
    help
        Aktiviert das OTA-Update-Subsystem.

config BC_FEATURE_BLE
    bool "BLE-Kommunikation aktivieren"
    default y

config BC_FEATURE_SD_LOGGING
    bool "SD-Karten-Logging aktivieren"
    default y

config BC_LOG_LEVEL
    int "Log-Level"
    default 3
    range 0 5
    help
        0=NONE, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE

endmenu
```

#### Zugriff im Code

```c
#if CONFIG_BC_DISPLAY_ILI9488
    #define DISPLAY_H_RES  480
    #define DISPLAY_V_RES  320
#elif CONFIG_BC_DISPLAY_ST7789
    #define DISPLAY_H_RES  240
    #define DISPLAY_V_RES  240
#elif CONFIG_BC_DISPLAY_EPAPER
    #define DISPLAY_H_RES  296
    #define DISPLAY_V_RES  128
#endif
```

### 1.4 Build-Varianten

Das Projekt unterstuetzt drei Build-Varianten mit unterschiedlichen
Optimierungs-Levels und Konfigurationen.

#### Debug

- Optimierung: `-O0` (keine Optimierung)
- Assertions: aktiviert
- Stack-Canaries: aktiviert
- Log-Level: VERBOSE (5)
- Heap-Tracing: aktiviert
- Task-Watchdog: 5 Sekunden Timeout

```bash
cd firmware
idf.py -B build_debug -DSDKCONFIG=debug.sdkconfig build
```

`debug.sdkconfig` enthaelt:

```
CONFIG_COMPILER_OPTIMIZATION_NONE=y
CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_ENABLE=y
CONFIG_LOG_DEFAULT_LEVEL_VERBOSE=y
CONFIG_HEAP_TRACING=y
CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=5
```

#### Release

- Optimierung: `-O2`
- Assertions: deaktiviert
- Log-Level: INFO (3)
- Task-Watchdog: 10 Sekunden

```
CONFIG_COMPILER_OPTIMIZATION_SIZE=y
CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE=y
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10
```

#### Production

- Optimierung: `-Os` (Size)
- Assertions: deaktiviert
- Log-Level: WARNING (2)
- Secure Boot V2: aktiviert
- Flash Encryption: aktiviert
- Stack-Canaries: aktiviert

```
CONFIG_COMPILER_OPTIMIZATION_SIZE=y
CONFIG_LOG_DEFAULT_LEVEL_WARN=y
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_FLASH_ENC_ENABLED=y
CONFIG_COMPILER_STACK_CHECK_MODE_STRONG=y
```

### 1.5 Cross-Compilation auf Mac und Linux

ESP-IDF verwendet denselben Cross-Compiler auf Mac (Intel und Apple Silicon)
und Linux. Der Compiler ist Teil der ESP-IDF-Installation und wird durch
das `export.sh`-Skript in den PATH aufgenommen.

```bash
# Compiler pruefen
xtensa-esp32s3-elf-gcc --version
# xtensa-esp32s3-elf-gcc (crosstool-NG esp-13.2.0) 13.2.0

# Build
cd firmware
idf.py build

# Flash + Monitor
idf.py -p /dev/cu.usbserial-XXXX flash monitor

# Auf Linux: /dev/ttyUSB0 statt /dev/cu.usbserial-XXXX
```

#### Apple Silicon

Auf Apple Silicon (M1/M2/M3) laeuft der Cross-Compiler nativ. Es ist
kein Rosetta erforderlich. Die ESP-IDF-Toolchain wird als ARM64-Binary
ausgeliefert.

#### CMake-Preset-Datei

Fuer reproduzierbare Builds kann eine `CMakePresets.json` angelegt werden:

```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "debug",
            "cacheVariables": {
                "SDKCONFIG": "debug.sdkconfig"
            }
        },
        {
            "name": "release",
            "cacheVariables": {
                "SDKCONFIG": "release.sdkconfig"
            }
        },
        {
            "name": "production",
            "cacheVariables": {
                "SDKCONFIG": "production.sdkconfig"
            }
        }
    ]
}
```

---

## 2. Partition Table

### 2.1 Custom Partition Table

Das Bordcomputer-Projekt verwendet eine benutzerdefinierte Partition
Table, die OTA-Updates, NVS-Konfiguration und SPIFFS/FAT-Storage
unterstuetzt.

```
# ESP32-S3 OTA Partition Table
# Name,   Type, SubType,  Offset,   Size,    Flags
nvs,      data, nvs,     0x9000,   0x6000,
phy_init, data, phy,     0xf000,   0x1000,
factory,  app,  factory, 0x10000,  0x300000,
ota_0,    app,  ota_0,   0x310000, 0x300000,
ota_1,    app,  ota_1,   0x610000, 0x300000,
otadata,  data, ota,     0x910000, 0x2000,
storage,  data, fat,     0x912000, 0x6EE000,
```

#### Partition-Arten

| Partition | Type | SubType | Beschreibung |
|-----------|------|---------|--------------|
| nvs | data | nvs | Non-Volatile Storage: Konfiguration, Kalibrierung |
| phy_init | data | phy | PHY-Kalibrierungsdaten (RF) |
| factory | app | factory | Werkseinstellung (Backup, 3 MB) |
| ota_0 | app | ota_0 | OTA-Slot A (3 MB) |
| ota_1 | app | ota_1 | OTA-Slot B (3 MB) |
| otadata | data | ota | OTA-Selection-Data (welche Partition booten) |
| storage | data | fat | FAT-Dateisystem fuer SD-Emulation / Assets |

### 2.2 Groessenberechnung

Der ESP32-S3-WROOM-1 hat 16 MB Flash. Die Aufteilung:

```
0x0000 - 0x8FFF   Bootloader + Partition Table  (~36 KB)
0x9000 - 0xEFFF   NVS                            (24 KB)
0xF000 - 0xFFFF   PHY Init                       (4 KB)
0x10000 - 0x30FFFF  factory app                  (3 MB)
0x310000 - 0x60FFFF  ota_0 app                  (3 MB)
0x610000 - 0x90FFFF  ota_1 app                  (3 MB)
0x910000 - 0x911FFF  otadata                    (8 KB)
0x912000 - 0xFFFFFF   storage (FAT)             (~7 MB)
```

Die OTA-Partitionen sind jeweils 3 MB gross — ausreichend fuer die
Firmware-Binaerdatei mit Komponenten, Assets und Ressourcen. Die
`factory`-Partition dient als notfall-Backup und kann per Hardware-
Pin (GPIO0 bei Boot) ausgewaehlt werden.

#### Berechnung der App-Partition-Groesse

Die App-Partition muss gross genug sein fuer:

1. **Firmware-Binaerdatei**: Typischerweise 1.5-2 MB (ESP-IDF +
   FreeRTOS + Komponenten + App-Code)
2. **Reserve fuer zukuenftige Features**: ~1 MB
3. **Alignment**: OTA-Partitionen muessen an 0x10000-Grenzen liegen

3 MB pro Partition ist ein bewaehrter Wert fuer ESP32-S3-Projekte
mit WiFi/BLE und mehreren Treibern.

### 2.3 NVS und SPIFFS

#### NVS (Non-Volatile Storage)

NVS speichert Schluessel-Wert-Paare im Flash, verschluesselt und mit
Leveldb-aehnlichem Wear-Leveling. Verwendungszwecke im Bordcomputer:

- Geraete-Konfiguration (`device_config_t` Struktur)
- Kalibrierungsdaten (IMU-Offsets, Fuel-Tabelle)
- OTA-Resume-State (Offset fuer Range-Request)
- WLAN/BLE-Bonds

```c
#include "nvs_flash.h"
#include "nvs.h"

// Schreiben
nvs_handle_t h;
nvs_open("config", NVS_READWRITE, &h);
nvs_set_u32(h, "log_interval", 5);
nvs_set_blob(h, "calibration", &cal_data, sizeof(cal_data));
nvs_commit(h);
nvs_close(h);

// Lesen
nvs_open("config", NVS_READONLY, &h);
uint32_t interval = 5;
nvs_get_u32(h, "log_interval", &interval);
size_t len = sizeof(cal_data);
nvs_get_blob(h, "calibration", &cal_data, &len);
nvs_close(h);
```

#### FAT-Dateisystem

Die `storage`-Partition wird als FAT-Dateisystem formatiert und ist
ueber die Standard-C-Datei-API zugaenglich. Sie wird fuer:

- Log-Dateien (CSV, GPX)
- Firmware-Binaries (OTA-From-SD)
- Exportierte Konfigurationen

Bei physischer SD-Karte wird der gleiche Code-Pfad verwendet, da
ESP-IDF VFS das FAT-Dateisystem transparent ueber SPI oder
 Wear-Leveling im Flash bereitstellt.

---

## 3. Flash-Layout

### 3.1 Bootloader-Konfiguration

Der ESP32-S3-Bootloader liegt im Flash-Bereich 0x1000-0x8FFF und wird
bei jedem Reset als erstes ausgefuehrt. Er ist fuer folgende Aufgaben
verantwortlich:

1. **Hardware-Initialisierung**: PLL, Flash, PSRAM
2. **Partition-Table-Laden**: Einlesen der `partitions.csv`
3. **App-Auswahl**: Welche OTA-Partition booten (otadata)
4. **Image-Verifikation**: Bei aktivem Secure Boot
5. **Decryption**: Bei aktivierter Flash-Encryption

Konfiguration ueber `menuconfig`:

- `CONFIG_BOOTLOADER_LOG_LEVEL` — Bootloader-Log-Level
- `CONFIG_BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP` — Schneller Wake-Up
- `CONFIG_BOOTLOADER_WDT_ENABLE` — Boot-Watchdog

### 3.2 Secure Boot V2 (RSA-3072)

Secure Boot V2 stellt sicher, dass nur signierte Firmware-Images
ausgefuehrt werden. Bei der ersten Inbetriebnahme wird ein RSA-3072-
Schluesselpaar generiert und der Public Key in den eFuse geschrieben.
Der Bootloader verifiziert bei jedem Boot die Signatur der App.

#### Aktivierung

Secure Boot V2 wird in drei Schritten aktiviert:

1. **Schluessel generieren**:

```bash
# Secure Boot Key generieren
espsecure.py generate_signing_key --version 2 --scheme rsa3072 \
    secure_boot_signing_key.pem
```

2. **Konfiguration aktivieren**:

```
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y
CONFIG_SECURE_BOOT_SIGNING_KEY="secure_boot_signing_key.pem"
```

3. **Erster Boot (Provisioning)**: Beim ersten Boot mit Secure Boot
   wird der Public Key in den eFuse "Secure Boot Key" gebrannt. Dieser
   Vorgang ist irreversibel — einmal gebrannt, kann der Key nicht
   mehr geaendert werden.

#### Verifikations-Fluss

```
  [Boot]
    │
    v
  Lese App-Partition
    │
    v
  Lese Signatur aus App-Header
    │
    v
  Berechne SHA-256 der App
    │
    v
  Verifiziere RSA-3072 Signatur mit eFuse Public Key
    │
    +-- gueltig -->  Starte App
    |
    +-- ungueltig -->  Boot-Fehler, versuche Rollback
```

#### Firmware-Signing im Build

Wenn `CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y` gesetzt ist, signiert
der Build-Prozess die Firmware-Binaerdatei automatisch mit dem in der
Konfiguration angegebenen Private Key:

```bash
idf.py build
# Erzeugt: build/transalp_bordcomputer.bin (signiert)
```

### 3.3 Flash-Verschluesselung

Flash-Verschluesslung schuetzt den Flash-Inhalt vor physischem
Auslesen. Bei Aktivierung wird ein AES-256-Key in den eFuse gebrannt
und der Flash-Inhalt verschluesselt gespeichert. Der Bootloader
entschluesselt beim Boot transparent.

```
CONFIG_SECURE_FLASH_ENC_ENABLED=y
```

Flash-Encryption ist ebenfalls irreversibel — einmal aktiviert, kann
sie nicht deaktiviert werden. Der Flash-Inhalt ist ohne den eFuse-Key
nicht lesbar.

#### Modi

- **Development** (Flash-Encryption laeuft, aber im Development-Modus):
  Der Key kann ausgelesen werden fuer Debugging
- **Release** (Production): Der Key ist nicht auslesbar, der eFuse-
  Block ist gegen Lesen geschuetzt

### 3.4 eFuse-Nutzung

eFuses sind einmalig programmierbare Bits im Chip. Sie koennen nur
von 0 nach 1 geschrieben werden (burn) und sind irreversibel.

| eFuse | Beschreibung | Verwendung |
|-------|-------------|-----------|
| SECURE_BOOT_KEY | RSA-3072 Public Key | Secure Boot V2 |
| SECURE_VERSION | Anti-Rollback Counter | OTA-Versions-Schutz |
| SPI_PAD_CONFIG | SPI-Pin-Konfiguration | Hardware-Config |
| DISABLE_DL_ENCRYPT | UART-Download-Encryption | Debug-Schutz |
| DIS_USB_JTAG | JTAG ueber USB deaktiviert | Debug-Schutz |

#### Anti-Rollback-Counter

Der `SECURE_VERSION` eFuse ist ein Counter, der bei jedem OTA-Update
erhoeht wird. Der Bootloader vergleicht die Firmware-Version mit dem
eFuse-Counter und weigert sich, eine aeltere Firmware zu booten.

```c
// In der OTA-Komponente:
esp_err_t ota_secure_check_anti_rollback(uint32_t new_version) {
    // Lese eFuse Counter
    uint32_t efuse_ver = 0;
    esp_efuse_read_field_blob(ESP_EFUSE_SECURE_VERSION,
                               &efuse_ver, 32);
    if (new_version < efuse_ver) {
        return ESP_ERR_INVALID_VERSION;  // Rollback verweigert
    }
    return ESP_OK;
}
```

Da eFuse-Bits nur 0->1 gesetzt werden koennen, wird der Counter als
"Anzahl gesetzter Bits" interpretiert. Jedes Update brennt ein
weiteres Bit.

---

## 4. Build-Pipeline & CI/CD

### 4.1 GitHub Actions Workflow

Das Projekt verwendet GitHub Actions fuer automatische Builds bei
jedem Push und Pull Request. Der Workflow kompiliert die Firmware,
fuehrt statische Analyse durch und erstellt Release-Binaries.

```yaml
# .github/workflows/firmware-ci.yml
name: Firmware CI

on:
  push:
    paths:
      - 'firmware/**'
    branches: [main, 'firmware/**']
  pull_request:
    paths:
      - 'firmware/**'

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        variant: [debug, release]

    steps:
      - uses: actions/checkout@v4

      - name: Setup ESP-IDF
        uses: espressif/esp-idf-ci-action@v1
        with:
          esp_idf_version: v5.4
          target: esp32s3

      - name: Configure
        working-directory: firmware
        run: |
          if [ "${{ matrix.variant }}" = "debug" ]; then
            cp sdkconfig.debug sdkconfig
          else
            cp sdkconfig.release sdkconfig
          fi

      - name: Build
        working-directory: firmware
        run: idf.py build

      - name: Static Analysis
        working-directory: firmware
        run: |
          idf.py cppcheck
          idf.py clang-tidy

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: firmware-${{ matrix.variant }}
          path: |
            firmware/build/transalp_bordcomputer.bin
            firmware/build/bootloader/bootloader.bin
            firmware/build/partition_table/partition-table.bin
```

### 4.2 Pre-Commit-Hooks

Pre-Commit-Hooks stellen sicher, dass Code-Qualitaetspruefungen vor
jedem Commit ausgefuehrt werden. Konfiguration ueber `.pre-commit-config.yaml`.

```yaml
# .pre-commit-config.yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.6.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-merge-conflict
      - id: check-added-large-files
        args: ['--maxkb=500']

  - repo: https://github.com/pocc/pre-commit-hooks
    rev: v1.3.5
    hooks:
      - id: clang-format
        args: ['-i', '--style=file']
        files: 'firmware/.*\.(c|h)$'
      - id: cppcheck
        args: ['--enable=all', '--suppress=missingInclude',
               '--inline-suppr', '--language=c']
        files: 'firmware/.*\.c$'
      - id: clang-tidy
        args: ['-p=firmware/build']
        files: 'firmware/.*\.c$'
```

#### clang-format Konfiguration

Die Datei `.clang-format` im Wurzelverzeichnis definiert den
Code-Stil:

```yaml
# .clang-format
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
AlignConsecutiveAssignments: true
AlignConsecutiveDeclarations: true
BreakBeforeBraces: Attach
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: false
SortIncludes: CaseSensitive
```

#### Installation

```bash
# Pre-Commit installieren
pip install pre-commit
cd /Users/tito/transalp-bordcomputer
pre-commit install
pre-commit autoupdate
```

### 4.3 Release-Automatisierung

Releases werden ueber Git-Tags ausgeloest. Ein GitHub Actions Workflow
baut bei einem neuen Tag (`v*`) automatisch die Release-Binaries und
erstellt ein GitHub Release.

```yaml
# .github/workflows/release.yml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  release:
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v4

      - name: Setup ESP-IDF
        uses: espressif/esp-idf-ci-action@v1
        with:
          esp_idf_version: v5.4
          target: esp32s3

      - name: Build Production Firmware
        working-directory: firmware
        run: |
          cp sdkconfig.production sdkconfig
          idf.py build

      - name: Generate Release Notes
        id: notes
        run: |
          # Extrahiere Changelog aus CHANGELOG.md
          awk '/^## /{p=0} /^## \[${GITHUB_REF_NAME#\v}\]/{p=1} p' \
            CHANGELOG.md > release_notes.md

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          body_path: release_notes.md
          files: |
            firmware/build/transalp_bordcomputer.bin
            firmware/build/bootloader/bootloader.bin
            firmware/build/partition_table/partition-table.bin
```

### 4.4 Firmware-Signing im CI

Bei aktiviertem Secure Boot muss die Firmware im CI mit dem Private
Key signiert werden. Der Private Key wird als GitHub Secret gespeichert
und ist nie im Repository sichtbar.

```yaml
      - name: Sign Firmware
        env:
          SECURE_BOOT_KEY: ${{ secrets.SECURE_BOOT_PRIVATE_KEY }}
        working-directory: firmware
        run: |
          echo "$SECURE_BOOT_KEY" > /tmp/signing_key.pem
          # ESP-IDF signiert automatisch wenn
          # CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y
          idf.py build
          rm /tmp/signing_key.pem
```

---

## 5. Debugging & Profiling

### 5.1 JTAG/Serial Debugging Setup

Der ESP32-S3 unterstuetzt JTAG-Debugging ueber USB-JTAG (eingebaut,
kein externer Adapter noetig) oder einen externen JTAG-Adapter ueber
die Standard-JTAG-Pins.

#### USB-JTAG (eingebaut)

Der ESP32-S3 hat einen eingebauten USB-JTAG-Adapter. Verbindung
einfach per USB-Kabel:

```bash
# OpenOCD starten (ESP-IDF liefert die Konfiguration)
idf.py openocd
```

Standard-Konfiguration:

```
# OpenOCD Config fuer ESP32-S3 USB-JTAG
source [find interface/esp_usb_jtag.cfg]
source [find target/esp32s3.cfg]
adapter speed 20000
```

#### GDB verbinden

```bash
# In einem Terminal: OpenOCD starten
idf.py openocd

# In einem zweiten Terminal: GDB starten
idf.py gdb
# oder manuell:
xtensa-esp32s3-elf-gdb build/transalp_bordcomputer.elf
(gdb) target remote :3333
(gdb) load
(gdb) monitor reset init
(gdb) break app_main
(gdb) continue
```

#### Haltepunkte und Watchpoints

```gdb
# Breakpoint
break ota_update_from_url
break main.c:30

# Conditional Breakpoint
break esp_ota_write if err != ESP_OK

# Watchpoint (auf Speicheradresse)
watch s_ota.state
watch s_ota.bytes_written

# Hardware-Watchpoint (bis zu 2 auf ESP32-S3)
hwatch s_ota.progress
```

### 5.2 ESP-IDF Monitor

Der ESP-IDF Monitor ist ein Serial-Monitor mit erweiterten Features:

- **Backtrace-Decoding**: Stack-Traces werden automatisch in
  Funktionsnamen aufgeloest
- **Core-Dump-Decoding**: Bei einem Crash wird der Core-Dump
  automatisch dekodiert
- **Address-to-Source**: Speicheradressen werden auf Quelldateien
  abgebildet

```bash
idf.py -p /dev/cu.usbserial-XXXX monitor
```

Im Monitor:

- `Ctrl-]` — Exit
- `Ctrl-T Ctrl-H` — Help
- `Ctrl-T Ctrl-B` — Backtrace anzeigen
- `Ctrl-T Ctrl-R` — Reset

#### Panic-Handler-Output

Bei einem Crash gibt der Panic-Handler einen Stack-Trace aus:

```
Guru Meditation Error: Core  0 panic'ed (Stack canary watchpoint triggered)
Backtrace: 0x42012a3a:0x3fceb150 0x42012c4e:0x3fceb170 0x42012f2c:0x3fceb190
```

ESP-IDF Monitor dekodiert dies automatisch:

```
0x42012a3a: ble_task at firmware/components/ble/ble.c:42
0x42012c4e: vTaskNode at /path/to/freertos/tasks.c:543
0x42012f2c: vPortStartFirstTask at portasm.S:68
```

### 5.3 Heap-Debugging

#### Heap-Tracing

ESP-IDF bietet detailliertes Heap-Tracing, um Speicherlecks zu finden.

```c
#include "esp_heap_trace.h"

#define NUM_RECORDS 100
static heap_trace_record_t trace_record[NUM_RECORDS];

void heap_trace_start(void) {
    heap_trace_init_standalone(trace_record, NUM_RECORDS);
    heap_trace_start(HEAP_TRACE_LEAKS);
}

void heap_trace_dump(void) {
    heap_trace_dump();
    // Zeigt alle nicht freigegebenen malloc-Calls mit Backtrace
}
```

Konfiguration in `menuconfig`:

```
CONFIG_HEAP_TRACING=y
CONFIG_HEAP_TRACE_HAS_HEAP_DUMP=y
```

#### Heap-Statistiken

```c
#include "esp_heap_caps.h"

void print_heap_info(void) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);

    printf("Total free: %u\n", info.total_free_bytes);
    printf("Total alloc: %u\n", info.total_allocated_bytes);
    printf("Min free ever: %u\n", info.minimum_free_bytes);
    printf("Largest free block: %u\n",
           heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
}
```

#### Leak-Detection via Heaps-Tool

```bash
# Heap-Status ueber JTAG abfragen
idf.py monitor --print-filter="*:I"
# Im Monitor:
# (monitor) heaps
#   Heap 0 (internal, 370KB)
#     Free: 234KB / Allocated: 136KB / Min-Free: 198KB
#   Heap 1 (PSRAM, 8MB)
#     Free: 7.6MB / Allocated: 400KB / Min-Free: 7.5MB
```

### 5.4 Stack-Overflow-Detection

ESP-IDF verwendet drei Mechanismen zur Stack-Overflow-Detection:

#### Stack-Canaries

Jeder Task-Stack wird mit einem Canary-Wert am Ende gefuellt. Bei
einem Overflow wird der Canary ueberschrieben und FreeRTOS erkennt
den Ueberlauf.

```c
// In sdkconfig:
// CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK=y
```

#### Stack-High-Water-Mark

```c
#include "freertos/task.h"

// Minimaler verbleibender Stack-Speicher (in Bytes)
UBaseType_t hw = uxTaskGetStackHighWaterMark(NULL);
ESP_LOGI(TAG, "Stack HWM: %lu bytes", (unsigned long)hw * sizeof(StackType_t));
```

Empfehlung: Stack-Size so waehlen, dass der High-Water-Mark > 25%
der Stack-Size bleibt.

#### MPU-Schutz (Memory Protection Unit)

Der ESP32-S3 hat eine MPU, die konfiguriert werden kann:

```
CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM=y
```

### 5.5 Performance-Profiling

#### esp_timer

`esp_timer` liefert Mikrosekunden-Genauigkeit fuer Performance-Messungen:

```c
#include "esp_timer.h"

int64_t t0 = esp_timer_get_time();

// ... Code messen ...

int64_t t1 = esp_timer_get_time();
ESP_LOGI(TAG, "Duration: %lld us", t1 - t0);
```

#### Task-Statistiken

```c
#include "freertos/task.h"

void print_task_stats(void) {
    TaskStatus_t *tasks;
    UBaseType_t count = uxTaskGetNumberOfTasks();
    tasks = malloc(count * sizeof(TaskStatus_t));
    count = uxTaskGetSystemState(tasks, count, NULL);

    printf("Task            Core  Prio  Stack  State\n");
    for (int i = 0; i < count; i++) {
        printf("%-15s %4d  %4d  %5lu  %s\n",
               tasks[i].pcTaskName,
               tasks[i].xCoreID,
               tasks[i].uxCurrentPriority,
               (unsigned long)tasks[i].usStackHighWaterMark,
               tasks[i].eCurrentState == eRunning ? "RUN" :
               tasks[i].eCurrentState == eBlocked ? "BLK" : "RDY");
    }
    free(tasks);
}
```

#### CPU-Auslastung

ESP-IDF kann die CPU-Auslastung pro Task anzeigen:

```bash
# Im Monitor:
# (monitor) tasks
#   Task            Core  Prio  State  CPU%
#   IDLE0           0      0     RDY    45%
#   IDLE1           1      0     RDY    52%
#   sensor_task     1      4     BLK     8%
#   display_task    0      2     BLK     3%
#   ble_task        0      3     BLK     1%
```

### 5.6 GDB-Stub fuer Post-Mortem-Debugging

Bei einem Crash kann ein Core-Dump in Flash gespeichert werden,
der spaeter mit GDB analysiert werden kann.

#### Core-Dump-Konfiguration

```
CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y
CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=y
CONFIG_ESP_COREDUMP_CHECKSUM_CRC32=y
```

Der Core-Dump wird in der Partition `coredump` gespeichert (muss in
der Partition Table hinzugefuegt werden).

#### Core-Dump auslesen

```bash
# Core-Dump aus Flash lesen
espcoredump.py info_core -t esp32s3 -p /dev/cu.usbserial-XXXX \
    --core build/transalp_bordcomputer.elf

# Core-Dump in ELF konvertieren und mit GDB analysieren
espcoredump.py dbg_corefile -t esp32s3 -p /dev/cu.usbserial-XXXX \
    build/transalp_bordcomputer.elf

xtensa-esp32s3-elf-gdb build/transalp_bordcomputer.elf core.elf
(gdb) bt
(gdb) info registers
(gdb) x/16xw $sp
```

---

## 6. Coding-Standards

### 6.1 MISRA-C Kompatibilitaet

Das Projekt orientiert sich an MISRA-C:2012, wo mit dem ESP-IDF-Framework
vereinbar. MISRA-C (Motor Industry Software Reliability Association) ist
ein Standard fuer sicherheitskritische C-Software, der besonders in der
Automobil- und Medizintechnik eingesetzt wird.

#### Angewandte MISRA-Regeln

| Regel | Beschreibung | Status |
|-------|-------------|--------|
| Dir 4.1 | Keine Verwendung von Run-Time-Assertions in Release | Eingehalten |
| Rule 8.7 | Objekte nur in eigener Translation Unit definieren | Eingehalten |
| Rule 8.9 | Variablen im kleinstmoeglichen Scope definieren | Eingehalten |
| Rule 17.3 | Keine rekursive Funktionsdefinition | Eingehalten |
| Rule 17.4 | Keine mehrfachen exits aus einer Funktion | Bestrebt |
| Rule 21.6 | Keine stdio-I/O in sicherheitskritischem Code | Eingehalten |
| Rule 21.13 | Keine Strings fuer numerische Werte | Eingehalten |

#### Abweichungen

- ESP-IDF verwendet teilweise stdio.h-Funktionen (z.B. `snprintf`)
- Die FreeRTOS-API ist nicht MISRA-konform (folgt POSIX-aehnlichen Patterns)
- mbedTLS-Integration erfordert dynamische Speicherallokation

### 6.2 Header-Guards und Include-Reihenfolge

#### Header-Guards

Alle Header-Dateien verwenden Include-Guards im `_FILE_H_`-Format:

```c
#ifndef _OTA_UPDATE_H_
#define _OTA_UPDATE_H_

// ... Inhalt ...

#endif /* _OTA_UPDATE_H_ */
```

#### Include-Reihenfolge

Die Include-Reihenfolge folgt dem Google C++ Style Guide, adaptiert
fuer C:

1. **System-Headers**: `<stdint.h>`, `<stdbool.h>`, `<string.h>`
2. **ESP-IDF-Headers**: `freertos/FreeRTOS.h`, `esp_log.h`, `esp_ota_ops.h`
3. **Projekt-Header**: `ota_update.h`, `event_bus.h`
4. **Eigene Header**: immer als letztes in der entsprechenden `.c`-Datei

```c
// firmware/components/ota/ota_update.c
#include <string.h>              // 1. System
#include <stdlib.h>

#include "freertos/FreeRTOS.h"   // 2. ESP-IDF
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "event_bus.h"           // 3. Projekt
#include "ota_update.h"          // 4. Eigener Header
```

### 6.3 Naming-Conventions

| Typ | Konvention | Beispiel |
|-----|-----------|----------|
| Funktionen | snake_case | `ota_update_from_url()` |
| Variablen | snake_case | `bytes_written` |
| Typen (typedef) | snake_case_t | `ota_state_t` |
| Makros | UPPER_CASE | `OTA_CHUNK_SIZE` |
| Konstanten | UPPER_CASE | `OTA_MAX_IMAGE_SIZE` |
| Enums | snake_case, UPPER_CASE values | `OTA_STATE_IDLE` |
| Dateinamen | snake_case.c / .h | `ota_update.c` |
| Globals | static + g_ Praefix | `g_event_group` |
| Strukturen | snake_case_t | `ota_chunk_t` |
| Header-Guards | _FILE_H_ | `_OTA_UPDATE_H_` |

#### Zusatzregeln

- Oeffentliche Funktionen haben das Praefix des Moduls: `ota_`, `ble_`, `gps_`
- Private (static) Funktionen haben keinen Praefix
- Callback-Typen enden auf `_cb_t`: `ota_progress_cb_t`
- Boolean-Variablen: `is_`, `has_`, `can_` Praefix: `is_connected`, `has_fix`

### 6.4 Static-Code-Analysis Tools

#### cppcheck

```bash
cppcheck --enable=all --suppress=missingInclude \
         --inline-suppr --language=c \
         --project=firmware/build/compile_commands.json \
         --xml --xml-version=2 firmware/ 2> cppcheck.xml
```

Im Code koennen False-Positives mit Kommentaren unterdrueckt werden:

```c
// cppcheck-suppress misra-c2012-8.7
static void internal_helper(void) { ... }
```

#### clang-tidy

```bash
clang-tidy -p=firmware/build firmware/components/ota/ota_update.c \
    --checks='-*,bugprone-*,performance-*,readability-*'
```

#### Analyse-Ergebnisse interpretieren

| Tool | Schwerpunkt | Typische Funde |
|------|------------|---------------|
| cppcheck | Logikfehler, Memory-Leaks | Null-Pointer, Buffer-Overflow |
| clang-tidy | Style, Performance | Ineffiziente String-Operationen |
| Misra-Checker | Compliance | Regelverletzungen |

### 6.5 Code-Review-Checkliste

Vor jedem Pull Request muss folgende Checkliste abgearbeitet werden:

- [ ] Code kompiliert ohne Warnungen (`-Wall -Wextra -Werror`)
- [ ] cppcheck zeigt keine neuen Fehler
- [ ] clang-tidy zeigt keine neuen Warnings
- [ ] clang-format wurde ausgefuehrt
- [ ] Keine hardcoded Pins/Magic Numbers (alle ueber `board_config.h`)
- [ ] Keine Markennamen in Code oder Kommentaren
- [ ] Funktionen haben Header-Kommentare (Doxygen-Stil)
- [ ] Oeffentliche API-Aenderungen sind dokumentiert
- [ ] Keine dynamischen Allocs in ISR-Context
- [ ] Keine `printf` in kritischen Tasks (nur `ESP_LOGx`)
- [ ] Mutexe fuer Shared State verwendet
- [ ] Stack-Size fuer neue Tasks berechnet
- [ ] Tests fuer neue Funktionen geschrieben

---

## 7. Testing

### 7.1 Unit-Testing mit ESP-IDF pytest Framework

ESP-IDF integriert ein pytest-basiertes Test-Framework, das sowohl
Host-Tests (ohne Hardware) als auch Hardware-Tests unterstuezt.

#### Host-Tests (Component Tests)

Host-Tests laufen auf dem Entwicklungsrechner ohne ESP32-Hardware.
Sie testen reine Logik-Funktionen, die keine Hardware-Abhaengigkeit
haben.

```c
// firmware/components/ota/test/test_ota_crc.c
#include <unity.h>
#include "ota_update.h"

void test_crc16_ccitt_known_values(void) {
    uint8_t data1[] = "123456789";
    TEST_ASSERT_EQUAL_UINT16(0x31C3,
        crc16_ccitt(data1, 9));

    uint8_t empty[] = "";
    TEST_ASSERT_EQUAL_UINT16(0xFFFF,
        crc16_ccitt(empty, 0));
}

void test_ota_state_machine_initial(void) {
    /* Nach ota_init muss State IDLE sein */
    ota_init(NULL, NULL);
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, ota_get_state());
    TEST_ASSERT_EQUAL(OTA_ERR_NONE, ota_get_error());
}

void test_ota_abort_from_idle(void) {
    ota_init(NULL, NULL);
    /* Abbruch aus IDLE sollte ohne Fehler gehen */
    esp_err_t err = ota_abort();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, ota_get_state());
}

void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_crc16_ccitt_known_values);
    RUN_TEST(test_ota_state_machine_initial);
    RUN_TEST(test_ota_abort_from_idle);
    UNITY_END();
}
```

#### Test-Verzeichnis-Struktur

```
firmware/components/ota/
├── CMakeLists.txt
├── ota_update.c
├── ota_secure_boot.c
├── include/
│   └── ota_update.h
└── test/
    ├── CMakeLists.txt
    └── test_ota_crc.c
```

#### CMakeLists fuer Tests

```cmake
# firmware/components/ota/test/CMakeLists.txt
idf_component_register(
    SRCS "test_ota_crc.c"
    INCLUDE_DIRS ".."
    REQUIRES unity ota
)
```

#### Tests ausfuehren

```bash
# Host-Test (QEMU)
idf.py create-component-test ota
idf.py test

# Hardware-Test
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

### 7.2 Hardware-in-the-Loop (HIL) Test-Setup

Fuer Hardware-nahe Tests wird ein HIL-Setup verwendet, bei dem der
ESP32 mit reeller Peripherie verbunden ist, aber durch einen Test-
Controller (ein zweiter ESP32 oder ein PC mit Python) gesteuert wird.

```
  +-----------+        UART/USB         +-----------+
  | Test-PC   |<----------------------->| ESP32-S3  |
  | (pytest)  |                         | (DUT)     |
  +-----------+                         +-----------+
        |                                    |
        | GPIO                               | I2C/SPI
        v                                    v
  +-----------+                         +-----------+
  | Signal-   |                         | Sensoren  |
  | Generator |                         | Display   |
  +-----------+                         +-----------+
```

#### pytest-Konfiguration

```python
# firmware/conftest.py
import pytest
from pytest_embedded import Dut

@pytest.mark.esp32s3
@pytest.mark.hardware
def test_ota_over_ble(dut: Dut) -> None:
    """Test OTA-Update ueber BLE."""
    dut.expect("OTA manager initialized")
    dut.expect("BLE GATT server started")

    # OTA ueber Test-Script ausloesen
    dut.write("ota_test_ble 1234")
    dut.expect("BLE OTA transfer started")
    dut.expect("OTA image verified")
    dut.expect("Rebooting")
```

#### Test-Kategorien

| Kategorie | Umgebung | Beispiele |
|-----------|---------|----------|
| Unit-Test | Host (QEMU) | CRC-Berechnung, State-Machine |
| Component-Test | ESP32 (ohne Peripherie) | NVS-Lesen, OTA-Partition |
| HIL-Test | ESP32 + Peripherie | Sensor-Lesen, Display-Ausgabe |
| Integration-Test | Vollstaendiges System | BLE-OTA-End-to-End |

### 7.3 Mock-Sensoren fuer CI-Tests

Fuer CI-Tests ohne Hardware werden Mock-Sensoren verwendet. ESP-IDF
unterstuetzt dies ueber bedingte Kompilierung:

```c
// firmware/components/sensors/sensor_mock.h
#ifdef CONFIG_BC_MOCK_SENSORS

/* Mock-Implementierungen fuer CI */
float mock_get_temperature(void) {
    return 85.0f;  /* Fixer Wert */
}

float mock_get_imu_roll(void) {
    static float angle = 0;
    angle += 0.5f;
    if (angle > 30.0f) angle = -30.0f;
    return angle;
}

uint16_t mock_get_rpm(void) {
    return 3500;  /* Konstant 3500 U/min */
}
#endif /* CONFIG_BC_MOCK_SENSORS */
```

In `menuconfig`:

```
CONFIG_BC_MOCK_SENSORS=y
```

### 7.4 Coverage-Metriken

Coverage-Messung ist fuer Embedded-Code nur eingeschraenkt moeglich
(dueer Code-Coverage-Counter benoetigen Speicher). Fuer Host-Tests
kann `gcov` verwendet werden:

```
CONFIG_COMPILER_COVERAGE=y
```

```bash
# Nach dem Test-Lauf:
gcov firmware/components/ota/ota_update.c -o build/
lcov --capture --directory build/ --output-file coverage.info
genhtml coverage.info --output-directory coverage_report/
```

Typische Coverage-Ziele:

| Modul | Ziel-Coverage | Bemerkung |
|-------|--------------|-----------|
| utils/ (filter, crc) | > 90% | Reine Logik |
| ota/ (state-machine) | > 80% | Logik + ESP-IDF |
| sensors/ | > 60% | Hardware-abhaengig |
| ble/ | > 50% | Stack-abhaengig |

---

## 8. Release-Management

### 8.1 Semantische Versionierung

Das Projekt verwendet Semantic Versioning (SemVer) im Format
`MAJOR.MINOR.PATCH`:

- **MAJOR**: Inkompatible API-Aenderungen (z.B. BLE-Protokoll-Versionswechsel)
- **MINOR**: Neue Features, abwaertskompatibel (z.B. neuer Sensor-Support)
- **PATCH**: Bug-Fixes, abwaertskompatibel (z.B. Display-Flicker-Fix)

Beispiel: `1.2.3` = Major 1, Minor 2, Patch 3

#### Versions-String im Code

```c
// firmware/main/main.c
#define FIRMWARE_VERSION "1.2.3"

// ESP-IDF ueber esp_app_desc_t automatisch:
// app_desc.version wird aus der Build-Version gesetzt
```

ESP-IDF setzt die Version automatisch aus der `PROJECT_VER`-Variable
in der Root-CMakeLists.txt:

```cmake
# firmware/CMakeLists.txt
set(EXTRA_COMPONENT_DIRS "components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(transalp_bordcomputer)
set(PROJECT_VER "1.2.3")
```

Oder aus einem Git-Tag:

```cmake
# Automatisch aus dem neuesten Git-Tag
execute_process(
    COMMAND git describe --tags --always
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(PROJECT_VER ${GIT_VERSION})
```

### 8.2 Changelog-Format

Das Projekt verwendet "Keep a Changelog" als Format:

```markdown
# Changelog

All notable changes to this project are documented in this file.

## [1.2.3] - 2026-10-13

### Added
- OTA-Update ueber SD-Karte
- Anti-Rollback-Schutz mit eFuse-Version-Counter
- Secure Boot V2 Signaturverifikation

### Changed
- OTA State-Machine um REBOOTING-State erweitert
- HTTP-Download-Timeout auf 30s erhoeht

### Fixed
- BLE-Chunk-Duplicate-Erkennung korrigiert
- Resume-Offset-Berechnung bei Range-Request

### Removed
- Veraltete ota_begin() API durch ota_update_from_ble() ersetzt

## [1.2.2] - 2026-10-08

### Fixed
- Display-Flackern bei Dirty-Rect-Merge behoben
...
```

#### Kategorien

- **Added**: Neue Features
- **Changed**: Aenderungen an bestehenden Features
- **Deprecated**: Bald entfernte Features
- **Removed**: entfernte Features
- **Fixed**: Bug-Fixes
- **Security**: Sicherheitsrelevante Aenderungen

### 8.3 Release-Notes Template

```markdown
# Release v1.2.3 - Transalp Bordcomputer Firmware

**Release-Datum**: 2026-10-13
**Kompatibilitaet**: Hardware Rev 2.x, Companion-App >= 1.2.0

## Highlights

- OTA-Update-System mit drei Transportwegen (HTTP, BLE, SD-Karte)
- Automatischer Rollback bei Boot-Fehler
- Secure Boot V2 mit RSA-3072-Signaturverifikation

## Neue Features

### OTA-Update ueber BLE
Die Companion-App kann nun Firmware-Updates direkt per BLE uebertragen.
Der Transfer erfolgt in 4KB-Chunks mit CRC16-Absicherung. Bei
Verbindungsabbruch kann der Transfer an der letzten Position
fortgesetzt werden.

### OTA-Update ueber SD-Karte
Firmware-Binaries koennen von der SD-Karte geflasht werden. Dies ist
der bevorzugte Weg fuer Updates ohne Smartphone-Verbindung.

## Bug-Fixes

- #42: OTA-Chunk-Offset-Konsistenzpruefung verbessert
- #38: HTTP-Range-Request bei Server ohne Range-Support

## Bekannte Einschraenkungen

- Secure Boot erfordert einmalige Provisionierung beim ersten Boot
- Flash-Encryption ist im Development-Modus noch deaktiviert

## Download

- `transalp_bordcomputer.bin` — Firmware (signiert)
- `bootloader.bin` — Bootloader (Secure-Boot-aktiviert)
- `partition-table.bin` — Partition Table

## Installationsanleitung

1. Kopiere `transalp_bordcomputer.bin` auf eine SD-Karte
2. Lege die SD-Karte in das Bordcomputer ein
3. Oeffne das Diagnose-Menue -> Firmware-Update -> Von SD-Karte
4. Warte auf den Neustart (ca. 30 Sekunden)
5. Pruefe die Version im Info-Menue
```

### 8.4 OTA-Update-Prozess aus User-Sicht

Der OTA-Update-Prozess ist fuer den Anwender transparent gestaltet:

#### Update ueber Companion-App (BLE)

1. Companion-App oeffnen und mit Bordcomputer verbinden
2. Einstellungen -> Firmware-Update
3. App prueft automatisch auf neue Version
4. Bei verfuegbarer Version: "Update starten" antippen
5. Fortschrittsbalken zeigt den Transfer an (0-100%)
6. Bei 100% startet das Bordcomputer automatisch neu
7. Nach dem Neustart: Self-Test laeuft automatisch
8. Bei erfolgreichem Self-Test: Neue Version aktiv
9. Bei fehlgeschlagenem Self-Test: Automatischer Rollback zur vorherigen Version

#### Update ueber SD-Karte

1. Firmware-Binary als `firmware.bin` auf SD-Karte kopieren
2. Bordcomputer ausschalten
3. SD-Karte einsetzen
4. Bordcomputer einschalten, im Diagnose-Menue "Firmware-Update" waehlen
5. "Von SD-Karte" auswaehlen
6. Fortschrittsbalken abwarten
7. Bei Erfolg: automatischer Neustart
8. Version im Info-Menue pruefen

#### Rollback

Wenn die neue Firmware nach dem Update nicht bootet oder den Self-Test
nicht besteht, wird automatisch die vorherige Version wiederhergestellt:

1. Boot der neuen Firmware
2. Self-Test schlaegt fehl
3. Bootloader setzt Boot-Partition zurueck
4. Neustart mit vorheriger Firmware
5. Diagnose-Log zeigt "B0103: OTA-Rollback ausgeloest"

Der Anwender sieht eine kurze Meldung im Display: "Update fehlgeschlagen -
vorherige Version wiederhergestellt".

---

## 9. OTA-Update System

Das OTA-Update-System ist als eigenstaendige Komponente unter
`firmware/components/ota/` implementiert. Es unterstuetzt drei
Transportwege und bietet sichere Verifikation mit automatischem Rollback.

### 9.1 Architektur

```
  +-----------+     +-----------+     +-----------+
  | HTTP-URL  |     | BLE       |     | SD-Karte  |
  | (WiFi)    |     | (Chunks)  |     | (FAT)     |
  +-----+-----+     +-----+-----+     +-----+-----+
        |                 |                 |
        v                 v                 v
  +-----+-----------------+-----------------+-----+
  |              OTA Update Manager              |
  |              (ota_update.c)                   |
  |                                               |
  |  State Machine: IDLE->DOWNLOADING->VERIFYING  |
  |                 ->FLASHING->REBOOTING         |
  |                                               |
  |  Partition: ota_0 / ota_1 (dual)              |
  |  Resume: NVS-gestuetzte Offset-Speicherung    |
  |  Verify: SHA-256 + RSA-3072 (Secure Boot)     |
  +-----------------------+-----------------------+
                          |
                          v
                  +-------+-------+
                  | esp_ota_ops   |
                  | (ESP-IDF)     |
                  +---------------+
```

### 9.2 State-Machine

```
    IDLE
     |
     v
 DOWNLOADING  <----- Resume (NVS offset)
     |
     v
 VERIFYING    (SHA-256 + RSA-Signatur)
     |
     v
 FLASHING     (set_boot_partition)
     |
     v
 REBOOTING    (esp_restart)
     |
     v
  [Boot]
     |
     v
 PENDING_VERIFY (Self-Test)
     |
     +--- OK --->  Mark valid
     |
     +--- FAIL ->  Rollback -> vorherige Partition
```

### 9.3 Transportwege

#### HTTP (Update-From-URL)

Laedt die Firmware-Binaerdatei via HTTP/HTTPS herunter. Unterstuetzt
Range-Requests fuer Resume nach Verbindungsabbruch. Der Resume-Offset
wird periodisch (alle 64 KB) in NVS gespeichert.

#### BLE (Update-From-BLE)

Empfaengt die Firmware in 4 KB-Chunks ueber die OTA-Data-Characteristic
(0x8002). Jeder Chunk hat einen CRC16-CCITT-Pruefsummen-Header. Bei
CRC-Fehler wird der Chunk verworfen und die App muss erneut senden.

#### SD-Karte (Update-From-SD)

Liest die Firmware-Binaerdatei direkt von der FAT-formatierten SD-Karte.
Dies ist der robusteste Weg fuer Updates ohne Netzwerkverbindung.

### 9.4 Sicherheitsfeatures

- **SHA-256-Verifikation**: Jedes Image wird nach dem Flashen gehasht
  und mit dem erwarteten Hash verglichen
- **RSA-3072-Signatur**: Bei aktivem Secure Boot wird die Signatur
  vom Bootloader geprueft; zusaetzlich bietet `ota_secure_boot.c` eine
  Anwendungs-Layer-Pruefung
- **Anti-Rollback**: eFuse-Version-Counter verhindert das Flashen
  aelterer Firmware-Versionen
- **Rollback-Schutz**: Neue Firmware bleibt im PENDING_VERIFY-State,
  bis ein Self-Test sie als gueltig markiert

### 9.5 API-Uebersicht

```c
// Initialisierung (in app_main)
ota_init(progress_callback, user_context);

// Update starten
ota_update_from_url("https://example.com/firmware.bin");
ota_update_from_ble(total_size);
ota_update_from_sd("/sd/firmware.bin");

// BLE-Transfer (wird von BLE-Callback aufgerufen)
ota_ble_write_chunk(&chunk);
ota_ble_commit(expected_hash);

// Status
ota_state_t state = ota_get_state();
uint8_t progress = ota_get_progress();
ota_error_t error = ota_get_error();

// Rollback
ota_rollback();

// Versions-Info
ota_version_info_t info;
ota_get_version_info(&info);
```

---

## 10. Anhang: Checklisten

### 10.1 Pre-Release-Checkliste

- [ ] Versionsnummer in CMakeLists.txt und CHANGELOG.md aktualisiert
- [ ] Alle Unit-Tests gruen
- [ ] cppcheck und clang-tidy ohne neue Warnings
- [ ] clang-format auf allen Dateien ausgefuehrt
- [ ] Release-Build erfolgreich (Production-Konfiguration)
- [ ] Secure-Boot-Signatur verifiziert
- [ ] OTA-Update von SD-Karte getestet
- [ ] OTA-Update ueber BLE getestet
- [ ] Rollback-Test: Neue Firmware mit absichtlichem Self-Test-Fail
- [ ] Flash-Verschluesselung getestet (falls Production)
- [ ] Companion-App-Kompatibilitaet geprueft
- [ ] Release-Notes geschrieben
- [ ] Git-Tag gesetzt: `git tag -a v1.2.3 -m "Release v1.2.3"`
- [ ] GitHub-Release mit Binaries erstellt

### 10.2 OTA-Pre-Flight-Checkliste

- [ ] OTA-Partitionen (ota_0, ota_1) in Partition Table
- [ ] `CONFIG_APP_ROLLBACK_ENABLE=y` in sdkconfig
- [ ] `otadata`-Partition vorhanden
- [ ] `ota_init()` wird in `app_main()` aufgerufen
- [ ] Progress-Callback mit UI verbunden
- [ ] NVS-Namespace "ota_meta" frei
- [ ] Secure-Boot-Konfiguration (falls Production)
- [ ] eFuse-Version-Counter-Block reserviert (falls Anti-Rollback)

### 10.3 Debugging-Quick-Reference

| Symptom | Erster Schritt |
|---------|---------------|
| Boot-Loop | ESP-IDF Monitor, Backtrace lesen |
| OTA schlaegt fehl | `ota_get_error()` im Log suchen |
| BLE verbindet nicht | `btmon`-Trace, Advertising pruefen |
| Display leer | SPI-Pins in `board_config.h` pruefen |
| Heap-Corruption | Heap-Tracing aktivieren, `CONFIG_HEAP_POISONING_COMPREHENSIVE=y` |
| Stack-Overflow | `uxTaskGetStackHighWaterMark()` fuer alle Tasks |
| Watchdog-Timeout | Task-Prioritaeten pruefen, `vTaskDelay` einfuegen |
| Flash-Write-Fail | `esp_ota_write` Return-Code pruefen |

---

*Dieses Dokument wird mit jeder Firmware-Version aktualisiert. Aenderungen
bitte im CHANGELOG.md eintragen.*
