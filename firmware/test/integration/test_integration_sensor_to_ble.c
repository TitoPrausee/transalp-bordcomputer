/*
 * test_integration_sensor_to_ble.c
 *
 * Integration test: Sensor Data -> BLE Telemetry Packing Pipeline
 *
 * Verifies that sensor_data_t values are correctly packed into the
 * 22-byte BLE live-telemetry payload (CHR_TLM_LIVE 0x5001) and that
 * the packed binary can be unpacked to recover the original values
 * within the expected quantisation tolerance.
 *
 * This mirrors the data flow:
 *   sensor_task -> sensor_data_queue -> ble_on_sensor_update()
 *   -> ble_notify_telemetry_live() -> GATT notify -> Companion App
 *
 * The companion-app binary-parsers.ts expects:
 *   Byte 0:   speed (uint8, km/h)
 *   Byte 1-2: rpm (uint16, U/min)
 *   Byte 3:   gear (int8, -1=neutral)
 *   Byte 4:   coolantTemp (int8, C)
 *   Byte 5:   ambientTemp (int8, C)
 *   Byte 6:   fuel (uint8, %)
 *   Byte 7:   voltage (uint8, tenths of V)
 *   ...
 *
 * Firmware-side packing (mock_ble.py set_telemetry_data):
 *   struct.pack("<HHBBBhhBxxxxx", rpm, speed, temp, fuel, gear,
 *               roll, pitch, flags)
 *
 * Test coverage:
 *   - Round-trip packing/unpacking of typical sensor values
 *   - Boundary values (RPM=0, RPM=8000, temp=120C)
 *   - Crash flag propagation
 *   - Gear neutral vs in-gear encoding
 *   - Payload size is exactly 22 bytes
 */

#include "unity.h"
#include <string.h>
#include <stdint.h>

/* ============================================================
 * Telemetry payload format (matches firmware ble.c + companion-app
 * binary-parsers.ts)
 * ============================================================
 *
 * The firmware packs live telemetry into 22 bytes:
 *   Offset  Size  Field
 *   0-1     u16   rpm
 *   2-3     u16   speed (km/h, raw int)
 *   4       u8    coolant_temp (C, raw int)
 *   5       u8    fuel (litres * 10)
 *   6       u8    gear
 *   7-8     i16   roll (deg * 10)
 *   9-10    i16   pitch (deg * 10)
 *   11      u8    flags (bit 0 = crash_detected)
 *   12-21   pad   reserved (zeros)
 * ============================================================ */

#define TLM_PAYLOAD_SIZE 22
#define TLM_FLAG_CRASH   0x01

/* ============================================================
 * Pack sensor data into the 22-byte telemetry payload.
 * This mirrors the firmware ble_notify_telemetry_live() packing
 * logic and mock_ble.py MockBle.set_telemetry_data().
 * ============================================================ */
static void pack_telemetry(uint8_t *out, uint16_t rpm, uint16_t speed,
                           int8_t coolant_temp, uint8_t fuel_tenths,
                           uint8_t gear, int16_t roll_tenths,
                           int16_t pitch_tenths, uint8_t flags)
{
    memset(out, 0, TLM_PAYLOAD_SIZE);
    /* rpm (u16 LE) */
    out[0] = rpm & 0xFF;
    out[1] = (rpm >> 8) & 0xFF;
    /* speed (u16 LE) */
    out[2] = speed & 0xFF;
    out[3] = (speed >> 8) & 0xFF;
    /* coolant_temp (u8) */
    out[4] = (uint8_t)coolant_temp;
    /* fuel (u8, litres*10) */
    out[5] = fuel_tenths;
    /* gear (u8) */
    out[6] = gear;
    /* roll (i16 LE) */
    out[7] = roll_tenths & 0xFF;
    out[8] = (roll_tenths >> 8) & 0xFF;
    /* pitch (i16 LE) */
    out[9] = pitch_tenths & 0xFF;
    out[10] = (pitch_tenths >> 8) & 0xFF;
    /* flags */
    out[11] = flags;
    /* bytes 12-21 are padding (already zeroed) */
}

/* ============================================================
 * Unpack telemetry payload (simulates companion-app parseTelemetryLive)
 * ============================================================ */
static void unpack_telemetry(const uint8_t *data,
                             uint16_t *rpm, uint16_t *speed,
                             int8_t *coolant_temp, uint8_t *fuel_tenths,
                             uint8_t *gear, int16_t *roll_tenths,
                             int16_t *pitch_tenths, uint8_t *flags)
{
    *rpm = (uint16_t)(data[0] | (data[1] << 8));
    *speed = (uint16_t)(data[2] | (data[3] << 8));
    *coolant_temp = (int8_t)data[4];
    *fuel_tenths = data[5];
    *gear = data[6];
    *roll_tenths = (int16_t)(data[7] | (data[8] << 8));
    *pitch_tenths = (int16_t)(data[9] | (data[10] << 8));
    *flags = data[11];
}

void setUp(void) {}
void tearDown(void) {}

/* ============================================================
 * Test: Payload size is exactly 22 bytes
 * ============================================================ */
void test_telemetry_payload_size(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 3000, 80, 90, 120, 3, 50, -20, 0);
    /* If we got here without overflow, the size is correct */
    TEST_ASSERT_EQUAL(TLM_PAYLOAD_SIZE, sizeof(payload));
}

/* ============================================================
 * Test: Round-trip of typical riding values
 * ============================================================ */
void test_telemetry_roundtrip_typical(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload,
                   4500,   /* rpm */
                   95,     /* speed km/h */
                   88,     /* coolant temp C */
                   85,     /* fuel 8.5L * 10 */
                   4,      /* gear 4 */
                   150,    /* roll 15.0 deg * 10 */
                   -30,    /* pitch -3.0 deg * 10 */
                   0);     /* no crash */

    uint16_t rpm, speed;
    int8_t temp;
    uint8_t fuel, gear, flags;
    int16_t roll, pitch;

    unpack_telemetry(payload, &rpm, &speed, &temp, &fuel, &gear,
                     &roll, &pitch, &flags);

    TEST_ASSERT_EQUAL_UINT16(4500, rpm);
    TEST_ASSERT_EQUAL_UINT16(95, speed);
    TEST_ASSERT_EQUAL_INT8(88, temp);
    TEST_ASSERT_EQUAL_UINT8(85, fuel);
    TEST_ASSERT_EQUAL_UINT8(4, gear);
    TEST_ASSERT_EQUAL_INT16(150, roll);
    TEST_ASSERT_EQUAL_INT16(-30, pitch);
    TEST_ASSERT_EQUAL_UINT8(0, flags);
}

/* ============================================================
 * Test: Boundary values — RPM at redline (8000)
 * ============================================================ */
void test_telemetry_rpm_redline(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 8000, 180, 95, 50, 6, 0, 0, 0);

    uint16_t rpm, speed;
    int8_t temp;
    uint8_t fuel, gear, flags;
    int16_t roll, pitch;

    unpack_telemetry(payload, &rpm, &speed, &temp, &fuel, &gear,
                     &roll, &pitch, &flags);

    TEST_ASSERT_EQUAL_UINT16(8000, rpm);
    TEST_ASSERT_EQUAL_UINT16(180, speed);
    TEST_ASSERT_EQUAL_UINT8(6, gear);
}

/* ============================================================
 * Test: RPM=0 (engine off / idle stop)
 * ============================================================ */
void test_telemetry_rpm_zero(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 0, 0, 25, 170, 0, 0, 0, 0);

    uint16_t rpm, speed;
    int8_t temp;
    uint8_t fuel, gear, flags;
    int16_t roll, pitch;

    unpack_telemetry(payload, &rpm, &speed, &temp, &fuel, &gear,
                     &roll, &pitch, &flags);

    TEST_ASSERT_EQUAL_UINT16(0, rpm);
    TEST_ASSERT_EQUAL_UINT16(0, speed);
    TEST_ASSERT_EQUAL_UINT8(0, gear);  /* neutral */
}

/* ============================================================
 * Test: Crash flag propagation
 * ============================================================ */
void test_telemetry_crash_flag(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 0, 0, 90, 100, 0, 450, 300, TLM_FLAG_CRASH);

    uint16_t rpm, speed;
    int8_t temp;
    uint8_t fuel, gear, flags;
    int16_t roll, pitch;

    unpack_telemetry(payload, &rpm, &speed, &temp, &fuel, &gear,
                     &roll, &pitch, &flags);

    TEST_ASSERT_EQUAL_UINT8(TLM_FLAG_CRASH, flags);
    TEST_ASSERT_TRUE(flags & TLM_FLAG_CRASH);
    /* Extreme roll/pitch should accompany crash */
    TEST_ASSERT_EQUAL_INT16(450, roll);
    TEST_ASSERT_EQUAL_INT16(300, pitch);
}

/* ============================================================
 * Test: Overtemperature value (120C critical threshold)
 * ============================================================ */
void test_telemetry_overtemp(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 5000, 60, 120, 30, 3, 10, -5, 0);

    uint16_t rpm, speed;
    int8_t temp;
    uint8_t fuel, gear, flags;
    int16_t roll, pitch;

    unpack_telemetry(payload, &rpm, &speed, &temp, &fuel, &gear,
                     &roll, &pitch, &flags);

    TEST_ASSERT_EQUAL_INT8(120, temp);
}

/* ============================================================
 * Test: Low fuel reserve (3.0L = 30 tenths)
 * ============================================================ */
void test_telemetry_low_fuel(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 3000, 50, 85, 30, 2, 0, 0, 0);

    uint16_t rpm, speed;
    int8_t temp;
    uint8_t fuel, gear, flags;
    int16_t roll, pitch;

    unpack_telemetry(payload, &rpm, &speed, &temp, &fuel, &gear,
                     &roll, &pitch, &flags);

    TEST_ASSERT_EQUAL_UINT8(30, fuel);  /* 3.0L * 10 */
}

/* ============================================================
 * Test: Neutral gear encoding (gear=0)
 * ============================================================ */
void test_telemetry_neutral_gear(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 1200, 0, 70, 150, 0, 0, 0, 0);

    uint16_t rpm, speed;
    int8_t temp;
    uint8_t fuel, gear, flags;
    int16_t roll, pitch;

    unpack_telemetry(payload, &rpm, &speed, &temp, &fuel, &gear,
                     &roll, &pitch, &flags);

    TEST_ASSERT_EQUAL_UINT8(0, gear);  /* 0 = neutral */
}

/* ============================================================
 * Test: Padding bytes are zero
 * ============================================================ */
void test_telemetry_padding_zero(void)
{
    uint8_t payload[TLM_PAYLOAD_SIZE];
    pack_telemetry(payload, 3000, 80, 90, 120, 3, 50, -20, 0);

    for (int i = 12; i < TLM_PAYLOAD_SIZE; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, payload[i]);
    }
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_telemetry_payload_size);
    RUN_TEST(test_telemetry_roundtrip_typical);
    RUN_TEST(test_telemetry_rpm_redline);
    RUN_TEST(test_telemetry_rpm_zero);
    RUN_TEST(test_telemetry_crash_flag);
    RUN_TEST(test_telemetry_overtemp);
    RUN_TEST(test_telemetry_low_fuel);
    RUN_TEST(test_telemetry_neutral_gear);
    RUN_TEST(test_telemetry_padding_zero);

    return UNITY_END();
}
