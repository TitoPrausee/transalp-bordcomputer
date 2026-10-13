#include "sensors.h"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "sensors";
QueueHandle_t sensor_data_queue = NULL;

#define I2C_MASTER_SDA_IO   8
#define I2C_MASTER_SCL_IO   9
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  400000

#define MPU6050_ADDR         0x68
#define BMP280_ADDR          0x76

void sensors_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    /* TODO: MPU6050 init (PWR_MGMT_1=0, config) */
    /* TODO: BMP280 init (ctrl_meas, config) */

    sensor_data_queue = xQueueCreate(4, sizeof(sensor_data_t));
    ESP_LOGI(TAG, "Sensors initialized (I2C @ %dHz)", I2C_MASTER_FREQ_HZ);
}

void sensor_task(void *arg)
{
    sensor_data_t data = {0};
    while (1) {
        /* TODO: BMP280 lesen (temp, pressure, altitude) */
        /* TODO: MPU6050 lesen (accel, gyro → roll/pitch) */
        /* TODO: Crash-Detection (|accel| > threshold) */
        xQueueOverwrite(sensor_data_queue, &data);
        vTaskDelay(pdMS_TO_TICKS(20)); /* 50Hz */
    }
}
