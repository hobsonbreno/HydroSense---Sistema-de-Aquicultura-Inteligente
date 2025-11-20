#include "mqtt_client.h"
#include "system_config.h"
#include "monitoring_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

static mqtt_client_t mqtt_client = {0};

extern SystemStatus_t g_system_status;
extern SemaphoreHandle_t system_data_mutex;

void mqtt_init(void) {
    printf("🌐 MQTT Client inicializado (modo placeholder)\n");
    strcpy(mqtt_client.broker_url, MQTT_BROKER_URL);
    mqtt_client.broker_port = MQTT_BROKER_PORT;
    mqtt_client.connected = false;
}

void mqtt_task(void *pvParameters) {
    printf("📡 Task MQTT iniciada\n");
    
    mqtt_init();
    
    while (1) {
        if (!mqtt_client.connected) {
            printf("📡 Tentando conectar ao broker MQTT: %s:%d\n", 
                   mqtt_client.broker_url, mqtt_client.broker_port);
            vTaskDelay(pdMS_TO_TICKS(30000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void mqtt_publish_sensor_data(SystemStatus_t *status) {
    printf("📤 [MQTT] Dados dos sensores:\n");
    printf("   - Temperatura: %.1f°C\n", status->temperature);
    printf("   - pH: %.1f\n", status->ph);
    printf("   - Turbidez: %.1f NTU\n", status->turbidity);
    printf("   - Nível: %.1f%%\n", status->water_level_percent);
}

void mqtt_publish_alert(const char* alert_message) {
    printf("🚨 [MQTT] Alerta: %s\n", alert_message);
}