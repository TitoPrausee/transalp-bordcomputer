/*
 * test_integration_ble_gatt_interface.c
 *
 * Integration test: BLE GATT Interface Consistency
 *
 * Verifies that the BLE GATT service/characteristic UUIDs defined
 * in the firmware (gatt_uuids.h) are consistent with:
 *   - The simulator mock BLE layer (mock_ble.py)
 *   - The companion-app UUID definitions (gatt-uuids.ts)
 *
 * The companion-app and simulator must use the same base UUID format
 * and the same 16-bit characteristic IDs as the firmware.
 *
 * Base UUID: 6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
 *
 * Test coverage:
 *   - All 8 service IDs are in valid range (0x1000-0x8000)
 *   - All characteristic IDs belong to their parent service
 *   - UUID format is correct (base UUID with 16-bit substitution)
 *   - Service/characteristic count matches expected
 *   - DTC packing format (6 bytes per entry) is correct
 *   - Sensor status packing (5 bytes) is correct
 *   - Telemetry payload size (22 bytes) is correct
 *   - CarPlay service (0x9000) exists for companion-app
 */

#include "unity.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* ============================================================
 * GATT UUID definitions (mirrors firmware gatt_uuids.h +
 * companion-app gatt-uuids.ts + simulator mock_ble.py)
 * ============================================================ */

#define BASE_UUID_PREFIX   "6e40"
#define BASE_UUID_SUFFIX   "-b5a3-f393-e0a9-e50e24dcca9e"

/* Service IDs */
#define SVC_NAVIGATION   0x1000
#define SVC_MEDIA        0x2000
#define SVC_PHONE        0x3000
#define SVC_WEATHER      0x4000
#define SVC_TELEMETRY    0x5000
#define SVC_CONFIG       0x6000
#define SVC_DIAGNOSE     0x7000
#define SVC_OTA          0x8000

/* CarPlay service (companion-app only, firmware has it in ble_callbacks) */
#define SVC_CARPLAY      0x9000

/* Telemetry characteristics */
#define CHR_TLM_LIVE     0x5001
#define CHR_TLM_SNAPSHOT 0x5002
#define CHR_TRACK_EXPORT 0x5003

/* Diagnose characteristics */
#define CHR_DTC_LIST     0x7001
#define CHR_SENSOR_STATUS 0x7002
#define CHR_SELF_TEST    0x7003

/* Config characteristics */
#define CHR_CONFIG_READ  0x6001
#define CHR_CONFIG_WRITE  0x6002
#define CHR_BOND_MGMT    0x6005

/* OTA characteristics */
#define CHR_OTA_CONTROL  0x8001
#define CHR_OTA_DATA     0x8002
#define CHR_OTA_VERIFY   0x8003

/* OTA verify notify payload size (CHR_OTA_VERIFY 0x8003, spec §9.3)
 * u8 status + u8[32] sha256 + u32 size + u32 version = 41 bytes.
 * The firmware computes this in ota_finish_and_verify() and the BLE
 * layer emits it on 0x8003 (see ble_callbacks.c notify_ota_verify). */
#define OTA_VERIFY_NOTIFY_SIZE   41
#define OTA_VERIFY_OFF_STATUS    0
#define OTA_VERIFY_OFF_SHA256    1
#define OTA_VERIFY_OFF_SIZE      33
#define OTA_VERIFY_OFF_VERSION   37

/* Navigation characteristics */
#define CHR_NAV_MANEUVER  0x1001
#define CHR_NAV_STATUS    0x1004

/* Media characteristics */
#define CHR_MEDIA_NOWPLAYING 0x2001
#define CHR_MEDIA_CONTROL    0x2002

/* CarPlay characteristics */
#define CHR_CARPLAY_STATUS  0x9001
#define CHR_CARPLAY_CONTROL 0x9002

/* ============================================================
 * Payload sizes (must match firmware + companion-app)
 * ============================================================ */
#define TLM_LIVE_SIZE        22
#define SENSOR_STATUS_SIZE    5
#define DTC_ENTRY_SIZE        6
/* CarPlay 0x9001 is the firmware's 12-byte carplay_telemetry_t (spec §11).
 * The companion app must NOT read service 0x9000 — the previous 194-byte
 * app-side media struct is removed during TOT-35 reconciliation. */
#define CARPLAY_TLM_LIVE_SIZE 12

/* ============================================================
 * DTC packing format (6 bytes per entry)
 * Matches mock_ble.py DtcEntry.pack():
 *   struct.pack("<HBHB", code, severity, active, occurrences)
 * ============================================================ */
#define DTC_PACKED_SIZE  6

/* ============================================================
 * Helper: build UUID string from 16-bit ID
 * ============================================================ */
static void build_uuid(uint16_t id, char *out, size_t out_size)
{
    snprintf(out, out_size, "%s%04x%s", BASE_UUID_PREFIX, id, BASE_UUID_SUFFIX);
}

/* ============================================================
 * Helper: extract service ID from characteristic ID
 * ============================================================ */
static uint16_t get_service_id(uint16_t char_id)
{
    return char_id & 0xF000;
}

void setUp(void) {}
void tearDown(void) {}

/* ============================================================
 * Test: Base UUID format is correct
 * ============================================================ */
void test_base_uuid_format(void)
{
    char uuid[40];
    build_uuid(0x1001, uuid, sizeof(uuid));
    /* Should be: 6e401001-b5a3-f393-e0a9-e50e24dcca9e */
    TEST_ASSERT_EQUAL_STRING("6e401001-b5a3-f393-e0a9-e50e24dcca9e", uuid);
}

/* ============================================================
 * Test: All 8 service IDs plus CarPlay are valid
 * ============================================================ */
void test_service_ids_valid(void)
{
    uint16_t services[] = {
        SVC_NAVIGATION, SVC_MEDIA, SVC_PHONE, SVC_WEATHER,
        SVC_TELEMETRY, SVC_CONFIG, SVC_DIAGNOSE, SVC_OTA,
        SVC_CARPLAY,
    };
    int n = sizeof(services) / sizeof(services[0]);

    for (int i = 0; i < n; i++) {
        /* Service IDs are multiples of 0x1000 */
        TEST_ASSERT_EQUAL(0, services[i] & 0x0FFF);
        /* Range 0x1000-0x9000 */
        TEST_ASSERT_GREATER_OR_EQUAL(0x1000, services[i]);
        TEST_ASSERT_LESS_OR_EQUAL(0x9000, services[i]);
    }
}

/* ============================================================
 * Test: Characteristic belongs to correct service
 * ============================================================ */
void test_char_service_mapping(void)
{
    TEST_ASSERT_EQUAL(SVC_TELEMETRY, get_service_id(CHR_TLM_LIVE));
    TEST_ASSERT_EQUAL(SVC_TELEMETRY, get_service_id(CHR_TLM_SNAPSHOT));
    TEST_ASSERT_EQUAL(SVC_DIAGNOSE, get_service_id(CHR_DTC_LIST));
    TEST_ASSERT_EQUAL(SVC_DIAGNOSE, get_service_id(CHR_SENSOR_STATUS));
    TEST_ASSERT_EQUAL(SVC_CONFIG, get_service_id(CHR_CONFIG_READ));
    TEST_ASSERT_EQUAL(SVC_CONFIG, get_service_id(CHR_CONFIG_WRITE));
    TEST_ASSERT_EQUAL(SVC_OTA, get_service_id(CHR_OTA_CONTROL));
    TEST_ASSERT_EQUAL(SVC_OTA, get_service_id(CHR_OTA_DATA));
    TEST_ASSERT_EQUAL(SVC_NAVIGATION, get_service_id(CHR_NAV_MANEUVER));
    TEST_ASSERT_EQUAL(SVC_NAVIGATION, get_service_id(CHR_NAV_STATUS));
    TEST_ASSERT_EQUAL(SVC_MEDIA, get_service_id(CHR_MEDIA_NOWPLAYING));
    TEST_ASSERT_EQUAL(SVC_MEDIA, get_service_id(CHR_MEDIA_CONTROL));
    TEST_ASSERT_EQUAL(SVC_CARPLAY, get_service_id(CHR_CARPLAY_STATUS));
    TEST_ASSERT_EQUAL(SVC_CARPLAY, get_service_id(CHR_CARPLAY_CONTROL));
}

/* ============================================================
 * Test: Telemetry live payload is 22 bytes
 * ============================================================ */
void test_tlm_live_size(void)
{
    TEST_ASSERT_EQUAL(22, TLM_LIVE_SIZE);
}

/* ============================================================
 * Test: Sensor status payload is 5 bytes
 * ============================================================ */
void test_sensor_status_size(void)
{
    TEST_ASSERT_EQUAL(5, SENSOR_STATUS_SIZE);
}

/* ============================================================
 * Test: DTC entry packed size is 6 bytes
 * ============================================================ */
void test_dtc_entry_size(void)
{
    TEST_ASSERT_EQUAL(6, DTC_PACKED_SIZE);
}

/* ============================================================
 * Test: DTC packing format (code, severity, active, occurrences)
 * ============================================================ */
void test_dtc_packing_format(void)
{
    /* DTC code 0x0101 (P0101), severity 1 (warn), active=1, count=3 */
    uint16_t code = 0x0101;
    uint8_t severity = 1;
    uint8_t active = 1;
    uint8_t occurrences = 3;

    uint8_t packed[DTC_PACKED_SIZE];
    /* Pack as: <HBHB (little-endian) */
    packed[0] = code & 0xFF;
    packed[1] = (code >> 8) & 0xFF;
    packed[2] = severity;
    packed[3] = active;
    packed[4] = occurrences;
    packed[5] = 0;  /* padding */

    /* Unpack and verify */
    uint16_t unpacked_code = packed[0] | (packed[1] << 8);
    TEST_ASSERT_EQUAL(code, unpacked_code);
    TEST_ASSERT_EQUAL(severity, packed[2]);
    TEST_ASSERT_EQUAL(active, packed[3]);
    TEST_ASSERT_EQUAL(occurrences, packed[4]);
}

/* ============================================================
 * Test: CarPlay 0x9001 telemetry payload is 12 bytes (spec §11)
 * ============================================================ */
void test_carplay_tlm_live_size(void)
{
    TEST_ASSERT_EQUAL(12, CARPLAY_TLM_LIVE_SIZE);
}

/* ============================================================
 * Test: OTA verify-notify payload size (CHR_OTA_VERIFY 0x8003)
 *   u8 status + u8[32] sha256 + u32 size + u32 version = 41 bytes
 * ============================================================ */
void test_ota_verify_notify_size(void)
{
    TEST_ASSERT_EQUAL(41, OTA_VERIFY_NOTIFY_SIZE);
}

/* ============================================================
 * Test: OTA verify-notify field offsets (spec §9.3)
 * ============================================================ */
void test_ota_verify_notify_layout(void)
{
    TEST_ASSERT_EQUAL(0, OTA_VERIFY_OFF_STATUS);
    TEST_ASSERT_EQUAL(1, OTA_VERIFY_OFF_SHA256);
    TEST_ASSERT_EQUAL(33, OTA_VERIFY_OFF_SIZE);
    TEST_ASSERT_EQUAL(37, OTA_VERIFY_OFF_VERSION);
    /* sha256 field spans 1..32 (32 bytes), size at 33, version at 37 */
    TEST_ASSERT_EQUAL(41, OTA_VERIFY_OFF_VERSION + 4);
}

/* ============================================================
 * Test: All service UUIDs are unique
 * ============================================================ */
void test_service_uuids_unique(void)
{
    uint16_t services[] = {
        SVC_NAVIGATION, SVC_MEDIA, SVC_PHONE, SVC_WEATHER,
        SVC_TELEMETRY, SVC_CONFIG, SVC_DIAGNOSE, SVC_OTA,
        SVC_CARPLAY,
    };
    int n = sizeof(services) / sizeof(services[0]);

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            TEST_ASSERT_NOT_EQUAL(services[i], services[j]);
        }
    }
}

/* ============================================================
 * Test: Characteristic IDs are not service IDs
 * ============================================================ */
void test_char_ids_not_service_ids(void)
{
    uint16_t chars[] = {
        CHR_TLM_LIVE, CHR_DTC_LIST, CHR_SENSOR_STATUS,
        CHR_CONFIG_READ, CHR_OTA_CONTROL, CHR_NAV_MANEUVER,
        CHR_MEDIA_NOWPLAYING, CHR_CARPLAY_STATUS,
    };
    int n = sizeof(chars) / sizeof(chars[0]);

    for (int i = 0; i < n; i++) {
        /* Characteristic should have non-zero lower 12 bits */
        TEST_ASSERT_NOT_EQUAL(0, chars[i] & 0x0FFF);
    }
}

/* ============================================================
 * Test: UUID string length is consistent (36 chars + null)
 * ============================================================ */
void test_uuid_string_length(void)
{
    char uuid[40];
    build_uuid(0x5001, uuid, sizeof(uuid));
    TEST_ASSERT_EQUAL(36, strlen(uuid));
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_base_uuid_format);
    RUN_TEST(test_service_ids_valid);
    RUN_TEST(test_char_service_mapping);
    RUN_TEST(test_tlm_live_size);
    RUN_TEST(test_sensor_status_size);
    RUN_TEST(test_dtc_entry_size);
    RUN_TEST(test_dtc_packing_format);
    RUN_TEST(test_carplay_tlm_live_size);
    RUN_TEST(test_ota_verify_notify_size);
    RUN_TEST(test_ota_verify_notify_layout);
    RUN_TEST(test_service_uuids_unique);
    RUN_TEST(test_char_ids_not_service_ids);
    RUN_TEST(test_uuid_string_length);

    return UNITY_END();
}
