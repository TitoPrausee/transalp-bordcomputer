/*
 * test_ble_privacy.c — Host unit tests for BLE address privacy (TOT-64).
 *
 * Verifies firmware/components/ble_privacy/ble_privacy.c:
 *   - ble_privacy_configure() sets the IRK/identity key-distribution knobs on
 *     struct ble_hs_cfg — the "config knob check on ble_hs_cfg" requested by
 *     TOT-64's acceptance criteria (bonded peers receive the IRK so they can
 *     resolve our RPA).
 *   - ble_privacy_init() prepares a static random identity address (Core Spec
 *     static-address marker set, persisted in NVS).
 *   - ble_privacy_own_addr_type() registers the identity address with the host
 *     (ble_hs_id_set_rnd) on first use and infers the RPA own address type
 *     (ble_hs_id_infer_auto(1)) -> BLE_OWN_ADDR_RPA_RANDOM_DEFAULT.
 *   - ble_privacy_own_addr_type() falls back to BLE_OWN_ADDR_PUBLIC when the
 *     host rejects the identity address or RPA inference fails, so
 *     advertising/connecting still work (degraded, non-private) instead of
 *     failing closed.
 *
 * The test includes ble_privacy.c directly and uses the shared NimBLE / NVS /
 * esp_system mocks under test/mocks/. Runs on the host PC with GCC + Unity.
 */

#ifdef TEST_HOST
#include "esp_idf_mock.h"
#endif

#include "unity.h"
#include "ble_privacy.h"

#include <string.h>
#include <stdint.h>

/* ============================================================
 * Mock ESP-IDF dependencies of ble_privacy.c
 * ============================================================ */
#ifdef TEST_HOST

/* Host-side logging no-op. Routing the format args through a helper (rather
 * than (void)0) keeps the file-scope TAG in ble_privacy.c marked as used,
 * avoiding -Wunused-variable noise from the mocked-out logging. */
static inline void _log_noop(const char *tag, const char *fmt, ...) {
    (void)tag; (void)fmt;
}

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#define ESP_LOGE(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...)  _log_noop(tag, fmt, ##__VA_ARGS__)

/* Include the actual ble_privacy.c implementation directly so the static
 * state (s_addr_prepared, s_identity_registered, s_own_addr_type,
 * s_identity_addr) and the NimBLE privacy API call sites are exercised. */
#include "../../components/ble_privacy/ble_privacy.c"

#endif /* TEST_HOST */

void setUp(void)
{
    /* Reset the privacy module state to the pre-init default (public,
     * not prepared) so each test starts from a known baseline. */
    s_addr_prepared = false;
    s_identity_registered = false;
    s_own_addr_type = BLE_OWN_ADDR_PUBLIC;
    memset(s_identity_addr, 0, sizeof(s_identity_addr));

    /* Reset the NimBLE privacy mock capture/counters. */
    g_mock_set_rnd_count = 0;
    g_mock_set_rnd_rc = 0;
    g_mock_infer_count = 0;
    g_mock_infer_rc = 0;
    g_mock_infer_addr_type = BLE_OWN_ADDR_RPA_RANDOM_DEFAULT;
    memset(g_mock_last_rnd_addr, 0, sizeof(g_mock_last_rnd_addr));

    /* Reset the NVS mock store so each init() generates a fresh address. */
    nvs_mock_reset();
}

void tearDown(void)
{
}

/* ============================================================
 * Config-knob tests on struct ble_hs_cfg (TOT-64 acceptance criterion)
 * ============================================================ */

void test_privacy_configure_sets_irk_distribution_knob(void)
{
    struct ble_hs_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));

    ble_privacy_configure(&cfg);

    /* IRK + identity address (ID) distributed to bonded peers so they can
     * resolve our RPA after pairing.  LTK (ENC) distributed so the link is
     * encrypted.  This is the ble_hs_cfg knob the acceptance criterion asks
     * to verify. */
    TEST_ASSERT_TRUE(cfg.sm_our_key_dist & BLE_SM_PAIR_KEY_DIST_ID);
    TEST_ASSERT_TRUE(cfg.sm_our_key_dist & BLE_SM_PAIR_KEY_DIST_ENC);
    /* Peer's IRK + identity address requested so we can resolve theirs. */
    TEST_ASSERT_TRUE(cfg.sm_their_key_dist & BLE_SM_PAIR_KEY_DIST_ID);
    TEST_ASSERT_TRUE(cfg.sm_their_key_dist & BLE_SM_PAIR_KEY_DIST_ENC);
}

void test_privacy_configure_preserves_existing_key_dist_flags(void)
{
    struct ble_hs_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    /* A caller may set additional distribution flags (e.g. SIGN). */
    cfg.sm_our_key_dist = 0xFF;
    cfg.sm_their_key_dist = 0x80;  /* a flag ble_privacy_configure does not set */

    ble_privacy_configure(&cfg);

    /* OR-ing must not clear pre-existing flags. */
    TEST_ASSERT_EQUAL(0xFF, cfg.sm_our_key_dist);
    TEST_ASSERT_EQUAL((uint8_t)(0x80 | BLE_SM_PAIR_KEY_DIST_ENC |
                                BLE_SM_PAIR_KEY_DIST_ID),
                      cfg.sm_their_key_dist);
}

void test_privacy_configure_null_cfg_is_noop(void)
{
    /* Must not crash when no config struct is supplied. */
    ble_privacy_configure(NULL);
    TEST_PASS();
}

/* ============================================================
 * Identity-address preparation (ble_privacy_init)
 * ============================================================ */

void test_privacy_init_prepares_static_random_identity_address(void)
{
    ble_privacy_init();

    TEST_ASSERT_TRUE(s_addr_prepared);
    /* Core Spec static random address: the two most significant bits of the
     * most significant byte (val[5]) must be 11 (0xC0). */
    TEST_ASSERT_EQUAL(0xC0, s_identity_addr[5] & 0xC0);
}

void test_privacy_init_generates_nonzero_identity_address(void)
{
    ble_privacy_init();

    /* A generated identity address must not be all-zero (esp_fill_random
     * fills non-zero), otherwise the address would be trivially predictable. */
    bool any_nonzero = false;
    for (int i = 0; i < (int)sizeof(s_identity_addr); i++) {
        if (s_identity_addr[i] != 0) {
            any_nonzero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(any_nonzero);
}

/* ============================================================
 * Runtime RPA enable / fallback (ble_privacy_own_addr_type)
 * ============================================================ */

void test_privacy_own_addr_type_returns_rpa_when_host_accepts_identity(void)
{
    g_mock_set_rnd_rc = 0;       /* host accepts the identity address */
    g_mock_infer_rc = 0;         /* RPA inference succeeds */
    g_mock_infer_addr_type = BLE_OWN_ADDR_RPA_RANDOM_DEFAULT;

    ble_privacy_init();
    uint8_t t = ble_privacy_own_addr_type();

    TEST_ASSERT_EQUAL(BLE_OWN_ADDR_RPA_RANDOM_DEFAULT, t);
    TEST_ASSERT_TRUE(ble_privacy_is_enabled());
    /* Identity address registered and RPA inferred exactly once. */
    TEST_ASSERT_EQUAL(1, g_mock_set_rnd_count);
    TEST_ASSERT_EQUAL(1, g_mock_infer_count);
}

void test_privacy_registers_static_random_identity_address(void)
{
    ble_privacy_init();
    (void)ble_privacy_own_addr_type();

    /* The address passed to ble_hs_id_set_rnd carries the Core Spec static
     * random marker, so the host treats it as a valid static identity. */
    TEST_ASSERT_EQUAL(0xC0, g_mock_last_rnd_addr[5] & 0xC0);
}

void test_privacy_own_addr_type_falls_back_when_identity_rejected(void)
{
    g_mock_set_rnd_rc = -1;  /* host rejects the identity address */

    ble_privacy_init();
    uint8_t t = ble_privacy_own_addr_type();

    /* Privacy disabled; advertising/connecting fall back to public so they
     * still work (degraded, non-private) instead of failing closed. */
    TEST_ASSERT_EQUAL(BLE_OWN_ADDR_PUBLIC, t);
    TEST_ASSERT_FALSE(ble_privacy_is_enabled());
    TEST_ASSERT_EQUAL(1, g_mock_set_rnd_count);
    /* Inference must not run after the identity address was rejected. */
    TEST_ASSERT_EQUAL(0, g_mock_infer_count);
}

void test_privacy_own_addr_type_falls_back_when_inference_fails(void)
{
    g_mock_set_rnd_rc = 0;   /* identity address accepted */
    g_mock_infer_rc = -1;    /* RPA inference fails */

    ble_privacy_init();
    uint8_t t = ble_privacy_own_addr_type();

    TEST_ASSERT_EQUAL(BLE_OWN_ADDR_PUBLIC, t);
    TEST_ASSERT_FALSE(ble_privacy_is_enabled());
    TEST_ASSERT_EQUAL(1, g_mock_set_rnd_count);
    TEST_ASSERT_EQUAL(1, g_mock_infer_count);
}

void test_privacy_own_addr_type_public_when_init_not_run(void)
{
    /* If ble_privacy_init() never ran, no identity address is prepared and
     * no HCI registration is attempted — the public address is used. */
    uint8_t t = ble_privacy_own_addr_type();

    TEST_ASSERT_EQUAL(BLE_OWN_ADDR_PUBLIC, t);
    TEST_ASSERT_FALSE(ble_privacy_is_enabled());
    TEST_ASSERT_EQUAL(0, g_mock_set_rnd_count);
    TEST_ASSERT_EQUAL(0, g_mock_infer_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_privacy_configure_sets_irk_distribution_knob);
    RUN_TEST(test_privacy_configure_preserves_existing_key_dist_flags);
    RUN_TEST(test_privacy_configure_null_cfg_is_noop);
    RUN_TEST(test_privacy_init_prepares_static_random_identity_address);
    RUN_TEST(test_privacy_init_generates_nonzero_identity_address);
    RUN_TEST(test_privacy_own_addr_type_returns_rpa_when_host_accepts_identity);
    RUN_TEST(test_privacy_registers_static_random_identity_address);
    RUN_TEST(test_privacy_own_addr_type_falls_back_when_identity_rejected);
    RUN_TEST(test_privacy_own_addr_type_falls_back_when_inference_fails);
    RUN_TEST(test_privacy_own_addr_type_public_when_init_not_run);
    return UNITY_END();
}
