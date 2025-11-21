#ifndef MONITORING_TASK_H
#define MONITORING_TASK_H

#include <stdbool.h>
#include <stdint.h>

// Protótipos das funções
void monitoring_task(void *pvParameters);
void read_all_sensors(void);

// Constantes
#define MONITORING_INTERVAL 2000  // 2 segundos

#endif // MONITORING_TASK_H