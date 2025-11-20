#ifndef MONITORING_TASK_H
#define MONITORING_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "system_config.h"

// Task principal de monitoramento
void monitoring_task(void *pvParameters);

// Funções auxiliares
void read_all_sensors(void);
void check_alerts(void);
void update_system_uptime(void);

#endif // MONITORING_TASK_H