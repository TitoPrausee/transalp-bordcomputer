/*
 * storage.c -- Storage-Modul Manager: Unified API
 *
 * Verwaltet SD-Karte, Logger und Konfiguration.
 * Bietet eine zentrale storage_init() die alle Subsysteme startet.
 *
 * Unified API:
 *   storage_write(path, data, len)  -- Schreibt Datei auf SD
 *   storage_read(path, buf, max)    -- Liest Datei von SD
 *
 * Auto-Mount:
 *   storage_process() wird zyklisch aufgerufen und prueft ob die SD-Karte
 *   noch gemountet ist. Bei Bedarf wird neu gemountet.
 *
 * Statistik:
 *   Bytes geschrieben/gelesen, Fehlerzaehler, Mount-Counter.
 *
 * Wear-Leveling:
 *   FATFS auf SD-Karte nutzt Flash-Translation-Layer des SD-Controllers.
 *   Fuer internen Flash (LittleFS-Fallback) ist Wear-Leveling integriert.
 */

#include "storage.h"
#include "board_config.h"
#include "event_bus.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "storage";

static storage_stats_t s_stats;

void storage_init(void)
{
    memset(&s_stats, 0, sizeof(s_stats));

    ESP_LOGI(TAG, "Initializing storage subsystem");

    /* 1. SD-Karte initialisieren (SPI-Bus + Mount) */
    sd_card_init();

    /* 2. Konfiguration aus NVS laden (unabhaengig von SD-Karte) */
    config_init();

    /* 3. Logger initialisieren (Ringbuffer, Mutexe) */
    logger_init();

    /* Auto-Start Logging falls konfiguriert */
    device_config_t *cfg = config_get();
    if (cfg && cfg->log_autostart && sd_card_is_mounted()) {
        ESP_LOGI(TAG, "Auto-starting tour logging");
        logger_start_tour();
    }

    s_stats.mount_count = sd_card_is_mounted() ? 1 : 0;

    ESP_LOGI(TAG, "Storage subsystem initialized (SD=%s, config v%d)",
             sd_card_is_mounted() ? "mounted" : "not mounted",
             cfg ? cfg->version : 0);
}

int storage_write(const char *path, const void *data, size_t len)
{
    if (!path || (!data && len > 0)) {
        return -1;
    }

    if (!sd_card_is_mounted()) {
        ESP_LOGW(TAG, "Write '%s': SD not mounted", path);
        s_stats.write_errors++;
        return -1;
    }

    int rc = sd_card_write_file(path, data, len);
    if (rc == SD_OK) {
        s_stats.bytes_written += (uint32_t)len;
        return (int)len;
    }

    s_stats.write_errors++;
    ESP_LOGE(TAG, "Write '%s' failed: %d", path, rc);
    return -1;
}

int storage_read(const char *path, void *buf, size_t max_len)
{
    if (!path || !buf || max_len == 0) {
        return -1;
    }

    if (!sd_card_is_mounted()) {
        ESP_LOGW(TAG, "Read '%s': SD not mounted", path);
        s_stats.read_errors++;
        return -1;
    }

    int rc = sd_card_read_file(path, buf, max_len);
    if (rc >= 0) {
        s_stats.bytes_read += (uint32_t)rc;
        return rc;
    }

    s_stats.read_errors++;
    ESP_LOGE(TAG, "Read '%s' failed: %d", path, rc);
    return -1;
}

const storage_stats_t *storage_get_stats(void)
{
    return &s_stats;
}

/*
 * Wird aus dem Main-Loop aufgerufen.
 * Prueft SD-Karten-Status und remountet bei Bedarf.
 * Da das Board keinen Hardware Card-Detect Pin hat, verwenden wir
 * einen Health-Check ueber Fehlerzaehler.
 *
 * Wear-Leveling Hinweis:
 *   Bei Verwendung vonLittleFS auf internem Flash ist Wear-Leveling
 *   durch LittleFS selbst gewaehrleistet. Fuer SD-Karten uebernimmt der
 *   SD-Controller das Wear-Leveling transparent.
 *   Die Anwendung sollte Schreibzyklen minimieren durch:
 *   - Batch-Writes im Logger (nicht jeden Punkt einzeln schreiben)
 *   - Konfiguration nur bei Aenderung speichern (nicht zyklisch)
 */
void storage_process(void)
{
    static uint32_t s_last_check_ms = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if ((now - s_last_check_ms) < 5000) {
        return;
    }
    s_last_check_ms = now;

    const sd_card_info_t *info = sd_card_get_info();
    if (!info->mounted && info->errors > 0) {
        ESP_LOGW(TAG, "SD card appears unmounted (%lu errors), retrying mount",
                 (unsigned long)info->errors);
        if (sd_card_mount()) {
            s_stats.mount_count++;
            ESP_LOGI(TAG, "SD card re-mounted successfully");
        } else {
            s_stats.unmount_count++;
            ESP_LOGE(TAG, "SD card re-mount failed");
        }
    }

    if (sd_card_is_mounted()) {
        const logger_stats_t *lstats = logger_get_stats();
        if (lstats) {
            uint32_t free_mb = sd_card_get_free_space_mb();
            if (free_mb < 10) {
                ESP_LOGW(TAG, "SD card almost full: %lu MB free",
                         (unsigned long)free_mb);
                event_bus_set(EVT_SDCARD_ERROR);
            }
        }
    }
}
