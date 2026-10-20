/*
 * test_ota_secure_boot.c — Host Unit Tests for OTA Secure Boot
 *
 * Compiles the REAL firmware source (firmware/components/ota/ota_secure_boot.c)
 * against the ESP-IDF mock headers in firmware/test/mocks, so the tests
 * exercise the actual RSA signature verification, fail-closed behavior,
 * and anti-rollback eFuse logic.
 *
 * Coverage (gap D3-8 from TOT-29 / TOT-55):
 *   - ota_secure_verify_signature: NULL arg rejection
 *   - ota_secure_verify_signature: valid signature accepted (ESP_OK)
 *   - ota_secure_verify_signature: invalid signature rejected (fail-closed)
 *   - ota_secure_verify_signature: key parse failure rejected (fail-closed)
 *   - ota_secure_verify_signature: SHA-256 computed when hash is NULL
 *   - ota_secure_check_anti_rollback: version below eFuse rejected
 *   - ota_secure_check_anti_rollback: version at/above eFuse accepted
 *   - ota_secure_update_version_counter: no burn when version <= current
 *   - ota_secure_update_version_counter: burns bits when version > current
 *   - ota_secure_is_enabled: returns false without CONFIG_SECURE_BOOT_V2_ENABLED
 *
 * Build (host, mirrors CI):
 *   gcc -Icomponents/ota/include -Icomponents/utils/include -Itest \
 *       -Itest/mocks -I/tmp/unity/src -DTEST_HOST -DUNITY_INCLUDE_DOUBLE \
 *       -std=c11 -Wall -Wextra -Wno-unused-parameter \
 *       test/test_ota_secure_boot.c /tmp/unity/src/unity.c -o test_ota_secure_boot -lm -lpthread
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"   /* freertos + esp_log mocks, defines guards */
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "nvs_flash.h"
#include "nvs.h"

/* ============================================================
 * Mock globals backing the esp_ota_ops / esp_partition / esp_system
 * mock headers (declared `extern` there, defined here).
 * ============================================================ */

uint8_t  g_ota_flash[4 * 1024 * 1024];
uint32_t g_ota_flash_len = 0;

uint8_t  g_ota_partition_state = 2;
uint8_t  g_ota_get_state_partition_ok = 0;

int g_ota_begin_result    = 0;
int g_ota_end_result      = 0;
int g_ota_write_result    = 0;
int g_ota_set_boot_result = 0;
int g_restart_called      = 0;
int g_ota_abort_called    = 0;
int g_ota_mark_valid_called     = 0;
int g_ota_mark_invalid_called   = 0;

/* event_bus stubs (event_bus.h declares them extern). */
void event_bus_set(uint32_t bits) { (void)bits; }
bool event_bus_wait(uint32_t bits, uint32_t timeout_ms) { (void)bits; (void)timeout_ms; return false; }

/* ============================================================
 * mbedTLS mock controllable outcomes (declared extern in mbedtls/pk.h).
 * ============================================================ */

int g_mbedtls_pk_parse_result  = 0;   /* 0 = success */
int g_mbedtls_pk_verify_result = 0;   /* 0 = success */

/* ============================================================
 * eFuse mock controllable outcomes (declared extern in esp_efuse.h).
 * ============================================================ */

esp_err_t g_efuse_read_result  = ESP_OK;
esp_err_t g_efuse_write_result = ESP_OK;
uint32_t  g_efuse_raw_value    = 0;
int       g_efuse_write_call_count = 0;

/* ============================================================
 * Reset all mock state to defaults.
 * ============================================================ */

static void secure_boot_mock_reset(void)
{
    g_ota_flash_len = 0;
    memset(g_ota_flash, 0, sizeof(g_ota_flash));

    g_mbedtls_pk_parse_result  = 0;
    g_mbedtls_pk_verify_result = 0;

    g_efuse_read_result     = ESP_OK;
    g_efuse_write_result    = ESP_OK;
    g_efuse_raw_value       = 0;
    g_efuse_write_call_count = 0;

    /* Reset the static eFuse version cache in ota_secure_boot.c.
     * ota_secure_check_anti_rollback caches the value on first call.
     * Setting it fresh each test requires busting the cache. We do
     * this by calling ota_secure_check_anti_rollback(0) after setting
     * g_efuse_raw_value = 0, which populates the cache with 0. */
}

/* ============================================================
 * Pull in the REAL secure-boot implementation. Because
 * ota_secure_boot.c is textually included, its static helpers
 * (ota_compute_partition_sha256, ota_efuse_*) are testable.
 * ============================================================ */
#include "../components/ota/ota_secure_boot.c"

#include "unity.h"

/* ============================================================
 * Unity fixtures
 * ============================================================ */

void setUp(void)
{
    secure_boot_mock_reset();
    /* Reset the static eFuse version cache in ota_secure_boot.c.
     * Accessible because ota_secure_boot.c is textually included. */
    s_efuse_version_cache = 0;
    s_efuse_version_cached = false;
}

void tearDown(void) {}

/* ============================================================
 * Helpers
 * ============================================================ */

static const esp_partition_t *get_test_partition(void)
{
    return esp_ota_get_next_update_partition(NULL);
}

static void load_flash(const uint8_t *data, uint32_t len)
{
    g_ota_flash_len = len;
    if (data && len) {
        memcpy(g_ota_flash, data, len);
    }
}

/* ============================================================
 * Tests: ota_secure_verify_signature — argument validation
 * ============================================================ */

void test_verify_signature_null_partition_rejected(void)
{
    uint8_t sig[OTA_SIGNATURE_LEN] = {0};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                       ota_secure_verify_signature(NULL, sig, NULL));
}

void test_verify_signature_null_signature_rejected(void)
{
    const esp_partition_t *p = get_test_partition();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                       ota_secure_verify_signature(p, NULL, NULL));
}

/* ============================================================
 * Tests: ota_secure_verify_signature — valid signature (happy path)
 * ============================================================ */

void test_verify_signature_valid_sig_accepted(void)
{
    const esp_partition_t *p = get_test_partition();
    uint8_t payload[64];
    for (int i = 0; i < 64; i++) payload[i] = (uint8_t)i;
    load_flash(payload, sizeof(payload));

    uint8_t hash[OTA_SHA256_LEN];
    memset(hash, 0xAA, sizeof(hash));

    uint8_t sig[OTA_SIGNATURE_LEN];
    memset(sig, 0xBB, sizeof(sig));

    /* Mock: key parse OK + signature verify OK */
    g_mbedtls_pk_parse_result  = 0;
    g_mbedtls_pk_verify_result = 0;

    TEST_ASSERT_EQUAL(ESP_OK,
                       ota_secure_verify_signature(p, sig, hash));
}

void test_verify_signature_computes_hash_when_null(void)
{
    const esp_partition_t *p = get_test_partition();
    uint8_t payload[128];
    for (int i = 0; i < 128; i++) payload[i] = (uint8_t)(i * 3);
    load_flash(payload, sizeof(payload));

    uint8_t sig[OTA_SIGNATURE_LEN];
    memset(sig, 0xCC, sizeof(sig));

    /* hash=NULL forces ota_compute_partition_sha256() to run.
     * It reads partition data via esp_partition_read (mock: g_ota_flash)
     * and hashes via mbedtls_sha256_* (mock: always succeeds). */
    g_mbedtls_pk_parse_result  = 0;
    g_mbedtls_pk_verify_result = 0;

    TEST_ASSERT_EQUAL(ESP_OK,
                       ota_secure_verify_signature(p, sig, NULL));
}

/* ============================================================
 * Tests: ota_secure_verify_signature — fail-closed (no dev fallback)
 * ============================================================ */

void test_verify_signature_invalid_sig_rejected_fail_closed(void)
{
    const esp_partition_t *p = get_test_partition();
    uint8_t hash[OTA_SHA256_LEN];
    memset(hash, 0xAA, sizeof(hash));
    uint8_t sig[OTA_SIGNATURE_LEN];
    memset(sig, 0xDD, sizeof(sig));

    /* Mock: key parse OK but signature verify FAILS */
    g_mbedtls_pk_parse_result  = 0;
    g_mbedtls_pk_verify_result = -0x4380;  /* MBEDTLS_ERR_RSA_VERIFY_FAILED */

    /* Fail-closed: must NOT return ESP_OK (no hash-only fallback) */
    esp_err_t ret = ota_secure_verify_signature(p, sig, hash);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_VERSION, ret);
}

void test_verify_signature_key_parse_failure_rejected_fail_closed(void)
{
    const esp_partition_t *p = get_test_partition();
    uint8_t hash[OTA_SHA256_LEN];
    memset(hash, 0xAA, sizeof(hash));
    uint8_t sig[OTA_SIGNATURE_LEN];
    memset(sig, 0xEE, sizeof(sig));

    /* Mock: key parse FAILS (e.g. corrupt key) */
    g_mbedtls_pk_parse_result  = -0x3E80;  /* MBEDTLS_ERR_PK_KEY_INVALID_FORMAT */
    g_mbedtls_pk_verify_result = 0;

    /* Fail-closed: must NOT fall back to hash-only verification */
    esp_err_t ret = ota_secure_verify_signature(p, sig, hash);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_VERSION, ret);
}

/* ============================================================
 * Tests: ota_secure_check_anti_rollback
 * ============================================================ */

void test_anti_rollback_rejects_old_version(void)
{
    /* Set eFuse version to 5 (popcount of g_efuse_raw_value) */
    g_efuse_raw_value = 0x1F;  /* 5 bits set */
    g_efuse_read_result = ESP_OK;

    /* Bust the cache: call with version 0 first to cache 0,
     * then set eFuse to 5 and call again. But the cache is static
     * in ota_secure_boot.c and persists across calls within a test.
     * Instead, we rely on the first call populating the cache. */
    /* First call caches the eFuse value (popcount(0x1F) = 5) */
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_VERSION,
                       ota_secure_check_anti_rollback(3));  /* 3 < 5 */
}

void test_anti_rollback_accepts_current_version(void)
{
    g_efuse_raw_value = 0x07;  /* 3 bits set */
    g_efuse_read_result = ESP_OK;

    TEST_ASSERT_EQUAL(ESP_OK,
                       ota_secure_check_anti_rollback(3));  /* 3 == 3 */
}

void test_anti_rollback_accepts_newer_version(void)
{
    g_efuse_raw_value = 0x03;  /* 2 bits set */
    g_efuse_read_result = ESP_OK;

    TEST_ASSERT_EQUAL(ESP_OK,
                       ota_secure_check_anti_rollback(10));  /* 10 > 2 */
}

/* ============================================================
 * Tests: ota_secure_update_version_counter
 * ============================================================ */

void test_update_version_counter_no_burn_when_equal(void)
{
    g_efuse_raw_value = 0x0F;  /* 4 bits */
    g_efuse_write_call_count = 0;

    TEST_ASSERT_EQUAL(ESP_OK,
                       ota_secure_update_version_counter(4));

    /* version == current: no bits should be burned */
    TEST_ASSERT_EQUAL_INT(0, g_efuse_write_call_count);
}

void test_update_version_counter_burns_diff_bits(void)
{
    g_efuse_raw_value = 0x03;  /* 2 bits */
    g_efuse_write_call_count = 0;
    g_efuse_write_result = ESP_OK;

    TEST_ASSERT_EQUAL(ESP_OK,
                       ota_secure_update_version_counter(5));

    /* 5 - 2 = 3 bits to burn */
    TEST_ASSERT_EQUAL_INT(3, g_efuse_write_call_count);
}

void test_update_version_counter_burn_failure_propagates(void)
{
    g_efuse_raw_value = 0x01;  /* 1 bit */
    g_efuse_write_call_count = 0;
    g_efuse_write_result = ESP_FAIL;

    esp_err_t ret = ota_secure_update_version_counter(3);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

/* ============================================================
 * Tests: ota_secure_is_enabled
 * ============================================================ */

void test_secure_is_enabled_without_config(void)
{
    /* Without CONFIG_SECURE_BOOT_V2_ENABLED defined (host test),
     * ota_secure_is_enabled() must return false. */
    TEST_ASSERT_FALSE(ota_secure_is_enabled());
}

/* ============================================================
 * Tests: embedded public key is valid (non-zero, correct length)
 * ============================================================ */

void test_public_key_is_not_placeholder(void)
{
    /* The embedded key must NOT be all zeros (placeholder check).
     * This verifies TOT-55 acceptance criterion: real RSA-3072 public
     * key embedded in firmware. */
    bool all_zero = true;
    for (size_t i = 0; i < OTA_PUBLIC_KEY_DER_LEN; i++) {
        if (ota_public_key_der[i] != 0x00) {
            all_zero = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(all_zero);
    TEST_ASSERT_EQUAL(422u, OTA_PUBLIC_KEY_DER_LEN);
}

/* ============================================================
 * Test Runner
 * ============================================================ */

int main(void)
{
    UNITY_BEGIN();

    /* Argument validation */
    RUN_TEST(test_verify_signature_null_partition_rejected);
    RUN_TEST(test_verify_signature_null_signature_rejected);

    /* Valid signature (happy path) */
    RUN_TEST(test_verify_signature_valid_sig_accepted);
    RUN_TEST(test_verify_signature_computes_hash_when_null);

    /* Fail-closed (no dev fallback) */
    RUN_TEST(test_verify_signature_invalid_sig_rejected_fail_closed);
    RUN_TEST(test_verify_signature_key_parse_failure_rejected_fail_closed);

    /* Anti-rollback */
    RUN_TEST(test_anti_rollback_rejects_old_version);
    RUN_TEST(test_anti_rollback_accepts_current_version);
    RUN_TEST(test_anti_rollback_accepts_newer_version);

    /* Version counter update */
    RUN_TEST(test_update_version_counter_no_burn_when_equal);
    RUN_TEST(test_update_version_counter_burns_diff_bits);
    RUN_TEST(test_update_version_counter_burn_failure_propagates);

    /* Secure boot status */
    RUN_TEST(test_secure_is_enabled_without_config);

    /* Embedded key validation */
    RUN_TEST(test_public_key_is_not_placeholder);

    return UNITY_END();
}
