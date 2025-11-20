#ifndef AUTOMATION_TASK_H
#define AUTOMATION_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "system_config.h"

void automation_task(void *pvParameters);
void execute_water_level_control(void);
void execute_tpa_if_needed(void);
bool should_trigger_tpa(void);

#endif // AUTOMATION_TASK_H