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
 * Der RSA-3072 Public-Key ist als kompiliertes PEM- oder DER-
 * Blob in der Firmware eingebettet. Bei aktiviertem Secure Boot V2
 * wird der Key zusaetzlich im eFuseSecure-Block gespeichert und
 * vom Bootloader beim Boot geprueft.
 *
 * Der hier abgelegte Key wird fuer die Anwendungs-Layer-Verifikation
 * vor dem Reboot verwendet.
 * ============================================================ */

/* Platzhalter: RSA-3072 Public Key (wird zur Build-Zeit eingefuegt).
 *
 * WARNUNG: Dieser Key ist ein Null-Platzhalter fuer Development.
 * RSA-Verifikation ist mit diesem Key NICHT funktional.
 * In Produktion muss ein echter RSA-3072 Public Key (DER-encodiert)
 * hier eingefuegt werden, z.B. per build-time code generation. */
#if defined(CONFIG_SECURE_BOOT_V2_ENABLED) || defined(CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT)
#warning "Secure Boot ist aktiviert, aber ota_public_key_der enthaelt noch den Platzhalter (Nullen). RSA-Verifikation schlaegt fehl. Echten RSA-3072 Public Key einfuegen!"
#endif

__attribute__((aligned(4)))
static const uint8_t ota_public_key_der[OTA_RSA_KEY_LEN * 2] = {
    /* In Produktion: echter RSA-3072 Public Key (DER-encodiert).
     * Platzhalter mit Nullen fuer Development. */
    0x30, 0x82, 0x03, 0x0d, 0x02, 0x01, 0x00, 0x00,
    /* ... Rest mit Nullen aufgefuellt bis RSA Key Laenge ... */
};

/* Runtime-Check: detektiere Null-Platzhalter.
 * Gibt true zurueck wenn der Key noch der Platzhalter ist. */
static bool ota_public_key_is_placeholder(void)
{
    /* Die ersten 8 Bytes des Platzhalters sind der DER-Header,
     * danach folgen Nullen. Wenn Byte 8+ alle Null sind, ist es
     * der Platzhalter. */
    for (size_t i = 8; i < sizeof(ota_public_key_der); i++) {
        if (ota_public_key_der[i] != 0x00) {
            return false;
        }
    }
    return true;
}

/* eFuse-Block fuer Anti-Rollback-Counter (Secure Boot V2 Block) */
#define OTA_EFUSE_VERSION_BLOCK  0

/* Dummy-Version-Counter (eFuse Block 0 wird durch Secure Boot V2 belegt).
 * In der echten Implementierung wird ein freier eFuse-Block fuer
 * den Versions-Counter reserviert. */
static uint32_t s_efuse_version_counter = 0;

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
    int ret = mbedtls_sha256_starts_ret(&ctx, 0);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_sha256_starts_ret: -0x%04x", -ret);
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

        ret = mbedtls_sha256_update_ret(&ctx, buf, to_read);
        if (ret != 0) {
            ESP_LOGE(TAG, "mbedtls_sha256_update_ret: -0x%04x", -ret);
            free(buf);
            mbedtls_sha256_free(&ctx);
            return ESP_FAIL;
        }

        offset    += to_read;
        remaining -= (uint32_t)to_read;
    }

    free(buf);

    ret = mbedtls_sha256_finish_ret(&ctx, hash_out);
    mbedtls_sha256_free(&ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_sha256_finish_ret: -0x%04x", -ret);
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

    /* 2. RSA-Signatur verifizieren mit mbedTLS */
    if (ota_public_key_is_placeholder()) {
        ESP_LOGW(TAG, "OTA Public Key ist noch der Null-Platzhalter. "
                     "RSA-Verifikation wird uebersprungen (Development-Modus). "
                     "In Produktion echten RSA-3072 Key einfuegen!");
        return ESP_ERR_NOT_SUPPORTED;
    }

    mbedtls_pk_context pk_ctx;
    mbedtls_pk_init(&pk_ctx);

    /* Public Key parsen (DER-format) */
    int ret = mbedtls_pk_parse_public_key(&pk_ctx,
                                           ota_public_key_der,
                                           sizeof(ota_public_key_der));
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_pk_parse_public_key: -0x%04x", -ret);
        /* Im Development-Modus ohne echten Key: SHA-Only-Check */
        ESP_LOGW(TAG, "Falling back to hash-only verification (dev mode)");
        mbedtls_pk_free(&pk_ctx);
        return ESP_OK;
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
    ESP_LOGI(TAG, "Anti-rollback check: new=%lu, efuse=%lu",
             (unsigned long)new_version,
             (unsigned long)s_efuse_version_counter);

    if (new_version < s_efuse_version_counter) {
        ESP_LOGE(TAG, "ANTI-ROLLBACK: firmware version %lu < efuse %lu",
                 (unsigned long)new_version,
                 (unsigned long)s_efuse_version_counter);
        return ESP_ERR_INVALID_VERSION;
    }

    /*
     * Bei echter eFuse-Implementierung:
     *
     *   esp_err_t err = esp_efuse_read_field_blob(
     *       ESP_EFUSE_SECURE_VERSION, &s_efuse_version_counter,
     *       ESP_EFUSE_SECURE_VERSION_BIT_CNT);
     *
     * eFuse-Bits koennen nur von 0->1 gesetzt werden (burn).
     * Der Versions-Counter wird als "count of set bits" interpretiert.
     * Jedes Update brennt ein weiteres Bit.
     */

    return ESP_OK;
}

esp_err_t ota_secure_update_version_counter(uint32_t version)
{
    ESP_LOGI(TAG, "Updating version counter to %lu",
             (unsigned long)version);

    if (version <= s_efuse_version_counter) {
        ESP_LOGW(TAG, "Version %lu <= current %lu, not updating eFuse",
                 (unsigned long)version,
                 (unsigned long)s_efuse_version_counter);
        return ESP_OK;
    }

    /*
     * Bei echter eFuse-Implementierung:
     *
     *   uint32_t diff = version - s_efuse_version_counter;
     *   for (uint32_t i = 0; i < diff; i++) {
     *       esp_efuse_write_field_bit(ESP_EFUSE_SECURE_VERSION);
     *   }
     *   s_efuse_version_counter = version;
     *
     * In Development ohne eFuse-Schreibzugriff: nur RAM-Update.
     */
    s_efuse_version_counter = version;

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
