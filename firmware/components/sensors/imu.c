/*
 * imu.c — MPU6050 (6-Achsen IMU) ueber I2C
 *
 * Konfiguration (siehe docs/research/03-software-stack.md Abschnitt 5.2):
 *   - Gyro: ±2000 °/s (Motorrad kann schnell drehen)
 *   - Accel: ±4g (ausreichend fuer Neigung, nicht fuer Crash)
 *   - DLPF: 42Hz Gyro / 44Hz Accel (filtert Motorvibration)
 *   - Sample-Rate: 50Hz (SMPLRT_DIV=19)
 *
 * Pin: I2C SDA=GPIO8, SCL=GPIO9, INT=GPIO2.
 * Adresse: 0x68 (AD0=LOW).
 *
 * TODO: I2C-Master initialisieren (neue v5.x API mit Handle).
 * TODO: Sensor-Fusion (Complementary Filter) fuer Roll/Pitch.
 * TODO: Crash-Detection (|accel| > Threshold -> Event).
 * TODO: MPU6050 INT-Pin fuer Data-Ready-IRQ nutzen (statt Polling).
 */

#include "sensors.h"
#include "board_config.h"
#include "filters.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "imu";

/* MPU6050 Register */
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG        0x1A
#define MPU6050_REG_GYRO_CONFIG   0x1B
#define MPU6050_REG_ACCEL_CONFIG  0x1C
#define MPU6050_REG_INT_PIN_CFG   0x37
#define MPU6050_REG_INT_ENABLE    0x38
#define MPU6050_REG_ACCEL_XOUT_H  0x3B
#define MPU6050_REG_PWR_MGMT_1    0x6B
#define MPU6050_REG_WHO_AM_I      0x75

/* Sensitivities */
#define ACCEL_SENSITIVITY   8192.0f   /* ±4g -> LSB/g */
#define GYRO_SENSITIVITY    16.4f     /* ±2000°/s -> LSB/(°/s) */

/* Complementary Filter fuer Roll/Pitch */
static complementary_filter_t roll_filter;
static complementary_filter_t pitch_filter;

/* I2C Handle (shared mit BMP280) */
static i2c_master_dev_handle_t mpu6050_dev = NULL;

/* Crash-Detection */
#define CRASH_THRESHOLD_G    3.0f

void imu_init(void)
{
    /* TODO: I2C-Master-Bus initialisieren (shared mit BMP280).
     *
     * i2c_master_bus_config_t bus_config = {
     *     .i2c_port = I2C_MASTER_NUM,
     *     .sda_io_num = I2C_MASTER_SDA_IO,
     *     .scl_io_num = I2C_MASTER_SCL_IO,
     *     .clk_source = I2C_CLK_SRC_DEFAULT,
     *     .glitch_ignore_cnt = 7,
     *     .flags.enable_internal_pullup = false,
     * };
     * i2c_new_master_bus(&bus_config, &i2c_bus_handle);
     *
     * i2c_device_config_t dev_config = {
     *     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
     *     .device_address = MPU6050_I2C_ADDR,
     *     .scl_speed_hz = I2C_MASTER_FREQ_HZ,
     * };
     * i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &mpu6050_dev);
     *
     * // Wake up + configure
     * mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x01);   // PLL, no sleep
     * vTaskDelay(pdMS_TO_TICKS(100));
     * mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 19);     // 50Hz
     * mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03);       // DLPF 42Hz
     * mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, 0x18);  // ±2000°/s
     * mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, 0x08); // ±4g
     */

    /* Filter initialisieren: alpha=0.98, dt=0.02s (50Hz) */
    comp_filter_init(&roll_filter, 0.98f, 0.02f);
    comp_filter_init(&pitch_filter, 0.98f, 0.02f);

    ESP_LOGI(TAG, "IMU (MPU6050) init (stub)");
}

void imu_read(sensor_data_t *data)
{
    /* TODO: 14 Bytes ab Reg 0x3B lesen:
     *   ax (2B), ay (2B), az (2B), temp (2B), gx (2B), gy (2B), gz (2B)
     *
     * uint8_t buf[14];
     * mpu6050_read_regs(MPU6050_REG_ACCEL_XOUT_H, buf, 14);
     * int16_t ax = (buf[0]<<8)|buf[1];
     * int16_t ay = (buf[2]<<8)|buf[3];
     * int16_t az = (buf[4]<<8)|buf[5];
     * int16_t gx = (buf[8]<<8)|buf[9];
     * int16_t gy = (buf[10]<<8)|buf[11];
     * int16_t gz = (buf[12]<<8)|buf[13];
     *
     * // Convert to physical units
     * float ax_g = ax / ACCEL_SENSITIVITY;
     * float ay_g = ay / ACCEL_SENSITIVITY;
     * float az_g = az / ACCEL_SENSITIVITY;
     * float gx_dps = gx / GYRO_SENSITIVITY;
     * float gy_dps = gy / GYRO_SENSITIVITY;
     * float gz_dps = gz / GYRO_SENSITIVITY;
     *
     * // Roll/Pitch from accel
     * float accel_roll = atan2f(ay_g, az_g) * 180.0f / M_PI;
     * float accel_pitch = atan2f(-ax_g, sqrtf(ay_g*ay_g + az_g*az_g)) * 180.0f / M_PI;
     *
     * // Complementary filter
     * data->roll_deg = comp_filter_update(&roll_filter, gx_dps, accel_roll);
     * data->pitch_deg = comp_filter_update(&pitch_filter, gy_dps, accel_pitch);
     *
     * // Crash detection
     * float total_accel = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
     * data->crash_detected = (total_accel > CRASH_THRESHOLD_G);
     */

    (void)data;  /* Stub */
}

/* TODO: I2C Read/Write Helpers
 *
 * static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t val) {
 *     uint8_t buf[2] = {reg, val};
 *     return i2c_master_transmit(mpu6050_dev, buf, 2, pdMS_TO_TICKS(100));
 * }
 *
 * static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *buf, size_t len) {
 *     return i2c_master_transmit_receive(mpu6050_dev, &reg, 1,
 *                                         buf, len, pdMS_TO_TICKS(100));
 * }
 */
