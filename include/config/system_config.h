#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

// Definições de pinos
#define TEMP_SENSOR_PIN         26  // ADC0
#define PH_SENSOR_PIN           27  // ADC1  
#define TURBIDITY_SENSOR_PIN    28  // ADC2
#define WATER_LEVEL_SDA         4   // I2C SDA
#define WATER_LEVEL_SCL         5   // I2C SCL
#define PUMP1_PIN               15  // Bomba TPA (PWM)
#define PUMP2_PIN               14  // Bomba reabastecimento (PWM)
#define SERVO_PIN               16  // Servo alimentador (PWM)

// Parâmetros ideais da água
#define TEMP_MIN_IDEAL          22.0f
#define TEMP_MAX_IDEAL          28.0f
#define PH_MIN_IDEAL            6.5f
#define PH_MAX_IDEAL            8.0f
#define TURBIDITY_MAX_ACCEPTABLE 10.0f
#define WATER_LEVEL_MIN         20.0f

// Intervalos das tarefas (ms)
#define MONITORING_INTERVAL     30000   // 30 segundos
#define AUTOMATION_INTERVAL     10000   // 10 segundos
#define FEEDING_CHECK_INTERVAL  60000   // 1 minuto

// URLs e configurações MQTT
#define MQTT_BROKER_URL         "broker.hivemq.com"
#define MQTT_BROKER_PORT        1883

// Estruturas de dados
typedef struct {
    float temperature;
    float ph;
    float turbidity;
    float water_level_percent;
    bool pump1_running;
    bool pump2_running;
    bool tpa_in_progress;
    uint32_t last_feeding_time;
    uint32_t system_uptime;
} SystemStatus_t;

typedef struct {
    bool temperature_alert;
    bool ph_alert;
    bool turbidity_alert;
    bool water_level_alert;
} SystemAlerts_t;

// Horários de alimentação (hora * 100 + minuto)
extern const uint16_t feeding_times[];
extern const uint8_t feeding_times_count;

// Funções de configuração
void system_config_init(void);
void get_system_status(SystemStatus_t *status);
void set_system_status(const SystemStatus_t *status);

#endif // SYSTEM_CONFIG_H