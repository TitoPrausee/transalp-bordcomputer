#ifndef _SENSORS_H_
#define _SENSORS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdbool.h>

typedef struct {
    float temp_c;
    float pressure_hpa;
    float altitude_m;
    float roll_deg;
    float pitch_deg;
    float accel_g;
    bool crash_detected;
    uint32_t timestamp_ms;
} sensor_data_t;

void sensors_init(void);
void sensor_task(void *arg);

extern QueueHandle_t sensor_data_queue;

#endif
