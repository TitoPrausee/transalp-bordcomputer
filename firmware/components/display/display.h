#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void display_init(void);
void display_task(void *arg);

#endif
