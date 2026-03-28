#include "automation_task.h"
#include "hydrosense_system.h"
#include "monitoring_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include "../hydrosense_utils.h"
#include "../config/system_config.h"
#include "pump_controller.h"
#include "sensors/ph_sensor.h"
#include "sensors/turbidity_sensor.h"

extern SystemStatus_t g_system_status;
extern SemaphoreHandle_t system_data_mutex;
extern QueueHandle_t alert_queue;

static bool tpa_in_progress = false;
static uint32_t tpa_start_time = 0;

void automation_task(void *pvParameters) {
    printf("🤖 Task de automação iniciada\n");
    
    pump_controller_init();
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        execute_water_level_control();
        execute_tpa_if_needed();
        
        SystemAlerts_t alerts;
        if (xQueueReceive(alert_queue, &alerts, 0) == pdTRUE) {
            if (alerts.ph_alert || alerts.turbidity_alert) {
                printf("🚨 ALERTA: Qualidade da água comprometida - TPA necessária\n");
            }
            if (alerts.water_level_alert) {
                printf("🚨 ALERTA: Nível da água crítico\n");
            }
            if (alerts.temperature_alert) {
                printf("🚨 ALERTA: Temperatura fora da faixa ideal\n");
            }
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(AUTOMATION_INTERVAL));
    }
}

void execute_water_level_control(void) {
    SystemStatus_t status;
    get_system_status(&status);
    
    // CORRIGIDO: Liga bomba 2 somente se nível < 90% (evita transbordamento)
    if (status.water_level_percent < 90.0f && !pump2_is_running() && !tpa_in_progress) {
        printf("💧 Nível baixo (%.1f%%) - Ligando bomba 2 para completar até 90%%\n", status.water_level_percent);
        pump2_start();
    }
    
    // CORRIGIDO: Para a bomba 2 ao atingir 90% (não 100%)
    if (status.water_level_percent >= 90.0f && pump2_is_running()) {
        printf("✅ Nível 90%% atingido - Desligando bomba 2 (segurança anti-transbordamento)\n");
        pump2_stop();
    }
}

void execute_tpa_if_needed(void) {
    if (tpa_in_progress) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        uint32_t tpa_duration = current_time - tpa_start_time;
        
        SystemStatus_t status;
        get_system_status(&status);
        
        if (tpa_duration < 300000 && status.water_level_percent > 25.0f) {
            if (!pump1_is_running()) {
                printf("🚰 TPA Fase 1: Drenando água suja até 25%%\n");
                pump1_start();
            }
        } else {
            if (pump1_is_running()) {
                pump1_stop();
                printf("✅ TPA Fase 1 concluída - Iniciando Fase 2\n");
            }
            
            // CORRIGIDO: Enche até 90% para evitar transbordamento
            if (status.water_level_percent < 90.0f && !pump2_is_running()) {
                printf("💧 TPA Fase 2: Reabastecendo com água limpa até 90%%\n");
                pump2_start();
            } else if (status.water_level_percent >= 90.0f) {
                pump2_stop();
                tpa_in_progress = false;
                printf("✅ TPA CONCLUÍDA com sucesso! (Nível: 90%%)\\n");
                
                if (xSemaphoreTake(system_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_system_status.tpa_in_progress = false;
                    xSemaphoreGive(system_data_mutex);
                }
            }
        }
    } else {
        if (should_trigger_tpa()) {
            printf("🚨 INICIANDO TPA - Troca Parcial de Água\n");
            tpa_in_progress = true;
            tpa_start_time = to_ms_since_boot(get_absolute_time());
            
            if (xSemaphoreTake(system_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_system_status.tpa_in_progress = true;
                xSemaphoreGive(system_data_mutex);
            }
        }
    }
}

bool should_trigger_tpa(void) {
    SystemStatus_t status;
    get_system_status(&status);
    
    bool ph_bad = !ph_is_ideal(status.ph);
    bool turbidity_bad = !turbidity_is_acceptable(status.turbidity);
    
    return (ph_bad || turbidity_bad) && !tpa_in_progress;
}