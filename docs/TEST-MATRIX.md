# Test Matrix: Controller x BLE-Bridge x Companion App x Simulator

> Stand: Oktober 2026
> Sprint: 2 — Integration & Production Readiness
> Ticket: TOT-32

---

## 1. Uebersicht

Dieses Dokument definiert die Test-Matrix fuer die Integration des TRANSALP
Bordcomputers. Es deckt alle Kombinationen der vier Hauptkomponenten ab:

| Komponente | Technologie | Ort |
|-----------|-------------|-----|
| **Controller** (Lenker-Controller) | ESP32-S3 GPIO/I2C/BLE, HID | `firmware/components/hid_controller/` |
| **BLE-Bridge** (Firmware GATT-Server) | NimBLE, ESP-IDF | `firmware/components/ble/` |
| **Companion App** | React Native, TypeScript | `companion-app/` |
| **Simulator** | Python, pytest | `tools/simulator/` |

---

## 2. Test-Ebenen

```
┌─────────────────────────────────────────────────────────────┐
│                     Feldtest (PD06)                          │
│   Vollstaendiges System auf dem Motorrad                     │
├─────────────────────────────────────────────────────────────┤
│                  Integration Tests                           │
│   Component-Paare, Pipeline-Tests, Interface-Konsistenz     │
├─────────────────────────────────────────────────────────────┤
│                    Unit Tests                                │
│   Einzelne Funktionen, Mock-basiert                          │
├─────────────────────────────────────────────────────────────┤
│                    CI / Regression                            │
│   Automatisiert bei jedem Push/PR                            │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Komponenten-Matrix

### 3.1 Controller (Lenker-Controller)

| Test-ID | Komponente | Test | Ebene | Datei | Status |
|---------|-----------|------|-------|-------|--------|
| C-01 | Controller | Button-Debounce (50ms) | Unit | `test_controller.c` | Pass |
| C-02 | Controller | Button-to-Event Mapping (11 Buttons) | Unit | `test_controller.c` | Pass |
| C-03 | Controller | I2C Button-Mask Parsing | Unit | `test_controller.c` | Pass |
| C-04 | Controller | Debounce + Mapping Integration | Integ. | `test_integration_controller_mapping.c` | Pass |
| C-05 | Controller | Alle 11 Buttons gemappt | Integ. | `test_integration_controller_mapping.c` | Pass |
| C-06 | Controller | Mapping ist bijektiv (1:1) | Integ. | `test_integration_controller_mapping.c` | Pass |
| C-07 | Controller | Release-Erkennung (Edge Detection) | Integ. | `test_integration_controller_mapping.c` | Pass |
| C-08 | Controller | Reset erlaubt sofortigen Re-Press | Integ. | `test_integration_controller_mapping.c` | Pass |
| C-09 | Controller | Button-Namen alle non-NULL | Integ. | `test_integration_controller_mapping.c` | Pass |
| C-10 | Controller (Mock) | Tastatur-Mapping (Key->Button) | Unit | `test_mock_controller.py` | Pass |
| C-11 | Controller (Mock) | Callback-Registrierung | Unit | `test_mock_controller.py` | Pass |
| C-12 | Controller (Mock) | Button-IDs matchen firmware | Integ. | `test_simulator_firmware_interface.py` | Pass |

### 3.2 BLE-Bridge (Firmware GATT-Server)

| Test-ID | Komponente | Test | Ebene | Datei | Status |
|---------|-----------|------|-------|-------|--------|
| B-01 | BLE | GATT-Service-Initialisierung | Unit | `test_ble_callbacks.c` | Pass |
| B-02 | BLE | Characteristic Read/Write | Unit | `test_ble_callbacks.c` | Pass |
| B-03 | BLE | BLE Callback Handling | Unit | `test_ble_callbacks.c` | Pass |
| B-04 | BLE | Telemetry-Packing (22 Bytes) | Integ. | `test_integration_sensor_to_ble.c` | Pass |
| B-05 | BLE | DTC-Packing (6 Bytes/Eintrag) | Integ. | `test_integration_ble_gatt_interface.c` | Pass |
| B-06 | BLE | Sensor-Status-Packing (5 Bytes) | Integ. | `test_integration_ble_gatt_interface.c` | Pass |
| B-07 | BLE | UUID-Format-Konsistenz | Integ. | `test_integration_ble_gatt_interface.c` | Pass |
| B-08 | BLE | Service/Characteristic-Zuordnung | Integ. | `test_integration_ble_gatt_interface.c` | Pass |
| B-09 | BLE | Telemetry Round-Trip (Pack->Unpack) | Integ. | `test_integration_sensor_to_ble.c` | Pass |
| B-10 | BLE | Crash-Flag Propagation | Integ. | `test_integration_sensor_to_ble.c` | Pass |
| B-11 | BLE (Mock) | GATT Read/Write/Subscribe | Unit | `test_mock_ble.py` | Pass |
| B-12 | BLE (Mock) | DTC Add/Clear/Notify | Unit | `test_mock_ble.py` | Pass |
| B-13 | BLE (Mock) | Telemetry-Payload Groesse | Integ. | `test_simulator_firmware_interface.py` | Pass |
| B-14 | BLE (Mock) | UUID-Format matcht Firmware | Integ. | `test_simulator_firmware_interface.py` | Pass |

### 3.3 Companion App

| Test-ID | Komponente | Test | Ebene | Datei | Status |
|---------|-----------|------|-------|-------|--------|
| A-01 | App | BLE Binary Parser (Telemetry) | Unit | `binary-parsers.test.ts` | Pass |
| A-02 | App | BLE Binary Parser (DTC) | Unit | `binary-parsers.test.ts` | Pass |
| A-03 | App | BLE Binary Parser (Sensor Status) | Unit | `binary-parsers.test.ts` | Pass |
| A-04 | App | BLE Binary Parser (CarPlay) | Unit | `binary-parsers.test.ts` | Pass |
| A-05 | App | MockBleProvider | Unit | `MockBleProvider.test.ts` | Pass |
| A-06 | App | BleManager Service | Unit | `BleManager.test.ts` | Pass |
| A-07 | App | Store/State Management | Unit | `store.test.ts` | Pass |
| A-08 | App | CarPlay Controller Screen | Unit | `CarPlayController.test.ts` | Pass |
| A-09 | App | UUID-Konsistenz (vs Firmware) | Integ. | `test_integration_ble_gatt_interface.c` | Pass |
| A-10 | App | Telemetry-Format (vs Firmware Mock) | Integ. | `test_simulator_firmware_interface.py` | Pass |

### 3.4 Simulator

| Test-ID | Komponente | Test | Ebene | Datei | Status |
|---------|-----------|------|-------|-------|--------|
| S-01 | Simulator | MockSensorLayer RPM/Temp/Fuel | Unit | `test_mock_sensors.py` | Pass |
| S-02 | Simulator | NmeaGenerator (GPX->NMEA) | Unit | `test_mock_gps.py` | Pass |
| S-03 | Simulator | HeadlessDisplay Framebuffer | Unit | `test_mock_display.py` | Pass |
| S-04 | Simulator | MockBle GATT-Server | Unit | `test_mock_ble.py` | Pass |
| S-05 | Simulator | MockController Debounce | Unit | `test_mock_controller.py` | Pass |
| S-06 | Simulator | Simulator End-to-End | Unit | `test_simulator.py` | Pass |
| S-07 | Simulator | 24h Simulated Ride (CI) | CI | `simulator-ci.yml` | Pass |
| S-08 | Simulator | SensorData matcht sensor_data_t | Integ. | `test_simulator_firmware_interface.py` | Pass |
| S-09 | Simulator | GpsData matcht gps_data_t | Integ. | `test_simulator_firmware_interface.py` | Pass |
| S-10 | Simulator | ControllerButton matcht controller_button_t | Integ. | `test_simulator_firmware_interface.py` | Pass |
| S-11 | Simulator | Display-Konstanten matchen display.h | Integ. | `test_simulator_firmware_interface.py` | Pass |
| S-12 | Simulator | NMEA-Checksummen korrekt | Integ. | `test_simulator_firmware_interface.py` | Pass |
| S-13 | Simulator | Sensor->BLE Pipeline | Integ. | `test_simulator_firmware_interface.py` | Pass |
| S-14 | Simulator | Controller->Display Mode Wechsel | Integ. | `test_simulator_firmware_interface.py` | Pass |
| S-15 | Simulator | Warning->BLE Status Propagation | Integ. | `test_simulator_firmware_interface.py` | Pass |

---

## 4. Cross-Component Test-Matrix

### 4.1 Controller x BLE-Bridge

| Szenario | Test | Ort |
|----------|------|-----|
| Controller-Button -> BLE Media-Control Write | Mock-Integration | `test_mock_ble.py` |
| Controller HOME -> Display Mode -> BLE Notify | Simulator-E2E | `test_simulator.py` |
| Controller-Button-IDs matchen BLE-Service-Zuordnung | Interface-Konsistenz | `test_integration_ble_gatt_interface.c` |

### 4.2 Controller x Companion App

| Szenario | Test | Ort |
|----------|------|-----|
| Controller-Tastatur-Mapping matcht App CONTROLLER_KEY_MAP | Interface-Konsistenz | `test_simulator_firmware_interface.py` |
| App ControllerButton Enum matcht Firmware controller_button_t | Interface-Konsistenz | `test_integration_controller_mapping.c` |

### 4.3 BLE-Bridge x Companion App

| Szenario | Test | Ort |
|----------|------|-----|
| Telemetry-Payload (22 Bytes) Pack/Unpack Round-Trip | Interface-Konsistenz | `test_integration_sensor_to_ble.c` |
| DTC-Payload (6 Bytes/Eintrag) Format | Interface-Konsistenz | `test_integration_ble_gatt_interface.c` |
| Sensor-Status (5 Bytes) Format | Interface-Konsistenz | `test_integration_ble_gatt_interface.c` |
| UUID Base-Format (6e40XXXX-...) | Interface-Konsistenz | `test_integration_ble_gatt_interface.c` |
| CarPlay-Status Payload (194 Bytes min) | Interface-Konsistenz | `test_integration_ble_gatt_interface.c` |

### 4.4 Simulator x Firmware

| Szenario | Test | Ort |
|----------|------|-----|
| SensorData Felder matchen sensor_data_t | Interface-Konsistenz | `test_simulator_firmware_interface.py` |
| GpsData Felder matchen gps_data_t | Interface-Konsistenz | `test_simulator_firmware_interface.py` |
| NMEA-Ausgabe parsebar von nmea_parser.c | Pipeline-Integration | `test_integration_gps_nmea_pipeline.c` |
| BLE-UUIDs matchen gatt_uuids.h | Interface-Konsistenz | `test_simulator_firmware_interface.py` |
| Sensor-Wertebereiche (RPM 0-8000, Temp 20-120, etc.) | Interface-Konsistenz | `test_simulator_firmware_interface.py` |
| Display-Dimensionen (TFT 240x320, EPD 296x128) | Interface-Konsistenz | `test_simulator_firmware_interface.py` |
| Controller-Button-IDs (0-10) matchen controller_input.h | Interface-Konsistenz | `test_simulator_firmware_interface.py` |

### 4.5 Sensor x Event Bus x Diagnostics

| Szenario | Test | Ort |
|----------|------|-----|
| Sensor -> Kalman/MA-Filter -> Glaettung | Pipeline-Integration | `test_integration_filter_sensor_pipeline.c` |
| DTC Report -> Event Bus Flag | Pipeline-Integration | `test_integration_dtc_diagnostics.c` |
| DTC Severity Mapping (Critical/Warn/Error) | Interface-Konsistenz | `test_integration_dtc_diagnostics.c` |
| Event-Bus Bits (EVT_OVERTEMP, EVT_FUEL_LOW, etc.) | Interface-Konsistenz | `test_integration_dtc_diagnostics.c` |
| DTC Freeze-Frame Struktur | Interface-Konsistenz | `test_integration_dtc_diagnostics.c` |

---

## 5. CI-Pipeline Matrix

| Workflow | Trigger | Tests | Datei |
|----------|---------|-------|-------|
| Firmware CI | push/PR auf `firmware/**` | Unit Tests (Unity+GCC), ESP-IDF Build, cppcheck, clang-format | `firmware-ci.yml` |
| Simulator CI | push/PR auf `tools/simulator/**` | pytest (121+61 Tests), 24h Ride, pyflakes | `simulator-ci.yml` |
| Companion App CI | push/PR auf `companion-app/**` | jest, eslint, tsc | `companion-app-ci.yml` |
| **Integration Tests** | push/PR auf **alle** | Firmware Integration (70 Tests), Simulator Integration (61 Tests) | `integration-tests.yml` |
| Docs CI | push/PR auf `docs/**` | markdownlint | `docs-ci.yml` |
| Security | push/PR | CodeQL, Semgrep, Trivy | `codeql.yml`, `semgrep.yml`, `trivy.yml` |

---

## 6. Test-Zusammenfassung

| Ebene | C-Tests (Firmware) | Python-Tests (Simulator) | TS-Tests (App) | Gesamt |
|-------|--------------------|-------------------------|-----------------|--------|
| Unit | 60+ | 121 | 30+ | 210+ |
| Integration | 70 | 61 | — | 131 |
| CI/E2E | 1 (24h Ride) | 1 (24h Ride) | — | 2 |
| **Total** | **130+** | **183** | **30+** | **343+** |

---

## 7. Abnahmekriterien

- [x] Integration-Test-Suite in `firmware/test/integration/` (70 C-Tests)
- [x] Simulator-Integration-Tests in `tools/simulator/tests/integration/` (61 Python-Tests)
- [x] Test-Matrix dokumentiert (dieses Dokument)
- [x] CI-Pipeline fuer Integration-Tests (`integration-tests.yml`)
- [ ] Feldtest-Protokoll aktualisiert (`FIELDTEST-PROTOCOL.md`)
- [ ] Safety-Checklist erstellt (`SAFETY-CHECKLIST.md`)
