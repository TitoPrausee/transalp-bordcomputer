/*
 * sensors.c — Task-Wrapper fuer alle Sensoren
 *
 * sensors_init():
 *   - Initialisiere I2C-Bus (shared mit IMU)
 *   - Initialisiere ADC1 (shared mit Temp + Fuel)
 *   - Initialisiere alle Sub-Module (rpm, temperature, fuel, gear, imu)
 *   - Erstelle sensor_data_queue
 *
 * sensor_task():
 *   - FreeRTOS Task auf Core 1, 50Hz (20ms Zyklus)
 *   - Sammle alle Sensorwerte in sensor_data_t
 *   - Publish ueber sensor_data_queue bei neuen Daten
 *   - Watchdog: Wenn keine neuen RPM-Daten fuer 2s -> Warning
 *
 * Thread-safe: Sensor-Task schreibt, BLE/Display lesen ueber Queue.
 */

#include "sensors.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "event_bus.h"
#include "diagnostics.h"
#include "filters.h"
#include "gps.h"
#include <string.h>

static const char *TAG = "sensors";

/* Shared Handles */
adc_oneshot_unit_handle_t g_adc1_handle = NULL;
adc_cali_handle_t g_adc_cali_handle = NULL;
i2c_master_bus_handle_t g_i2c_bus = NULL;

/* Sensor-Data Queue (neueste Daten fuer BLE/Display/Logger) */
QueueHandle_t sensor_data_queue = NULL;

/* RPM Watchdog: letzter Zeitpunkt mit gueltigen RPM-Daten */
static uint32_t s_last_rpm_update_ms = 0;

/* ADC-Konfiguration (shared fuer Temp + Fuel) */
static void init_adc(void)
{
    /* ADC1 Unit initialisieren */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &g_adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Kanal fuer Kuehlmittel-Temperatur (NTC) konfigurieren */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,       /* bis ~3.1V */
        .bitwidth = ADC_BITWIDTH_12,   /* 12-bit: 0-4095 */
    };
    adc_oneshot_config_channel(g_adc1_handle,
                               (adc_channel_t)TEMP_NTC_ADC_CHANNEL, &chan_cfg);

    /* Kanal fuer Kraftstoffstand konfigurieren */
    adc_oneshot_config_channel(g_adc1_handle,
                               (adc_channel_t)FUEL_ADC_CHANNEL, &chan_cfg);

    /* Kanal fuer Batteriespannung konfigurieren */
    adc_oneshot_config_channel(g_adc1_handle,
                               (adc_channel_t)BATT_ADC_CHANNEL, &chan_cfg);

    /* Kalibrierung (curve fitting for ESP32-S3) */
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &g_adc_cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration failed: %s (using raw values)",
                 esp_err_to_name(ret));
        g_adc_cali_handle = NULL;
    }
}

/* I2C-Bus initialisieren (shared mit IMU) */
static void init_i2c(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &g_i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Externe Pull-Ups bereits auf Platine (4.7k Ohm) */
    ESP_LOGI(TAG, "I2C bus initialized (SDA=%d, SCL=%d, %dHz)",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
}

void sensors_init(void)
{
    ESP_LOGI(TAG, "Initializing sensor subsystem...");

    /* I2C-Bus zuerst (IMU benoetigt es) */
    init_i2c();

    /* ADC1 (shared mit Temp + Fuel) */
    init_adc();

    /* Sub-Module initialisieren */
    rpm_init();
    temperature_init();
    fuel_level_init();
    gear_indicator_init();
    imu_init();

    /* Queue fuer Sensor-Daten erstellen (Depth 4: neueste ueberschreibt aelteste) */
    sensor_data_queue = xQueueCreate(4, sizeof(sensor_data_t));
    if (sensor_data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor_data_queue");
    }

    s_last_rpm_update_ms = 0;

    ESP_LOGI(TAG, "Sensor subsystem initialized (50Hz task on Core %d)",
             TASK_CORE_RT);
}

/*
 * Hilfsfunktion: Millisekunden seit Boot.
 */
static uint32_t get_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void sensor_task(void *arg)
{
    sensor_data_t data = {0};
    memset(&data, 0, sizeof(data));

    /* GPS-Speed aus GPS-Queue lesen (non-blocking) */
    float gps_speed_kmh = 0.0f;

    ESP_LOGI(TAG, "Sensor task started on Core %d", xPortGetCoreID());

    while (1) {
        /* GPS-Speed abholen (falls verfuegbar) */
        gps_data_t gps_data;
        if (xQueuePeek(gps_data_queue, &gps_data, 0) == pdPASS) {
            gps_speed_kmh = gps_data.speed_kmh;
        }

        /* --- IMU lesen (50Hz) --- */
        imu_read(&data);

        /* --- RPM lesen (10Hz intern, aber jeder Task-Tick fragt ab) --- */
        uint16_t rpm = rpm_read();
        data.rpm = rpm;

        if (rpm > 0) {
            s_last_rpm_update_ms = get_millis();
        } else {
            /* Watchdog: keine RPM-Daten fuer 2s -> Warning */
            if (s_last_rpm_update_ms > 0) {
                uint32_t now = get_millis();
                if ((now - s_last_rpm_update_ms) > 2000) {
                    event_bus_set(EVT_RPM_TIMEOUT);
                    dtc_report(DTC_P0102);
                    /* Reset, damit DTC nicht jeden Tick erneut feuert */
                    s_last_rpm_update_ms = now;
                }
            }
        }

        /* --- Kuehlmittel-Temperatur (alle 500ms = jeder 25. Tick) --- */
        static uint16_t temp_counter = 0;
        if (temp_counter == 0) {
            data.coolant_temp_c = temperature_read_coolant();
        }
        temp_counter = (temp_counter + 1) % 25;

        /* --- Kraftstoffstand (alle 500ms = jeder 25. Tick) --- */
        static uint16_t fuel_counter = 0;
        if (fuel_counter == 0) {
            data.fuel_level_l = fuel_level_read_liters();
        }
        fuel_counter = (fuel_counter + 1) % 25;

        /* --- Ganganzeige (jeder Tick, da RPM+Speed verfuegbar) --- */
        data.speed_kmh = gps_speed_kmh;
        data.gear = gear_indicator_calc(data.speed_kmh, data.rpm);

        /* --- Timestamp --- */
        data.timestamp_ms = get_millis();

        /* --- Daten publishen (ueberschreibt aelteste wenn Queue voll) --- */
        xQueueOverwrite(sensor_data_queue, &data);

        /* 50Hz = 20ms Zyklus */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
