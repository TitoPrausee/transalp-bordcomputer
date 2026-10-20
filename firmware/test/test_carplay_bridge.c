/*
 * test_carplay_bridge.c — Unit Tests fuer CarPlay BLE-Bridge
 *
 * Testet firmware/components/carplay_bridge/carplay_telemetry.c:
 *   - Telemetry-Live Packet (12 bytes packed, Little-Endian)
 *   - Feld-Offsets und Byte-Layout
 *   - carplay_telemetry_build() Konvertierung und Clamping
 *   - Fuel-Percent-Berechnung und Fuel-Low Flag
 *   - Gear-Neutral Mapping (0 -> -1)
 *   - Voltage-Umrechnung (Volt -> Zehntelvolt)
 *   - NULL-Safety
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "../components/carplay_bridge/carplay_internal.h"
#include "../components/carplay_bridge/carplay_uuids.h"

#include <string.h>
#include <stdint.h>

/* ============================================================
 * Include the actual carplay_telemetry.c implementation directly.
 * We include it to test the internal helper functions.
 * ============================================================ */
#ifdef TEST_HOST
#include "../components/carplay_bridge/carplay_telemetry.c"
#endif

void setUp(void)
{
}

void tearDown(void)
{
}

/* ============================================================
 * Test: carplay_telemetry_t is exactly 12 bytes (packed)
 * ============================================================ */

void test_telemetry_packed_size(void)
{
    TEST_ASSERT_EQUAL(CARPLAY_TLM_LIVE_SIZE, sizeof(carplay_telemetry_t));
    TEST_ASSERT_EQUAL(12, sizeof(carplay_telemetry_t));
}

/* ============================================================
 * Test: field offsets match the wire-format spec
 *
 * Layout (Little-Endian):
 *   0     speed_kmh        uint8
 *   1-2   rpm              uint16
 *   3     gear             int8
 *   4     coolant_temp_c   int8
 *   5     fuel_percent     uint8
 *   6-7   voltage_tenth    uint16
 *   8-9   ambient_temp_c   int16
 *   10    fuel_low         uint8
 *   11    reserved         uint8
 * ============================================================ */

void test_telemetry_field_offsets(void)
{
    carplay_telemetry_t pkt;
    uint8_t *raw = (uint8_t *)&pkt;

    /* Verify struct offsets via offsetof-style pointer arithmetic */
    TEST_ASSERT_EQUAL(0,  (size_t)((uint8_t *)&pkt.speed_kmh       - raw));
    TEST_ASSERT_EQUAL(1,  (size_t)((uint8_t *)&pkt.rpm             - raw));
    TEST_ASSERT_EQUAL(3,  (size_t)((uint8_t *)&pkt.gear            - raw));
    TEST_ASSERT_EQUAL(4,  (size_t)((uint8_t *)&pkt.coolant_temp_c  - raw));
    TEST_ASSERT_EQUAL(5,  (size_t)((uint8_t *)&pkt.fuel_percent    - raw));
    TEST_ASSERT_EQUAL(6,  (size_t)((uint8_t *)&pkt.voltage_tenth   - raw));
    TEST_ASSERT_EQUAL(8,  (size_t)((uint8_t *)&pkt.ambient_temp_c  - raw));
    TEST_ASSERT_EQUAL(10, (size_t)((uint8_t *)&pkt.fuel_low         - raw));
    TEST_ASSERT_EQUAL(11, (size_t)((uint8_t *)&pkt.reserved         - raw));
}

/* ============================================================
 * Test: normal values are packed correctly (Little-Endian)
 * ============================================================ */

void test_telemetry_build_normal_values(void)
{
    carplay_telemetry_input_t in = {
        .speed_kmh = 120.0f,
        .rpm = 6500,
        .gear = 4,
        .coolant_temp_c = 88.0f,
        .ambient_temp_c = 22.0f,
        .fuel_level_l = 12.0f,
        .battery_voltage = 12.6f,
        .tank_capacity_l = 15.0f,
        .fuel_reserve_l = 2.0f,
    };

    carplay_telemetry_t pkt;
    carplay_telemetry_build(&pkt, &in);

    uint8_t *raw = (uint8_t *)&pkt;

    TEST_ASSERT_EQUAL_UINT8(120, pkt.speed_kmh);
    TEST_ASSERT_EQUAL_UINT16(6500, pkt.rpm);
    TEST_ASSERT_EQUAL_INT8(4, pkt.gear);
    TEST_ASSERT_EQUAL_INT8(88, pkt.coolant_temp_c);
    TEST_ASSERT_EQUAL_UINT8(80, pkt.fuel_percent);  /* 12/15*100 = 80 */
    TEST_ASSERT_EQUAL_UINT16(126, pkt.voltage_tenth); /* 12.6 * 10 = 126 */
    TEST_ASSERT_EQUAL_INT16(22, pkt.ambient_temp_c);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_low);  /* 12 > 2, not low */
    TEST_ASSERT_EQUAL_UINT8(0, pkt.reserved);

    /* Byte-level LE check for rpm (6500 = 0x1964) */
    TEST_ASSERT_EQUAL_UINT8(0x64, raw[1]);
    TEST_ASSERT_EQUAL_UINT8(0x19, raw[2]);

    /* Byte-level LE check for voltage (126 = 0x007E) */
    TEST_ASSERT_EQUAL_UINT8(0x7E, raw[6]);
    TEST_ASSERT_EQUAL_UINT8(0x00, raw[7]);
}

/* ============================================================
 * Test: speed is clamped to 0-255
 * ============================================================ */

void test_telemetry_speed_clamping(void)
{
    carplay_telemetry_input_t in = { .tank_capacity_l = 10.0f };
    carplay_telemetry_t pkt;

    /* Negative speed -> 0 */
    in.speed_kmh = -10.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.speed_kmh);

    /* Over 255 -> 255 */
    in.speed_kmh = 300.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(255, pkt.speed_kmh);

    /* Exactly 255 */
    in.speed_kmh = 255.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(255, pkt.speed_kmh);

    /* Rounding: 120.4 -> 120, 120.5 -> 121 */
    in.speed_kmh = 120.4f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(120, pkt.speed_kmh);

    in.speed_kmh = 120.5f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(121, pkt.speed_kmh);
}

/* ============================================================
 * Test: gear=0 (neutral) is mapped to -1 in wire format
 * ============================================================ */

void test_telemetry_gear_neutral_mapping(void)
{
    carplay_telemetry_input_t in = { .tank_capacity_l = 10.0f };
    carplay_telemetry_t pkt;

    /* gear=0 -> -1 (neutral sentinel) */
    in.gear = 0;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT8(-1, pkt.gear);

    /* gear=1 stays 1 */
    in.gear = 1;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT8(1, pkt.gear);

    /* gear=6 stays 6 */
    in.gear = 6;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT8(6, pkt.gear);
}

/* ============================================================
 * Test: fuel percent calculation and fuel_low flag
 * ============================================================ */

void test_telemetry_fuel_calculation(void)
{
    carplay_telemetry_t pkt;

    /* 50% fuel, not low (5L in 10L tank, reserve 2L) */
    carplay_telemetry_input_t in = {
        .fuel_level_l = 5.0f,
        .tank_capacity_l = 10.0f,
        .fuel_reserve_l = 2.0f,
    };
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(50, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_low);

    /* At reserve threshold: low=1 (<=) */
    in.fuel_level_l = 2.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(20, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(1, pkt.fuel_low);

    /* Below reserve: low=1 */
    in.fuel_level_l = 1.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(10, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(1, pkt.fuel_low);

    /* Above reserve: low=0 */
    in.fuel_level_l = 3.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(30, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_low);

    /* Empty tank -> 0%, low=1 */
    in.fuel_level_l = 0.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(1, pkt.fuel_low);

    /* Full tank -> 100%, low=0 */
    in.fuel_level_l = 10.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(100, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_low);

    /* Overfull -> clamped to 100% */
    in.fuel_level_l = 12.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(100, pkt.fuel_percent);
}

/* ============================================================
 * Test: fuel_low=0 when no reserve is configured (reserve_l=0)
 * ============================================================ */

void test_telemetry_no_reserve_no_low_flag(void)
{
    carplay_telemetry_input_t in = {
        .fuel_level_l = 1.0f,
        .tank_capacity_l = 10.0f,
        .fuel_reserve_l = 0.0f,  /* no reserve threshold */
    };
    carplay_telemetry_t pkt;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(10, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_low);
}

/* ============================================================
 * Test: voltage conversion (Volt -> Zehntelvolt)
 * ============================================================ */

void test_telemetry_voltage_conversion(void)
{
    carplay_telemetry_input_t in = { .tank_capacity_l = 10.0f };
    carplay_telemetry_t pkt;

    in.battery_voltage = 12.6f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT16(126, pkt.voltage_tenth);

    in.battery_voltage = 0.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.voltage_tenth);

    /* Negative voltage -> 0 */
    in.battery_voltage = -1.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.voltage_tenth);

    /* Over 65.53V -> clamped to 65.53, voltage_tenth=655 */
    in.battery_voltage = 70.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT16(655, pkt.voltage_tenth);
}

/* ============================================================
 * Test: coolant temperature clamping (int8: -128..127)
 * ============================================================ */

void test_telemetry_coolant_clamping(void)
{
    carplay_telemetry_input_t in = { .tank_capacity_l = 10.0f };
    carplay_telemetry_t pkt;

    in.coolant_temp_c = 200.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT8(127, pkt.coolant_temp_c);

    in.coolant_temp_c = -200.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT8(-128, pkt.coolant_temp_c);

    in.coolant_temp_c = 90.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT8(90, pkt.coolant_temp_c);
}

/* ============================================================
 * Test: ambient temperature (int16)
 * ============================================================ */

void test_telemetry_ambient_temp(void)
{
    carplay_telemetry_input_t in = { .tank_capacity_l = 10.0f };
    carplay_telemetry_t pkt;

    in.ambient_temp_c = -15.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT16(-15, pkt.ambient_temp_c);

    in.ambient_temp_c = 42.0f;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_INT16(42, pkt.ambient_temp_c);
}

/* ============================================================
 * Test: NULL safety (no crash)
 * ============================================================ */

void test_telemetry_build_null_safety(void)
{
    carplay_telemetry_input_t in = { .tank_capacity_l = 10.0f };
    carplay_telemetry_t pkt;
    memset(&pkt, 0xFF, sizeof(pkt));  /* fill with non-zero garbage */

    /* NULL pkt — should not crash */
    carplay_telemetry_build(NULL, &in);

    /* NULL input — should not crash, pkt must be untouched */
    carplay_telemetry_build(&pkt, NULL);
    TEST_ASSERT_EQUAL_UINT8(0xFF, pkt.speed_kmh);  /* unchanged */

    /* Both NULL — should not crash */
    carplay_telemetry_build(NULL, NULL);
}

/* ============================================================
 * Test: tank_capacity_l=0 doesn't divide by zero
 * ============================================================ */

void test_telemetry_zero_tank_capacity(void)
{
    carplay_telemetry_input_t in = {
        .fuel_level_l = 5.0f,
        .tank_capacity_l = 0.0f,  /* avoid division by zero */
        .fuel_reserve_l = 2.0f,
    };
    carplay_telemetry_t pkt;
    carplay_telemetry_build(&pkt, &in);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_percent);
}

/* ============================================================
 * Test: full byte-level packet verification
 *
 * Constructs a known packet and verifies every byte.
 * ============================================================ */

void test_telemetry_full_byte_layout(void)
{
    carplay_telemetry_input_t in = {
        .speed_kmh = 100.0f,
        .rpm = 5000,          /* 0x1388 */
        .gear = 3,
        .coolant_temp_c = 85.0f,
        .ambient_temp_c = 15.0f,
        .fuel_level_l = 7.5f,
        .battery_voltage = 13.2f,
        .tank_capacity_l = 15.0f,  /* 50% */
        .fuel_reserve_l = 2.0f,   /* not low */
    };

    carplay_telemetry_t pkt;
    carplay_telemetry_build(&pkt, &in);

    uint8_t expected[12] = {
        100,        /* [0]  speed_kmh = 100 */
        0x88, 0x13, /* [1-2] rpm = 5000 LE */
        3,          /* [3]  gear = 3 */
        85,         /* [4]  coolant_temp_c = 85 */
        50,         /* [5]  fuel_percent = 50 */
        0x84, 0x00, /* [6-7] voltage_tenth = 132 LE */
        15, 0x00,   /* [8-9] ambient_temp_c = 15 LE */
        0,          /* [10] fuel_low = 0 */
        0,          /* [11] reserved = 0 */
    };

    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, (uint8_t *)&pkt, 12);
}

/* ============================================================
 * Test: zeroed input produces zeroed packet
 * ============================================================ */

void test_telemetry_zeroed_input(void)
{
    carplay_telemetry_input_t in;
    memset(&in, 0, sizeof(in));
    /* tank_capacity_l=0 -> fuel_percent=0, fuel_low=0 (no reserve) */

    carplay_telemetry_t pkt;
    carplay_telemetry_build(&pkt, &in);

    TEST_ASSERT_EQUAL_UINT8(0, pkt.speed_kmh);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.rpm);
    TEST_ASSERT_EQUAL_INT8(-1, pkt.gear);  /* gear=0 -> -1 */
    TEST_ASSERT_EQUAL_INT8(0, pkt.coolant_temp_c);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_percent);
    TEST_ASSERT_EQUAL_UINT16(0, pkt.voltage_tenth);
    TEST_ASSERT_EQUAL_INT16(0, pkt.ambient_temp_c);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.fuel_low);
    TEST_ASSERT_EQUAL_UINT8(0, pkt.reserved);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_telemetry_packed_size);
    RUN_TEST(test_telemetry_field_offsets);

    RUN_TEST(test_telemetry_build_normal_values);
    RUN_TEST(test_telemetry_speed_clamping);
    RUN_TEST(test_telemetry_gear_neutral_mapping);
    RUN_TEST(test_telemetry_fuel_calculation);
    RUN_TEST(test_telemetry_no_reserve_no_low_flag);
    RUN_TEST(test_telemetry_voltage_conversion);
    RUN_TEST(test_telemetry_coolant_clamping);
    RUN_TEST(test_telemetry_ambient_temp);
    RUN_TEST(test_telemetry_build_null_safety);
    RUN_TEST(test_telemetry_zero_tank_capacity);
    RUN_TEST(test_telemetry_full_byte_layout);
    RUN_TEST(test_telemetry_zeroed_input);

    return UNITY_END();
}
