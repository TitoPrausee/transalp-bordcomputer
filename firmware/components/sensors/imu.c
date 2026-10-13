/*
 * imu.c — 6-Achsen IMU ueber I2C
 *
 * Sensor: 6-Achsen IMU, Adresse 0x68 (AD0=LOW).
 * I2C: SDA=GPIO8, SCL=GPIO9, 400kHz.
 * INT-Pin: GPIO2 (Data-Ready IRQ, optional).
 *
 * Konfiguration:
 *   - Gyro: ±250 Grad/s (ausreichend fuer Motorrad-Neigung)
 *   - Accel: ±2g (ausreichend fuer Neigung, Crash-Erkennung)
 *   - DLPF: 42Hz Gyro / 44Hz Accel (filtert Motorvibration)
 *   - Sample-Rate: 50Hz (SMPLRT_DIV=19)
 *
 * Komplementaerfilter fuer Roll/Pitch (alpha=0.98).
 * Crash-Erkennung: Beschleunigung >3g ploetzlich.
 * Temperaturreading vom Sensor-Die.
 *
 * Alle 20ms (50Hz) lesen.
 */

#include "sensors.h"
#include "board_config.h"
#include "filters.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "event_bus.h"
#include "diagnostics.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "imu";

/* Sensor Register */
#define REG_SMPLRT_DIV       0x19
#define REG_CONFIG            0x1A
#define REG_GYRO_CONFIG       0x1B
#define REG_ACCEL_CONFIG      0x1C
#define REG_INT_PIN_CFG       0x37
#define REG_INT_ENABLE        0x38
#define REG_ACCEL_XOUT_H       0x3B
#define REG_TEMP_OUT_H         0x41
#define REG_GYRO_XOUT_H        0x43
#define REG_PWR_MGMT_1         0x6B
#define REG_WHO_AM_I           0x75

#define IMU_I2C_ADDR          0x68
#define IMU_WHO_AM_I_VAL      0x68

/* Sensitivities */
/* ±2g -> 16384 LSB/g, ±250 Grad/s -> 131 LSB/(Grad/s) */
#define ACCEL_SENSITIVITY     16384.0f
#define GYRO_SENSITIVITY      131.0f

/* Crash-Detection: |accel_total| > 3g */
#define CRASH_THRESHOLD_G     3.0f

/* Complementary Filter */
static complementary_filter_t s_roll_filter;
static complementary_filter_t s_pitch_filter;

/* I2C Device Handle */
static i2c_master_dev_handle_t s_imu_dev = NULL;
static bool s_imu_present = false;

/* I2C Read/Write Helpers */

static esp_err_t imu_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_imu_dev, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t imu_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(s_imu_dev, &reg, 1,
                                       buf, len, pdMS_TO_TICKS(100));
}

static bool imu_check_present(void)
{
    uint8_t whoami = 0;
    esp_err_t ret = imu_read_regs(REG_WHO_AM_I, &whoami, 1);
    if (ret != ESP_OK || whoami != IMU_WHO_AM_I_VAL) {
        return false;
    }
    return true;
}

void imu_init(void)
{
    /* I2C Device zum Bus hinzufuegen (Bus wird von sensors_init erstellt) */
    if (!g_i2c_bus) {
        ESP_LOGE(TAG, "I2C bus not initialized — call sensors_init first");
        return;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IMU_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(g_i2c_bus, &dev_cfg, &s_imu_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(ret));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    /* Sensor pruefen */
    s_imu_present = imu_check_present();
    if (!s_imu_present) {
        ESP_LOGE(TAG, "IMU not responding (WHO_AM_I mismatch)");
        dtc_report(DTC_S0401);
        return;
    }

    /* Wake up: PWR_MGMT_1 = 0x01 (PLL, no sleep) */
    imu_write_reg(REG_PWR_MGMT_1, 0x01);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Sample Rate: 50Hz (SMPLRT_DIV=19, 1kHz/(1+19)=50Hz) */
    imu_write_reg(REG_SMPLRT_DIV, 19);

    /* DLPF: 42Hz Gyro / 44Hz Accel (CONFIG=0x03) */
    imu_write_reg(REG_CONFIG, 0x03);

    /* Gyro: ±250 Grad/s (GYRO_CONFIG=0x00) */
    imu_write_reg(REG_GYRO_CONFIG, 0x00);

    /* Accel: ±2g (ACCEL_CONFIG=0x00) */
    imu_write_reg(REG_ACCEL_CONFIG, 0x00);

    /* INT Pin: Push-Pull, Active High, Latch */
    imu_write_reg(REG_INT_PIN_CFG, 0x20);

    /* INT Enable: Data Ready */
    imu_write_reg(REG_INT_ENABLE, 0x01);

    /* Filter initialisieren: alpha=0.98, dt=0.02s (50Hz) */
    comp_filter_init(&s_roll_filter, 0.98f, 0.02f);
    comp_filter_init(&s_pitch_filter, 0.98f, 0.02f);

    ESP_LOGI(TAG, "IMU initialized (addr=0x%02X, 50Hz, ±2g, ±250°/s)",
             IMU_I2C_ADDR);
}

void imu_read(sensor_data_t *data)
{
    if (!data) {
        return;
    }

    if (!s_imu_present || !s_imu_dev) {
        /* Sensor nicht verfuegbar -> Werte auf 0 */
        data->accel_x_g = 0.0f;
        data->accel_y_g = 0.0f;
        data->accel_z_g = 0.0f;
        data->gyro_x_dps = 0.0f;
        data->gyro_y_dps = 0.0f;
        data->gyro_z_dps = 0.0f;
        data->roll_deg = 0.0f;
        data->pitch_deg = 0.0f;
        data->crash_detected = false;
        data->temp_c = 0.0f;
        return;
    }

    /* 14 Bytes ab Reg 0x3B lesen:
     *   ax (2B), ay (2B), az (2B), temp (2B), gx (2B), gy (2B), gz (2B)
     */
    uint8_t buf[14];
    esp_err_t ret = imu_read_regs(REG_ACCEL_XOUT_H, buf, 14);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        /* DTC nur einmal melden, nicht bei jedem Lesen */
        static uint32_t s_last_dtc_report = 0;
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        if (now - s_last_dtc_report > 2000) {
            dtc_report(DTC_S0401);
            s_last_dtc_report = now;
        }
        return;
    }

    /* Rohdaten zusammenbauen (Big-Endian) */
    int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t temp_raw = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t gx = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

    /* In physikalische Einheiten umrechnen */
    float ax_g = (float)ax / ACCEL_SENSITIVITY;
    float ay_g = (float)ay / ACCEL_SENSITIVITY;
    float az_g = (float)az / ACCEL_SENSITIVITY;
    float gx_dps = (float)gx / GYRO_SENSITIVITY;
    float gy_dps = (float)gy / GYRO_SENSITIVITY;
    float gz_dps = (float)gz / GYRO_SENSITIVITY;

    /* Die-Temperatur: temp_c = temp_raw/340 + 36.53 */
    float die_temp = (float)temp_raw / 340.0f + 36.53f;

    /* Roll/Pitch aus Accel (Referenzwinkel) */
    float accel_roll = atan2f(ay_g, az_g) * 57.2957795131f;  /* RAD_TO_DEG */
    float accel_pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g))
                        * 57.2957795131f;

    /* Komplementaerfilter: kombiniert Gyro (kurzfristig) und Accel (langfristig) */
    float roll = comp_filter_update(&s_roll_filter, gx_dps, accel_roll);
    float pitch = comp_filter_update(&s_pitch_filter, gy_dps, accel_pitch);

    /* Crash-Erkennung: Vektorbetrag der Beschleunigung > Threshold */
    float total_accel = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
    bool crash = (total_accel > CRASH_THRESHOLD_G);

    if (crash) {
        event_bus_set(EVT_CRASH_DETECTED);
        ESP_LOGW(TAG, "Crash detected: |a|=%.2fg", (double)total_accel);
    }

    /* Daten in struct schreiben */
    data->accel_x_g = ax_g;
    data->accel_y_g = ay_g;
    data->accel_z_g = az_g;
    data->gyro_x_dps = gx_dps;
    data->gyro_y_dps = gy_dps;
    data->gyro_z_dps = gz_dps;
    data->roll_deg = roll;
    data->pitch_deg = pitch;
    data->crash_detected = crash;
    data->temp_c = die_temp;
}
