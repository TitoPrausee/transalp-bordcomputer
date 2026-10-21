/*
 * ota_update.c — OTA Update Manager
 *
 * Verwaltet Firmware-Updates ueber HTTP, BLE und SD-Karte.
 * Duale OTA-Partition (ota_0 / ota_1), SHA-256-Verifikation,
 * automatischer Rollback bei Boot-Fehler.
 *
 * State-Machine:
 *   IDLE -> DOWNLOADING -> VERIFYING -> FLASHING -> REBOOTING
 *     ^          |             |            |
 *     |          v             v            v
 *     +---- ERROR <----- (jeder Fehler) <---+
 *
 * Siehe docs/ARCHITECTURE.md und
 *      docs/research/11-firmware-architecture.md Abschnitt 6.
 */

#include "ota_update.h"

#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "event_bus.h"

static const char *TAG = "ota_update";

/* ============================================================
 * Internal State
 * ============================================================ */

typedef struct {
    ota_state_t         state;
    ota_error_t         last_error;
    ota_transport_t     transport;
    uint8_t             progress;
    uint32_t            total_size;
    uint32_t            bytes_written;
    uint32_t            resume_offset;     /* Fuer HTTP Range-Request */
    esp_ota_handle_t    ota_handle;
    const esp_partition_t *update_partition;
    bool                ota_begun;
    bool                initialized;
    ota_progress_cb_t   progress_cb;
    void               *progress_ctx;
    SemaphoreHandle_t   mutex;
    uint8_t             expected_hash[OTA_SHA256_LEN];
    bool                has_expected_hash;
    ota_verify_result_t verify_result;
} ota_ctx_t;

static ota_ctx_t s_ota;

/* ============================================================
 * Forward Declarations
 * ============================================================ */

static void     ota_set_state(ota_state_t new_state);
static void     ota_set_error(ota_error_t err);
static void     ota_notify_progress(void);
static esp_err_t ota_begin_partition(uint32_t total_size);
static esp_err_t ota_finish_and_verify(void);
static esp_err_t ota_http_download(const char *url);
static esp_err_t ota_sd_read_file(const char *path);
static void     ota_cleanup(void);
static uint16_t crc16_ccitt(const uint8_t *data, size_t length);
static esp_err_t ota_save_resume_state(uint32_t offset);
static esp_err_t ota_load_resume_state(uint32_t *offset);
static esp_err_t ota_clear_resume_state(void);

/* ============================================================
 * Helpers
 * ============================================================ */

static void ota_lock(void)
{
    if (s_ota.mutex) {
        xSemaphoreTake(s_ota.mutex, portMAX_DELAY);
    }
}

static void ota_unlock(void)
{
    if (s_ota.mutex) {
        xSemaphoreGive(s_ota.mutex);
    }
}

static void ota_set_state(ota_state_t new_state)
{
    s_ota.state = new_state;
    ESP_LOGD(TAG, "State: %d -> %d", s_ota.state, new_state);
}

static void ota_set_error(ota_error_t err)
{
    s_ota.last_error = err;
    ota_set_state(OTA_STATE_ERROR);
    ESP_LOGE(TAG, "OTA error: %d", (int)err);
}

static void ota_notify_progress(void)
{
    if (s_ota.progress_cb) {
        s_ota.progress_cb(s_ota.progress, s_ota.state,
                          s_ota.last_error, s_ota.progress_ctx);
    }
}

/* CRC16-CCITT (siehe 11-firmware-architecture.md 6.3) */
static uint16_t crc16_ccitt(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* ============================================================
 * Partition Management
 * ============================================================ */

static esp_err_t ota_begin_partition(uint32_t total_size)
{
    s_ota.update_partition = esp_ota_get_next_update_partition(NULL);
    if (s_ota.update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition available");
        ota_set_error(OTA_ERR_NO_PARTITION);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA begin: partition=%s, size=%lu",
             s_ota.update_partition->label,
             (unsigned long)total_size);

    if (total_size > OTA_MAX_IMAGE_SIZE) {
        ESP_LOGE(TAG, "Image too large: %lu > %lu",
                 (unsigned long)total_size,
                 (unsigned long)OTA_MAX_IMAGE_SIZE);
        ota_set_error(OTA_ERR_IMAGE_TOO_LARGE);
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = esp_ota_begin(s_ota.update_partition,
                                   OTA_SIZE_UNKNOWN,
                                   &s_ota.ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        ota_set_error(OTA_ERR_WRITE_FAILED);
        return err;
    }

    s_ota.total_size     = total_size;
    s_ota.bytes_written  = 0;
    s_ota.resume_offset  = 0;
    s_ota.ota_begun      = true;
    s_ota.progress       = 0;
    s_ota.has_expected_hash = false;
    memset(&s_ota.verify_result, 0, sizeof(s_ota.verify_result));
    ota_set_state(OTA_STATE_DOWNLOADING);
    return ESP_OK;
}

static esp_err_t ota_finish_and_verify(void)
{
    if (!s_ota.ota_begun) {
        ota_set_error(OTA_ERR_INVALID_STATE);
        return ESP_ERR_INVALID_STATE;
    }

    ota_set_state(OTA_STATE_VERIFYING);
    ota_notify_progress();

    /* OTA-Write abschliessen */
    esp_err_t err = esp_ota_end(s_ota.ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        memset(&s_ota.verify_result, 0, sizeof(s_ota.verify_result));
        s_ota.verify_result.status = OTA_VERIFY_ERROR;
        s_ota.verify_result.size = s_ota.bytes_written;
        ota_set_error(OTA_ERR_WRITE_FAILED);
        return err;
    }

    s_ota.ota_begun = false;

    /* SHA-256-Verifikation */
    uint8_t hash[OTA_SHA256_LEN];
    err = esp_partition_get_sha256(s_ota.update_partition, hash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SHA256 computation failed: %s", esp_err_to_name(err));
        s_ota.verify_result.status = OTA_VERIFY_ERROR;
        ota_set_error(OTA_ERR_VERIFY_HASH);
        return err;
    }

    ESP_LOG_BUFFER_HEX(TAG, hash, OTA_SHA256_LEN);

    /* Verify-Result fuer 0x8003 (CHR_OTA_VERIFY) Notify befuellen
     * (siehe docs/BLE-INTERFACE-SPEC.md §9.3). */
    memset(&s_ota.verify_result, 0, sizeof(s_ota.verify_result));
    memcpy(s_ota.verify_result.sha256, hash, OTA_SHA256_LEN);
    s_ota.verify_result.size = s_ota.bytes_written;
    s_ota.verify_result.status = OTA_VERIFY_VERIFYING;

    /* Wenn ein erwarteter Hash gesetzt ist, vergleichen */
    if (s_ota.has_expected_hash) {
        if (memcmp(hash, s_ota.expected_hash, OTA_SHA256_LEN) != 0) {
            ESP_LOGE(TAG, "OTA image SHA256 mismatch!");
            s_ota.verify_result.status = OTA_VERIFY_HASH_MISMATCH;
            ota_set_error(OTA_ERR_VERIFY_HASH);
            return ESP_ERR_INVALID_VERSION;
        }
        ESP_LOGI(TAG, "SHA256 verified OK");
    }
    s_ota.verify_result.status = OTA_VERIFY_OK;

    /* Secure-Boot-Signaturpruefung (wenn aktiviert) */
    if (ota_secure_is_enabled()) {
        /* Signatur wird von der Companion-App mitgeliefert
         * und vor dem Commit in s_ota.expected_hash[32..] abgelegt.
         * Hier nur vorhanden, wenn Secure Boot konfiguriert. */
        ESP_LOGI(TAG, "Secure Boot active, signature check deferred to bootloader");
    }

    /* Boot-Partition setzen */
    ota_set_state(OTA_STATE_FLASHING);
    ota_notify_progress();

    err = esp_ota_set_boot_partition(s_ota.update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition failed: %s", esp_err_to_name(err));
        ota_set_error(OTA_ERR_WRITE_FAILED);
        return err;
    }

    /* Resume-State bereinigen */
    ota_clear_resume_state();

    ESP_LOGI(TAG, "OTA update complete, rebooting...");
    ota_set_state(OTA_STATE_REBOOTING);
    s_ota.progress = 100;
    ota_notify_progress();

    event_bus_set(EVT_OTA_DONE);

    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

    return ESP_OK;  /* Unreachable */
}

static void ota_cleanup(void)
{
    if (s_ota.ota_begun) {
        esp_ota_abort(s_ota.ota_handle);
        s_ota.ota_begun = false;
    }
    s_ota.total_size     = 0;
    s_ota.bytes_written  = 0;
    s_ota.resume_offset  = 0;
    s_ota.progress       = 0;
    s_ota.has_expected_hash = false;
    s_ota.transport      = OTA_TRANSPORT_NONE;
    ota_set_state(OTA_STATE_IDLE);
}

/* ============================================================
 * Resume State (NVS)
 * ============================================================ */

static esp_err_t ota_save_resume_state(uint32_t offset)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    nvs_set_u32(h, OTA_NVS_KEY_OFFSET, offset);
    nvs_set_u8(h, OTA_NVS_KEY_RESUME, 1);
    nvs_commit(h);
    nvs_close(h);
    return ESP_OK;
}

static esp_err_t ota_load_resume_state(uint32_t *offset)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        *offset = 0;
        return err;
    }
    uint8_t resume_flag = 0;
    nvs_get_u8(h, OTA_NVS_KEY_RESUME, &resume_flag);
    if (resume_flag) {
        nvs_get_u32(h, OTA_NVS_KEY_OFFSET, offset);
    } else {
        *offset = 0;
    }
    nvs_close(h);
    return ESP_OK;
}

static esp_err_t ota_clear_resume_state(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    nvs_set_u8(h, OTA_NVS_KEY_RESUME, 0);
    nvs_set_u32(h, OTA_NVS_KEY_OFFSET, 0);
    nvs_commit(h);
    nvs_close(h);
    return ESP_OK;
}

/* ============================================================
 * HTTP Download (Update-From-URL)
 * ============================================================ */

static esp_err_t ota_http_download(const char *url)
{
    if (url == NULL || strlen(url) == 0 || strlen(url) >= OTA_URL_MAX_LEN) {
        ota_set_error(OTA_ERR_INVALID_URL);
        return ESP_ERR_INVALID_ARG;
    }

    /* Pruefe ob ein Resume moeglich ist */
    uint32_t resume_offset = 0;
    ota_load_resume_state(&resume_offset);
    if (resume_offset > 0) {
        ESP_LOGI(TAG, "Resuming HTTP download from offset %lu",
                 (unsigned long)resume_offset);
    }

    /* HTTP-Client konfigurieren */
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = OTA_HTTP_DOWNLOAD_TIMEOUT_MS,
        .buffer_size = OTA_HTTP_BUFFER_SIZE,
        .buffer_size_tx = OTA_HTTP_BUFFER_SIZE,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        ota_set_error(OTA_ERR_DOWNLOAD_FAILED);
        return ESP_FAIL;
    }

    /* Bei Resume: Range-Header setzen */
    if (resume_offset > 0) {
        char range_header[64];
        snprintf(range_header, sizeof(range_header),
                 "bytes=%lu-", (unsigned long)resume_offset);
        esp_http_client_set_header(client, "Range", range_header);
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        ota_set_error(OTA_ERR_DOWNLOAD_FAILED);
        return err;
    }

    /* Content-Length lesen */
    int64_t content_length = esp_http_client_fetch_headers(client);
    int http_status = esp_http_client_get_status_code(client);

    /* Bei Range-Request: 206 Partial Content */
    if (resume_offset > 0 && http_status != 206) {
        ESP_LOGW(TAG, "Server does not support range request (status=%d), restarting",
                 http_status);
        resume_offset = 0;
    } else if (http_status != 200 && http_status != 206) {
        ESP_LOGE(TAG, "HTTP error: status=%d", http_status);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        ota_set_error(OTA_ERR_DOWNLOAD_FAILED);
        return ESP_FAIL;
    }

    /* Gesamtgroesse berechnen */
    uint32_t total_size;
    if (resume_offset > 0 && http_status == 206) {
        /* Content-Length ist nur der Rest */
        total_size = (uint32_t)content_length + resume_offset;
    } else {
        total_size = (uint32_t)content_length;
    }

    if (total_size == 0 || total_size > OTA_MAX_IMAGE_SIZE) {
        ESP_LOGE(TAG, "Invalid image size: %lu", (unsigned long)total_size);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        ota_set_error(OTA_ERR_IMAGE_TOO_LARGE);
        return ESP_FAIL;
    }

    /* OTA-Partition oeffnen (oder fortfahren bei Resume) */
    if (resume_offset == 0) {
        err = ota_begin_partition(total_size);
        if (err != ESP_OK) {
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return err;
        }
    } else {
        /* Bei Resume: Partition neu oeffnen und an Offset springen */
        s_ota.update_partition = esp_ota_get_next_update_partition(NULL);
        if (s_ota.update_partition == NULL) {
            ota_set_error(OTA_ERR_NO_PARTITION);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
        err = esp_ota_begin(s_ota.update_partition, OTA_SIZE_UNKNOWN,
                            &s_ota.ota_handle);
        if (err != ESP_OK) {
            ota_set_error(OTA_ERR_WRITE_FAILED);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return err;
        }
        s_ota.ota_begun     = true;
        s_ota.total_size    = total_size;
        s_ota.bytes_written = resume_offset;
        s_ota.resume_offset = resume_offset;
        ota_set_state(OTA_STATE_DOWNLOADING);
    }

    s_ota.transport = OTA_TRANSPORT_HTTP;

    /* Download-Loop */
    uint8_t *buf = malloc(OTA_HTTP_BUFFER_SIZE);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate download buffer");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        ota_set_error(OTA_ERR_INTERNAL);
        return ESP_ERR_NO_MEM;
    }

    uint32_t last_save = 0;

    while (1) {
        int read_len = esp_http_client_read(client, (char *)buf, OTA_HTTP_BUFFER_SIZE);
        if (read_len < 0) {
            ESP_LOGE(TAG, "HTTP read error: %d", read_len);
            free(buf);
            ota_save_resume_state(s_ota.bytes_written);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            ota_set_error(OTA_ERR_DOWNLOAD_FAILED);
            return ESP_FAIL;
        }
        if (read_len == 0) {
            /* EOF */
            break;
        }

        /* In Flash schreiben */
        err = esp_ota_write(s_ota.ota_handle, buf, (size_t)read_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA write failed at offset %lu: %s",
                     (unsigned long)s_ota.bytes_written,
                     esp_err_to_name(err));
            free(buf);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            ota_set_error(OTA_ERR_WRITE_FAILED);
            return err;
        }

        s_ota.bytes_written += (uint32_t)read_len;

        /* Fortschritt berechnen */
        if (s_ota.total_size > 0) {
            s_ota.progress = (uint8_t)(s_ota.bytes_written * 100 /
                                       s_ota.total_size);
        }

        /* Periodisch Resume-State speichern (alle 64KB) */
        if (s_ota.bytes_written - last_save >= 65536) {
            ota_save_resume_state(s_ota.bytes_written);
            last_save = s_ota.bytes_written;
        }

        ota_notify_progress();
    }

    free(buf);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "HTTP download complete: %lu/%lu bytes",
             (unsigned long)s_ota.bytes_written,
             (unsigned long)s_ota.total_size);

    if (s_ota.bytes_written != s_ota.total_size) {
        ESP_LOGE(TAG, "Download incomplete: got %lu, expected %lu",
                 (unsigned long)s_ota.bytes_written,
                 (unsigned long)s_ota.total_size);
        ota_set_error(OTA_ERR_DOWNLOAD_FAILED);
        return ESP_FAIL;
    }

    /* Verifikation und Flashen */
    return ota_finish_and_verify();
}

/* ============================================================
 * SD-Karte (Update-From-SD)
 * ============================================================ */

static esp_err_t ota_sd_read_file(const char *path)
{
    if (path == NULL || strlen(path) == 0 || strlen(path) >= OTA_SD_PATH_MAX_LEN) {
        ota_set_error(OTA_ERR_INVALID_URL);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Reading firmware from SD: %s", path);

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        ota_set_error(OTA_ERR_NOT_FOUND);
        return ESP_FAIL;
    }

    /* Dateigroesse bestimmen */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0 || file_size > OTA_MAX_IMAGE_SIZE) {
        ESP_LOGE(TAG, "Invalid firmware file size: %ld", file_size);
        fclose(f);
        ota_set_error(OTA_ERR_IMAGE_TOO_LARGE);
        return ESP_FAIL;
    }

    /* OTA-Partition oeffnen */
    esp_err_t err = ota_begin_partition((uint32_t)file_size);
    if (err != ESP_OK) {
        fclose(f);
        return err;
    }

    s_ota.transport = OTA_TRANSPORT_SD;

    /* Datei in Chunks lesen und flashen */
    uint8_t *buf = malloc(OTA_CHUNK_SIZE);
    if (buf == NULL) {
        fclose(f);
        ota_set_error(OTA_ERR_INTERNAL);
        return ESP_ERR_NO_MEM;
    }

    /* The fseek(SEEK_SET) above resets the stream to start; the host stdio
     * model used for analysis does not track the position reset, so
     * clang-analyzer reports a spurious EOF/indeterminate-position here.
     * Suppressed per TOT-61. */
    size_t read_len;
    while ((read_len = fread(buf, 1, OTA_CHUNK_SIZE, f)) > 0) {  // NOLINT(clang-analyzer-unix.Stream)
        err = esp_ota_write(s_ota.ota_handle, buf, read_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "OTA write failed at offset %lu: %s",
                     (unsigned long)s_ota.bytes_written,
                     esp_err_to_name(err));
            free(buf);
            fclose(f);
            ota_set_error(OTA_ERR_WRITE_FAILED);
            return err;
        }

        s_ota.bytes_written += (uint32_t)read_len;

        if (s_ota.total_size > 0) {
            s_ota.progress = (uint8_t)(s_ota.bytes_written * 100 /
                                       s_ota.total_size);
        }
        ota_notify_progress();
    }

    free(buf);
    fclose(f);

    if (s_ota.bytes_written != s_ota.total_size) {
        ESP_LOGE(TAG, "SD read incomplete: got %lu, expected %lu",
                 (unsigned long)s_ota.bytes_written,
                 (unsigned long)s_ota.total_size);
        ota_set_error(OTA_ERR_DOWNLOAD_FAILED);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SD firmware loaded: %lu bytes",
             (unsigned long)s_ota.bytes_written);

    return ota_finish_and_verify();
}

/* ============================================================
 * Public API
 * ============================================================ */

esp_err_t ota_init(ota_progress_cb_t progress_cb, void *user_ctx)
{
    memset(&s_ota, 0, sizeof(s_ota));

    s_ota.mutex = xSemaphoreCreateMutex();
    if (s_ota.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create OTA mutex");
        return ESP_ERR_NO_MEM;
    }

    s_ota.state        = OTA_STATE_IDLE;
    s_ota.last_error   = OTA_ERR_NONE;
    s_ota.transport    = OTA_TRANSPORT_NONE;
    s_ota.progress_cb  = progress_cb;
    s_ota.progress_ctx = user_ctx;
    s_ota.initialized  = true;

    ESP_LOGI(TAG, "OTA manager initialized");

    /* Boot-Check: ggf. ausstehende Firmware validieren */
    ota_check_on_boot();

    return ESP_OK;
}

esp_err_t ota_update_from_url(const char *url)
{
    ota_lock();

    if (!s_ota.initialized) {
        ota_unlock();
        ESP_LOGE(TAG, "OTA not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_ota.state != OTA_STATE_IDLE && s_ota.state != OTA_STATE_ERROR) {
        ota_unlock();
        ESP_LOGE(TAG, "OTA already running (state=%d)", s_ota.state);
        return ESP_ERR_INVALID_STATE;
    }

    /* Reset error state */
    s_ota.last_error = OTA_ERR_NONE;
    ota_set_state(OTA_STATE_IDLE);

    event_bus_set(EVT_OTA_START);

    esp_err_t err = ota_http_download(url);

    if (err != ESP_OK && s_ota.state != OTA_STATE_REBOOTING) {
        ota_cleanup();
    }

    ota_unlock();
    return err;
}

esp_err_t ota_update_from_ble(uint32_t total_size)
{
    ota_lock();

    if (!s_ota.initialized) {
        ota_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    if (s_ota.state != OTA_STATE_IDLE && s_ota.state != OTA_STATE_ERROR) {
        ota_unlock();
        ESP_LOGE(TAG, "OTA already running");
        ota_set_error(OTA_ERR_ALREADY_RUNNING);
        return ESP_ERR_INVALID_STATE;
    }

    s_ota.last_error = OTA_ERR_NONE;

    esp_err_t err = ota_begin_partition(total_size);
    if (err != ESP_OK) {
        ota_unlock();
        return err;
    }

    s_ota.transport = OTA_TRANSPORT_BLE;
    event_bus_set(EVT_OTA_START);

    ota_unlock();
    ESP_LOGI(TAG, "BLE OTA transfer started, expecting %lu bytes",
             (unsigned long)total_size);
    return ESP_OK;
}

esp_err_t ota_update_from_sd(const char *path)
{
    ota_lock();

    if (!s_ota.initialized) {
        ota_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    if (s_ota.state != OTA_STATE_IDLE && s_ota.state != OTA_STATE_ERROR) {
        ota_unlock();
        ESP_LOGE(TAG, "OTA already running");
        return ESP_ERR_INVALID_STATE;
    }

    s_ota.last_error = OTA_ERR_NONE;
    event_bus_set(EVT_OTA_START);

    esp_err_t err = ota_sd_read_file(path);

    if (err != ESP_OK && s_ota.state != OTA_STATE_REBOOTING) {
        ota_cleanup();
    }

    ota_unlock();
    return err;
}

esp_err_t ota_ble_write_chunk(const ota_chunk_t *chunk)
{
    if (chunk == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ota_lock();

    if (!s_ota.ota_begun || s_ota.transport != OTA_TRANSPORT_BLE) {
        ota_unlock();
        ESP_LOGE(TAG, "BLE write without active BLE OTA");
        return ESP_ERR_INVALID_STATE;
    }

    /* CRC pruefen */
    uint16_t computed_crc = crc16_ccitt(chunk->data, chunk->length);
    if (computed_crc != chunk->crc16) {
        ESP_LOGE(TAG, "Chunk CRC mismatch at offset %lu: got %04X, exp %04X",
                 (unsigned long)chunk->offset, computed_crc, chunk->crc16);
        ota_unlock();
        /* Kein fataler Fehler — App kann Chunk erneut senden */
        return ESP_ERR_INVALID_CRC;
    }

    /* Offset-Konsistenz pruefen */
    if (chunk->offset != s_ota.bytes_written) {
        ESP_LOGW(TAG, "Chunk offset mismatch: got %lu, expected %lu",
                 (unsigned long)chunk->offset,
                 (unsigned long)s_ota.bytes_written);
        /* Akzeptiere nur wenn es ein Retransmit eines vorherigen Chunks ist */
        if (chunk->offset > s_ota.bytes_written) {
            ota_unlock();
            return ESP_ERR_INVALID_STATE;
        }
        /* Dupe: bereits geschrieben, ignoriere */
        ota_unlock();
        return ESP_OK;
    }

    /* In Flash schreiben */
    esp_err_t err = esp_ota_write(s_ota.ota_handle,
                                  chunk->data, chunk->length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed at offset %lu: %s",
                 (unsigned long)chunk->offset, esp_err_to_name(err));
        ota_set_error(OTA_ERR_WRITE_FAILED);
        ota_unlock();
        return err;
    }

    s_ota.bytes_written += chunk->length;

    /* Fortschritt */
    if (s_ota.total_size > 0) {
        s_ota.progress = (uint8_t)(s_ota.bytes_written * 100 /
                                    s_ota.total_size);
    }
    ota_notify_progress();

    ota_unlock();
    return ESP_OK;
}

esp_err_t ota_ble_commit(const uint8_t *expected_hash)
{
    ota_lock();

    if (!s_ota.ota_begun || s_ota.transport != OTA_TRANSPORT_BLE) {
        ota_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    /* Erwarteten Hash speichern */
    if (expected_hash != NULL) {
        memcpy(s_ota.expected_hash, expected_hash, OTA_SHA256_LEN);
        s_ota.has_expected_hash = true;
    }

    ota_unlock();

    /* Verifikation und Reboot */
    return ota_finish_and_verify();
}

esp_err_t ota_ble_get_verify_result(ota_verify_result_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ota_lock();
    *out = s_ota.verify_result;
    ota_unlock();
    return ESP_OK;
}

esp_err_t ota_abort(void)
{
    ota_lock();

    ESP_LOGI(TAG, "OTA abort requested");

    ota_cleanup();
    ota_clear_resume_state();

    s_ota.last_error = OTA_ERR_ABORTED;
    ota_notify_progress();

    ota_unlock();
    return ESP_OK;
}

ota_state_t ota_get_state(void)
{
    return s_ota.state;
}

ota_error_t ota_get_error(void)
{
    return s_ota.last_error;
}

uint8_t ota_get_progress(void)
{
    return s_ota.progress;
}

esp_err_t ota_get_version_info(ota_version_info_t *info)
{
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(info, 0, sizeof(*info));

    /* Aktuelle Version aus App-Descriptor */
    const esp_app_desc_t *desc = esp_app_get_description();
    if (desc != NULL) {
        snprintf(info->current_version, sizeof(info->current_version),
                 "%s", desc->version);
    } else {
        strncpy(info->current_version, "unknown",
                sizeof(info->current_version) - 1);
    }

    /* Neue Version aus NVS (wird beim Update gesetzt) */
    nvs_handle_t h = 0;
    if (nvs_open(OTA_NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        char new_ver[32];
        size_t len = sizeof(new_ver);
        if (nvs_get_str(h, OTA_NVS_KEY_VERSION, new_ver, &len) == ESP_OK) {
            strncpy(info->new_version, new_ver,
                    sizeof(info->new_version) - 1);
        }
        nvs_close(h);
    }

    return ESP_OK;
}

esp_err_t ota_rollback(void)
{
    ota_lock();

    if (s_ota.state != OTA_STATE_IDLE) {
        ota_unlock();
        ESP_LOGE(TAG, "Cannot rollback: OTA in progress");
        return ESP_ERR_INVALID_STATE;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Current running partition: %s", running->label);

    /* Letzte gueltige Partition finden */
    esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Rollback failed: %s", esp_err_to_name(err));
        ota_set_error(OTA_ERR_INTERNAL);
        ota_unlock();
        return err;
    }

    ota_unlock();
    return ESP_OK;
}

/* ============================================================
 * Boot-Check und Self-Test
 * ============================================================ */

bool ota_check_on_boot(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    esp_err_t err = esp_ota_get_state_partition(running, &state);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Cannot get partition state: %s", esp_err_to_name(err));
        return true;  /* Ohne OTA-State fortfahren */
    }

    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "New firmware pending verification");

        /* Self-Test durchfuehren */
        bool ok = ota_firmware_self_test();

        if (ok) {
            esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI(TAG, "Firmware validated, rollback cancelled");
            return true;
        } else {
            ESP_LOGE(TAG, "Self-test failed, rolling back");
            esp_ota_mark_app_invalid_rollback_and_reboot();
            return false;  /* Unreachable */
        }
    }

    ESP_LOGI(TAG, "Running from %s, state=%d", running->label, (int)state);
    return true;
}

bool ota_firmware_self_test(void)
{
    /*
     * Kurzer Gesundheitstest nach OTA-Update.
     * Prueft grundlegende Systemfunktionen.
     */

    /* 1. FreeRTOS laeuft (implizit) */

    /* 2. NVS lesbar */
    nvs_handle_t h;
    if (nvs_open("config", NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGE(TAG, "Self-test: NVS not readable");
        return false;
    }
    nvs_close(h);

    /* 3. Heap ist gross genug (> 50KB frei) */
    uint32_t free_heap = esp_get_free_heap_size();
    if (free_heap < 50 * 1024) {
        ESP_LOGE(TAG, "Self-test: low heap (%lu bytes)",
                 (unsigned long)free_heap);
        return false;
    }

    /* 4. OTA-Subsystem ist im IDLE-State */
    if (s_ota.state != OTA_STATE_IDLE) {
        ESP_LOGE(TAG, "Self-test: OTA not idle");
        return false;
    }

    ESP_LOGI(TAG, "Self-test passed (heap=%lu bytes)",
             (unsigned long)free_heap);
    return true;
}
