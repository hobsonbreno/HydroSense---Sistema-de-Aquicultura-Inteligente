#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Implementação simplificada para o HydroSense
uint32_t sys_now(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void sys_arch_protect(void) {
    taskENTER_CRITICAL();
}

void sys_arch_unprotect(void) {
    taskEXIT_CRITICAL();
}