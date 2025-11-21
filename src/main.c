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
#include "actuators/servo_feeder.h"
#include "tasks/monitoring_task.h"
#include "tasks/feeding_task.h"

// LED onboard para indicação
#define LED_PIN 25

// Estrutura para dados dos sensores (simplificada)
typedef struct {
    float temperature;
    float ph_value;
    float turbidity;
    bool sensors_ok;
} sensor_data_t;

// Variáveis globais do sistema
SystemStatus_t g_system_status = {0};
SemaphoreHandle_t system_data_mutex;
QueueHandle_t alert_queue;
QueueHandle_t sensor_queue;

void blink_status_led(int times) {
    for (int i = 0; i < times; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(200);
    }
}

static void hardware_init(void) {
    stdio_init_all();
    
    // Inicializa LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
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
    
    // Inicializa servo com controle manual
    servo_feeder_init();
    
    // Cria mutex para proteção de dados compartilhados
    system_data_mutex = xSemaphoreCreateMutex();
    if (system_data_mutex == NULL) {
        printf("❌ Erro ao criar mutex do sistema\n");
        return -1;
    }
    
    // Cria fila para alertas
    alert_queue = xQueueCreate(5, sizeof(int));
    sensor_queue = xQueueCreate(5, sizeof(sensor_data_t));
    if (alert_queue == NULL || sensor_queue == NULL) {
        printf("❌ Erro ao criar filas\n");
        return -1;
    }
    
    // Cria as tarefas do sistema
    BaseType_t task_result;
    
    task_result = xTaskCreate(
        monitoring_task,
        "Monitor",
        1024,
        NULL,
        2,
        NULL
    );
    if (task_result != pdPASS) {
        printf("❌ Erro ao criar task de monitoramento\n");
        return -1;
    }
    
    task_result = xTaskCreate(
        feeding_task,
        "Feeding",
        2048,
        NULL,
        3,
        NULL
    );
    if (task_result != pdPASS) {
        printf("❌ Erro ao criar task de alimentação\n");
        return -1;
    }
    
    printf("✅ Todas as tarefas criadas com sucesso!\n");
    printf("🚀 Iniciando sistema HydroSense...\n");
    printf("🔘 Botão A (GPIO 5): Alimentação manual\n");
    printf("🔄 Alimentação automática: A cada 30 segundos\n\n");
    
    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();
    
    // Nunca deveria chegar aqui
    printf("❌ Erro crítico: Scheduler parou!\n");
    return -1;
}