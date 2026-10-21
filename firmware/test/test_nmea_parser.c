/*
 * test_nmea_parser.c — Unit Tests fuer den NMEA-0183 Parser
 *
 * Testet die in firmware/components/gps/nmea_parser.c implementierten
 * Funktionen mit bekannten NMEA-Saetzen und erwarteten Werten.
 *
 * Getestet wird:
 *   - GGA-Satz: Position, Satelliten, Hoehe, Fix-Qualitaet
 *   - RMC-Satz: Position, Speed (knoten->km/h), Kurs, Datum, Status
 *   - VTG-Satz: Speed (km/h direkt), Kurs
 *   - Checksum-Validierung (gueltig, falsch, fehlend)
 *   - Fehlerbehandlung (unvollstaendig, leer, unbekannt)
 *   - Parser-Statistik
 *
 * Die Tests verwenden das Unity-Test-Framework und laufen auf dem
 * Host-PC mit GCC. Die ESP-IDF-Abhaengigkeiten werden ueber Mocks
 * ersetzt (esp_log -> stderr, freertos -> POSIX-Threads).
 */

#ifdef TEST_HOST
/* Host build: use mock headers instead of ESP-IDF */
#include "esp_idf_mock.h"
/* Define the types from gps.h that depend on FreeRTOS */
#define freertos_queue_h  1
#endif

#include "unity.h"

/* Pull in the gps.h types (gps_data_t, nmea_stats_t, etc.) */
#include "gps.h"

/* ---- Link to the actual parser implementation ---- */
/* We declare the API here so the test can call it directly. */
extern bool nmea_parse(const char *sentence, gps_data_t *out);
extern void gps_get_stats(nmea_stats_t *out);

/*
 * For host testing we need the nmea_parser.c implementation.
 * Instead of linking the .c file (which includes esp_log.h),
 * we include it directly with a mock override.
 */
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

/* Provide stubs for functions referenced in the parser */
static const char *s_tag_stub = "nmea";

/* Include the parser source directly so we test the real implementation */
#include "../../components/gps/nmea_parser.c"
#endif

/* ============================================================
 * Helper: compute NMEA checksum (same algorithm as the parser)
 * ============================================================ */
static uint8_t test_checksum(const char *sentence)
{
    const char *p = sentence;
    uint8_t cksum = 0;
    if (*p == '$') p++;
    while (*p && *p != '*') {
        cksum ^= (uint8_t)(*p);
        p++;
    }
    return cksum;
}

static char *append_checksum(const char *body)
{
    static char buf[256];
    uint8_t cksum = test_checksum(body);
    snprintf(buf, sizeof(buf), "%s*%02X", body, cksum);
    return buf;
}

/* ============================================================
 * setUp / tearDown
 * ============================================================ */

void setUp(void)
{
    /* Reset parser statistics before each test.
     * s_stats is a static variable in nmea_parser.c, but since
     * we included the .c file, we can reset it by calling parse
     * with invalid data, or we accept that stats accumulate.
     * For correctness, we note that stats are cumulative across
     * tests in this file. */
}

void tearDown(void)
{
}

/* ============================================================
 * Test: GGA gueltig — Position, Satelliten, Hoehe
 * ============================================================ */
void test_gga_valid_fix(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* $GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,*47 */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_EQUAL(GPS_FIX_3D, data.fix_quality); /* GGA quality 1 = 3D fix */
    TEST_ASSERT_EQUAL_UINT8(9, data.satellites);
    TEST_ASSERT_EQUAL_FLOAT(245.3f, data.altitude_m);
    TEST_ASSERT_EQUAL(14, data.hour);
    TEST_ASSERT_EQUAL(23, data.minute);
    TEST_ASSERT_EQUAL(1, data.second);

    /* Lat: 5056.1234 -> 50 + 56.1234/60 = 50.93539 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.93539f, data.lat);
    /* Lon: 01135.2345 -> 11 + 35.2345/60 = 11.58724 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 11.58724f, data.lon);
}

/* ============================================================
 * Test: GGA ohne Fix
 * ============================================================ */
void test_gga_no_fix(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *body = "$GNGGA,142301.0,,,,,0,00,99.0,,,,,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(data.has_fix);
    TEST_ASSERT_EQUAL_UINT8(0, data.satellites);
}

/* ============================================================
 * Test: RMC gueltig — Speed, Kurs, Datum
 * ============================================================ */
void test_rmc_valid_speed_course(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* $GNRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A */
    const char *body = "$GNRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix); /* RMC status A = active */

    /* Speed: 45.3 knots * 1.852 = 83.8956 km/h */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 83.8956f, data.speed_kmh);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 270.0f, data.course_deg);

    /* Datum: 131026 -> 13.10.2026 */
    TEST_ASSERT_EQUAL_UINT8(13, data.day);
    TEST_ASSERT_EQUAL_UINT8(10, data.month);
    TEST_ASSERT_EQUAL_UINT16(2026, data.year);
}

/* ============================================================
 * Test: RMC void (kein Fix)
 * ============================================================ */
void test_rmc_void_status(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *body = "$GNRMC,142301.0,V,,,,,,,131026,,,N";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(data.has_fix);
    TEST_ASSERT_FALSE(data.valid);
}

/* ============================================================
 * Test: VTG — Speed in km/h direkt
 * ============================================================ */
void test_vtg_speed_kmh(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *body = "$GNVTG,270.0,T,268.5,M,45.3,N,83.9,K";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    /* Field 6 = 83.9 km/h (direct) */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 83.9f, data.speed_kmh);
    /* Field 0 = 270.0 course */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 270.0f, data.course_deg);
}

/* ============================================================
 * Test: Checksum-Fehler
 * ============================================================ */
void test_checksum_failure(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Deliberately wrong checksum: *00 */
    const char *sentence = "$GPGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,*00";

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Unvollstaendiger Satz (kein Stern/Checksum)
 * ============================================================ */
void test_incomplete_sentence(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *sentence = "$GPGGA,142301.0,5056";  /* no * */

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Leerer Satz
 * ============================================================ */
void test_empty_sentence(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *sentence = "$\r\n";

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Unbekannter Satz-Typ
 * ============================================================ */
void test_unknown_sentence_type(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *body = "$GPXYZ,123,456,789";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: NULL-Eingabe
 * ============================================================ */
void test_null_input(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    bool result = nmea_parse(NULL, &data);
    TEST_ASSERT_FALSE(result);

    result = nmea_parse("$GPGGA*00", NULL);
    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Satz ohne $-Praefix
 * ============================================================ */
void test_no_dollar_prefix(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    bool result = nmea_parse("GPGGA,123*00", &data);
    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test: Mehrere Saetze nacheinander (Statistik)
 * ============================================================ */
void test_multi_sentence_stats(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Parse 3 valid sentences */
    nmea_parse(append_checksum("$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,"), &data);
    nmea_parse(append_checksum("$GNRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A"), &data);
    nmea_parse(append_checksum("$GNVTG,270.0,T,268.5,M,83.9,K,45.3,N"), &data);

    /* Parse 1 invalid (checksum error) */
    nmea_parse("$GPGGA,123*00", &data);

    nmea_stats_t stats;
    gps_get_stats(&stats);

    TEST_ASSERT_GREATER_OR_EQUAL(4, stats.total);     /* 4 calls total */
    TEST_ASSERT_GREATER_OR_EQUAL(3, stats.valid);     /* 3 valid */
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.checksum_failed); /* 1 checksum fail */
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.gga);
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.rmc);
    TEST_ASSERT_GREATER_OR_EQUAL(1, stats.vtg);
}

/* ============================================================
 * Test: GSV Satz (Satellites in View)
 * ============================================================ */
void test_gsv_sentence(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *body = "$GPGSV,3,1,12,01,40,083,46,02,35,045,42,03,30,120,40,04,25,200,35";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    /* GSV should be parsed but not modify gps_data fields */
}

/* ============================================================
 * Test: Negative Koordinaten (Sued / West)
 * ============================================================ */
void test_south_west_coordinates(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    const char *body = "$GNGGA,100000.0,3356.1234,S,15135.2345,W,1,05,1.0,50.0,M,,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    /* Lat: 3356.1234 S -> -(33 + 56.1234/60) = -33.93539 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -33.93539f, data.lat);
    /* Lon: 15135.2345 W -> -(151 + 35.2345/60) = -151.58724 */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -151.58724f, data.lon);
}

/* ============================================================
 * Test: Incremental update (bestehende Werte bleiben erhalten)
 * ============================================================ */
void test_incremental_update(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));
    data.altitude_m = 999.0f;  /* Pre-set value */

    /* Parse RMC (does not set altitude) */
    const char *body = "$GNRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    /* Altitude should be unchanged (RMC does not contain altitude) */
    TEST_ASSERT_EQUAL_FLOAT(999.0f, data.altitude_m);
}

/* ============================================================
 * Coverage expansion tests (TOT-59)
 *
 * These tests close gaps in the NMEA parser coverage:
 *   - GGA DGPS fix quality (quality=2)
 *   - GGA HDOP field parsing + valid threshold
 *   - VTG speed from knots when km/h field absent
 *   - Different talker prefixes (GN, GL)
 *   - Boundary coordinates (0,0)
 *   - Max latitude (9000.0000 N)
 *   - RMC missing speed field keeps zero
 *   - Stats accumulation across multiple GGA sentences
 *   - Checksum with single-char body
 * ============================================================ */

void test_gga_dgps_fix_quality(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* quality=2 -> GPS_FIX_DGPS */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,2,09,1.2,245.3,M,47.1,M,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_EQUAL(GPS_FIX_DGPS, data.fix_quality);
}

void test_gga_hdop_parsing(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* HDOP = 1.2 in field 7 */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,";
    char *sentence = append_checksum(body);

    nmea_parse(sentence, &data);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.2f, data.hdop);
}

void test_gga_hdop_valid_threshold(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* HDOP = 6.0 (> 5.0 threshold -> valid stays false even with fix) */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,6.0,245.3,M,47.1,M,,";
    char *sentence = append_checksum(body);

    nmea_parse(sentence, &data);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_FALSE(data.valid);  /* HDOP too high */
}

void test_vtg_knots_fallback(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* VTG with only knots speed (field 4), no km/h field 6.
     * 45.3 knots * 1.852 = 83.8956 km/h */
    const char *body = "$GNVTG,270.0,T,268.5,M,45.3,N,,K";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 83.8956f, data.speed_kmh);
}

void test_talker_prefix_gn(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* GLONASS talker prefix $GL — should parse the same as $GP */
    const char *body = "$GLGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_EQUAL_UINT8(9, data.satellites);
}

void test_talker_prefix_gl(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* $GN prefix (GNSS combined) */
    const char *body = "$GNGGA,142301.0,3356.1234,S,15135.2345,W,1,05,1.0,50.0,M,,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -33.93539f, data.lat);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -151.58724f, data.lon);
}

void test_boundary_coordinates_zero(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Lat=0000.0000 N, Lon=00000.0000 E -> (0.0, 0.0) */
    const char *body = "$GNGGA,142301.0,0000.0000,N,00000.0000,E,1,09,1.2,0.0,M,,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.lat);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.lon);
}

void test_max_latitude(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Lat=9000.0000 N -> 90.0 degrees */
    const char *body = "$GNGGA,142301.0,9000.0000,N,01135.2345,E,1,09,1.2,245.3,M,,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, data.lat);
}

void test_rmc_missing_speed_keeps_zero(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* RMC with empty speed field -> speed stays 0 */
    const char *body = "$GNRMC,142301.0,A,5056.1234,N,01135.2345,E,,270.0,131026,,,A";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.speed_kmh);
}

void test_multiple_gga_stats_accumulate(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    nmea_stats_t before;
    gps_get_stats(&before);

    /* Parse 3 GGA sentences */
    for (int i = 0; i < 3; i++) {
        char *sentence = append_checksum(
            "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,");
        nmea_parse(sentence, &data);
    }

    nmea_stats_t after;
    gps_get_stats(&after);

    TEST_ASSERT_EQUAL(before.gga + 3, after.gga);
    TEST_ASSERT_EQUAL(before.valid + 3, after.valid);
}

void test_checksum_single_char(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Minimal valid sentence body with a single char before the '*' */
    const char *body = "$GNGGA";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);
    /* GGA with no data fields — parser should still accept the
     * checksum (it's valid) but not crash. */
    TEST_ASSERT_TRUE(result);
}

/* ============================================================
 * Coverage expansion (TOT-62): malformed/empty-field edge cases
 * ============================================================ */

void test_checksum_lowercase_hex_accepted(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* strtol(base=16) accepts lowercase — build a sentence whose
     * checksum has a-f digits and verify it is accepted. */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,,,";
    uint8_t calc = test_checksum(body);
    char lower[256];
    snprintf(lower, sizeof(lower), "%s*%02x", body, calc);

    bool result = nmea_parse(lower, &data);
    TEST_ASSERT_TRUE(result);
}

void test_checksum_no_hex_digits_rejected(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Sentence with '*' but no hex digits after it */
    const char *sentence = "$GNGGA,142301.0*";
    bool result = nmea_parse(sentence, &data);
    TEST_ASSERT_FALSE(result);
}

void test_rmc_empty_date_keeps_zero(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* RMC with empty date field (consecutive commas) */
    const char *body = "$GNRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(0, data.day);
    TEST_ASSERT_EQUAL_UINT8(0, data.month);
    TEST_ASSERT_EQUAL_UINT16(0, data.year);
}

void test_rmc_empty_coordinates_kept_zero(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* RMC active but with empty coordinate fields */
    const char *body = "$GNRMC,142301.0,A,,,,,45.3,270.0,131026,,,A";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.lat);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, data.lon);
}

void test_gga_empty_fix_quality_no_fix(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* GGA with empty fix-quality field (field 5 empty) */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,,09,1.2,245.3,M,,,";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(data.has_fix);  /* empty quality -> no fix */
}

void test_vtg_empty_course_keeps_zero(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* VTG with empty course field, only speed */
    const char *body = "$GNVTG,,T,,M,45.3,N,83.9,K";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.course_deg);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 83.9f, data.speed_kmh);
}

void test_gga_hdop_boundary_5_0_invalid(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* HDOP exactly 5.0 — valid check is hdop < 5.0, so 5.0 → invalid */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,5.0,245.3,M,,,";
    char *sentence = append_checksum(body);

    nmea_parse(sentence, &data);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_FALSE(data.valid);  /* exactly 5.0 is NOT < 5.0 */
}

void test_gga_hdop_just_below_5_valid(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* HDOP 4.99 — just below threshold → valid */
    const char *body = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,4.99,245.3,M,,,";
    char *sentence = append_checksum(body);

    nmea_parse(sentence, &data);
    TEST_ASSERT_TRUE(data.has_fix);
    TEST_ASSERT_TRUE(data.valid);
}

void test_rmc_time_parsing(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* RMC with time 093015.5 = 09:30:15 */
    const char *body = "$GNRMC,093015.5,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A";
    char *sentence = append_checksum(body);

    nmea_parse(sentence, &data);
    TEST_ASSERT_EQUAL_UINT8(9, data.hour);
    TEST_ASSERT_EQUAL_UINT8(30, data.minute);
    TEST_ASSERT_EQUAL_UINT8(15, data.second);
}

void test_sentence_no_star_rejected(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Valid-looking sentence but no '*' delimiter for checksum */
    const char *sentence = "$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3";
    bool result = nmea_parse(sentence, &data);
    TEST_ASSERT_FALSE(result);
}

void test_short_talker_prefix_rejected(void)
{
    gps_data_t data;
    memset(&data, 0, sizeof(data));

    /* Only '$G' before type — get_sentence_type needs $ + 2 talker chars */
    const char *body = "$GGGA,142301.0";
    char *sentence = append_checksum(body);

    bool result = nmea_parse(sentence, &data);
    /* strlen(type) < 3 check in get_sentence_type */
    TEST_ASSERT_FALSE(result);
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_gga_valid_fix);
    RUN_TEST(test_gga_no_fix);
    RUN_TEST(test_rmc_valid_speed_course);
    RUN_TEST(test_rmc_void_status);
    RUN_TEST(test_vtg_speed_kmh);
    RUN_TEST(test_checksum_failure);
    RUN_TEST(test_incomplete_sentence);
    RUN_TEST(test_empty_sentence);
    RUN_TEST(test_unknown_sentence_type);
    RUN_TEST(test_null_input);
    RUN_TEST(test_no_dollar_prefix);
    RUN_TEST(test_multi_sentence_stats);
    RUN_TEST(test_gsv_sentence);
    RUN_TEST(test_south_west_coordinates);
    RUN_TEST(test_incremental_update);
    /* ---- Coverage expansion (TOT-59) ---- */
    RUN_TEST(test_gga_dgps_fix_quality);
    RUN_TEST(test_gga_hdop_parsing);
    RUN_TEST(test_vtg_knots_fallback);
    RUN_TEST(test_talker_prefix_gn);
    RUN_TEST(test_talker_prefix_gl);
    RUN_TEST(test_boundary_coordinates_zero);
    RUN_TEST(test_gga_hdop_valid_threshold);
    RUN_TEST(test_rmc_missing_speed_keeps_zero);
    RUN_TEST(test_multiple_gga_stats_accumulate);
    RUN_TEST(test_max_latitude);
    RUN_TEST(test_checksum_single_char);

    /* ---- Coverage expansion (TOT-62): malformed/empty-field edges ---- */
    RUN_TEST(test_checksum_lowercase_hex_accepted);
    RUN_TEST(test_checksum_no_hex_digits_rejected);
    RUN_TEST(test_rmc_empty_date_keeps_zero);
    RUN_TEST(test_rmc_empty_coordinates_kept_zero);
    RUN_TEST(test_gga_empty_fix_quality_no_fix);
    RUN_TEST(test_vtg_empty_course_keeps_zero);
    RUN_TEST(test_gga_hdop_boundary_5_0_invalid);
    RUN_TEST(test_gga_hdop_just_below_5_valid);
    RUN_TEST(test_rmc_time_parsing);
    RUN_TEST(test_sentence_no_star_rejected);
    RUN_TEST(test_short_talker_prefix_rejected);

    return UNITY_END();
}
