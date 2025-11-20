#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>

// Headers do projeto
#include "system_config.h"
#include "monitoring_task.h"
#include "automation_task.h"
#include "feeding_task.h"
#include "mqtt_client.h"

// Variáveis globais do sistema
SystemStatus_t g_system_status = {0};
SemaphoreHandle_t system_data_mutex;
QueueHandle_t alert_queue;

static void hardware_init(void) {
    stdio_init_all();
    
    // Inicializa ADC para sensores
    adc_init();
    
    // Inicializa RTC
    rtc_init();
    datetime_t t = {
        .year  = 2025,
        .month = 11,
        .day   = 20,
        .dotw  = 3, // 0 = Sunday
        .hour  = 12,
        .min   = 0,
        .sec   = 0
    };
    rtc_set_datetime(&t);
    
    printf("🐟 HydroSense - Sistema de Monitoramento de Aquicultura 🐟\n");
    printf("============================================================\n");
}

int main() {
    // Inicialização do hardware
    hardware_init();
    
    // Cria mutex para proteção de dados compartilhados
    system_data_mutex = xSemaphoreCreateMutex();
    if (system_data_mutex == NULL) {
        printf("❌ Erro ao criar mutex do sistema\n");
        return -1;
    }
    
    // Cria fila para alertas
    alert_queue = xQueueCreate(10, sizeof(SystemAlerts_t));
    if (alert_queue == NULL) {
        printf("❌ Erro ao criar fila de alertas\n");
        return -1;
    }
    
    // Cria as tarefas do sistema
    BaseType_t task_result;
    
    task_result = xTaskCreate(
        monitoring_task,
        "Monitor",
        2048,
        NULL,
        3,
        NULL
    );
    if (task_result != pdPASS) {
        printf("❌ Erro ao criar task de monitoramento\n");
        return -1;
    }
    
    task_result = xTaskCreate(
        automation_task,
        "Automation",
        2048,
        NULL,
        2,
        NULL
    );
    if (task_result != pdPASS) {
        printf("❌ Erro ao criar task de automação\n");
        return -1;
    }
    
    task_result = xTaskCreate(
        feeding_task,
        "Feeding",
        1024,
        NULL,
        1,
        NULL
    );
    if (task_result != pdPASS) {
        printf("❌ Erro ao criar task de alimentação\n");
        return -1;
    }
    
    task_result = xTaskCreate(
        mqtt_task,
        "MQTT",
        2048,
        NULL,
        1,
        NULL
    );
    if (task_result != pdPASS) {
        printf("❌ Erro ao criar task MQTT\n");
        return -1;
    }
    
    printf("✅ Todas as tarefas criadas com sucesso!\n");
    printf("🚀 Iniciando sistema HydroSense...\n\n");
    
    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();
    
    // Nunca deveria chegar aqui
    printf("❌ Erro crítico: Scheduler parou!\n");
    return -1;
}