/*
 * carplay_pairing.c — NVS Pairing Storage fuer CarPlay-Bridge
 *
 * Speichert die BLE-Adresse des gekoppelten CarPlay-Displays in NVS,
 * damit der Bordcomputer beim Start automatisch als GATT-Client
 * wieder verbinden kann.
 *
 * Funktionen sind so gehalten, dass sie im Host-Unit-Test mit
 * Mock-NVS lauffaehig sind (siehe test/test_carplay_bridge.c).
 *
 * Siehe docs/CARPLAY-INTEGRATION.md Abschnitt Pairing.
 */

#include "carplay_internal.h"
#include "carplay_uuids.h"

#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "carplay_pair";

/* ============================================================
 * Store paired CarPlay display address in NVS
 * ============================================================ */

bool carplay_pairing_store(const uint8_t *addr, uint8_t addr_type)
{
    if (addr == NULL) {
        return false;
    }

    nvs_handle_t h;
    esp_err_t err = nvs_open(CARPLAY_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_blob(h, CARPLAY_NVS_KEY_PEER, addr, CARPLAY_PEER_ADDR_LEN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_blob failed: %s", esp_err_to_name(err));
        nvs_close(h);
        return false;
    }

    err = nvs_set_u8(h, "peer_type", addr_type);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_u8(addr_type) failed: %s", esp_err_to_name(err));
        nvs_close(h);
        return false;
    }

    err = nvs_commit(h);
    nvs_close(h);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "CarPlay display paired (type=%u)", (unsigned)addr_type);
    return true;
}

/* ============================================================
 * Load paired CarPlay display address from NVS
 * ============================================================ */

bool carplay_pairing_load(carplay_peer_t *peer)
{
    if (peer == NULL) {
        return false;
    }

    memset(peer, 0, sizeof(*peer));

    nvs_handle_t h;
    esp_err_t err = nvs_open(CARPLAY_NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        /* No pairing stored yet — not an error. */
        return false;
    }

    size_t required = 0;
    err = nvs_get_blob(h, CARPLAY_NVS_KEY_PEER, NULL, &required);
    if (err != ESP_OK || required != CARPLAY_PEER_ADDR_LEN) {
        nvs_close(h);
        return false;
    }

    err = nvs_get_blob(h, CARPLAY_NVS_KEY_PEER, peer->addr, &required);
    if (err != ESP_OK) {
        nvs_close(h);
        return false;
    }

    uint8_t addr_type = 0;
    err = nvs_get_u8(h, "peer_type", &addr_type);
    if (err != ESP_OK) {
        addr_type = 0;  /* default: public address */
    }

    peer->addr_type = addr_type;
    peer->valid = true;

    nvs_close(h);
    return true;
}

/* ============================================================
 * Erase stored CarPlay display pairing
 * ============================================================ */

bool carplay_pairing_erase(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(CARPLAY_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return false;
    }

    err = nvs_erase_key(h, CARPLAY_NVS_KEY_PEER);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        /* Already erased — treat as success. */
        err = ESP_OK;
    }

    if (err == ESP_OK) {
        err = nvs_commit(h);
    }

    nvs_close(h);

    ESP_LOGI(TAG, "CarPlay pairing erased");
    return (err == ESP_OK);
}
