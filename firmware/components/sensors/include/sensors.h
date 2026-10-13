#ifndef _SENSORS_H_
#define _SENSORS_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "driver/i2c_master.h"

/*
 * sensors.h — Sensor-Datenstrukturen und Task-Interface
 *
 * Sensor-Task laeuft auf Core 1 mit 50Hz.
 * Sammelt: IMU (6-Achsen), RPM (Pulse-Counter),
 *           Kuehlmittel-Temp (DS18B20/NTC), Kraftstoff (ADC),
 *           Gang (berechnet aus Speed+RPM).
 */

typedef struct {
    /* IMU (6-Achsen) */
    float accel_x_g, accel_y_g, accel_z_g;
    float gyro_x_dps, gyro_y_dps, gyro_z_dps;
    float roll_deg, pitch_deg;

    /* Temperatur */
    float temp_c;           /* Celsius (Sensor-Die-Temp) */
    float pressure_hpa;     /* hPa (nicht implementiert) */
    float altitude_m;       /* Meter ueber NN (nicht implementiert) */

    /* Fahrzeug-Sensoren */
    uint16_t rpm;           /* Drehzahl (U/min) */
    float speed_kmh;        /* Geschwindigkeit (km/h) */
    uint8_t gear;           /* Gang (0=Neutral, 1-6) */
    float coolant_temp_c;   /* Kuehlmittel-Temperatur */
    float fuel_level_l;     /* Tankinhalt (Liter) */

    /* Diagnose */
    bool crash_detected;
    uint32_t timestamp_ms;
} sensor_data_t;

/* API */
void sensors_init(void);
void sensor_task(void *arg);

/* Queue: neueste Sensor-Daten (Andere Tasks lesen hieraus) */
extern QueueHandle_t sensor_data_queue;

/* Shared handles (von sensors_init initialisiert) */
extern adc_oneshot_unit_handle_t g_adc1_handle;
extern adc_cali_handle_t g_adc_cali_handle;
extern i2c_master_bus_handle_t g_i2c_bus;

/* Sub-Modul API */
void rpm_init(void);
uint16_t rpm_read(void);

void temperature_init(void);
float temperature_read_coolant(void);

void fuel_level_init(void);
float fuel_level_read_liters(void);

void gear_indicator_init(void);
uint8_t gear_indicator_calc(float speed_kmh, uint16_t rpm);

void imu_init(void);
void imu_read(sensor_data_t *data);

#endif /* _SENSORS_H_ */
