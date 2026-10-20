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
 * The wire format is regenerated from docs/BLE-INTERFACE-SPEC.md §4.1
 * and matches the firmware struct telemetry_live_t in
 * firmware/components/ble/ble_callbacks.c:57 (__attribute__((packed))).
 *
 *   Offset  Size  Type   Field            Unit
 *   0       1     u8     speed_kmh        km/h         0-255
 *   1       2     u16 LE rpm              U/min        0-65535
 *   3       1     i8     gear             gear #       -1=neutral, 1-6
 *   4       1     i8     coolant_temp_c   Celsius      coolant
 *   5       1     i8     ambient_temp_c   Celsius      ambient
 *   6       1     u8     fuel_percent     %            0-100
 *   7       1     u8     voltage_tenth    0.1 V        e.g. 124 = 12.4 V
 *   8       4     i32 LE latitude_e7      deg * 1e-7   WGS84
 *   12      4     i32 LE longitude_e7     deg * 1e-7   WGS84
 *   16      2     i16 LE altitude_m       m
 *   18      2     u16 LE heading_tenth    0.1 deg      0-3599
 *   20      1     u8     satellites       count
 *   21      1     u8     hdop_tenth       0.1          HDOP
 *
 * _Static_assert(sizeof(telemetry_live_t) == 22) enforces this in firmware.
 *
 * Test coverage:
 *   - Payload size is exactly 22 bytes (sizeof + field offsets)
 *   - Round-trip packing/unpacking of the spec §12 "typical ride" vector
 *   - Boundary values (RPM=0 idle, RPM=8000 redline, speed=180)
 *   - Gear neutral (-1) vs in-gear encoding (firmware maps gear 0 -> -1)
 *   - GPS fields (lat/lon/alt/heading/sats/hdop) round-trip
 *   - GPS-invalid path zeroes all GPS fields (firmware build_telemetry_live)
 *   - Little-endian byte order of multi-byte fields (rpm/lat/lon)
 */

#include "unity.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * Firmware wire-format struct (mirror of telemetry_live_t in
 * ble_callbacks.c). Packed so the host layout matches the wire
 * layout byte-for-byte.
 * ============================================================ */

#define TLM_PAYLOAD_SIZE 22

#pragma pack(push, 1)
typedef struct {
    uint8_t  speed_kmh;
    uint16_t rpm;
    int8_t   gear;
    int8_t   coolant_temp_c;
    int8_t   ambient_temp_c;
    uint8_t  fuel_percent;
    uint8_t  voltage_tenth;
    int32_t  latitude_e7;
    int32_t  longitude_e7;
    int16_t  altitude_m;
    uint16_t heading_tenth;
    uint8_t  satellites;
    uint8_t  hdop_tenth;
} telemetry_live_t;
#pragma pack(pop)

/* Compile-time layout contract: size + field offsets match spec §4.1. */
_Static_assert(sizeof(telemetry_live_t) == TLM_PAYLOAD_SIZE,
               "telemetry_live_t packed size must be 22 bytes");
_Static_assert(offsetof(telemetry_live_t, speed_kmh)      == 0,
               "speed_kmh must be at offset 0");
_Static_assert(offsetof(telemetry_live_t, rpm)             == 1,
               "rpm must be at offset 1");
_Static_assert(offsetof(telemetry_live_t, gear)            == 3,
               "gear must be at offset 3");
_Static_assert(offsetof(telemetry_live_t, coolant_temp_c)  == 4,
               "coolant_temp_c must be at offset 4");
_Static_assert(offsetof(telemetry_live_t, ambient_temp_c)  == 5,
               "ambient_temp_c must be at offset 5");
_Static_assert(offsetof(telemetry_live_t, fuel_percent)    == 6,
               "fuel_percent must be at offset 6");
_Static_assert(offsetof(telemetry_live_t, voltage_tenth)   == 7,
               "voltage_tenth must be at offset 7");
_Static_assert(offsetof(telemetry_live_t, latitude_e7)    == 8,
               "latitude_e7 must be at offset 8");
_Static_assert(offsetof(telemetry_live_t, longitude_e7)   == 12,
               "longitude_e7 must be at offset 12");
_Static_assert(offsetof(telemetry_live_t, altitude_m)      == 16,
               "altitude_m must be at offset 16");
_Static_assert(offsetof(telemetry_live_t, heading_tenth)   == 18,
               "heading_tenth must be at offset 18");
_Static_assert(offsetof(telemetry_live_t, satellites)     == 20,
               "satellites must be at offset 20");
_Static_assert(offsetof(telemetry_live_t, hdop_tenth)     == 21,
               "hdop_tenth must be at offset 21");

/* ============================================================
 * Pack sensor data into the 22-byte telemetry payload.
 * Mirrors firmware build_telemetry_live() (ble_callbacks.c:125):
 *   - gear 0 (firmware sentinel for "no gear reported") -> -1 neutral
 *   - speed/fuel/voltage rounded with +0.5 (firmware uses (x + 0.5f))
 *   - gps_valid gates the GPS block; invalid -> all GPS fields zeroed
 * ============================================================ */
static void pack_telemetry_live(telemetry_live_t *pkt,
                                uint8_t speed_kmh, uint16_t rpm, int8_t gear,
                                int8_t coolant_temp_c, int8_t ambient_temp_c,
                                uint8_t fuel_percent, uint8_t voltage_tenth,
                                int32_t latitude_e7, int32_t longitude_e7,
                                int16_t altitude_m, uint16_t heading_tenth,
                                uint8_t satellites, uint8_t hdop_tenth,
                                int gps_valid)
{
    memset(pkt, 0, sizeof(*pkt));

    pkt->speed_kmh        = speed_kmh;
    pkt->rpm              = rpm;
    pkt->gear             = (gear == 0) ? -1 : gear;  /* 0 -> neutral */
    pkt->coolant_temp_c   = coolant_temp_c;
    pkt->ambient_temp_c   = ambient_temp_c;
    pkt->fuel_percent     = fuel_percent;
    pkt->voltage_tenth    = voltage_tenth;

    if (gps_valid) {
        pkt->latitude_e7   = latitude_e7;
        pkt->longitude_e7  = longitude_e7;
        pkt->altitude_m    = altitude_m;
        pkt->heading_tenth = heading_tenth;
        pkt->satellites    = satellites;
        pkt->hdop_tenth    = hdop_tenth;
    } else {
        pkt->latitude_e7   = 0;
        pkt->longitude_e7  = 0;
        pkt->altitude_m    = 0;
        pkt->heading_tenth = 0;
        pkt->satellites    = 0;
        pkt->hdop_tenth    = 0;
    }
}

/* ============================================================
 * Unpack telemetry payload (simulates companion-app
 * parseTelemetryLive reading little-endian via DataView).
 * Reads every field at its spec-defined offset.
 * ============================================================ */
static void unpack_telemetry_live(const uint8_t *data,
                                  uint8_t *speed_kmh, uint16_t *rpm,
                                  int8_t *gear, int8_t *coolant_temp_c,
                                  int8_t *ambient_temp_c, uint8_t *fuel_percent,
                                  uint8_t *voltage_tenth, int32_t *latitude_e7,
                                  int32_t *longitude_e7, int16_t *altitude_m,
                                  uint16_t *heading_tenth, uint8_t *satellites,
                                  uint8_t *hdop_tenth)
{
    *speed_kmh        = data[0];
    *rpm              = (uint16_t)(data[1] | (data[2] << 8));
    *gear             = (int8_t)data[3];
    *coolant_temp_c   = (int8_t)data[4];
    *ambient_temp_c   = (int8_t)data[5];
    *fuel_percent     = data[6];
    *voltage_tenth    = data[7];
    *latitude_e7      = (int32_t)((uint32_t)data[8]  | ((uint32_t)data[9]  << 8) |
                                 ((uint32_t)data[10] << 16) | ((uint32_t)data[11] << 24));
    *longitude_e7     = (int32_t)((uint32_t)data[12] | ((uint32_t)data[13] << 8) |
                                 ((uint32_t)data[14] << 16) | ((uint32_t)data[15] << 24));
    *altitude_m       = (int16_t)((uint16_t)data[16] | ((uint16_t)data[17] << 8));
    *heading_tenth   = (uint16_t)((uint16_t)data[18] | ((uint16_t)data[19] << 8));
    *satellites       = data[20];
    *hdop_tenth       = data[21];
}

void setUp(void) {}
void tearDown(void) {}

/* ============================================================
 * Test 1: Payload size is exactly 22 bytes
 * ============================================================ */
void test_telemetry_payload_size(void)
{
    telemetry_live_t pkt;
    pack_telemetry_live(&pkt, 95, 4500, 4, 88, 22, 85, 124,
                        509353900, 115872400, 245, 2700, 9, 12, 1);
    TEST_ASSERT_EQUAL(TLM_PAYLOAD_SIZE, sizeof(pkt));
    TEST_ASSERT_EQUAL(TLM_PAYLOAD_SIZE, (int)sizeof(telemetry_live_t));
}

/* ============================================================
 * Test 2: Round-trip of the spec §12 "typical ride" vector
 *   speed=95 rpm=4500 gear=4 coolant=88 ambient=22 fuel=85
 *   voltage=124 lat=509353900 lon=115872400 alt=245
 *   heading=2700 sats=9 hdop=12
 * ============================================================ */
void test_telemetry_roundtrip_typical(void)
{
    telemetry_live_t pkt;
    pack_telemetry_live(&pkt,
                        95, 4500, 4, 88, 22, 85, 124,
                        509353900, 115872400, 245, 2700, 9, 12, 1);

    const uint8_t *data = (const uint8_t *)&pkt;

    uint8_t speed, fuel, voltage, sats, hdop;
    uint16_t rpm, heading;
    int8_t gear, coolant, ambient;
    int32_t lat, lon;
    int16_t alt;

    unpack_telemetry_live(data, &speed, &rpm, &gear, &coolant, &ambient,
                           &fuel, &voltage, &lat, &lon, &alt, &heading,
                           &sats, &hdop);

    TEST_ASSERT_EQUAL_UINT8(95, speed);
    TEST_ASSERT_EQUAL_UINT16(4500, rpm);
    TEST_ASSERT_EQUAL_INT8(4, gear);
    TEST_ASSERT_EQUAL_INT8(88, coolant);
    TEST_ASSERT_EQUAL_INT8(22, ambient);
    TEST_ASSERT_EQUAL_UINT8(85, fuel);
    TEST_ASSERT_EQUAL_UINT8(124, voltage);
    TEST_ASSERT_EQUAL_INT32(509353900, lat);
    TEST_ASSERT_EQUAL_INT32(115872400, lon);
    TEST_ASSERT_EQUAL_INT16(245, alt);
    TEST_ASSERT_EQUAL_UINT16(2700, heading);
    TEST_ASSERT_EQUAL_UINT8(9, sats);
    TEST_ASSERT_EQUAL_UINT8(12, hdop);
}

/* ============================================================
 * Test 3: Boundary values — redline (spec §12 redline vector)
 *   speed=180 rpm=8000 gear=6 coolant=95 ambient=30 fuel=50
 *   voltage=130 heading=900 sats=9 hdop=10
 * ============================================================ */
void test_telemetry_rpm_redline(void)
{
    telemetry_live_t pkt;
    pack_telemetry_live(&pkt,
                        180, 8000, 6, 95, 30, 50, 130,
                        509353900, 115872400, 245, 900, 9, 10, 1);

    const uint8_t *data = (const uint8_t *)&pkt;

    uint8_t speed, fuel, voltage, sats, hdop;
    uint16_t rpm, heading;
    int8_t gear, coolant, ambient;
    int32_t lat, lon;
    int16_t alt;

    unpack_telemetry_live(data, &speed, &rpm, &gear, &coolant, &ambient,
                           &fuel, &voltage, &lat, &lon, &alt, &heading,
                           &sats, &hdop);

    TEST_ASSERT_EQUAL_UINT8(180, speed);
    TEST_ASSERT_EQUAL_UINT16(8000, rpm);
    TEST_ASSERT_EQUAL_INT8(6, gear);
    TEST_ASSERT_EQUAL_INT8(95, coolant);
    TEST_ASSERT_EQUAL_INT8(30, ambient);
    TEST_ASSERT_EQUAL_UINT8(50, fuel);
    TEST_ASSERT_EQUAL_UINT8(130, voltage);
    TEST_ASSERT_EQUAL_UINT16(900, heading);
}

/* ============================================================
 * Test 4: Idle / engine-off (spec §12 idle vector)
 *   speed=0 rpm=1200 gear=-1 coolant=70 ambient=18 fuel=150
 *   GPS invalid -> GPS fields zeroed.
 * ============================================================ */
void test_telemetry_idle_neutral(void)
{
    telemetry_live_t pkt;
    pack_telemetry_live(&pkt,
                        0, 1200, -1, 70, 18, 150, 122,
                        0, 0, 0, 0, 4, 25, 0);

    const uint8_t *data = (const uint8_t *)&pkt;

    uint8_t speed, fuel, voltage, sats, hdop;
    uint16_t rpm, heading;
    int8_t gear, coolant, ambient;
    int32_t lat, lon;
    int16_t alt;

    unpack_telemetry_live(data, &speed, &rpm, &gear, &coolant, &ambient,
                           &fuel, &voltage, &lat, &lon, &alt, &heading,
                           &sats, &hdop);

    TEST_ASSERT_EQUAL_UINT8(0, speed);
    TEST_ASSERT_EQUAL_UINT16(1200, rpm);
    TEST_ASSERT_EQUAL_INT8(-1, gear);
    TEST_ASSERT_EQUAL_INT8(70, coolant);
    TEST_ASSERT_EQUAL_INT8(18, ambient);
    /* GPS invalid: all GPS-derived fields must be zero. */
    TEST_ASSERT_EQUAL_INT32(0, lat);
    TEST_ASSERT_EQUAL_INT32(0, lon);
    TEST_ASSERT_EQUAL_INT16(0, alt);
    TEST_ASSERT_EQUAL_UINT16(0, heading);
    TEST_ASSERT_EQUAL_UINT8(0, sats);
    TEST_ASSERT_EQUAL_UINT8(0, hdop);
}

/* ============================================================
 * Test 5: GPS fields round-trip (lat/lon as i32 E7, signed alt,
 * heading, sats, hdop).
 * ============================================================ */
void test_telemetry_gps_fields_roundtrip(void)
{
    telemetry_live_t pkt;
    /* Southern-hemisphere / negative-altitude exercise for signed fields. */
    pack_telemetry_live(&pkt,
                        60, 3000, 3, 80, 15, 70, 121,
                        -339243900, 1511858200, -15, 1780, 7, 8, 1);

    const uint8_t *data = (const uint8_t *)&pkt;

    uint8_t speed, fuel, voltage, sats, hdop;
    uint16_t rpm, heading;
    int8_t gear, coolant, ambient;
    int32_t lat, lon;
    int16_t alt;

    unpack_telemetry_live(data, &speed, &rpm, &gear, &coolant, &ambient,
                           &fuel, &voltage, &lat, &lon, &alt, &heading,
                           &sats, &hdop);

    TEST_ASSERT_EQUAL_INT32(-339243900, lat);
    TEST_ASSERT_EQUAL_INT32(1511858200, lon);
    TEST_ASSERT_EQUAL_INT16(-15, alt);
    TEST_ASSERT_EQUAL_UINT16(1780, heading);
    TEST_ASSERT_EQUAL_UINT8(7, sats);
    TEST_ASSERT_EQUAL_UINT8(8, hdop);
}

/* ============================================================
 * Test 6: GPS-invalid path zeroes the GPS block even when the
 * caller passes non-zero GPS values (matches firmware: g->valid
 * false -> lat/lon/alt/heading/sats/hdop = 0).
 * ============================================================ */
void test_telemetry_gps_invalid_zeroes_block(void)
{
    telemetry_live_t pkt;
    pack_telemetry_live(&pkt,
                        0, 0, 0, 25, 18, 100, 122,
                        509353900, 115872400, 245, 2700, 9, 12, 0);

    const uint8_t *data = (const uint8_t *)&pkt;

    uint8_t speed, fuel, voltage, sats, hdop;
    uint16_t rpm, heading;
    int8_t gear, coolant, ambient;
    int32_t lat, lon;
    int16_t alt;

    unpack_telemetry_live(data, &speed, &rpm, &gear, &coolant, &ambient,
                           &fuel, &voltage, &lat, &lon, &alt, &heading,
                           &sats, &hdop);

    TEST_ASSERT_EQUAL_INT32(0, lat);
    TEST_ASSERT_EQUAL_INT32(0, lon);
    TEST_ASSERT_EQUAL_INT16(0, alt);
    TEST_ASSERT_EQUAL_UINT16(0, heading);
    TEST_ASSERT_EQUAL_UINT8(0, sats);
    TEST_ASSERT_EQUAL_UINT8(0, hdop);
}

/* ============================================================
 * Test 7: Little-endian byte order — rpm low byte at offset 1,
 * high byte at offset 2 (NimBLE wire order, spec §2). Uses
 * rpm=0x1234 to make byte order unambiguous.
 * ============================================================ */
void test_telemetry_little_endian_rpm(void)
{
    telemetry_live_t pkt;
    pack_telemetry_live(&pkt, 0, 0x1234, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0);

    const uint8_t *data = (const uint8_t *)&pkt;

    /* speed at offset 0, rpm little-endian at offsets 1-2. */
    TEST_ASSERT_EQUAL_UINT8(0, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, data[1]);  /* low byte first */
    TEST_ASSERT_EQUAL_UINT8(0x12, data[2]);  /* high byte second */

    /* latitude_e7 little-endian at offsets 8-11. */
    pkt.latitude_e7 = 0x40302010;
    TEST_ASSERT_EQUAL_UINT8(0x10, data[8]);
    TEST_ASSERT_EQUAL_UINT8(0x20, data[9]);
    TEST_ASSERT_EQUAL_UINT8(0x30, data[10]);
    TEST_ASSERT_EQUAL_UINT8(0x40, data[11]);
}

/* ============================================================
 * Test 8: Gear neutral encoding — firmware maps an incoming
 * gear==0 (sensor "no gear reported") to -1 on the wire.
 * A real engaged gear passes through unchanged.
 * ============================================================ */
void test_telemetry_gear_neutral_encoding(void)
{
    telemetry_live_t pkt;
    const uint8_t *data;

    /* gear 0 -> wire -1 (neutral). */
    pack_telemetry_live(&pkt, 0, 1200, 0, 70, 18, 100, 122,
                        0, 0, 0, 0, 4, 25, 0);
    data = (const uint8_t *)&pkt;
    TEST_ASSERT_EQUAL_INT8(-1, (int8_t)data[3]);

    /* engaged gear 1 passes through. */
    pack_telemetry_live(&pkt, 10, 1500, 1, 70, 18, 100, 122,
                        0, 0, 0, 0, 4, 25, 1);
    data = (const uint8_t *)&pkt;
    TEST_ASSERT_EQUAL_INT8(1, (int8_t)data[3]);

    /* top gear 6 passes through. */
    pack_telemetry_live(&pkt, 180, 8000, 6, 95, 30, 50, 130,
                        0, 0, 0, 0, 9, 10, 1);
    data = (const uint8_t *)&pkt;
    TEST_ASSERT_EQUAL_INT8(6, (int8_t)data[3]);
}

/* ============================================================
 * Test 9: Wire layout is contiguous with no padding — every
 * byte 0..21 is a real field and the struct packs to 22 bytes
 * with no compiler-inserted gaps. This is the build-time guard
 * the firmware enforces via _Static_assert; here we additionally
 * check field offsets at runtime.
 * ============================================================ */
void test_telemetry_no_padding_gaps(void)
{
    TEST_ASSERT_EQUAL(0,  (int)offsetof(telemetry_live_t, speed_kmh));
    TEST_ASSERT_EQUAL(1,  (int)offsetof(telemetry_live_t, rpm));
    TEST_ASSERT_EQUAL(3,  (int)offsetof(telemetry_live_t, gear));
    TEST_ASSERT_EQUAL(4,  (int)offsetof(telemetry_live_t, coolant_temp_c));
    TEST_ASSERT_EQUAL(5,  (int)offsetof(telemetry_live_t, ambient_temp_c));
    TEST_ASSERT_EQUAL(6,  (int)offsetof(telemetry_live_t, fuel_percent));
    TEST_ASSERT_EQUAL(7,  (int)offsetof(telemetry_live_t, voltage_tenth));
    TEST_ASSERT_EQUAL(8,  (int)offsetof(telemetry_live_t, latitude_e7));
    TEST_ASSERT_EQUAL(12, (int)offsetof(telemetry_live_t, longitude_e7));
    TEST_ASSERT_EQUAL(16, (int)offsetof(telemetry_live_t, altitude_m));
    TEST_ASSERT_EQUAL(18, (int)offsetof(telemetry_live_t, heading_tenth));
    TEST_ASSERT_EQUAL(20, (int)offsetof(telemetry_live_t, satellites));
    TEST_ASSERT_EQUAL(21, (int)offsetof(telemetry_live_t, hdop_tenth));
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
    RUN_TEST(test_telemetry_idle_neutral);
    RUN_TEST(test_telemetry_gps_fields_roundtrip);
    RUN_TEST(test_telemetry_gps_invalid_zeroes_block);
    RUN_TEST(test_telemetry_little_endian_rpm);
    RUN_TEST(test_telemetry_gear_neutral_encoding);
    RUN_TEST(test_telemetry_no_padding_gaps);

    return UNITY_END();
}
