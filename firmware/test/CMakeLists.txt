# firmware/test/CMakeLists.txt — Test build configuration
#
# Supports two modes:
# 1. Host build (GCC): compiles unit tests with mocks for fast CI feedback
# 2. ESP-IDF test component: registers as an ESP-IDF test component for
#    target-side integration tests on the ESP32-S3
#
# For host builds, the CI workflow compiles each test file directly with GCC:
#   gcc -I<includes> test/test_*.c mocks/*.c unity.c -o test_runner -lm -lpthread
#
# This CMakeLists.txt is used when the test suite is built via CMake
# (optional alternative to the direct GCC invocation in CI).

# ---- Host build mode (when not building with ESP-IDF) ----
if(NOT IDF_PATH)
    cmake_minimum_required(VERSION 3.16)
    project(transalp_test C)

    # Unity test framework (assumed cloned to /tmp/unity or a local path)
    set(UNITY_DIR ${UNITY_DIR} CACHE PATH "Path to Unity framework")
    if(NOT UNITY_DIR)
        set(UNITY_DIR "/tmp/unity")
    endif()

    # Include paths: component headers + test mocks + unity
    set(TEST_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/gps/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/utils/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/diagnostics/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/sensors/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/power/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/storage/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/ble/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/display/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/ota/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/hid_controller/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../components/carplay_bridge/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../main
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
        ${UNITY_DIR}/src
    )

    # Test sources
    set(TEST_SOURCES
        test_nmea_parser.c
        test_event_loop.c
        test_dtc.c
        test_filters.c
        test_config.c
        test_dtc_extended.c
        test_ble_callbacks.c
        test_power_manager.c
        test_controller.c
        test_carplay_bridge.c
    )

    # Compiler flags
    add_compile_options(
        -std=c11
        -Wall
        -Wextra
        -Wno-unused-parameter
        -DTEST_HOST
        -DUNITY_INCLUDE_DOUBLE
    )

    # Build each test as a separate executable
    foreach(TEST_SRC ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        add_executable(${TEST_NAME}
            ${TEST_SRC}
            ${UNITY_DIR}/src/unity.c
        )
        target_include_directories(${TEST_NAME} PRIVATE ${TEST_INCLUDES})
        target_link_libraries(${TEST_NAME} PRIVATE m pthread)

        # Register with CTest
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()

    enable_testing()

    message(STATUS "Test suite configured (host mode, Unity at ${UNITY_DIR})")
    return()
endif()

# ---- ESP-IDF test component mode ----
# When built with ESP-IDF (IDF_PATH is set), register as a test component.
# This enables `idf.py test` for target-side integration tests.
idf_component_register(
    SRCS
        "test_nmea_parser.c"
        "test_event_loop.c"
        "test_dtc.c"
        "test_filters.c"
        "test_config.c"
        "test_dtc_extended.c"
        "test_ble_callbacks.c"
        "test_power_manager.c"
        "test_controller.c"
        "test_carplay_bridge.c"
    INCLUDE_DIRS
        "."
        "mocks"
    REQUIRES
        unity
        gps
        utils
        diagnostics
        carplay_bridge
)
