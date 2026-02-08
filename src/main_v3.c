/**
 * HydroSense v3.0 - Sistema de Aquicultura Inteligente
 * 
 * Arquivo principal com todas as tasks FreeRTOS
 * 
 * Recursos:
 * - Tanque de 20 litros com 10 peixes
 * - Sensor VL53L0X (nível de água)
 * - Sensor AHT10 (temperatura e umidade)
 * - Sensor TCS3200 (cor/turbidez)
 * - Servo SG90 (alimentador)
 * - 2 bombas (drenagem e enchimento)
 * - Servidor HTTP com dashboard
 * - Sistema de logs
 * - TPA automática (sujeira e rotatividade)
 */

#include "hydrosense_config.h"
#include "sensors/vl53l0x.h"
#include "sensors/aht10.h"
#include "sensors/tcs3200.h"
#include "tpa_system.h"
#include "feeding_system.h"
#include "webserver.h"
#include "log_system.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/rtc.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <stdio.h>
#include <string.h>

// ============================================================
// Variáveis Globais
// ============================================================
system_status_t g_status = {0};
SemaphoreHandle_t g_status_mutex = NULL;
QueueHandle_t g_log_queue = NULL;

// Handles das tasks
static TaskHandle_t task_sensor_handle = NULL;
static TaskHandle_t task_tpa_handle = NULL;
static TaskHandle_t task_feeding_handle = NULL;
static TaskHandle_t task_webserver_handle = NULL;
static TaskHandle_t task_log_handle = NULL;
static TaskHandle_t task_display_handle = NULL;

// ============================================================
// Task de Sensores
// ============================================================
void sensor_task(void* pvParameters) {
    printf("📊 Task de sensores iniciada\n");
    
    // Inicializa sensores
    bool vl53_ok = vl53l0x_init();
    bool aht_ok = aht10_init();
    bool tcs_ok = tcs3200_init();
    
    printf("📊 Status sensores:\n");
    printf("   VL53L0X (nível): %s\n", vl53_ok ? "✅ OK" : "❌ ERRO");
    printf("   AHT10 (temp/umid): %s\n", aht_ok ? "✅ OK" : "❌ ERRO");
    printf("   TCS3200 (cor): %s\n", tcs_ok ? "✅ OK" : "❌ ERRO");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Lê todos os sensores
        float temperatura = 0, umidade = 0;
        float nivel_litros = 0, nivel_percent = 0;
        float turbidez = 0;
        
        // VL53L0X - Nível de água
        if (vl53_ok) {
            nivel_litros = vl53l0x_get_water_level_liters();
            nivel_percent = vl53l0x_get_water_level_percent();
        }
        
        // AHT10 - Temperatura e Umidade
        if (aht_ok) {
            aht10_read(&temperatura, &umidade);
        }
        
        // TCS3200 - Turbidez
        if (tcs_ok) {
            tcs3200_rgb_t rgb;
            tcs3200_read_rgb(&rgb);
            turbidez = tcs3200_calcular_turbidez();
        }
        
        // Atualiza status global
        if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_status.temperatura = temperatura;
            g_status.umidade = umidade;
            g_status.nivel_litros = nivel_litros;
            g_status.nivel_percentual = nivel_percent;
            g_status.turbidez = turbidez;
            
            // Verifica alertas
            g_status.alerta_temperatura = !aht10_is_temperature_ok(temperatura);
            g_status.alerta_nivel = (nivel_percent < 20.0f);
            g_status.alerta_turbidez = (turbidez >= TURBIDEZ_LIMITE_SUJEIRA);
            
            // Atualiza uptime
            g_status.uptime_segundos = to_ms_since_boot(get_absolute_time()) / 1000;
            
            xSemaphoreGive(g_status_mutex);
        }
        
        // Log periódico
        static uint8_t log_counter = 0;
        if (++log_counter >= 6) {  // A cada 30 segundos (6 * 5s)
            log_counter = 0;
            printf("═══════════════════════════════════════════════════════════════\n");
            printf("📊 LEITURA DOS SENSORES\n");
            printf("🌡️ Temperatura: %.1f°C | Umidade: %.1f%%\n", temperatura, umidade);
            printf("🌊 Nível: %.1fL (%.0f%%) | Turbidez: %.1f%%\n", 
                   nivel_litros, nivel_percent, turbidez);
            printf("═══════════════════════════════════════════════════════════════\n");
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_SENSOR_INTERVAL_MS));
    }
}

// ============================================================
// Task de Display (console)
// ============================================================
void display_task(void* pvParameters) {
    printf("🖥️ Task de display iniciada\n");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Atualiza watchdog
        watchdog_update();
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_DISPLAY_INTERVAL_MS));
    }
}

// ============================================================
// Inicialização do Sistema
// ============================================================
void system_init(void) {
    stdio_init_all();
    sleep_ms(2000);
    
    // Verifica reset por watchdog
    if (watchdog_caused_reboot()) {
        printf("\n⚠️ SISTEMA REINICIADO PELO WATCHDOG!\n\n");
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                              ║\n");
    printf("║   🐟 HydroSense v%s                                       ║\n", HYDROSENSE_VERSION);
    printf("║   Sistema de Aquicultura Inteligente + Hidroponia            ║\n");
    printf("║                                                              ║\n");
    printf("║   📅 Build: %s %s                            ║\n", HYDROSENSE_BUILD_DATE, HYDROSENSE_BUILD_TIME);
    printf("║                                                              ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║   📋 Configuração do Sistema                                 ║\n");
    printf("║   • Tanque: %.0fL | Peixes: %d                               ║\n", 
           TANQUE_CAPACIDADE_LITROS, TANQUE_QTD_PEIXES);
    printf("║   • Alimentação: %02d:00 e %02d:00 (%dg/refeição)              ║\n",
           ALIMENTACAO_HORARIO_1, ALIMENTACAO_HORARIO_2, ALIMENTACAO_GRAMAS);
    printf("║   • TPA Rotatividade: a cada 2h (50%% -> 100%%)               ║\n");
    printf("║   • TPA Sujeira: automática (25%% -> 100%%)                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    // Inicializa RTC
    rtc_init();
    datetime_t t = {
        .year = 2024,
        .month = 1,
        .day = 1,
        .dotw = 1,
        .hour = 12,
        .min = 0,
        .sec = 0
    };
    rtc_set_datetime(&t);
    printf("🕐 RTC inicializado: %04d-%02d-%02d %02d:%02d\n\n", 
           t.year, t.month, t.day, t.hour, t.min);
    
    // Inicializa ADC
    adc_init();
    
    // Cria mutex para status global
    g_status_mutex = xSemaphoreCreateMutex();
    if (g_status_mutex == NULL) {
        printf("❌ ERRO CRÍTICO: Falha ao criar mutex!\n");
        while (1) { tight_loop_contents(); }
    }
    
    // Inicializa status global
    memset(&g_status, 0, sizeof(g_status));
    g_status.nivel_percentual = 100.0f;
    g_status.nivel_litros = 20.0f;
    g_status.temperatura = 25.0f;
    g_status.umidade = 60.0f;
    
    // Inicializa subsistemas
    printf("🔧 Inicializando subsistemas...\n\n");
    
    log_init();
    log_sistema_inicio();
    
    tpa_init();
    feeder_init();
}

// ============================================================
// Criação das Tasks
// ============================================================
void create_tasks(void) {
    printf("🔧 Criando tasks FreeRTOS...\n\n");
    
    BaseType_t ret;
    
    // Task de Sensores
    ret = xTaskCreate(sensor_task, "Sensors", TASK_SENSOR_STACK, NULL, 
                      TASK_SENSOR_PRIORITY, &task_sensor_handle);
    printf("   📊 Task Sensores: %s\n", ret == pdPASS ? "✅ OK" : "❌ ERRO");
    
    // Task TPA
    ret = xTaskCreate(tpa_task, "TPA", TASK_TPA_STACK, NULL,
                      TASK_TPA_PRIORITY, &task_tpa_handle);
    printf("   🚰 Task TPA: %s\n", ret == pdPASS ? "✅ OK" : "❌ ERRO");
    
    // Task Alimentação
    ret = xTaskCreate(feeder_task, "Feeder", TASK_FEEDING_STACK, NULL,
                      TASK_FEEDING_PRIORITY, &task_feeding_handle);
    printf("   🐟 Task Alimentação: %s\n", ret == pdPASS ? "✅ OK" : "❌ ERRO");
    
    // Task Servidor Web
    ret = xTaskCreate(webserver_task, "WebServer", TASK_WEBSERVER_STACK, NULL,
                      TASK_WEBSERVER_PRIORITY, &task_webserver_handle);
    printf("   🌐 Task WebServer: %s\n", ret == pdPASS ? "✅ OK" : "❌ ERRO");
    
    // Task Logs
    ret = xTaskCreate(log_task, "Logs", TASK_LOG_STACK, NULL,
                      TASK_LOG_PRIORITY, &task_log_handle);
    printf("   📋 Task Logs: %s\n", ret == pdPASS ? "✅ OK" : "❌ ERRO");
    
    // Task Display
    ret = xTaskCreate(display_task, "Display", 256, NULL,
                      tskIDLE_PRIORITY + 1, &task_display_handle);
    printf("   🖥️ Task Display: %s\n", ret == pdPASS ? "✅ OK" : "❌ ERRO");
    
    printf("\n");
}

// ============================================================
// Main
// ============================================================
int main() {
    // Inicializa o sistema
    system_init();
    
    // Desabilita watchdog temporariamente para debug
    // watchdog_enable(8000, true);
    // printf("🐕 Watchdog habilitado (8s timeout)\n\n");
    printf("⚠️ Watchdog DESABILITADO (modo debug)\n\n");
    
    // Cria as tasks
    create_tasks();
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ SISTEMA PRONTO!                                          ║\n");
    printf("║                                                              ║\n");
    printf("║  🌐 Dashboard: http://192.168.4.1/                           ║\n");
    printf("║  📶 Wi-Fi: HydroSense (senha: hydro2024)                     ║\n");
    printf("║                                                              ║\n");
    printf("║  🔄 Iniciando scheduler FreeRTOS...                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    // Inicia o scheduler FreeRTOS
    vTaskStartScheduler();
    
    // Nunca deve chegar aqui
    printf("❌ ERRO CRÍTICO: Scheduler falhou!\n");
    
    while (1) {
        tight_loop_contents();
    }
    
    return 0;
}
