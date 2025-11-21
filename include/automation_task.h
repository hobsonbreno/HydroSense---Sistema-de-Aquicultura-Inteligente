#ifndef AUTOMATION_TASK_H
#define AUTOMATION_TASK_H

#include <stdbool.h>
#include <stdint.h>

// Protótipos das funções
void automation_task(void *pvParameters);
void execute_water_level_control(void);
void execute_tpa_if_needed(void);
bool should_trigger_tpa(void);

// Constantes
#define AUTOMATION_INTERVAL 5000  // 5 segundos

#endif // AUTOMATION_TASK_H