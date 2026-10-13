#ifndef _POWER_H_
#define _POWER_H_

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * power.h — Power-Management: Akku-Ueberwachung, Sleep-Modi
 *
 * Power-Task laeuft auf Core 0 mit 1Hz.
 * ADC: GPIO4 (Spannungsteiler 2:1, max 3.3V).
 */

typedef enum {
    POWER_STATE_ACTIVE = 0,
    POWER_STATE_IDLE,
    POWER_STATE_SLEEP,
    POWER_STATE_SHUTDOWN,
} power_state_t;

/* API */
void power_init(void);
void power_task(void *arg);

float power_get_battery_voltage(void);
uint8_t power_get_battery_percent(void);
power_state_t power_get_state(void);

#endif /* _POWER_H_ */
