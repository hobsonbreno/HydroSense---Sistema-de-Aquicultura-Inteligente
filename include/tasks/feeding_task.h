#ifndef FEEDING_TASK_H
#define FEEDING_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "system_config.h"

void feeding_task(void *pvParameters);
bool is_feeding_time(void);
uint16_t get_current_time_hhmm(void);

#endif // FEEDING_TASK_H