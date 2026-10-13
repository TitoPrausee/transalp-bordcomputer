#ifndef _SENSORS_H_
#define _SENSORS_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*
 * sensors.h — Sensor-Datenstrukturen und Task-Interface
 *
 * Sensor-Task laeuft auf Core 1 mit 50Hz.
 * Sammelt: MPU6050 (IMU), BMP280 (Temp/Druck/Hoehe),
 *           RPM (Pulse-Counter), NTC (Temperatur), Fuel (ADC).
 */

typedef struct {
    /* IMU (MPU6050) */
    float accel_x_g, accel_y_g, accel_z_g;
    float gyro_x_dps, gyro_y_dps, gyro_z_dps;
    float roll_deg, pitch_deg;

    /* BMP280 */
    float temp_c;           /* Celsius */
    float pressure_hpa;     /* hPa */
    float altitude_m;       /* Meter ueber NN */

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

#endif /* _SENSORS_H_ */
