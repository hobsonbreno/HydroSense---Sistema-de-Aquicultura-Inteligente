#include "system_config.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>

// Variáveis externas
extern SystemStatus_t g_system_status;
extern SemaphoreHandle_t system_data_mutex;

// Horários de alimentação: 00:00, 08:00, 16:00 (de 8 em 8 horas)
const uint16_t feeding_times[] = {0, 800, 1600}; // 00:00, 08:00, 16:00
const uint8_t feeding_times_count = 3;

void system_config_init(void) {
    printf("⚙️ Inicializando configurações do sistema...\n");
    
    // Inicializa status do sistema
    if (xSemaphoreTake(system_data_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        g_system_status.temperature = 0.0f;
        g_system_status.ph = 7.0f;
        g_system_status.turbidity = 0.0f;
        g_system_status.water_level_percent = 100.0f;
        g_system_status.pump1_running = false;
        g_system_status.pump2_running = false;
        g_system_status.tpa_in_progress = false;
        g_system_status.last_feeding_time = 0;
        g_system_status.system_uptime = 0;
        xSemaphoreGive(system_data_mutex);
    }
    
    printf("✅ Configurações do sistema carregadas\n");
}

void get_system_status(SystemStatus_t *status) {
    if (xSemaphoreTake(system_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        *status = g_system_status;
        xSemaphoreGive(system_data_mutex);
    }
}

void set_system_status(const SystemStatus_t *status) {
    if (xSemaphoreTake(system_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_system_status = *status;
        xSemaphoreGive(system_data_mutex);
    }
}