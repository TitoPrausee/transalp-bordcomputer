# Integration Test Suite — TRANSALP Bordcomputer

Integration tests verify interactions between firmware components,
the simulator mock layer, and the companion-app BLE interface.

## Overview

These tests sit above unit tests (which test individual functions)
and below end-to-end/HIL tests (which run on real hardware). They
verify that components work correctly together when data flows
across module boundaries.

## Test Modules

| File | Tests | Components Covered |
|------|-------|-------------------|
| `test_integration_sensor_to_ble.c` | 9 | Sensor data → BLE telemetry packing (22-byte payload) |
| `test_integration_gps_nmea_pipeline.c` | 10 | NMEA sentence → firmware parser → gps_data_t |
| `test_integration_controller_mapping.c` | 14 | Button → debounce → mapping → event dispatch |
| `test_integration_dtc_diagnostics.c` | 15 | DTC report → diagnostics → event bus flags |
| `test_integration_filter_sensor_pipeline.c` | 11 | Kalman / Complementary / Moving Average filters |
| `test_integration_ble_gatt_interface.c` | 11 | GATT UUID consistency (firmware + simulator + app) |

**Total: 70 integration tests**

## Build & Run

### Host build (GCC + Unity)

```bash
# Clone Unity test framework
git clone --depth 1 https://github.com/ThrowTheSwitch/Unity.git /tmp/unity

# Build and run all integration tests
UNITY_DIR=/tmp/unity/src
INCLUDES="-I../../components/gps/include"
INCLUDES="$INCLUDES -I../../components/utils/include"
INCLUDES="$INCLUDES -I../../components/diagnostics/include"
INCLUDES="$INCLUDES -I../../components/sensors/include"
INCLUDES="$INCLUDES -I../../components/power/include"
INCLUDES="$INCLUDES -I../../components/storage/include"
INCLUDES="$INCLUDES -I../../components/ble/include"
INCLUDES="$INCLUDES -I../../components/display/include"
INCLUDES="$INCLUDES -I../../components/ota/include"
INCLUDES="$INCLUDES -I../../components/hid_controller/include"
INCLUDES="$INCLUDES -I../../components/carplay_bridge/include"
INCLUDES="$INCLUDES -I../mocks"
INCLUDES="$INCLUDES -I$UNITY_DIR"

# Build each test
gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
  test_integration_sensor_to_ble.c $UNITY_DIR/unity.c -o /tmp/test_int_sensor_ble -lm -lpthread

gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
  test_integration_gps_nmea_pipeline.c $UNITY_DIR/unity.c -o /tmp/test_int_gps -lm -lpthread

gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
  -include ../mocks/esp_idf_mock.h \
  test_integration_controller_mapping.c \
  ../../components/hid_controller/controller_input.c \
  ../../components/hid_controller/controller_mapping.c \
  $UNITY_DIR/unity.c -o /tmp/test_int_controller -lm -lpthread

gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
  test_integration_dtc_diagnostics.c $UNITY_DIR/unity.c -o /tmp/test_int_dtc -lm -lpthread

gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
  test_integration_filter_sensor_pipeline.c $UNITY_DIR/unity.c -o /tmp/test_int_filter -lm -lpthread

gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
  test_integration_ble_gatt_interface.c $UNITY_DIR/unity.c -o /tmp/test_int_ble -lm -lpthread

# Run all
for t in /tmp/test_int_*; do echo "--- $t ---"; "$t"; done
```

### CMake build (optional)

```bash
mkdir build && cd build
cmake -DUNITY_DIR=/tmp/unity/src ..
make
ctest --output-on-failure
```

### ESP-IDF target build

When built with ESP-IDF (`IDF_PATH` set), the tests register as an
ESP-IDF test component and can run on the ESP32-S3 target:

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py flash monitor  # runs tests on target
```

## CI Integration

The `.github/workflows/integration-tests.yml` workflow runs these
tests on every push and pull request.

## Coverage Matrix

| Component Pair | Integration Test | Data Flow |
|---------------|-----------------|-----------|
| Sensors → BLE | `test_integration_sensor_to_ble` | sensor_data_t → 22-byte packed telemetry |
| GPS → Parser | `test_integration_gps_nmea_pipeline` | NMEA string → gps_data_t |
| Controller → Event Bus | `test_integration_controller_mapping` | button press → event ID |
| DTC → Event Bus | `test_integration_dtc_diagnostics` | dtc_report() → event_bus_set() |
| Filters → Sensors | `test_integration_filter_sensor_pipeline` | raw ADC → filtered value |
| BLE GATT → App | `test_integration_ble_gatt_interface` | UUID consistency across layers |
