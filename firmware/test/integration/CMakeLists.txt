# firmware/test/integration/CMakeLists.txt
#
# Integration test build configuration.
#
# Integration tests verify component interactions:
#   - Sensor -> BLE telemetry packing pipeline
#   - GPS NMEA generation -> firmware parser
#   - Controller button -> debounce -> mapping -> event dispatch
#   - DTC reporting -> diagnostics -> event bus
#   - Signal filters -> sensor data pipeline
#   - BLE GATT interface consistency (firmware + simulator + companion-app)
#
# For host builds, the CI workflow compiles each test file with GCC.
# This CMakeLists provides an optional CMake-based build alternative.

if(NOT IDF_PATH)
    cmake_minimum_required(VERSION 3.16)
    project(transalp_integration_test C)

    set(UNITY_DIR ${UNITY_DIR} CACHE PATH "Path to Unity framework")
    if(NOT UNITY_DIR)
        set(UNITY_DIR "/tmp/unity")
    endif()

    set(COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../components)
    set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

    set(TEST_INCLUDES
        ${COMPONENTS_DIR}/gps/include
        ${COMPONENTS_DIR}/utils/include
        ${COMPONENTS_DIR}/diagnostics/include
        ${COMPONENTS_DIR}/sensors/include
        ${COMPONENTS_DIR}/power/include
        ${COMPONENTS_DIR}/storage/include
        ${COMPONENTS_DIR}/ble/include
        ${COMPONENTS_DIR}/display/include
        ${COMPONENTS_DIR}/ota/include
        ${COMPONENTS_DIR}/hid_controller/include
        ${COMPONENTS_DIR}/carplay_bridge/include
        ${TEST_DIR}
        ${TEST_DIR}/mocks
        ${UNITY_DIR}/src
    )

    add_compile_options(
        -std=c11
        -Wall
        -Wextra
        -Wno-unused-parameter
        -DTEST_HOST
        -DUNITY_INCLUDE_DOUBLE
    )

    # ---- Integration test executables ----

    # 1. Sensor -> BLE telemetry pipeline (standalone, no component sources)
    add_executable(test_integration_sensor_to_ble
        test_integration_sensor_to_ble.c
        ${UNITY_DIR}/src/unity.c
    )
    target_include_directories(test_integration_sensor_to_ble PRIVATE ${TEST_INCLUDES})
    target_link_libraries(test_integration_sensor_to_ble PRIVATE m pthread)
    add_test(NAME test_integration_sensor_to_ble COMMAND test_integration_sensor_to_ble)

    # 2. GPS NMEA pipeline (includes nmea_parser.c directly)
    add_executable(test_integration_gps_nmea_pipeline
        test_integration_gps_nmea_pipeline.c
        ${UNITY_DIR}/src/unity.c
    )
    target_include_directories(test_integration_gps_nmea_pipeline PRIVATE ${TEST_INCLUDES})
    target_link_libraries(test_integration_gps_nmea_pipeline PRIVATE m pthread)
    add_test(NAME test_integration_gps_nmea_pipeline COMMAND test_integration_gps_nmea_pipeline)

    # 3. Controller mapping (links component sources)
    add_executable(test_integration_controller_mapping
        test_integration_controller_mapping.c
        ${COMPONENTS_DIR}/hid_controller/controller_input.c
        ${COMPONENTS_DIR}/hid_controller/controller_mapping.c
        ${UNITY_DIR}/src/unity.c
    )
    target_include_directories(test_integration_controller_mapping PRIVATE ${TEST_INCLUDES})
    target_link_libraries(test_integration_controller_mapping PRIVATE m pthread)
    add_test(NAME test_integration_controller_mapping COMMAND test_integration_controller_mapping)

    # 4. DTC + diagnostics + event bus (includes sources directly)
    add_executable(test_integration_dtc_diagnostics
        test_integration_dtc_diagnostics.c
        ${UNITY_DIR}/src/unity.c
    )
    target_include_directories(test_integration_dtc_diagnostics PRIVATE ${TEST_INCLUDES})
    target_link_libraries(test_integration_dtc_diagnostics PRIVATE m pthread)
    add_test(NAME test_integration_dtc_diagnostics COMMAND test_integration_dtc_diagnostics)

    # 5. Filter + sensor pipeline (includes filters.c directly)
    add_executable(test_integration_filter_sensor_pipeline
        test_integration_filter_sensor_pipeline.c
        ${UNITY_DIR}/src/unity.c
    )
    target_include_directories(test_integration_filter_sensor_pipeline PRIVATE ${TEST_INCLUDES})
    target_link_libraries(test_integration_filter_sensor_pipeline PRIVATE m pthread)
    add_test(NAME test_integration_filter_sensor_pipeline COMMAND test_integration_filter_sensor_pipeline)

    # 6. BLE GATT interface consistency (standalone)
    add_executable(test_integration_ble_gatt_interface
        test_integration_ble_gatt_interface.c
        ${UNITY_DIR}/src/unity.c
    )
    target_include_directories(test_integration_ble_gatt_interface PRIVATE ${TEST_INCLUDES})
    target_link_libraries(test_integration_ble_gatt_interface PRIVATE m pthread)
    add_test(NAME test_integration_ble_gatt_interface COMMAND test_integration_ble_gatt_interface)

    enable_testing()

    message(STATUS "Integration test suite configured (host mode, Unity at ${UNITY_DIR})")
    return()
endif()

# ---- ESP-IDF test component mode ----
idf_component_register(
    SRCS
        "test_integration_sensor_to_ble.c"
        "test_integration_gps_nmea_pipeline.c"
        "test_integration_controller_mapping.c"
        "test_integration_dtc_diagnostics.c"
        "test_integration_filter_sensor_pipeline.c"
        "test_integration_ble_gatt_interface.c"
    INCLUDE_DIRS
        "."
        "../mocks"
    REQUIRES
        unity
        gps
        utils
        diagnostics
        hid_controller
        carplay_bridge
)
