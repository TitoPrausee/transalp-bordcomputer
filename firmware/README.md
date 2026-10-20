# TRANSALP Bordcomputer — Firmware

ESP32-S3 firmware for the TRANSALP Bordcomputer, a motorcycle
instrument computer for the Honda Transalp PD06. Built on ESP-IDF
with FreeRTOS SMP, targeting the ESP32-S3-WROOM-1 (N16R8: 16 MB
Flash, 8 MB Octal PSRAM).

## Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| ESP-IDF | v5.2+ (developed against v5.4) | Build, flash, monitor |
| Python | 3.10+ | ESP-IDF toolchain scripts |
| CMake | 3.16+ | Build-system generator (bundled with ESP-IDF) |
| Ninja | any | Build backend (bundled with ESP-IDF) |
| GCC | any modern | Host unit tests only |

Install ESP-IDF (one-time):

```bash
mkdir -p ~/esp && cd ~/esp
git clone -b v5.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
```

Activate the environment in every new terminal:

```bash
. ~/esp/esp-idf/export.sh
```

Verify with `idf.py --version`.

## Build

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
```

Optional — adjust board configuration interactively:

```bash
idf.py menuconfig
```

Build artifacts land in `build/`:

| File | Description |
|------|-------------|
| `transalp_bordcomputer.bin` | Main firmware image (signed) |
| `bootloader.bin` | Bootloader |
| `partition_table/partition-table.bin` | Partition table |

First build takes 3–5 minutes; incremental builds run in under 30 seconds
via Ninja. Expected binary size: ~1.2–1.8 MB.

## Flash

Connect the ESP32-S3 via USB and flash:

```bash
# macOS:  /dev/cu.SLAB_USBtoUART
# Linux:  /dev/ttyUSB0
idf.py -p /dev/cu.SLAB_USBtoUART flash monitor
```

`Ctrl-]` exits the serial monitor. If the device has stale NVS after a
config change, erase flash first:

```bash
idf.py -p /dev/cu.SLAB_USBtoUART erase-flash
idf.py -p /dev/cu.SLAB_USBtoUART flash monitor
```

## Project Layout

```
firmware/
├── CMakeLists.txt          Root CMake (project + EXTRA_COMPONENT_DIRS)
├── sdkconfig.defaults      Build defaults (PSRAM, NimBLE, FAT, OTA, Secure Boot)
├── partitions.csv           OTA-capable custom partition table
├── secure_boot/             Secure Boot V2 signing key + public key digest
├── scripts/                 Build helpers (gen_ota_key_header.py)
├── main/                    app_main(), event loop, board_config.h
├── components/              11 firmware components (see below)
└── test/                    Unity unit tests + integration tests
```

## Components

| Component | Purpose |
|-----------|---------|
| `ble` | NimBLE GATT server — BLE communication with companion app (telemetry, OTA transfer) |
| `carplay_bridge` | BLE bridge to an external CarPlay display — pushes telemetry (GATT server) and receives navigation/music metadata (GATT client) |
| `diagnostics` | Diagnostic Trouble Codes (DTC), self-test, serial console |
| `display` | TFT (ILI9488) and E-Paper drivers, rendering engine, UI widgets |
| `gps` | NMEA sentence parser, GPS task |
| `hid_controller` | Handlebar controller input — GPIO / I2C / BLE sources, button debounce and mapping, event-bus dispatch |
| `ota` | OTA update system with dual-partition rollback, SHA-256 + RSA-3072 verification (HTTP, BLE, SD card transports) |
| `power` | Battery monitoring, power management, sleep state machine |
| `sensors` | IMU, RPM, temperature, fuel level, gear indicator |
| `storage` | SD card (FAT), logger, NVS-backed configuration |
| `utils` | Event bus (FreeRTOS event groups), signal filters |

## Configuration

`sdkconfig.defaults` holds the project defaults that seed `menuconfig`.
Key settings:

| Setting | Value | Notes |
|---------|-------|-------|
| `CONFIG_ESPTOOLPY_FLASHSIZE` | 16 MB | Onboard flash size |
| `CONFIG_SPIRAM_MODE_OCT` | Octal | PSRAM at 80 MHz |
| `CONFIG_FREERTOS_HZ` | 1000 | 1 kHz tick for precise timing |
| `CONFIG_BT_NIMBLE_ENABLED` | y | NimBLE stack (peripheral + central, 2 connections) |
| `CONFIG_FATFS_CODEPAGE_850` | y | SD card filesystem with long-name support |
| `CONFIG_PARTITION_TABLE_CUSTOM` | y | Uses `partitions.csv` (OTA-capable, offset 0x10000) |
| `CONFIG_APP_ROLLBACK_ENABLE` | y | OTA rollback support |
| `CONFIG_SECURE_BOOT_V2_ENABLED` | y | Secure Boot V2 with RSA-3072 signing |
| `CONFIG_SECURE_FLASH_ENC_ENABLED` | y | Flash encryption |
| `CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK` | y | eFuse SECURE_VERSION anti-rollback |

The custom partition table (`partitions.csv`) defines two 3 MB OTA
slots (`ota_0`, `ota_1`), NVS, PHY init, otadata, and a ~7 MB FAT
storage partition. No factory partition — anti-rollback requires
OTA-only app slots.

For build variants (debug / release / production), full Kconfig
options, and partition details, see
[docs/FIRMWARE-GUIDE.md](../docs/FIRMWARE-GUIDE.md).

## Testing

### Host unit tests (Unity + GCC)

Unit tests run on the development machine without ESP32 hardware.
ESP-IDF dependencies are replaced by mocks under `test/mocks/`.

```bash
cd firmware/test

# Clone Unity (if not present)
git clone --depth 1 https://github.com/ThrowTheSwitch/Unity.git /tmp/unity

# CMake build
mkdir build-test-host && cd build-test-host
cmake -DUNITY_DIR=/tmp/unity/src ..
make
ctest --output-on-failure
```

Test modules cover NMEA parsing, event loop, DTC, filters, config,
BLE callbacks, power manager, controller input, CarPlay bridge, and
OTA logic.

### Integration tests

Cross-component integration tests live in `test/integration/` and
verify data flow across module boundaries (70 tests total). See
[test/integration/README.md](test/integration/README.md) for the full
build instructions and coverage matrix.

### Target tests (ESP32-S3)

When built with ESP-IDF (`IDF_PATH` set), the test suite registers as
a test component for on-target execution:

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.SLAB_USBtoUART flash monitor
```

## Further Documentation

| Document | Content |
|----------|---------|
| [docs/FIRMWARE-GUIDE.md](../docs/FIRMWARE-GUIDE.md) | Comprehensive build, flash, debug, OTA, secure boot, and release guide |
| [docs/DEVELOPER-QUICKSTART.md](../docs/DEVELOPER-QUICKSTART.md) | Repo setup, first build (firmware + companion app), testing, debugging |
| [docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md) | System architecture and task model |
| [docs/API-REFERENCE.md](../docs/API-REFERENCE.md) | BLE GATT API (services and characteristics) |
| [docs/PINMAP.md](../docs/PINMAP.md) | ESP32-S3 pin assignments |
| [docs/TROUBLESHOOTING.md](../docs/TROUBLESHOOTING.md) | Common issues and solutions |
