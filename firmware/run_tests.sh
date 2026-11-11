#!/usr/bin/env bash
# Baseline test runner — mirrors firmware-ci.yml host build
set -u
cd "$(dirname "$0")"
rm -rf test_results && mkdir -p test_results
UNITY_DIR=/tmp/unity/src
INCLUDES="-Icomponents/gps/include -Icomponents/utils/include -Icomponents/diagnostics/include"
INCLUDES="$INCLUDES -Icomponents/sensors/include -Icomponents/power/include -Icomponents/storage/include"
INCLUDES="$INCLUDES -Icomponents/ble/include -Icomponents/ble -Icomponents/display/include"
INCLUDES="$INCLUDES -Icomponents/ota/include -Icomponents/hid_controller/include"
INCLUDES="$INCLUDES -Icomponents/carplay_bridge/include -Icomponents/carplay_bridge"
INCLUDES="$INCLUDES -Imain -Itest -Itest/mocks -I$UNITY_DIR"

TESTS="test_nmea_parser test_event_loop test_event_bridge test_dtc test_filters test_config test_dtc_extended test_ble_callbacks test_power_manager test_controller test_carplay_bridge test_carplay_media test_ota test_ota_secure_boot test_gear_indicator test_fuel_level test_rpm test_temperature test_logger"

PASS=0; FAIL=0; TOTAL_TESTS=0; BUILT=0
# Build all except test_controller (special) in the loop
LOOP_TESTS="test_nmea_parser test_event_loop test_event_bridge test_dtc test_filters test_config test_dtc_extended test_ble_callbacks test_power_manager test_carplay_bridge test_carplay_media test_ota test_ota_secure_boot test_gear_indicator test_fuel_level test_rpm test_temperature test_logger"
for t in $LOOP_TESTS; do
  if gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
       -Wno-unused-parameter -Wno-deprecated-declarations \
       test/${t}.c $UNITY_DIR/unity.c -o test_results/${t} -lm -lpthread \
       2> test_results/${t}.build.log; then
    BUILT=$((BUILT+1))
  else
    FAIL=$((FAIL+1)); echo "FAIL(build): $t"; tail -6 test_results/${t}.build.log
  fi
done

# test_controller needs component sources + -include mock header
if gcc $INCLUDES -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
     -Wno-unused-parameter -Wno-deprecated-declarations \
     -include test/mocks/esp_idf_mock.h \
     test/test_controller.c \
     components/hid_controller/controller_input.c \
     components/hid_controller/controller_mapping.c \
     $UNITY_DIR/unity.c -o test_results/test_controller -lm -lpthread \
     2> test_results/test_controller.build.log; then
  BUILT=$((BUILT+1))
else
  FAIL=$((FAIL+1)); echo "FAIL(build): test_controller"; tail -6 test_results/test_controller.build.log
fi

# Run all built tests
for t in $TESTS; do
  if [ -x "test_results/$t" ]; then
    OUT=$("test_results/$t" 2>&1); RC=$?
    NT=$(echo "$OUT" | grep -oE '[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored' | grep -oE '^[0-9]+' | head -1)
    [ -z "$NT" ] && NT=0
    TOTAL_TESTS=$((TOTAL_TESTS + NT))
    if [ $RC -eq 0 ]; then PASS=$((PASS+1)); else FAIL=$((FAIL+1)); echo "FAIL(run): $t"; echo "$OUT" | tail -4; fi
  fi
done
echo "=== SUMMARY: built=$BUILT pass=$PASS fail=$FAIL total_assertions=$TOTAL_TESTS ==="
