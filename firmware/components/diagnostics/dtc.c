/*
 * dtc.c — Diagnostic Trouble Code Manager
 *
 * Verwaltet Fehlercodes mit Severity, Zeitstempel, Occurrence-Count,
 * Active/History-Flag und Freeze-Frame (Sensor-Snapshot bei Ausloesung).
 *
 * Features:
 *   - Thread-safe (FreeRTOS Mutex)
 *   - NVS-Persistenz (aktive DTCs ueberleben Reboot)
 *   - Auto-Clear: behobene Fehler werden zu History
 *   - Freeze-Frame: Sensor-Werte zum Zeitpunkt des Fehlers
 *   - Getrennte Active- und History-Listen
 */

#include "diagnostics.h"
#include "sensors.h"
#include "gps.h"
#include "power.h"
#include "ble.h"
#include "storage.h"
#include "board_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "dtc";

static dtc_entry_t s_dtc_list[DTC_MAX_ENTRIES];
static uint8_t     s_dtc_count = 0;

static SemaphoreHandle_t s_dtc_mutex = NULL;
static StaticSemaphore_t s_dtc_mutex_buffer;

static nvs_handle_t s_nvs_handle;

#define DTC_NVS_NAMESPACE  "dtc_store"
#define DTC_NVS_KEY        "dtc_blob"
#define DTC_NVS_VERSION    1

typedef struct __attribute__((packed)) {
    uint16_t version;
    uint16_t entry_count;
} dtc_blob_header_t;

static const struct {
    dtc_code_t    code;
    const char   *str;
    dtc_severity_t sev;
} s_dtc_table[] = {
    { DTC_P0101, "P0101", DTC_SEV_WARN },
    { DTC_P0102, "P0102", DTC_SEV_ERROR },
    { DTC_P0103, "P0103", DTC_SEV_WARN },
    { DTC_P0201, "P0201", DTC_SEV_WARN },
    { DTC_P0202, "P0202", DTC_SEV_ERROR },
    { DTC_P0301, "P0301", DTC_SEV_WARN },
    { DTC_P0401, "P0401", DTC_SEV_WARN },
    { DTC_P0402, "P0402", DTC_SEV_CRITICAL },
    { DTC_P0403, "P0403", DTC_SEV_WARN },
    { DTC_P0404, "P0404", DTC_SEV_INFO },
    { DTC_S0101, "S0101", DTC_SEV_ERROR },
    { DTC_S0102, "S0102", DTC_SEV_WARN },
    { DTC_S0103, "S0103", DTC_SEV_CRITICAL },
    { DTC_S0201, "S0201", DTC_SEV_ERROR },
    { DTC_S0202, "S0202", DTC_SEV_WARN },
    { DTC_S0301, "S0301", DTC_SEV_ERROR },
    { DTC_S0302, "S0302", DTC_SEV_WARN },
    { DTC_S0401, "S0401", DTC_SEV_ERROR },
    { DTC_S0402, "S0402", DTC_SEV_WARN },
    { DTC_S0501, "S0501", DTC_SEV_WARN },
    { DTC_S0502, "S0502", DTC_SEV_WARN },
    { DTC_S0503, "S0503", DTC_SEV_INFO },
    { DTC_S0601, "S0601", DTC_SEV_ERROR },
    { DTC_S0602, "S0602", DTC_SEV_WARN },
    { DTC_C0101, "C0101", DTC_SEV_ERROR },
    { DTC_C0102, "C0102", DTC_SEV_WARN },
    { DTC_C0103, "C0103", DTC_SEV_ERROR },
    { DTC_C0104, "C0104", DTC_SEV_WARN },
    { DTC_C0201, "C0201", DTC_SEV_WARN },
    { DTC_C0202, "C0202", DTC_SEV_WARN },
    { DTC_C0203, "C0203", DTC_SEV_ERROR },
    { DTC_C0301, "C0301", DTC_SEV_ERROR },
    { DTC_C0302, "C0302", DTC_SEV_WARN },
    { DTC_C0401, "C0401", DTC_SEV_WARN },
    { DTC_C0402, "C0402", DTC_SEV_ERROR },
    { DTC_B0101, "B0101", DTC_SEV_ERROR },
    { DTC_B0102, "B0102", DTC_SEV_WARN },
    { DTC_B0301, "B0301", DTC_SEV_WARN },
    { DTC_B0401, "B0401", DTC_SEV_WARN },
    { DTC_B0402, "B0402", DTC_SEV_WARN },
    { DTC_D0101, "D0101", DTC_SEV_ERROR },
    { DTC_D0102, "D0102", DTC_SEV_WARN },
    { DTC_D0103, "D0103", DTC_SEV_ERROR },
    { DTC_D0201, "D0201", DTC_SEV_ERROR },
    { DTC_D0202, "D0202", DTC_SEV_WARN },
    { DTC_D0301, "D0301", DTC_SEV_ERROR },
    { DTC_D0302, "D0302", DTC_SEV_WARN },
    { DTC_D0401, "D0401", DTC_SEV_ERROR },
    { DTC_D0402, "D0402", DTC_SEV_INFO },
    { DTC_D0501, "D0501", DTC_SEV_ERROR },
    { DTC_D0502, "D0502", DTC_SEV_WARN },
    { DTC_D0503, "D0503", DTC_SEV_WARN },
    { DTC_D0601, "D0601", DTC_SEV_ERROR },
};

#define DTC_TABLE_LEN  (sizeof(s_dtc_table) / sizeof(s_dtc_table[0]))

const char *dtc_code_to_string(dtc_code_t code)
{
    for (uint16_t i = 0; i < DTC_TABLE_LEN; i++) {
        if (s_dtc_table[i].code == code)
            return s_dtc_table[i].str;
    }
    return "????";
}

dtc_severity_t dtc_default_severity(dtc_code_t code)
{
    for (uint16_t i = 0; i < DTC_TABLE_LEN; i++) {
        if (s_dtc_table[i].code == code)
            return s_dtc_table[i].sev;
    }
    return DTC_SEV_WARN;
}

const char *dtc_severity_to_string(dtc_severity_t sev)
{
    switch (sev) {
        case DTC_SEV_INFO:     return "INFO";
        case DTC_SEV_WARN:     return "WARN";
        case DTC_SEV_ERROR:    return "ERROR";
        case DTC_SEV_CRITICAL: return "CRITICAL";
        default:               return "????";
    }
}

const char *dtc_status_to_string(dtc_status_t status)
{
    switch (status) {
        case DTC_STATUS_ACTIVE:  return "ACTIVE";
        case DTC_STATUS_HISTORY: return "HISTORY";
        case DTC_STATUS_PENDING: return "PENDING";
        default:                 return "????";
    }
}

static void capture_freeze_frame(dtc_freeze_frame_t *ff)
{
    memset(ff, 0, sizeof(*ff));

    if (sensor_data_queue != NULL) {
        sensor_data_t sd;
        if (xQueuePeek(sensor_data_queue, &sd, 0) == pdTRUE) {
            ff->coolant_temp_c = sd.coolant_temp_c;
            ff->ambient_temp_c = sd.temp_c;
            ff->fuel_percent   = sd.fuel_level_l;
            ff->rpm            = (float)sd.rpm;
            ff->speed_kmh      = sd.speed_kmh;
            ff->roll_deg       = sd.roll_deg;
            ff->pitch_deg      = sd.pitch_deg;
        }
    }

    if (gps_data_queue != NULL) {
        gps_data_t gd;
        if (xQueuePeek(gps_data_queue, &gd, 0) == pdTRUE) {
            ff->gps_sats   = gd.satellites;
            ff->gps_fix    = (gd.fix_quality != GPS_FIX_NONE);
            ff->altitude_m = gd.altitude_m;
        }
    }

    ff->battery_v = power_get_battery_voltage();
    ff->ble_connected = ble_is_connected();
    ff->sd_mounted = sd_card_is_mounted();
}

static void dtc_save_to_nvs(void)
{
    if (s_dtc_mutex == NULL)
        return;

    xSemaphoreTake(s_dtc_mutex, portMAX_DELAY);

    size_t blob_size = sizeof(dtc_blob_header_t) +
                       (size_t)s_dtc_count * sizeof(dtc_entry_t);

    uint8_t blob[sizeof(dtc_blob_header_t) + DTC_MAX_ENTRIES * sizeof(dtc_entry_t)];
    dtc_blob_header_t *hdr = (dtc_blob_header_t *)blob;
    hdr->version     = DTC_NVS_VERSION;
    hdr->entry_count = s_dtc_count;

    if (s_dtc_count > 0) {
        memcpy(blob + sizeof(dtc_blob_header_t),
               s_dtc_list,
               (size_t)s_dtc_count * sizeof(dtc_entry_t));
    }

    xSemaphoreGive(s_dtc_mutex);

    esp_err_t err = nvs_set_blob(s_nvs_handle, DTC_NVS_KEY, blob, blob_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS write failed: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(s_nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
        }
    }
}

static void dtc_load_from_nvs(void)
{
    size_t required_size = 0;
    esp_err_t err = nvs_get_blob(s_nvs_handle, DTC_NVS_KEY, NULL, &required_size);
    if (err != ESP_OK || required_size < sizeof(dtc_blob_header_t)) {
        ESP_LOGI(TAG, "No persisted DTCs found in NVS");
        return;
    }

    uint8_t *blob = malloc(required_size);
    if (blob == NULL) {
        ESP_LOGE(TAG, "Cannot allocate %zu bytes for DTC blob", required_size);
        return;
    }

    err = nvs_get_blob(s_nvs_handle, DTC_NVS_KEY, blob, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS read failed: %s", esp_err_to_name(err));
        free(blob);
        return;
    }

    dtc_blob_header_t *hdr = (dtc_blob_header_t *)blob;
    if (hdr->version != DTC_NVS_VERSION) {
        ESP_LOGW(TAG, "DTC blob version mismatch (%d != %d), discarding",
                 hdr->version, DTC_NVS_VERSION);
        free(blob);
        return;
    }

    uint16_t entries = hdr->entry_count;
    if (entries > DTC_MAX_ENTRIES) {
        ESP_LOGW(TAG, "DTC blob has %u entries, clamping to %d", entries, DTC_MAX_ENTRIES);
        entries = DTC_MAX_ENTRIES;
    }

    size_t data_size = (size_t)entries * sizeof(dtc_entry_t);
    if (required_size < sizeof(dtc_blob_header_t) + data_size) {
        ESP_LOGW(TAG, "DTC blob truncated, discarding");
        free(blob);
        return;
    }

    memcpy(s_dtc_list, blob + sizeof(dtc_blob_header_t), data_size);
    s_dtc_count = (uint8_t)entries;

    free(blob);
    ESP_LOGI(TAG, "Loaded %u DTCs from NVS", s_dtc_count);
}

void dtc_init(void)
{
    s_dtc_mutex = xSemaphoreCreateMutexStatic(&s_dtc_mutex_buffer);

    memset(s_dtc_list, 0, sizeof(s_dtc_list));
    s_dtc_count = 0;

    esp_err_t err = nvs_open(DTC_NVS_NAMESPACE, NVS_READWRITE, &s_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cannot open NVS namespace '%s': %s",
                 DTC_NVS_NAMESPACE, esp_err_to_name(err));
    } else {
        dtc_load_from_nvs();
    }

    ESP_LOGI(TAG, "DTC system initialized (%u persisted entries)", s_dtc_count);
}

void dtc_register(dtc_code_t code, dtc_severity_t sev)
{
    if (code == DTC_NONE || code >= DTC_CODE_MAX) {
        ESP_LOGW(TAG, "Invalid DTC code %d", code);
        return;
    }

    if (s_dtc_mutex == NULL) {
        ESP_LOGE(TAG, "DTC mutex not initialized");
        return;
    }

    xSemaphoreTake(s_dtc_mutex, portMAX_DELAY);

    for (uint8_t i = 0; i < s_dtc_count; i++) {
        if (s_dtc_list[i].code == code) {
            s_dtc_list[i].count++;
            s_dtc_list[i].last_occurrence_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            s_dtc_list[i].status = DTC_STATUS_ACTIVE;
            xSemaphoreGive(s_dtc_mutex);

            ESP_LOGW(TAG, "DTC %s reoccurred (count=%d, sev=%s)",
                     dtc_code_to_string(code), s_dtc_list[i].count,
                     dtc_severity_to_string(sev));

            dtc_save_to_nvs();
            return;
        }
    }

    if (s_dtc_count < DTC_MAX_ENTRIES) {
        dtc_entry_t *e = &s_dtc_list[s_dtc_count];
        memset(e, 0, sizeof(*e));
        e->code              = code;
        e->severity          = sev;
        e->status            = DTC_STATUS_ACTIVE;
        e->first_occurrence_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        e->last_occurrence_ms  = e->first_occurrence_ms;
        e->count             = 1;

        capture_freeze_frame(&e->freeze_frame);

        s_dtc_count++;
        xSemaphoreGive(s_dtc_mutex);

        ESP_LOGW(TAG, "DTC %s registered (sev=%s)",
                 dtc_code_to_string(code), dtc_severity_to_string(sev));

        dtc_save_to_nvs();
    } else {
        xSemaphoreGive(s_dtc_mutex);
        ESP_LOGE(TAG, "DTC list full (max %d), cannot add %s",
                 DTC_MAX_ENTRIES, dtc_code_to_string(code));
    }
}

void dtc_report(dtc_code_t code)
{
    dtc_severity_t sev = dtc_default_severity(code);
    dtc_register(code, sev);
}

void dtc_clear(dtc_code_t code)
{
    if (s_dtc_mutex == NULL)
        return;

    xSemaphoreTake(s_dtc_mutex, portMAX_DELAY);

    for (uint8_t i = 0; i < s_dtc_count; i++) {
        if (s_dtc_list[i].code == code) {
            s_dtc_list[i].status = DTC_STATUS_HISTORY;
            ESP_LOGI(TAG, "DTC %s cleared (now history)", dtc_code_to_string(code));
            xSemaphoreGive(s_dtc_mutex);
            dtc_save_to_nvs();
            return;
        }
    }

    xSemaphoreGive(s_dtc_mutex);
    ESP_LOGI(TAG, "DTC %s not found (nothing to clear)", dtc_code_to_string(code));
}

void dtc_clear_all(void)
{
    if (s_dtc_mutex == NULL)
        return;

    xSemaphoreTake(s_dtc_mutex, portMAX_DELAY);

    for (uint8_t i = 0; i < s_dtc_count; i++) {
        s_dtc_list[i].status = DTC_STATUS_HISTORY;
    }

    xSemaphoreGive(s_dtc_mutex);

    ESP_LOGI(TAG, "All DTCs cleared (moved to history)");
    dtc_save_to_nvs();
}

uint8_t dtc_get_active(dtc_entry_t *out, int max_count)
{
    if (out == NULL || max_count <= 0 || s_dtc_mutex == NULL)
        return 0;

    uint8_t count = 0;

    xSemaphoreTake(s_dtc_mutex, portMAX_DELAY);
    for (uint8_t i = 0; i < s_dtc_count && count < max_count; i++) {
        if (s_dtc_list[i].status == DTC_STATUS_ACTIVE) {
            out[count++] = s_dtc_list[i];
        }
    }
    xSemaphoreGive(s_dtc_mutex);

    return count;
}

uint8_t dtc_get_history(dtc_entry_t *out, int max_count)
{
    if (out == NULL || max_count <= 0 || s_dtc_mutex == NULL)
        return 0;

    uint8_t count = 0;

    xSemaphoreTake(s_dtc_mutex, portMAX_DELAY);
    for (uint8_t i = 0; i < s_dtc_count && count < max_count; i++) {
        if (s_dtc_list[i].status == DTC_STATUS_HISTORY) {
            out[count++] = s_dtc_list[i];
        }
    }
    xSemaphoreGive(s_dtc_mutex);

    return count;
}

uint8_t dtc_get_all(dtc_entry_t *out, int max_count)
{
    if (out == NULL || max_count <= 0 || s_dtc_mutex == NULL)
        return 0;

    xSemaphoreTake(s_dtc_mutex, portMAX_DELAY);
    uint8_t count = (s_dtc_count < max_count) ? s_dtc_count : max_count;
    memcpy(out, s_dtc_list, (size_t)count * sizeof(dtc_entry_t));
    xSemaphoreGive(s_dtc_mutex);

    return count;
}
