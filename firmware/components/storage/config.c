/*
 * config.c — JSON-Konfiguration auf SD-Karte und NVS
 *
 * Zwei Speicher:
 *   - NVS: kleine Werte (device name, calibration offsets, thresholds)
 *   - SD/JSON: grosse Konfiguration (Display-Layout, Log-Format)
 *
 * TODO: JSON-Parser (cJSON oder eigen) fuer SD-Karten-Config.
 * TODO: NVS-BLOB fuer device_config_t (siehe 03-software-stack.md Abschnitt 9).
 * TODO: Migration bei NVS-Versionsaenderung.
 */

#include "storage.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "config";

#define NVS_NAMESPACE  "config"
#define NVS_KEY_CONFIG "dev_cfg"

/* Default-Konfiguration
 * static const device_config_t default_config = {
 *     .gps_rx_pin = 44,
 *     .gps_tx_pin = 43,
 *     .display_cs_pin = 10,
 *     .gyro_offset_x = 0, .gyro_offset_y = 0, .gyro_offset_z = 0,
 *     .bmp_sea_level_pa = 101325,
 *     .rpm_redline = 8000,
 *     .rpm_shift_light = 7000,
 *     .battery_low_v = 3.3f,
 *     .battery_shutdown_v = 3.0f,
 *     .log_interval_s = 5,
 *     .log_format = 0,   // CSV
 *     .gps_rate_hz = 10,
 *     .display_mode = 0, // Tour
 *     .device_name = "Transalp-BC",
 * };
 */

void config_init(void)
{
    /* NVS initialisieren */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* TODO: Konfiguration aus NVS laden oder Defaults setzen.
     * TODO: JSON-Config von SD-Karte laden (falls vorhanden).
     */

    ESP_LOGI(TAG, "Config initialized (stub)");
}

int config_load(void *cfg, uint16_t max_len)
{
    /* TODO: NVS-BLOB lesen.
     *
     * nvs_handle_t handle;
     * size_t len = max_len;
     * if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK) {
     *     esp_err_t err = nvs_get_blob(handle, NVS_KEY_CONFIG, cfg, &len);
     *     nvs_close(handle);
     *     if (err == ESP_OK) return (int)len;
     * }
     * return -1;  // keine gespeicherte Konfiguration
     */
    (void)cfg;
    (void)max_len;
    return -1;  /* Stub: keine Konfiguration gefunden */
}

int config_save(const void *cfg, uint16_t len)
{
    /* TODO: NVS-BLOB schreiben.
     *
     * nvs_handle_t handle;
     * if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
     *     nvs_set_blob(handle, NVS_KEY_CONFIG, cfg, len);
     *     nvs_commit(handle);
     *     nvs_close(handle);
     *     return 0;
     * }
     * return -1;
     */
    (void)cfg;
    (void)len;
    return -1;  /* Stub */
}
