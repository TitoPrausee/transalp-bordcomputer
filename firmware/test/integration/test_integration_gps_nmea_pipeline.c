/*
 * test_integration_gps_nmea_pipeline.c
 *
 * Integration test: GPS NMEA Generation -> Firmware Parser Pipeline
 *
 * Verifies that NMEA sentences produced by the simulator's
 * NmeaGenerator (tools/simulator/mock_gps.py) are correctly parsed
 * by the firmware's nmea_parser.c. This is the critical cross-layer
 * integration point: the simulator generates NMEA-0183 sentences
 * (GGA, RMC, VTG) that must be accepted by the firmware parser.
 *
 * Data flow:
 *   GPX track -> NmeaGenerator -> NMEA sentence (string)
 *   -> UART -> nmea_parse() -> gps_data_t
 *
 * Test coverage:
 *   - GGA sentence parsing (fix, satellites, altitude, position)
 *   - RMC sentence parsing (speed, course, date/time, status)
 *   - VTG sentence parsing (speed km/h, course)
 *   - Multiple sentence types in sequence (incremental update)
 *   - Checksum validation (correct, wrong, missing)
 *   - Position accuracy (N/S, E/W hemispheres)
 *   - Speed unit conversion (knots -> km/h)
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#define freertos_queue_h 1
#endif

#include "unity.h"
#include "gps.h"

#include <string.h>
#include <math.h>

/* ============================================================
 * Mock ESP log for host build
 * ============================================================ */
#ifdef TEST_HOST
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV
#define ESP_LOGE(tag, fmt, ...)  (void)0
#define ESP_LOGW(tag, fmt, ...)  (void)0
#define ESP_LOGI(tag, fmt, ...)  (void)0
#define ESP_LOGD(tag, fmt, ...)  (void)0
#define ESP_LOGV(tag, fmt, ...)  (void)0

#include "../../components/gps/nmea_parser.c"
#endif

/* ============================================================
 * Helper: compute and append NMEA XOR checksum
 * This mirrors mock_gps.py NmeaGenerator._wrap()
 * ============================================================ */
static char *make_nmea(const char *body)
{
    static char buf[256];
    uint8_t cksum = 0;
    const char *p = body;
    while (*p) {
        cksum ^= (uint8_t)(*p);
        p++;
    }
    snprintf(buf, sizeof(buf), "$%s*%02X\r\n", body, cksum);
    return buf;
}

void setUp(void) {}
void tearDown(void) {}

/* ============================================================
 * Test: GGA sentence — full fix with all fields
 * Simulates output from NmeaGenerator._gen_gga()
 * ============================================================ */
void test_gga_full_fix(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* GPGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,, */
    char *sentence = make_nmea("GPGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,");

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_EQUAL(GPS_FIX_3D, data.fix_quality);
    TEST_ASSERT_EQUAL_UINT8(9, data.satellites);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 245.3f, data.altitude_m);
    /* Lat: 50 + 56.1234/60 = 50.93539 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.93539f, data.lat);
    /* Lon: 11 + 35.2345/60 = 11.58724 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 11.58724f, data.lon);
}

/* ============================================================
 * Test: RMC sentence — active fix with speed and course
 * Simulates output from NmeaGenerator._gen_rmc()
 * ============================================================ */
void test_rmc_active_fix(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    char *sentence = make_nmea("GPRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A");

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix);
    /* Speed: 45.3 knots * 1.852 = 83.8956 km/h */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 83.8956f, data.speed_kmh);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 270.0f, data.course_deg);
    /* Date: 13.10.2026 */
    TEST_ASSERT_EQUAL_UINT8(13, data.day);
    TEST_ASSERT_EQUAL_UINT8(10, data.month);
    TEST_ASSERT_EQUAL_UINT16(2026, data.year);
}

/* ============================================================
 * Test: VTG sentence — direct km/h speed
 * Simulates output from NmeaGenerator._gen_vtg()
 * ============================================================ */
void test_vtg_speed(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    char *sentence = make_nmea("GPVTG,270.0,T,,M,45.3,N,83.9,K,");

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 83.9f, data.speed_kmh);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 270.0f, data.course_deg);
}

/* ============================================================
 * Test: Incremental update — GGA then RMC preserves altitude
 * The parser should only update fields present in each sentence.
 * ============================================================ */
void test_incremental_update_preserves_altitude(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* First: GGA sets altitude */
    char *gga = make_nmea("GPGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,");
    nmea_parse(gga, &data);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 245.3f, data.altitude_m);

    /* Second: RMC does not contain altitude — should be preserved */
    char *rmc = make_nmea("GPRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A");
    nmea_parse(rmc, &data);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 245.3f, data.altitude_m);
    /* But speed should now be set */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 83.8956f, data.speed_kmh);
}

/* ============================================================
 * Test: Full pipeline — GGA + RMC + VTG in sequence
 * Simulates a complete GPS update tick from the simulator
 * ============================================================ */
void test_full_gps_pipeline(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Simulate NmeaGenerator producing GGA, RMC, VTG */
    nmea_parse(make_nmea("GPGGA,120000.0,4748.2345,N,01135.6789,E,1,09,1.2,620.5,M,47.1,M,,"), &data);
    nmea_parse(make_nmea("GPRMC,120000.0,A,4748.2345,N,01135.6789,E,60.0,180.5,201026,,,A"), &data);
    nmea_parse(make_nmea("GPVTG,180.5,T,,M,32.4,N,60.0,K,"), &data);

    /* All fields should now be populated */
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_EQUAL(GPS_FIX_3D, data.fix_quality);
    TEST_ASSERT_EQUAL_UINT8(9, data.satellites);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 620.5f, data.altitude_m);
    /* Lat: 47 + 48.2345/60 = 47.80392 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 47.80392f, data.lat);
    /* Speed: VTG gives 60.0 km/h directly */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.0f, data.speed_kmh);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 180.5f, data.course_deg);
}

/* ============================================================
 * Test: Southern/Western hemisphere coordinates
 * ============================================================ */
void test_south_west_hemisphere(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    nmea_parse(make_nmea("GPGGA,100000.0,3356.1234,S,15135.2345,W,1,05,1.0,50.0,M,,,"), &data);

    /* Lat: 3356.1234 S -> -(33 + 56.1234/60) = -33.93539 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -33.93539f, data.lat);
    /* Lon: 15135.2345 W -> -(151 + 35.2345/60) = -151.58724 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -151.58724f, data.lon);
}

/* ============================================================
 * Test: RMC void status (no fix)
 * ============================================================ */
void test_rmc_no_fix(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    nmea_parse(make_nmea("GPRMC,142301.0,V,,,,,,,131026,,,N"), &data);

    TEST_ASSERT_FALSE(data.has_fix);
    TEST_ASSERT_FALSE(data.valid);
}

/* ============================================================
 * Test: Wrong checksum is rejected
 * ============================================================ */
void test_wrong_checksum_rejected(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Deliberately wrong checksum */
    const char *sentence = "$GPGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,*00";
    bool result = nmea_parse(sentence, &data);
    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Parser statistics accumulate correctly
 * ============================================================ */
void test_parser_statistics(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    nmea_parse(make_nmea("GPGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,"), &data);
    nmea_parse(make_nmea("GPRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A"), &data);
    nmea_parse(make_nmea("GPVTG,270.0,T,,M,45.3,N,83.9,K,"), &data);
    nmea_parse("$GPGGA,123*00", &data);  /* bad checksum */

    nmea_stats_t stats;
    gps_get_stats(&stats);

    TEST_ASSERT_GREATER_OR_EQUAL(4, stats.total);
    TEST_ASSERT_GREATER_OR_EQUAL(3, stats.valid);
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.checksum_failed);
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.gga);
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.rmc);
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.vtg);
}

/* ============================================================
 * Test: High speed value (motorway cruising ~180 km/h)
 * ============================================================ */
void test_high_speed_value(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* 97.2 knots = 180.0 km/h */
    nmea_parse(make_nmea("GPVTG,90.0,T,,M,97.2,N,180.0,K,"), &data);

    TEST_ASSERT_FLOAT_WITHIN(0.5f, 180.0f, data.speed_kmh);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_gga_full_fix);
    RUN_TEST(test_rmc_active_fix);
    RUN_TEST(test_vtg_speed);
    RUN_TEST(test_incremental_update_preserves_altitude);
    RUN_TEST(test_full_gps_pipeline);
    RUN_TEST(test_south_west_hemisphere);
    RUN_TEST(test_rmc_no_fix);
    RUN_TEST(test_wrong_checksum_rejected);
    RUN_TEST(test_parser_statistics);
    RUN_TEST(test_high_speed_value);

    return UNITY_END();
}
