/*
 * dtc.c — Diagnostic Trouble Codes
 *
 * Speichert Fehler mit Zeitstempel und Zaehler.
 * Aehnlich OBD-II: Fehler bleiben aktiv bis sie geloescht werden.
 *
 * TODO: DTCs persistent in NVS speichern (ueber Boot hinweg).
 * TODO: DTC-Liste per BLE an Companion-App uebertragen.
 */

#include "diagnostics.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "dtc";

static dtc_entry_t s_dtc_list[DTC_MAX_ENTRIES];
static uint8_t s_dtc_count = 0;

void dtc_init(void)
{
    memset(s_dtc_list, 0, sizeof(s_dtc_list));
    s_dtc_count = 0;
    ESP_LOGI(TAG, "DTC system initialized");
}

void dtc_report(dtc_code_t code)
{
    /* Pruefen ob DTC bereits existiert */
    for (uint8_t i = 0; i < s_dtc_count; i++) {
        if (s_dtc_list[i].code == code) {
            s_dtc_list[i].count++;
            s_dtc_list[i].timestamp_ms = xthal_get_ccount() / 240000;
            s_dtc_list[i].active = true;
            ESP_LOGW(TAG, "DTC %d reoccurred (count=%d)", code, s_dtc_list[i].count);
            return;
        }
    }

    /* Neuen DTC hinzufuegen */
    if (s_dtc_count < DTC_MAX_ENTRIES) {
        s_dtc_list[s_dtc_count].code = code;
        s_dtc_list[s_dtc_count].timestamp_ms = xthal_get_ccount() / 240000;
        s_dtc_list[s_dtc_count].count = 1;
        s_dtc_list[s_dtc_count].active = true;
        s_dtc_count++;
        ESP_LOGW(TAG, "DTC %d reported", code);
    } else {
        ESP_LOGE(TAG, "DTC list full, cannot add code %d", code);
    }
}

void dtc_clear(dtc_code_t code)
{
    for (uint8_t i = 0; i < s_dtc_count; i++) {
        if (s_dtc_list[i].code == code) {
            s_dtc_list[i].active = false;
            ESP_LOGI(TAG, "DTC %d cleared", code);
            return;
        }
    }
}

void dtc_clear_all(void)
{
    for (uint8_t i = 0; i < s_dtc_count; i++) {
        s_dtc_list[i].active = false;
    }
    ESP_LOGI(TAG, "All DTCs cleared");
}

uint8_t dtc_get_active(dtc_entry_t *entries, uint8_t max_entries)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < s_dtc_count && count < max_entries; i++) {
        if (s_dtc_list[i].active) {
            entries[count++] = s_dtc_list[i];
        }
    }
    return count;
}
