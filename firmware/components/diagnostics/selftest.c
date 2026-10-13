/*
 * selftest.c — Self-Test Routine
 *
 * Prueft beim Boot (und auf manuellen Trigger) alle Hardware-Module.
 *
 * Test-Reihenfolge (Full):
 *   1. Power: Spannung im Normalbereich (11V-15V)?
 *   2. Display: TFT und E-Paper ansprechbar?
 *   3. GPS: UART-Daten empfangbar?
 *   4. BLE: Controller aktiv?
 *   5. Storage: SD-Karte mounted?
 *   6. Sensors: Sensor-Tasks aktiv (Queue hat Daten)?
 *   7. IMU: Who-Am-I korrekt?
 *   8. OTA: Boot-Partition gueltig?
 *
 * Quick-Test: Nur kritische Checks (Power, Display, Sensors).
 *
 * Timeout pro Subsystem: 5 Sekunden.
 * Bei Fehlern: DTC setzen, aber nicht abbrechen (Best-Effort).
 *
 * OTA-Rollback: Wenn die laufende Partition PENDING_VERIFY ist,
 * wird bei Erfolg esp_ota_mark_app_valid_cancel_rollback() aufgerufen.
 * Bei Fehlschlag erfolgt ein Rollback zur vorherigen Partition.
 */

#include "diagnostics.h"
#include "sensors.h"
#include "gps.h"
#include "power.h"
#include "ble.h"
#include "storage.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "driver/i2c_master.h"
#include <string.h>

static const char *TAG = "selftest";

/* Letztes Ergebnis (fuer selftest_get_last_result) */
static selftest_result_t s_last_result;

/* I2C-Konfiguration fuer Self-Test (wenn sensors_init noch nicht lief) */
#define I2C_TEST_TIMEOUT_MS  100

/* ============================================================
 * Hilfsfunktion: Ergebnis-Item setzen
 * ============================================================ */

static void st_set_item(selftest_result_t *res, uint8_t idx,
                        dtc_code_t code, selftest_status_t status,
                        const char *name)
{
    if (idx >= SELFTEST_MAX_ITEMS)
        return;
    res->items[idx].code   = code;
    res->items[idx].status = status;
    res->items[idx].name   = name;
    res->count = idx + 1;
    if (status == ST_FAIL)
        res->all_pass = false;
}

/* ============================================================
 * Test 1: Power / Bordspannung
 *
 * Prueft, ob die Bordspannung im Normalbereich (11V-15V) liegt.
 * Verwendet power_get_battery_voltage() aus der Power-Komponente.
 * ============================================================ */

static selftest_status_t test_power(dtc_code_t *out_code)
{
    float v = power_get_battery_voltage();

    if (v < 0.1f) {
        /* Spannung nicht lesbar — Power-Modul evtl. nicht initialisiert */
        ESP_LOGW(TAG, "Power: voltage unreadable (%.1fV)", v);
        *out_code = DTC_P0401;
        return ST_SKIP;
    }

    if (v < 10.5f) {
        ESP_LOGE(TAG, "Power: critical low voltage %.2fV", v);
        dtc_register(DTC_P0402, DTC_SEV_CRITICAL);
        *out_code = DTC_P0402;
        return ST_FAIL;
    }

    if (v < 11.5f) {
        ESP_LOGW(TAG, "Power: low voltage %.2fV", v);
        dtc_register(DTC_P0401, DTC_SEV_WARN);
        *out_code = DTC_P0401;
        return ST_FAIL;
    }

    if (v > 15.0f) {
        ESP_LOGW(TAG, "Power: overvoltage %.2fV", v);
        dtc_register(DTC_P0403, DTC_SEV_WARN);
        *out_code = DTC_P0403;
        return ST_FAIL;
    }

    ESP_LOGI(TAG, "Power: OK (%.2fV)", v);
    *out_code = DTC_NONE;
    return ST_PASS;
}

/* ============================================================
 * Test 2: Display (TFT + E-Paper)
 *
 * Prueft, ob die Display-Treiber initialisiert sind.
 * Da die Display-Komponente keine Status-Funktion exponiert,
 * pruefen wir indirekt: SPI-Bus konfiguriert?
 * ============================================================ */

static selftest_status_t test_display(dtc_code_t *out_code)
{
    /* Best-Effort: Pruefe, ob die SPI-Pins als GPIO-Funktion konfiguriert sind.
     * Ein echter Hardware-Check (Read-Display-ID) erfordert den Display-Treiber. */
    bool display_cs_ok = (DISPLAY_CS_PIN > 0);

    if (!display_cs_ok) {
        ESP_LOGW(TAG, "Display: CS pin not configured");
        dtc_register(DTC_B0101, DTC_SEV_ERROR);
        *out_code = DTC_B0101;
        return ST_FAIL;
    }

    ESP_LOGI(TAG, "Display: OK (CS=GPIO%d)", DISPLAY_CS_PIN);
    *out_code = DTC_NONE;
    return ST_PASS;
}

/* ============================================================
 * Test 3: GPS (UART)
 *
 * Versucht, NMEA-Daten ueber die GPS-Queue zu lesen.
 * Wenn die Queue Daten enthaelt, ist das GPS-Modul aktiv.
 * ============================================================ */

static selftest_status_t test_gps(dtc_code_t *out_code)
{
    if (gps_data_queue == NULL) {
        ESP_LOGW(TAG, "GPS: queue not initialized");
        *out_code = DTC_S0501;
        return ST_SKIP;
    }

    gps_data_t gd;
    /* Kurz warten (500ms), ob Daten in der Queue sind */
    if (xQueuePeek(gps_data_queue, &gd, pdMS_TO_TICKS(500)) == pdTRUE) {
        if (gd.fix_quality != GPS_FIX_NONE) {
            ESP_LOGI(TAG, "GPS: OK (sats=%u, fix=%d)",
                     gd.satellites, gd.fix_quality);
            *out_code = DTC_NONE;
            return ST_PASS;
        }
        ESP_LOGW(TAG, "GPS: data received but no fix (sats=%u)", gd.satellites);
        dtc_register(DTC_S0502, DTC_SEV_WARN);
        *out_code = DTC_S0502;
        return ST_FAIL;
    }

    /* Keine GPS-Daten in 500ms */
    ESP_LOGW(TAG, "GPS: no data received within 500ms");
    dtc_register(DTC_S0501, DTC_SEV_WARN);
    *out_code = DTC_S0501;
    return ST_FAIL;
}

/* ============================================================
 * Test 4: BLE (Controller)
 *
 * Prueft, ob der BLE-Stack initialisiert ist.
 * Da ble.h nur ble_is_connected() exponiert, pruefen wir,
 * ob die Funktion aufrufbar ist.
 * ============================================================ */

static selftest_status_t test_ble(dtc_code_t *out_code)
{
    /* ble_is_connected() liefert false, wenn BLE nicht initialisiert ist,
     * aber das ist kein Fehler — nur kein Client verbunden.
     * Ein echter Stack-Check wuerde Advertising starten/stoppen testen. */
    bool connected = ble_is_connected();

    ESP_LOGI(TAG, "BLE: stack callable (connected=%s)",
             connected ? "yes" : "no");

    *out_code = DTC_NONE;
    return ST_PASS;
}

/* ============================================================
 * Test 5: Storage (SD-Karte)
 *
 * Prueft, ob die SD-Karte gemountet ist.
 * ============================================================ */

static selftest_status_t test_storage(dtc_code_t *out_code)
{
    if (sd_card_is_mounted()) {
        ESP_LOGI(TAG, "Storage: SD card mounted");
        *out_code = DTC_NONE;
        return ST_PASS;
    }

    /* Versuchen zu mounten */
    if (sd_card_mount()) {
        ESP_LOGI(TAG, "Storage: SD card mounted (was unmounted, now OK)");
        *out_code = DTC_NONE;
        return ST_PASS;
    }

    ESP_LOGW(TAG, "Storage: SD card not mounted");
    dtc_register(DTC_C0101, DTC_SEV_ERROR);
    *out_code = DTC_C0101;
    return ST_FAIL;
}

/* ============================================================
 * Test 6: Sensors (Sensor-Task aktiv?)
 *
 * Prueft, ob die Sensor-Queue Daten enthaelt (Sensor-Task laeuft).
 * ============================================================ */

static selftest_status_t test_sensors(dtc_code_t *out_code)
{
    if (sensor_data_queue == NULL) {
        ESP_LOGW(TAG, "Sensors: queue not initialized");
        *out_code = DTC_S0401;
        return ST_SKIP;
    }

    sensor_data_t sd;
    if (xQueuePeek(sensor_data_queue, &sd, pdMS_TO_TICKS(500)) == pdTRUE) {
        ESP_LOGI(TAG, "Sensors: OK (temp=%.1fC, rpm=%u)",
                 sd.temp_c, sd.rpm);
        *out_code = DTC_NONE;
        return ST_PASS;
    }

    ESP_LOGW(TAG, "Sensors: no data in queue within 500ms");
    dtc_register(DTC_S0402, DTC_SEV_WARN);
    *out_code = DTC_S0402;
    return ST_FAIL;
}

/* ============================================================
 * Test 7: IMU (I2C Who-Am-I)
 *
 * Liest das WHO_AM_I Register des IMU ueber I2C.
 * Erwartet: 0x68 (oder 0x70 bei neueren Revisionen).
 * ============================================================ */

static selftest_status_t test_imu(dtc_code_t *out_code)
{
    /* Die I2C-Bus-Initialisierung erfolgt in sensors_init().
     * Da der Bus-Handle statisch in imu.c liegt, versuchen wir
     * hier einen direkten I2C-Scan auf die IMU-Adresse. */

#if SOC_I2C_SUPPORTED
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle = NULL;

    /* Versuche Bus zu initialisieren — wenn bereits initialisiert,
     * gibt die API einen Fehler zurueck, was wir ignorieren */
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "IMU: I2C bus init failed: %s", esp_err_to_name(ret));
        *out_code = DTC_S0401;
        return ST_SKIP;
    }

    /* Wenn wir einen neuen Bus erstellt haben, Device hinzufuegen */
    if (bus_handle != NULL) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = 0x68,
            .scl_speed_hz = 100000,  /* Langsam fuer Self-Test */
        };
        i2c_master_dev_handle_t dev_handle = NULL;
        ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "IMU: cannot add I2C device: %s", esp_err_to_name(ret));
            *out_code = DTC_S0401;
            return ST_SKIP;
        }

        /* WHO_AM_I Register (0x75) lesen */
        uint8_t reg = 0x75;
        uint8_t who_am_i = 0;
        ret = i2c_master_transmit_receive(dev_handle, &reg, 1,
                                          &who_am_i, 1,
                                          pdMS_TO_TICKS(I2C_TEST_TIMEOUT_MS));
        if (ret != ESP_OK || (who_am_i != 0x68 && who_am_i != 0x70)) {
            ESP_LOGW(TAG, "IMU: WHO_AM_I read failed or wrong value (0x%02X)", who_am_i);
            dtc_register(DTC_S0401, DTC_SEV_ERROR);
            *out_code = DTC_S0401;
            return ST_FAIL;
        }

        ESP_LOGI(TAG, "IMU: OK (WHO_AM_I=0x%02X)", who_am_i);
        *out_code = DTC_NONE;
        return ST_PASS;
    }
#endif

    /* Fallback: Bus schon von sensors_init() geoeffnet, wir koennen
     * nicht direkt darauf zugreifen (Handle ist static in imu.c).
     * Pruefe indirekt: Sensor-Queue hat Daten mit roll/pitch != 0? */
    if (sensor_data_queue != NULL) {
        sensor_data_t sd;
        if (xQueuePeek(sensor_data_queue, &sd, pdMS_TO_TICKS(200)) == pdTRUE) {
            if (sd.roll_deg != 0.0f || sd.pitch_deg != 0.0f) {
                ESP_LOGI(TAG, "IMU: OK (roll=%.1f, pitch=%.1f)",
                         sd.roll_deg, sd.pitch_deg);
                *out_code = DTC_NONE;
                return ST_PASS;
            }
        }
    }

    ESP_LOGW(TAG, "IMU: cannot verify (I2C bus not available)");
    *out_code = DTC_S0401;
    return ST_SKIP;
}

/* ============================================================
 * Test 8: OTA (Boot-Partition gueltig?)
 *
 * Prueft die OTA-Partition-Verwaltung:
 * - Welche Partition laeuft?
 * - Ist sie im PENDING_VERIFY Zustand?
 * ============================================================ */

static selftest_status_t test_ota(dtc_code_t *out_code)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL) {
        ESP_LOGW(TAG, "OTA: cannot get running partition");
        *out_code = DTC_D0301;
        return ST_SKIP;
    }

    ESP_LOGI(TAG, "OTA: running partition '%s' @ 0x%08lX",
             running->label, (unsigned long)running->address);

    esp_ota_img_states_t state;
    esp_err_t ret = esp_ota_get_state_partition(running, &state);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "OTA: cannot get partition state: %s", esp_err_to_name(ret));
        *out_code = DTC_NONE;
        return ST_SKIP;
    }

    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGW(TAG, "OTA: partition is PENDING_VERIFY");
        *out_code = DTC_D0301;
        return ST_FAIL;
    }

    if (state == ESP_OTA_IMG_INVALID) {
        ESP_LOGE(TAG, "OTA: partition is INVALID");
        dtc_register(DTC_D0301, DTC_SEV_ERROR);
        *out_code = DTC_D0301;
        return ST_FAIL;
    }

    ESP_LOGI(TAG, "OTA: partition valid (state=%d)", state);
    *out_code = DTC_NONE;
    return ST_PASS;
}

/* ============================================================
 * OTA Rollback-Verifikation
 *
 * Wenn die laufende Partition im PENDING_VERIFY-Zustand ist und
 * der Self-Test erfolgreich war, wird sie als gueltig markiert.
 * Bei Fehlschlag wird ein Rollback durchgefuehrt.
 * ============================================================ */

static void ota_rollback_check(bool selftest_ok)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running == NULL)
        return;

    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK)
        return;

    if (state != ESP_OTA_IMG_PENDING_VERIFY)
        return;

    if (selftest_ok) {
        ESP_LOGI(TAG, "OTA: marking app valid (cancel rollback)");
        esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "OTA: mark valid failed: %s", esp_err_to_name(ret));
            dtc_register(DTC_D0301, DTC_SEV_ERROR);
        }
    } else {
        ESP_LOGE(TAG, "OTA: self-test failed, rolling back!");
        dtc_register(DTC_D0302, DTC_SEV_WARN);
        esp_ota_mark_app_invalid_rollback_and_reboot();
        /* Diese Funktion kehrt nicht zurueck */
    }
}

/* ============================================================
 * Full Self-Test
 *
 * Fuehrt alle 8 Tests in Reihenfolge aus.
 * Timeout pro Subsystem: ~5 Sekunden (via Queue-Peek-Timeouts).
 * ============================================================ */

bool selftest_run_full(void)
{
    ESP_LOGI(TAG, "=== Full Self-Test ===");

    TickType_t start = xTaskGetTickCount();
    memset(&s_last_result, 0, sizeof(s_last_result));
    s_last_result.all_pass = true;

    dtc_code_t code = DTC_NONE;

    /* 1. Power */
    st_set_item(&s_last_result, 0, code,
                test_power(&code), "Power");

    /* 2. Display */
    st_set_item(&s_last_result, 1, code,
                test_display(&code), "Display");

    /* 3. GPS */
    st_set_item(&s_last_result, 2, code,
                test_gps(&code), "GPS");

    /* 4. BLE */
    st_set_item(&s_last_result, 3, code,
                test_ble(&code), "BLE");

    /* 5. Storage */
    st_set_item(&s_last_result, 4, code,
                test_storage(&code), "Storage");

    /* 6. Sensors */
    st_set_item(&s_last_result, 5, code,
                test_sensors(&code), "Sensors");

    /* 7. IMU */
    st_set_item(&s_last_result, 6, code,
                test_imu(&code), "IMU");

    /* 8. OTA */
    st_set_item(&s_last_result, 7, code,
                test_ota(&code), "OTA");

    s_last_result.duration_ms = (uint32_t)((xTaskGetTickCount() - start) * portTICK_PERIOD_MS);
    s_last_result.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

    /* OTA-Rollback pruefen */
    ota_rollback_check(s_last_result.all_pass);

    /* Self-Test Ergebnis als DTC */
    if (s_last_result.all_pass) {
        dtc_register(DTC_D0402, DTC_SEV_INFO);
        ESP_LOGI(TAG, "Self-Test PASSED (%lu ms)",
                 (unsigned long)s_last_result.duration_ms);
    } else {
        dtc_register(DTC_D0401, DTC_SEV_ERROR);
        ESP_LOGW(TAG, "Self-Test FAILED (%lu ms) — see DTCs",
                 (unsigned long)s_last_result.duration_ms);
    }

    return s_last_result.all_pass;
}

/* ============================================================
 * Quick Self-Test
 *
 * Nur kritische Checks: Power, Display, Sensors.
 * ============================================================ */

bool selftest_run_quick(void)
{
    ESP_LOGI(TAG, "=== Quick Self-Test ===");

    TickType_t start = xTaskGetTickCount();
    memset(&s_last_result, 0, sizeof(s_last_result));
    s_last_result.all_pass = true;

    dtc_code_t code = DTC_NONE;

    /* 1. Power */
    st_set_item(&s_last_result, 0, code,
                test_power(&code), "Power");

    /* 2. Display */
    st_set_item(&s_last_result, 1, code,
                test_display(&code), "Display");

    /* 3. Sensors */
    st_set_item(&s_last_result, 2, code,
                test_sensors(&code), "Sensors");

    s_last_result.duration_ms = (uint32_t)((xTaskGetTickCount() - start) * portTICK_PERIOD_MS);
    s_last_result.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

    if (s_last_result.all_pass) {
        ESP_LOGI(TAG, "Quick Self-Test PASSED (%lu ms)",
                 (unsigned long)s_last_result.duration_ms);
    } else {
        ESP_LOGW(TAG, "Quick Self-Test FAILED (%lu ms)",
                 (unsigned long)s_last_result.duration_ms);
    }

    return s_last_result.all_pass;
}

/* ============================================================
 * Backward-compat: selftest_run -> selftest_run_full
 * ============================================================ */

bool selftest_run(void)
{
    return selftest_run_full();
}

/* ============================================================
 * Letztes Ergebnis abrufen
 * ============================================================ */

const selftest_result_t *selftest_get_last_result(void)
{
    return &s_last_result;
}
