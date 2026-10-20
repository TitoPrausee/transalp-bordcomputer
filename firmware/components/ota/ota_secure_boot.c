/*
 * ota_secure_boot.c — Secure Boot Support
 *
 * RSA-Signatur-Verifikation fuer OTA-Firmware-Images.
 * Anti-Rollback-Schutz ueber eFuse-Version-Counter.
 *
 * Siehe docs/FIRMWARE-GUIDE.md Abschnitt "Flash-Layout"
 *      docs/research/11-firmware-architecture.md Abschnitt 6
 *
 * Hinweis: Secure Boot V2 verwendet RSA-3072. Die Verifikation
 * erfolgt durch den Bootloader bei aktiviertem Secure Boot.
 * Diese Komponente bietet eine Anwendungs-Layer-Pruefung zusaetzlich
 * zum Bootloader-Check.
 */

#include "ota_update.h"
#include "ota_public_key_der.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_secure_boot.h"
#include "mbedtls/sha256.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509.h"
#include "mbedtls/md.h"
#include "mbedtls/platform.h"

static const char *TAG = "ota_secure";

/* ============================================================
 * Public-Key Storage
 *
 * Der RSA-3072 Public Key wird zur Build-Zeit aus dem
 * Secure-Boot-Signing-Key (PEM) extrahiert und als DER-Blob
 * in ota_public_key_der.h eingebettet (build-time codegen
 * via scripts/gen_ota_key_header.py).
 *
 * Bei aktiviertem Secure Boot V2 wird der Key zusaetzlich im
 * eFuse-Secure-Block gespeichert und vom Bootloader beim Boot
 * geprueft. Der hier abgelegte Key wird fuer die Anwendungs-
 * Layer-Verifikation vor dem Reboot verwendet.
 * ============================================================ */

/* ============================================================
 * Anti-Rollback: eFuse SECURE_VERSION
 *
 * Das ESP32-S3 eFuse-Feld SECURE_VERSION ist 32 Bits breit.
 * eFuse-Bits koennen nur von 0->1 gesetzt werden (burn).
 * Der Versions-Counter wird als "count of set bits" interpretiert
 * (popcount des Rohwerts). Jedes Update brennt ein weiteres Bit,
 * so dass die Version monoton steigt und nie zurueckgesetzt
 * werden kann.
 * ============================================================ */

/* Cached eFuse version (read once at init to avoid repeated eFuse reads). */
static uint32_t s_efuse_version_cache = 0;
static bool s_efuse_version_cached = false;

/* Liest die aktuelle Secure-Version aus dem eFuse SECURE_VERSION-Feld.
 * Gibt die Anzahl der gesetzten Bits (popcount) zurueck. */
static uint32_t ota_efuse_read_secure_version(void)
{
    uint32_t raw = 0;
    esp_err_t err = esp_efuse_read_field_blob(
        ESP_EFUSE_SECURE_VERSION, &raw, sizeof(raw) * 8);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SECURE_VERSION eFuse: %s",
                 esp_err_to_name(err));
        return 0;
    }
    return __builtin_popcount(raw);
}

/* Brennt 'count' zusaetzliche Bits ins SECURE_VERSION-Feld.
 * Jeder Aufruf von esp_efuse_write_field_bit setzt das naechste
 * freie Bit (0->1). */
static esp_err_t ota_efuse_burn_version_bits(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        esp_err_t err = esp_efuse_write_field_bit(ESP_EFUSE_SECURE_VERSION);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to burn SECURE_VERSION bit %lu: %s",
                     (unsigned long)(i + 1), esp_err_to_name(err));
            return err;
        }
    }
    return ESP_OK;
}

/* ============================================================
 * Internal Helpers
 * ============================================================ */

static esp_err_t ota_compute_partition_sha256(const esp_partition_t *partition,
                                               uint8_t *hash_out)
{
    if (partition == NULL || hash_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    int ret = mbedtls_sha256_starts(&ctx, 0);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_sha256_starts: -0x%04x", -ret);
        mbedtls_sha256_free(&ctx);
        return ESP_FAIL;
    }

    /* In Chunks lesen und hashen */
    static const size_t chunk_size = 4096;
    uint8_t *buf = malloc(chunk_size);
    if (buf == NULL) {
        mbedtls_sha256_free(&ctx);
        return ESP_ERR_NO_MEM;
    }

    uint32_t offset = 0;
    uint32_t remaining = partition->size;

    while (remaining > 0) {
        size_t to_read = (remaining < chunk_size) ? remaining : chunk_size;
        esp_err_t err = esp_partition_read(partition, offset, buf, to_read);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Partition read failed at %lu: %s",
                     (unsigned long)offset, esp_err_to_name(err));
            free(buf);
            mbedtls_sha256_free(&ctx);
            return err;
        }

        ret = mbedtls_sha256_update(&ctx, buf, to_read);
        if (ret != 0) {
            ESP_LOGE(TAG, "mbedtls_sha256_update: -0x%04x", -ret);
            free(buf);
            mbedtls_sha256_free(&ctx);
            return ESP_FAIL;
        }

        offset    += to_read;
        remaining -= (uint32_t)to_read;
    }

    free(buf);

    ret = mbedtls_sha256_finish(&ctx, hash_out);
    mbedtls_sha256_free(&ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_sha256_finish: -0x%04x", -ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* ============================================================
 * RSA-Signatur-Verifikation
 * ============================================================ */

esp_err_t ota_secure_verify_signature(const esp_partition_t *partition,
                                       const uint8_t *signature,
                                       const uint8_t *hash)
{
    if (partition == NULL || signature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Verifying RSA signature on partition %s",
             partition->label);

    /* 1. SHA-256-Hash berechnen (falls nicht uebergeben) */
    uint8_t computed_hash[OTA_SHA256_LEN];
    const uint8_t *hash_to_use = hash;

    if (hash_to_use == NULL) {
        esp_err_t err = ota_compute_partition_sha256(partition, computed_hash);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "SHA256 computation failed");
            return err;
        }
        hash_to_use = computed_hash;
        ESP_LOG_BUFFER_HEX(TAG, hash_to_use, OTA_SHA256_LEN);
    }

    /* 2. RSA-Signatur mit mbedTLS verifizieren.
     * Der Public Key wird zur Build-Zeit aus dem Signing-Key generiert
     * (ota_public_key_der.h) und ist kein Null-Platzhalter mehr. */
    mbedtls_pk_context pk_ctx;
    mbedtls_pk_init(&pk_ctx);

    int ret = mbedtls_pk_parse_public_key(&pk_ctx,
                                          ota_public_key_der,
                                          OTA_PUBLIC_KEY_DER_LEN);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_parse_public_key: -0x%04x", -ret);
        ESP_LOGE(TAG, "RSA public key parse failed — rejecting image");
        mbedtls_pk_free(&pk_ctx);
        return ESP_ERR_INVALID_VERSION;
    }

    /* RSA-Verifikation */
    ret = mbedtls_pk_verify(&pk_ctx,
                             MBEDTLS_MD_SHA256,
                             hash_to_use, OTA_SHA256_LEN,
                             signature, OTA_SIGNATURE_LEN);

    mbedtls_pk_free(&pk_ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "RSA signature verification FAILED: -0x%04x", -ret);
        return ESP_ERR_INVALID_VERSION;
    }

    ESP_LOGI(TAG, "RSA signature verified OK");
    return ESP_OK;
}

/* ============================================================
 * Anti-Rollback: eFuse-Version-Counter
 * ============================================================ */

esp_err_t ota_secure_check_anti_rollback(uint32_t new_version)
{
    if (!s_efuse_version_cached) {
        s_efuse_version_cache = ota_efuse_read_secure_version();
        s_efuse_version_cached = true;
    }

    ESP_LOGI(TAG, "Anti-rollback check: new=%lu, efuse=%lu",
             (unsigned long)new_version,
             (unsigned long)s_efuse_version_cache);

    if (new_version < s_efuse_version_cache) {
        ESP_LOGE(TAG, "ANTI-ROLLBACK: firmware version %lu < efuse %lu",
                 (unsigned long)new_version,
                 (unsigned long)s_efuse_version_cache);
        return ESP_ERR_INVALID_VERSION;
    }

    return ESP_OK;
}

esp_err_t ota_secure_update_version_counter(uint32_t version)
{
    uint32_t current = ota_efuse_read_secure_version();

    ESP_LOGI(TAG, "Updating version counter: target=%lu, current=%lu",
             (unsigned long)version, (unsigned long)current);

    if (version <= current) {
        ESP_LOGW(TAG, "Version %lu <= current %lu, not updating eFuse",
                 (unsigned long)version, (unsigned long)current);
        s_efuse_version_cache = current;
        s_efuse_version_cached = true;
        return ESP_OK;
    }

    /* eFuse-Bits koennen nur 0->1 gesetzt werden.
     * Version = popcount(SECURE_VERSION). Um von 'current' auf
     * 'version' zu kommen, brennen wir (version - current) Bits. */
    uint32_t diff = version - current;
    esp_err_t err = ota_efuse_burn_version_bits(diff);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to burn %lu eFuse version bits: %s",
                 (unsigned long)diff, esp_err_to_name(err));
        return err;
    }

    /* Cache aktualisieren */
    s_efuse_version_cache = ota_efuse_read_secure_version();
    s_efuse_version_cached = true;

    ESP_LOGI(TAG, "eFuse version counter updated to %lu",
             (unsigned long)s_efuse_version_cache);

    return ESP_OK;
}

/* ============================================================
 * Secure-Boot Status
 * ============================================================ */

bool ota_secure_is_enabled(void)
{
#if CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT || CONFIG_SECURE_BOOT_V2_ENABLED
    return true;
#else
    return false;
#endif
}
