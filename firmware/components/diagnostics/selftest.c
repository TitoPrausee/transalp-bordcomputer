/*
 * selftest.c — Boot Self-Test
 *
 * Prueft beim Boot alle Hardware-Module:
 *   1. I2C: MPU6050 WHO_AM_I lesen
 *   2. I2C: BMP280 ID lesen
 *   3. SD-Karte gemountet?
 *   4. GPS UART: Daten empfangbar?
 *   5. ADC: Akku-Spannung im gueltigen Bereich?
 *   6. SPI: Display antwortet?
 *   7. BLE: NimBLE gestartet?
 *
 * Bei Fehlern: DTC setzen, aber nicht abbrechen (Best-Effort).
 * Bei OTA-Rollback: selftest_run() muss erfolgreich sein, sonst Rollback.
 *
 * TODO: Implementiere echte Hardware-Checks.
 * TODO: OTA-Verifikation: esp_ota_mark_app_valid_cancel_rollback()
 *       bei erfolgreichem Self-Test aufrufen.
 */

#include "diagnostics.h"
#include "storage.h"
#include "esp_log.h"

static const char *TAG = "selftest";

bool selftest_run(void)
{
    ESP_LOGI(TAG, "=== Boot Self-Test ===");
    bool all_ok = true;

    /* 1. I2C: MPU6050 */
    /* TODO: WHO_AM_I Register (0x75) lesen, erwartet 0x68 oder 0x70.
     * if (mpu6050_read_reg(0x75) != 0x68) {
     *     dtc_report(DTC_IMU_NOT_RESPONDING);
     *     all_ok = false;
     * }
     */

    /* 2. I2C: BMP280 */
    /* TODO: ID Register (0xD0) lesen, erwartet 0x58.
     * if (bmp280_read_reg(0xD0) != 0x58) {
     *     dtc_report(DTC_BARO_NOT_RESPONDING);
     *     all_ok = false;
     * }
     */

    /* 3. SD-Karte */
    if (!sd_card_is_mounted()) {
        dtc_report(DTC_SD_MOUNT_FAILED);
        all_ok = false;
    }

    /* 4. GPS UART */
    /* TODO: Pruefen ob UART-Daten ankommen (Timeout 2s). */

    /* 5. ADC / Akku */
    /* TODO: power_get_battery_voltage() im Bereich 3.0V-4.2V? */

    /* 6. Display */
    /* TODO: Display-Treiber Init-Check. */

    /* 7. BLE */
    /* TODO: NimBLE gestartet? */

    if (all_ok) {
        ESP_LOGI(TAG, "Self-Test PASSED");
    } else {
        ESP_LOGW(TAG, "Self-Test completed with warnings (see DTCs)");
    }

    /* TODO: OTA Rollback-Verifikation
     * const esp_partition_t *running = esp_ota_get_running_partition();
     * esp_ota_img_states_t state;
     * if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
     *     if (state == ESP_OTA_IMG_PENDING_VERIFY) {
     *         if (all_ok) {
     *             esp_ota_mark_app_valid_cancel_rollback();
     *         } else {
     *             esp_ota_mark_app_invalid_rollback_and_reboot();
     *         }
     *     }
     * }
     */

    return all_ok;
}
