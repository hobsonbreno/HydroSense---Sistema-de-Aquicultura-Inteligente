#include "monitoring_task.h"
#include "../hydrosense_utils.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <system_config.h>

#ifndef AUTOMATION_INTERVAL
#define AUTOMATION_INTERVAL 10000 // 10 segundos
#endif

extern SemaphoreHandle_t system_data_mutex;
extern QueueHandle_t alert_queue;

void temperature_sensor_init(void);
void ph_sensor_init(void);
void turbidity_sensor_init(void);
void water_level_sensor_init(void);
float temperature_read_value(void);
float ph_read_value(void);
float turbidity_read_value(void);
float water_level_read_percent(void);
void check_alerts(void);
void update_system_uptime(void);

void monitoring_task(void *pvParameters) {
    printf("📊 Task de monitoramento iniciada\n");
    
    // Inicializa todos os sensores
    temperature_sensor_init();
    ph_sensor_init();
    turbidity_sensor_init();
    water_level_sensor_init();
    
    system_config_init();
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Lê todos os sensores
        read_all_sensors();
        
        // Verifica alertas
        check_alerts();
        
        // Atualiza tempo de funcionamento
        update_system_uptime();
        
        // Exibe status atual
        SystemStatus_t status;
        get_system_status(&status);
        
        printf("📊 [Monitor] Temp: %.1f°C | pH: %.1f | Turbidez: %.1f NTU | Nível: %.1f%%\n",
               status.temperature, status.ph, status.turbidity, status.water_level_percent);
        
        // Aguarda próximo ciclo
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MONITORING_INTERVAL));
    }
}

void read_all_sensors(void) {
    SystemStatus_t status;
    get_system_status(&status);
    
    // Lê sensores
    status.temperature = temperature_read_value();
    status.ph = ph_read_value();
    status.turbidity = turbidity_read_value();
    status.water_level_percent = water_level_read_percent();
    
    set_system_status(&status);
}

void check_alerts(void) {
    SystemStatus_t status;
    get_system_status(&status);
    
    SystemAlerts_t alerts = {0};
    
    // Verifica alertas de temperatura
    if (status.temperature < TEMP_MIN_IDEAL || status.temperature > TEMP_MAX_IDEAL) {
        alerts.temperature_alert = true;
    }
    
    // Verifica alertas de pH
    if (status.ph < PH_MIN_IDEAL || status.ph > PH_MAX_IDEAL) {
        alerts.ph_alert = true;
    }
    
    // Verifica alertas de turbidez
    if (status.turbidity > TURBIDITY_MAX_ACCEPTABLE) {
        alerts.turbidity_alert = true;
    }
    
    // Verifica alertas de nível da água
    if (status.water_level_percent < WATER_LEVEL_MIN) {
        alerts.water_level_alert = true;
    }
    
    // Envia alertas se necessário
    if (alerts.temperature_alert || alerts.ph_alert || 
        alerts.turbidity_alert || alerts.water_level_alert) {
        xQueueSend(alert_queue, &alerts, 0);
    }
}

void update_system_uptime(void) {
    SystemStatus_t status;
    get_system_status(&status);
    
    status.system_uptime = to_ms_since_boot(get_absolute_time()) / 1000; // em segundos
    
    set_system_status(&status);
}