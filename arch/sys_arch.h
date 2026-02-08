/**
 * HydroSense - lwIP FreeRTOS Architecture
 * 
 * Header para integração lwIP com FreeRTOS
 */

#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define SYS_MBOX_NULL   NULL
#define SYS_SEM_NULL    NULL

typedef SemaphoreHandle_t sys_sem_t;
typedef SemaphoreHandle_t sys_mutex_t;
typedef QueueHandle_t     sys_mbox_t;
typedef TaskHandle_t      sys_thread_t;

typedef int sys_prot_t;

#endif /* LWIP_ARCH_SYS_ARCH_H */
