#ifndef _POWER_H_
#define _POWER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

typedef enum {
    POWER_STATE_ACTIVE,
    POWER_STATE_IDLE,
    POWER_STATE_SLEEP,
    POWER_STATE_SHUTDOWN,
} power_state_t;

void power_init(void);
void power_task(void *arg);

float power_get_battery_voltage(void);
power_state_t power_get_state(void);

#endif
