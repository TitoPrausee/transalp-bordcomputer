#ifndef _GPS_H_
#define _GPS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct {
    float lat;
    float lon;
    float speed_kmh;
    float course_deg;
    float altitude_m;
    uint8_t satellites;
    bool fix;
    uint32_t timestamp_ms;
} gps_data_t;

void gps_init(void);
void gps_task(void *arg);

/* Queue handle für andere Tasks */
extern QueueHandle_t gps_data_queue;

#endif
