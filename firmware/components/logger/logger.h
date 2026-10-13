#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void logger_init(void);
void logger_task(void *arg);

#endif
