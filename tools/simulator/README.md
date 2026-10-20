# TRANSALP Bordcomputer — Firmware Simulator

PC-basierter Simulator für Tests ohne echte Hardware.

## Übersicht

Der Simulator implementiert eine komplette Mock-Schicht, die alle
Firmware-Schnittstellen des TRANSALP Bordcomputer nachbildet:

| Modul | Firmware-Referenz | Beschreibung |
|-------|-------------------|--------------|
| `mock_sensors.py` | `components/sensors/` | RPM (0-8000), Temp (20-120°C), Fuel (0-17L), Gear (0-6), IMU (Roll/Pitch) |
| `mock_gps.py` | `components/gps/` | NMEA-0183 Generator (GGA, RMC, VTG) aus GPX-Tracks |
| `mock_display.py` | `components/display/` | Headless Framebuffer (TFT 240×320, E-Paper 296×128) |
| `mock_ble.py` | `components/ble/` | GATT-Server mit 8 Services und 32 Characteristics |
| `mock_controller.py` | `components/hid_controller/` | Tastatur → 11 Controller-Buttons mit Debounce |
| `simulator.py` | `main/main.c` | Orchestrator, CLI, CI-Integration |

## Quick Start

```bash
# Tests ausführen
pip install -r requirements.txt
python -m pytest tests/ -v

# Simulator starten (10 Min, Standard-Track)
python3 simulator.py --ci-mode --duration 600 --seed 42

# 24h simulierte Fahrt (beschleunigt, für CI)
python3 simulator.py \
  --track tracks/sample_alps.gpx \
  --duration 86400 \
  --speed-multiplier 1000 \
  --tick-hz 1 \
  --ci-mode \
  --fast \
  --skip-display \
  --seed 42

# Status als JSON (ein Tick)
python3 simulator.py --status-json --seed 42
```

## CLI-Optionen

| Option | Default | Beschreibung |
|--------|---------|--------------|
| `--track FILE` | `tracks/sample_alps.gpx` | GPX-Track für GPS-Simulation |
| `--duration N` | `600` | Simulationsdauer in Sekunden |
| `--speed-multiplier N` | `10` | GPX-Wiedergabe-Beschleunigung |
| `--tick-hz N` | `50` | Sensor-Update-Rate in Hz |
| `--sdl` | off | SDL2-Fenster für lokale Visualisierung |
| `--epaper` | off | E-Paper statt TFT simulieren |
| `--ci-mode` | off | Periodische Status-Ausgabe |
| `--fast` | off | Kein Real-Time-Sleep (für CI) |
| `--skip-display` | off | Display-Updates überspringen (für CI) |
| `--log-nmea` | off | NMEA-Sätze protokollieren |
| `--seed N` | `42` | Random-Seed für reproduzierbare Läufe |
| `--status-json` | off | Status als JSON ausgeben und beenden |

## Mock-Sensor-Schicht

Sensoren werden über konfigurierbare Keyframe-Curves generiert:

```python
from mock_sensors import MockSensorLayer, SensorCurve

# Eigene Kurve definieren
custom_rpm = SensorCurve(
    keyframes=[(0, 1200), (30_000, 6000), (60_000, 1200)],
    noise=50,
    clamp_min=0,
    clamp_max=8000,
)

sensors = MockSensorLayer(
    rpm_curve=custom_rpm,
    seed=42,
)
sensors.start()

for _ in range(500):
    data = sensors.update(20)  # 20ms Tick (50Hz)
    print(f"RPM={data.rpm} Speed={data.speed_kmh:.1f} Gear={data.gear}")
```

### Sensor-Wertebereiche

| Sensor | Bereich | Einheit | Quelle |
|--------|---------|---------|--------|
| RPM | 0–8000 | U/min | `board_config.h: RPM_MAX_VALUE` |
| Coolant Temp | 20–120 | °C | `board_config.h: TEMP_OVERTEMP_C` |
| Fuel | 0–17 | Liter | `board_config.h: TANK_CAPACITY_L` |
| Gear | 0–6 | — | 0=Neutral, berechnet aus RPM |
| Roll/Pitch | ±45/±20 | Grad | MPU6050 Simulation |
| Speed | 0–~200 | km/h | Aus RPM + Gear + Übersetzung |

## Mock-GPS (NMEA aus GPX)

Der NMEA-Generator erzeugt korrekte NMEA-0183 Sätze, die vom
Firmware-Parser (`nmea_parser.c`) akzeptiert werden:

```python
from mock_gps import NmeaGenerator

gen = NmeaGenerator("tracks/sample_alps.gpx", speed_multiplier=100)
for sentence in gen:
    print(sentence.strip())  # $GPRMC,...*XX\r\n
```

Unterstützte Satztypen: GGA, RMC, VTG (mit korrekten XOR-Checksummen).

## Mock-Display

Headless Framebuffer für CI, optional SDL2 für lokale Visualisierung:

```python
from mock_display import HeadlessDisplay, DisplayType, COLOR_WHITE

# Headless (für CI)
display = HeadlessDisplay(DisplayType.TFT)  # 240×320 RGB565
display.fill_rect(10, 10, 50, 50, COLOR_WHITE)
fb = display.get_framebuffer()  # bytearray

# SDL2 (für lokale Visualisierung)
# display = SdlDisplay(DisplayType.TFT, scale=2)
```

## Mock-BLE

GATT-Server mit allen 8 Services und 32 Characteristics:

```python
from mock_ble import MockBle, CHR_TLM_LIVE, CHR_DTC_LIST

ble = MockBle()
ble.connect()
ble.subscribe(CHR_TLM_LIVE)
ble.set_telemetry_data(sensor_data)
ble.add_dtc(0x0101)
```

## Mock-Controller

Tastatur → Controller-Events mit Debounce:

```python
from mock_controller import MockController, ControllerButton

ctrl = MockController()
ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
events = ctrl.poll_events()
```

| Taste | Button |
|-------|--------|
| Pfeiltasten | NAV_UP/DOWN/LEFT/RIGHT |
| Enter | SELECT |
| Escape | BACK |
| Pos1/Home | HOME |
| +/- | VOL_UP/DOWN |
| n/p | MEDIA_NEXT/PREV |

## CI-Integration

GitHub Actions Workflow: `.github/workflows/simulator-ci.yml`

### Jobs

1. **pytest** — 121 Tests auf Ubuntu + macOS, Python 3.10/3.11/3.12
2. **simulated-ride** — 24h simulierte Fahrt (beschleunigt)
3. **lint** — Pyflakes Static Analysis

### 24h Simulated Ride

```bash
python3 simulator.py \
  --track tracks/sample_alps.gpx \
  --duration 86400 \
  --speed-multiplier 1000 \
  --tick-hz 1 \
  --ci-mode \
  --fast \
  --skip-display \
  --seed 42
```

Verarbeitet 86.400 Ticks (1Hz × 24h) ohne Real-Time-Sleep.

## Test-Suite

121 Tests in 6 Test-Modulen:

| Datei | Tests | Modul |
|-------|-------|-------|
| `test_mock_sensors.py` | 17 | RPM, Temp, Fuel, Gear, IMU, Curves |
| `test_mock_gps.py` | 15 | GPX-Parsing, NMEA-Checksummen, Geo-Berechnungen |
| `test_mock_display.py` | 21 | Framebuffer, Pixel, Drawing, RGB565 |
| `test_mock_ble.py` | 26 | GATT Read/Write, Subscribe, DTC, Telemetry |
| `test_mock_controller.py` | 20 | Buttons, Debounce, Key-Mapping, Callbacks |
| `test_simulator.py` | 22 | Integration, End-to-End, CI-Mode |

```bash
python -m pytest tests/ -v
# 121 passed in 0.19s
```

## Projektstruktur

```
tools/simulator/
├── simulator.py          # Haupt-Orchestrator + CLI
├── mock_sensors.py        # Mock-Sensor-Schicht
├── mock_gps.py            # NMEA-Generator aus GPX
├── mock_display.py        # Headless Framebuffer + SDL2
├── mock_ble.py            # GATT-Server Simulation
├── mock_controller.py     # Tastatur → Controller-Events
├── requirements.txt       # pytest
├── tracks/
│   └── sample_alps.gpx    # Test-Track (60 Wegpunkte, Alpenroute)
└── tests/
    ├── conftest.py
    ├── test_mock_sensors.py
    ├── test_mock_gps.py
    ├── test_mock_display.py
    ├── test_mock_ble.py
    ├── test_mock_controller.py
    └── test_simulator.py
```

## Firmware-Referenzen

Alle Mock-Schichten sind an die Firmware-Strukturen angepasst:

- `sensor_data_t` → `SensorData` (sensors.h)
- `gps_data_t` → `GpsData` (gps.h)
- `controller_button_t` → `ControllerButton` (controller_input.h)
- GATT UUIDs aus `gatt_uuids.h`
- Display-Konstanten aus `display.h` + `board_config.h`
- NMEA-Format kompatibel mit `nmea_parser.c`
