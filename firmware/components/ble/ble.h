#ifndef _BLE_H_
#define _BLE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void ble_init(void);
void ble_task(void *arg);

#endif
