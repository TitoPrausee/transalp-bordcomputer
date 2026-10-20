/*
 * test_ota.c — Host Unit Tests for the OTA Update Manager
 *
 * Compiles the REAL firmware source (firmware/components/ota/ota_update.c)
 * against the ESP-IDF mock headers in firmware/test/mocks, so the tests
 * exercise the actual OTA state machine, CRC16-CCITT chunk validation,
 * BLE transfer path, SHA-256 verify logic, NVS resume state, and abort
 * — not a replica of the logic.
 *
 * Coverage (gap H2 from TOT-29):
 *   - ota_init / IDLE state
 *   - CRC16-CCITT standard check vector ("123456789" -> 0x29B1)
 *   - State machine: IDLE -> DOWNLOADING -> VERIFYING -> FLASHING -> REBOOTING
 *   - BLE chunk write happy path (bytes written + progress)
 *   - BLE chunk CRC mismatch (ESP_ERR_INVALID_CRC, no write)
 *   - BLE chunk offset gap (ESP_ERR_INVALID_STATE)
 *   - BLE chunk duplicate/retransmit (ESP_OK, idempotent)
 *   - Image too large rejection
 *   - Already-running guard
 *   - Commit with matching hash (OTA_VERIFY_OK + reboot)
 *   - Commit with mismatched hash (OTA_VERIFY_HASH_MISMATCH + ERROR)
 *   - Abort returns to IDLE
 *   - Write/commit without active OTA (guards)
 *   - NVS resume state round-trip (save -> load -> clear)
 *
 * Build (host, mirrors CI):
 *   gcc -Icomponents/ota/include -Icomponents/utils/include -Itest \
 *       -Itest/mocks -I/tmp/unity/src -DTEST_HOST -DUNITY_INCLUDE_DOUBLE \
 *       -std=c11 -Wall -Wextra -Wno-unused-parameter \
 *       test/test_ota.c /tmp/unity/src/unity.c -o test_ota -lm -lpthread
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"   /* freertos + esp_log mocks, defines guards */
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Pull in the NVS mock early so nvs_mock_reset() (used by ota_mock_reset)
 * is visible. nvs.h guards itself, so ota_update.c's later #include is a no-op. */
#include "nvs_flash.h"
#include "nvs.h"

/* ============================================================
 * Mock globals backing the esp_ota_ops / esp_partition / esp_system
 * mock headers (declared `extern` there, defined here).
 * ============================================================ */

uint8_t  g_ota_flash[4 * 1024 * 1024];   /* OTA_MOCK_FLASH_SIZE */
uint32_t g_ota_flash_len = 0;

uint8_t  g_ota_partition_state = 2;       /* ESP_OTA_IMG_VALID */
uint8_t  g_ota_get_state_partition_ok = 0;

int g_ota_begin_result    = 0;
int g_ota_end_result      = 0;
int g_ota_write_result    = 0;
int g_ota_set_boot_result = 0;
int g_restart_called      = 0;
int g_ota_abort_called    = 0;
int g_ota_mark_valid_called     = 0;
int g_ota_mark_invalid_called   = 0;

/* event_bus_set/wait stubs (event_bus.h declares them extern). */
void event_bus_set(uint32_t bits) { (void)bits; }
bool event_bus_wait(uint32_t bits, uint32_t timeout_ms) { (void)bits; (void)timeout_ms; return false; }

/* Secure-boot stub: ota_update.c calls ota_secure_is_enabled() only.
 * Return false so the signature-check branch is skipped on host. */
bool ota_secure_is_enabled(void) { return false; }

void ota_mock_reset(void)
{
    g_ota_flash_len = 0;
    g_ota_begin_result    = 0;
    g_ota_end_result      = 0;
    g_ota_write_result   = 0;
    g_ota_set_boot_result = 0;
    g_restart_called      = 0;
    g_ota_abort_called    = 0;
    g_ota_mark_valid_called   = 0;
    g_ota_mark_invalid_called = 0;
    g_ota_partition_state       = 2;  /* ESP_OTA_IMG_VALID */
    g_ota_get_state_partition_ok = 0; /* esp_ota_get_state_partition -> FAIL */
    nvs_mock_reset();
}

/* ============================================================
 * Pull in the REAL OTA implementation. Because ota_update.c is
 * textually included, its static helpers (crc16_ccitt,
 * ota_save/load_resume_state) are directly testable here.
 * ============================================================ */
#include "../components/ota/ota_update.c"

#include "unity.h"

/* ============================================================
 * Unity fixtures
 * ============================================================ */

void setUp(void)
{
    ota_mock_reset();
    /* ota_init resets s_ota, creates the mutex, and runs the boot check
     * (which returns true because esp_ota_get_state_partition fails). */
    TEST_ASSERT_EQUAL(ESP_OK, ota_init(NULL, NULL));
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, ota_get_state());
}

void tearDown(void) {}

/* ============================================================
 * Helpers
 * ============================================================ */

/* Build a BLE chunk with a correct CRC for the given payload. */
static ota_chunk_t make_chunk(uint32_t offset, const uint8_t *data, uint16_t len)
{
    ota_chunk_t c;
    memset(&c, 0, sizeof(c));
    c.offset = offset;
    c.length = len;
    if (data && len) {
        memcpy(c.data, data, len);
    }
    c.crc16 = crc16_ccitt(c.data, len);
    return c;
}

/* ============================================================
 * Tests
 * ============================================================ */

void test_ota_init_idle_state(void)
{
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, ota_get_state());
    TEST_ASSERT_EQUAL(OTA_ERR_NONE, ota_get_error());
    TEST_ASSERT_EQUAL_UINT8(0, ota_get_progress());
}

/* CRC16-CCITT (CRC-CCITT-FALSE, init 0xFFFF, poly 0x1021).
 * Standard check vector: "123456789" -> 0x29B1. */
void test_crc16_ccitt_known_vectors(void)
{
    const char *vec = "123456789";
    TEST_ASSERT_EQUAL_UINT16(0x29B1, crc16_ccitt((const uint8_t *)vec, 9));

    /* Empty input returns the init value. */
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, crc16_ccitt((const uint8_t *)"", 0));

    /* Single zero byte: crc = 0xFFFF ^ (0<<8) = 0xFFFF, then 8 shifts. */
    uint8_t z = 0x00;
    uint16_t r = crc16_ccitt(&z, 1);
    /* Deterministic: 0xFFFF ^ 0x0000, 8 rounds -> 0xE1F0 (known). */
    TEST_ASSERT_EQUAL_UINT16(0xE1F0, r);
    (void)r;
}

void test_ota_ble_start_sets_downloading(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(1024));
    TEST_ASSERT_EQUAL(OTA_STATE_DOWNLOADING, ota_get_state());
    TEST_ASSERT_EQUAL_UINT8(0, ota_get_progress());
}

void test_ota_ble_chunk_writes_bytes_and_progress(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(1000));

    uint8_t payload[100];
    for (int i = 0; i < 100; i++) payload[i] = (uint8_t)i;
    ota_chunk_t c = make_chunk(0, payload, 100);

    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_write_chunk(&c));
    TEST_ASSERT_EQUAL_UINT32(100, g_ota_flash_len);
    TEST_ASSERT_EQUAL_UINT8(10, ota_get_progress());  /* 100/1000 = 10% */

    /* Bytes landed in the mock flash verbatim. */
    TEST_ASSERT_EQUAL_UINT8(0, g_ota_flash[0]);
    TEST_ASSERT_EQUAL_UINT8(99, g_ota_flash[99]);
}

void test_ota_ble_chunk_crc_mismatch_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(1000));

    uint8_t payload[4] = {1, 2, 3, 4};
    ota_chunk_t c = make_chunk(0, payload, 4);
    c.crc16 = (uint16_t)(c.crc16 ^ 0xFFFF);  /* corrupt CRC */

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_CRC, ota_ble_write_chunk(&c));
    TEST_ASSERT_EQUAL_UINT32(0, g_ota_flash_len);  /* nothing written */
}

void test_ota_ble_chunk_offset_gap_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(1000));

    uint8_t payload[4] = {1, 2, 3, 4};
    /* Expect offset 0, send offset 100 -> gap -> INVALID_STATE. */
    ota_chunk_t c = make_chunk(100, payload, 4);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ota_ble_write_chunk(&c));
    TEST_ASSERT_EQUAL_UINT32(0, g_ota_flash_len);
}

void test_ota_ble_chunk_dupe_retransmit_idempotent(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(1000));

    uint8_t payload[4] = {1, 2, 3, 4};
    ota_chunk_t c = make_chunk(0, payload, 4);
    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_write_chunk(&c));
    TEST_ASSERT_EQUAL_UINT32(4, g_ota_flash_len);

    /* Retransmit the same offset -> accepted, no double write. */
    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_write_chunk(&c));
    TEST_ASSERT_EQUAL_UINT32(4, g_ota_flash_len);
}

void test_ota_image_too_large_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE,
                       ota_update_from_ble(OTA_MAX_IMAGE_SIZE + 1));
    TEST_ASSERT_EQUAL(OTA_STATE_ERROR, ota_get_state());
    TEST_ASSERT_EQUAL(OTA_ERR_IMAGE_TOO_LARGE, ota_get_error());
}

void test_ota_already_running_guard(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(1000));
    /* Second start while the first is still DOWNLOADING -> rejected. */
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ota_update_from_ble(1000));
    TEST_ASSERT_EQUAL(OTA_ERR_ALREADY_RUNNING, ota_get_error());
}

void test_ota_commit_hash_match_reboots(void)
{
    const uint32_t total = 8;
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(total));

    uint8_t payload[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78};
    ota_chunk_t c = make_chunk(0, payload, total);
    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_write_chunk(&c));
    TEST_ASSERT_EQUAL_UINT32(total, g_ota_flash_len);

    /* Compute the hash the mock will produce during verify. */
    uint8_t expected[OTA_SHA256_LEN];
    const esp_partition_t *p = esp_ota_get_next_update_partition(NULL);
    TEST_ASSERT_EQUAL(ESP_OK, esp_partition_get_sha256(p, expected));

    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_commit(expected));
    TEST_ASSERT_EQUAL(OTA_STATE_REBOOTING, ota_get_state());
    TEST_ASSERT_EQUAL_UINT8(100, ota_get_progress());
    TEST_ASSERT_EQUAL_INT(1, g_restart_called);

    ota_verify_result_t vr;
    memset(&vr, 0, sizeof(vr));
    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_get_verify_result(&vr));
    TEST_ASSERT_EQUAL_UINT8(OTA_VERIFY_OK, vr.status);
    TEST_ASSERT_EQUAL_UINT32(total, vr.size);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, vr.sha256, OTA_SHA256_LEN);
}

void test_ota_commit_hash_mismatch_errors(void)
{
    const uint32_t total = 8;
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(total));

    uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ota_chunk_t c = make_chunk(0, payload, total);
    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_write_chunk(&c));

    /* Compute the real hash, then pass a wrong one. */
    uint8_t real_hash[OTA_SHA256_LEN];
    const esp_partition_t *p = esp_ota_get_next_update_partition(NULL);
    TEST_ASSERT_EQUAL(ESP_OK, esp_partition_get_sha256(p, real_hash));

    uint8_t wrong_hash[OTA_SHA256_LEN];
    memset(wrong_hash, 0xAA, sizeof(wrong_hash));

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_VERSION, ota_ble_commit(wrong_hash));
    TEST_ASSERT_EQUAL(OTA_STATE_ERROR, ota_get_state());
    TEST_ASSERT_EQUAL(OTA_ERR_VERIFY_HASH, ota_get_error());
    TEST_ASSERT_EQUAL_INT(0, g_restart_called);

    ota_verify_result_t vr;
    memset(&vr, 0, sizeof(vr));
    TEST_ASSERT_EQUAL(ESP_OK, ota_ble_get_verify_result(&vr));
    TEST_ASSERT_EQUAL_UINT8(OTA_VERIFY_HASH_MISMATCH, vr.status);
    TEST_ASSERT_EQUAL_UINT32(total, vr.size);
    /* Verify result carries the computed hash, not the expected one. */
    TEST_ASSERT_EQUAL_UINT8_ARRAY(real_hash, vr.sha256, OTA_SHA256_LEN);
}

void test_ota_abort_returns_to_idle(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(1000));
    TEST_ASSERT_EQUAL(OTA_STATE_DOWNLOADING, ota_get_state());

    TEST_ASSERT_EQUAL(ESP_OK, ota_abort());
    TEST_ASSERT_EQUAL(OTA_STATE_IDLE, ota_get_state());
    TEST_ASSERT_EQUAL(OTA_ERR_ABORTED, ota_get_error());
    TEST_ASSERT_EQUAL_INT(1, g_ota_abort_called);

    /* After abort, a new OTA can start. */
    TEST_ASSERT_EQUAL(ESP_OK, ota_update_from_ble(500));
    TEST_ASSERT_EQUAL(OTA_STATE_DOWNLOADING, ota_get_state());
}

void test_ota_write_without_start_rejected(void)
{
    uint8_t payload[4] = {1, 2, 3, 4};
    ota_chunk_t c = make_chunk(0, payload, 4);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ota_ble_write_chunk(&c));
}

void test_ota_commit_without_start_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ota_ble_commit(NULL));
}

void test_ota_nvs_resume_roundtrip(void)
{
    uint32_t offset = 0;

    /* Fresh store: no resume flag -> offset 0. */
    TEST_ASSERT_EQUAL(ESP_OK, ota_load_resume_state(&offset));
    TEST_ASSERT_EQUAL_UINT32(0, offset);

    /* Save a resume point and read it back. */
    TEST_ASSERT_EQUAL(ESP_OK, ota_save_resume_state(0x12345));
    TEST_ASSERT_EQUAL(ESP_OK, ota_load_resume_state(&offset));
    TEST_ASSERT_EQUAL_UINT32(0x12345, offset);

    /* Clear -> offset 0 again. */
    TEST_ASSERT_EQUAL(ESP_OK, ota_clear_resume_state());
    TEST_ASSERT_EQUAL(ESP_OK, ota_load_resume_state(&offset));
    TEST_ASSERT_EQUAL_UINT32(0, offset);
}

void test_ota_chunk_struct_packed_size(void)
{
    /* offset(4) + length(2) + crc16(2) + data[4096] = 4104 bytes packed. */
    TEST_ASSERT_EQUAL(4104, (int)sizeof(ota_chunk_t));
}

void test_ota_get_version_info(void)
{
    ota_version_info_t info;
    memset(&info, 0, sizeof(info));
    TEST_ASSERT_EQUAL(ESP_OK, ota_get_version_info(&info));
    TEST_ASSERT_EQUAL_STRING("1.0.0-test", info.current_version);
}

void test_ota_get_version_info_null_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ota_get_version_info(NULL));
}

void test_ota_ble_get_verify_result_null_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ota_ble_get_verify_result(NULL));
}

void test_ota_ble_write_chunk_null_rejected(void)
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ota_ble_write_chunk(NULL));
}

/* ============================================================
 * Test Runner
 * ============================================================ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ota_init_idle_state);
    RUN_TEST(test_crc16_ccitt_known_vectors);
    RUN_TEST(test_ota_ble_start_sets_downloading);
    RUN_TEST(test_ota_ble_chunk_writes_bytes_and_progress);
    RUN_TEST(test_ota_ble_chunk_crc_mismatch_rejected);
    RUN_TEST(test_ota_ble_chunk_offset_gap_rejected);
    RUN_TEST(test_ota_ble_chunk_dupe_retransmit_idempotent);
    RUN_TEST(test_ota_image_too_large_rejected);
    RUN_TEST(test_ota_already_running_guard);
    RUN_TEST(test_ota_commit_hash_match_reboots);
    RUN_TEST(test_ota_commit_hash_mismatch_errors);
    RUN_TEST(test_ota_abort_returns_to_idle);
    RUN_TEST(test_ota_write_without_start_rejected);
    RUN_TEST(test_ota_commit_without_start_rejected);
    RUN_TEST(test_ota_nvs_resume_roundtrip);
    RUN_TEST(test_ota_chunk_struct_packed_size);
    RUN_TEST(test_ota_get_version_info);
    RUN_TEST(test_ota_get_version_info_null_rejected);
    RUN_TEST(test_ota_ble_get_verify_result_null_rejected);
    RUN_TEST(test_ota_ble_write_chunk_null_rejected);

    return UNITY_END();
}
