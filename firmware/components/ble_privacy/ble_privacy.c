/*
 * ble_privacy.c — BLE address privacy (RPA + IRK distribution).
 *
 * See ble_privacy.h for the lifecycle and rationale.  This module owns the
 * device's static random IDENTITY ADDRESS: it generates and persists it in NVS
 * (so bonded peers' stored identity stays valid across reboots) and registers
 * it with the NimBLE host.  The host derives the Resolvable Private Address
 * (RPA) on-air from the device IRK, which NimBLE generates and persists
 * automatically in the bond store (CONFIG_BT_NIMBLE_NVS_PERSIST=y) during host
 * startup — the application does not manage the IRK itself.
 *
 * Both the BLE GATT server (advertising) and the CarPlay central client
 * (connection initiation) read the resulting own address type via
 * ble_privacy_own_addr_type(), which is a thin wrapper around the public
 * ble_hs_id_infer_auto(1, ...) API.
 *
 * TOT-64: mitigate long-term device-address tracking.
 */

#include "ble_privacy.h"

#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "host/ble_hs.h"

static const char *TAG = "ble_priv";

/* NVS namespace + key for the persisted static random identity address.  It is
 * stored separately from the NimBLE bond store so it survives
 * ble_store_clear() (bond deletion) — the identity address must outlive
 * individual bonds, otherwise previously-bonded peers could never resolve the
 * device again. */
#define BLE_PRIVACY_NVS_NS      "ble_priv"
#define BLE_PRIVACY_KEY_ADDR    "rnd_addr"

#define BLE_ADDR_LEN            6

/* Static random address marker: the two most significant bits of the most
 * significant byte (val[5]) must be 11 (Bluetooth Core Spec, Vol 6,
 * Part B, §1.3.2.1 "Static device address"). */
#define BLE_STATIC_ADDR_MSB_MASK 0xC0

/* Module state:
 *  - s_addr_prepared:      a static random identity address is loaded in
 *                           s_identity_addr (from NVS or freshly generated).
 *  - s_identity_registered: ble_hs_id_set_rnd() has registered the address
 *                           with the host (runs lazily, in the host-controller
 *                           sync context, since it issues an HCI command).
 *  - s_own_addr_type:       cached result of ble_hs_id_infer_auto(1, ...);
 *                           BLE_OWN_ADDR_PUBLIC until RPA inference succeeds. */
static bool    s_addr_prepared = false;
static bool    s_identity_registered = false;
static uint8_t s_own_addr_type = BLE_OWN_ADDR_PUBLIC;
static uint8_t s_identity_addr[BLE_ADDR_LEN] = {0};

void ble_privacy_configure(struct ble_hs_cfg *cfg)
{
    if (cfg == NULL) {
        return;
    }

    /* Distribute LTK (ENC) + IRK & identity address (ID) to bonded peers so
     * they can resolve our RPA after pairing.  Request the peer's IRK too so
     * the device can resolve the companion app's RPA.  OR-ing keeps any flags
     * the caller already set.  The IRK itself is generated/persisted by
     * NimBLE during host startup; this knob only controls its distribution. */
    cfg->sm_our_key_dist   |= BLE_SM_PAIR_KEY_DIST_ENC |
                              BLE_SM_PAIR_KEY_DIST_ID;
    cfg->sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC |
                              BLE_SM_PAIR_KEY_DIST_ID;
}

/* Load a blob from NVS.  Returns true and copies the blob when a value of the
 * expected length is stored; returns false (and leaves out untouched) when the
 * key is absent, the length differs, or NVS is unavailable — the caller then
 * generates a fresh value. */
static bool load_blob(nvs_handle_t h, const char *key,
                      uint8_t *out, size_t expected_len)
{
    size_t len = expected_len;
    if (nvs_get_blob(h, key, out, &len) != ESP_OK) {
        return false;
    }
    return len == expected_len;
}

static bool store_blob(nvs_handle_t h, const char *key,
                       const uint8_t *data, size_t len)
{
    esp_err_t err = nvs_set_blob(h, key, data, len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set_blob('%s') failed: %s", key, esp_err_to_name(err));
        return false;
    }
    return nvs_commit(h) == ESP_OK;
}

/* Load or generate the static random identity address.  A static random
 * address has the two most significant bits of byte[5] set to 11 (Core Spec).
 * The address is persisted so bonded peers' stored identity stays valid across
 * reboots.  *generated is set true when a new address was created and stored. */
static void load_or_generate_identity_addr(nvs_handle_t h, uint8_t *addr_out,
                                           bool *generated)
{
    if (load_blob(h, BLE_PRIVACY_KEY_ADDR, addr_out, BLE_ADDR_LEN)) {
        /* Guard against a corrupted stored value missing the static marker. */
        addr_out[5] |= BLE_STATIC_ADDR_MSB_MASK;
        *generated = false;
        return;
    }

    esp_fill_random(addr_out, BLE_ADDR_LEN);
    addr_out[5] |= BLE_STATIC_ADDR_MSB_MASK;
    *generated = true;
    if (!store_blob(h, BLE_PRIVACY_KEY_ADDR, addr_out, BLE_ADDR_LEN)) {
        ESP_LOGW(TAG, "could not persist identity address; using volatile address");
    }
}

void ble_privacy_init(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(BLE_PRIVACY_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        /* NVS unavailable — generate a volatile identity address so privacy
         * still works this session.  Bonded peers from a prior session will
         * not resolve it, but this is a degraded fallback, not a crash. */
        esp_fill_random(s_identity_addr, BLE_ADDR_LEN);
        s_identity_addr[5] |= BLE_STATIC_ADDR_MSB_MASK;
        s_addr_prepared = true;
        ESP_LOGW(TAG, "nvs_open('%s') failed: %s — using volatile identity address",
                 BLE_PRIVACY_NVS_NS, esp_err_to_name(err));
        return;
    }

    bool generated = false;
    load_or_generate_identity_addr(h, s_identity_addr, &generated);
    nvs_close(h);

    s_addr_prepared = true;
    ESP_LOGI(TAG, "BLE static random identity address %s from NVS",
             generated ? "generated" : "loaded");
}

uint8_t ble_privacy_own_addr_type(void)
{
    if (!s_addr_prepared) {
        /* ble_privacy_init() never ran — advertise with the public address. */
        return BLE_OWN_ADDR_PUBLIC;
    }

    /* Lazy registration: register the prepared identity address with the host
     * on first use.  This runs in the host-controller sync context
     * (ble_on_sync -> ble_start_advertising, or cp_connect) where HCI commands
     * are valid — ble_hs_id_set_rnd() issues HCI_LE_Set_Random_Address, which
     * requires the controller to be synced.  Done once; cached thereafter. */
    if (!s_identity_registered) {
        int rc = ble_hs_id_set_rnd(s_identity_addr);
        if (rc != 0) {
            ESP_LOGE(TAG, "ble_hs_id_set_rnd failed rc=%d — privacy disabled "
                     "(advertising with public address)", rc);
            s_own_addr_type = BLE_OWN_ADDR_PUBLIC;
            return s_own_addr_type;
        }
        s_identity_registered = true;
    }

    /* Infer the RPA own address type once after the identity address is
     * registered.  ble_hs_id_infer_auto(1) returns BLE_OWN_ADDR_RPA_RANDOM_*
     * because a static random identity address is now set.  Cached so repeated
     * adv/connect calls do not re-lock the host. */
    if (s_own_addr_type == BLE_OWN_ADDR_PUBLIC) {
        uint8_t type = BLE_OWN_ADDR_PUBLIC;
        int rc = ble_hs_id_infer_auto(1, &type);
        if (rc == 0) {
            s_own_addr_type = type;
        } else {
            ESP_LOGW(TAG, "ble_hs_id_infer_auto(privacy=1) rc=%d — using public "
                     "address", rc);
        }
    }

    return s_own_addr_type;
}

bool ble_privacy_is_enabled(void)
{
    /* Privacy is active iff we are not advertising with the public address. */
    return s_own_addr_type != BLE_OWN_ADDR_PUBLIC;
}
