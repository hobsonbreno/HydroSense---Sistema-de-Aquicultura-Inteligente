/**
 * HydroSense v3.0 - Configuração do Sistema
 * 
 * Configurações para sistema de aquicultura com hidroponia
 * Tanque: 20 litros, 10 peixes
 */

#ifndef HYDROSENSE_CONFIG_H
#define HYDROSENSE_CONFIG_H

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// ============================================================
// VERSÃO DO SISTEMA
// ============================================================
#define HYDROSENSE_VERSION          "3.0.0"
#define HYDROSENSE_BUILD_DATE       __DATE__
#define HYDROSENSE_BUILD_TIME       __TIME__

// ============================================================
// CONFIGURAÇÃO DO TANQUE
// ============================================================
#define TANQUE_CAPACIDADE_LITROS    20.0f   // Capacidade total
#define TANQUE_ALTURA_CM            30.0f   // Altura interna
#define TANQUE_QTD_PEIXES           10      // Quantidade de peixes

// Níveis em litros
#define NIVEL_100_LITROS            20.0f   // Cheio
#define NIVEL_75_LITROS             15.0f   // 75%
#define NIVEL_50_LITROS             10.0f   // Metade (rotatividade hidroponia)
#define NIVEL_25_LITROS             5.0f    // Mínimo para TPA
#define NIVEL_20_LITROS             4.0f    // 20% (crítico)

// ============================================================
// MAPEAMENTO DE HARDWARE - SENSORES
// ============================================================
// VL53L0X - Sensor de distância laser (nível água)
#define VL53L0X_I2C_PORT            i2c0
#define VL53L0X_SDA_PIN             2
#define VL53L0X_SCL_PIN             3

// AHT10 - Sensor de temperatura e umidade
#define AHT10_I2C_PORT              i2c1
#define AHT10_SDA_PIN               14
#define AHT10_SCL_PIN               15

// TCS3200 - Sensor de cor (turbidez)
#define TCS3200_S0_PIN              8
#define TCS3200_S1_PIN              9
#define TCS3200_S2_PIN              10
#define TCS3200_S3_PIN              11
#define TCS3200_OUT_PIN             12
#define TCS3200_OE_PIN              13

// ============================================================
// MAPEAMENTO DE HARDWARE - ATUADORES
// ============================================================
// Servo SG90 - Alimentador
#define SERVO_PIN                   16
#define SERVO_MIN_PULSE_US          500     // 0 graus
#define SERVO_MAX_PULSE_US          2400    // 180 graus
#define SERVO_FREQ_HZ               50

// Bombas d'água
#define BOMBA1_PIN                  17      // Esvaziar (drenar)
#define BOMBA2_PIN                  19      // Encher (água limpa)

// ============================================================
// MAPEAMENTO DE HARDWARE - INTERFACE
// ============================================================
// Display OLED SSD1306
#define OLED_I2C_PORT               i2c1
#define OLED_SDA_PIN                14      // Compartilhado com AHT10
#define OLED_SCL_PIN                15
#define OLED_I2C_ADDR               0x3C

// Matriz NeoPixel 5x5
#define NEOPIXEL_PIN                7
#define NEOPIXEL_COUNT              25

// Botões
#define BUTTON_A_PIN                5
#define BUTTON_B_PIN                6

// Buzzer (desabilitado)
#define BUZZER_PIN                  21

// ============================================================
// PARÂMETROS DE QUALIDADE DA ÁGUA
// ============================================================
#define TEMP_MIN_IDEAL              22.0f   // °C
#define TEMP_MAX_IDEAL              28.0f   // °C
#define TEMP_IDEAL                  25.0f   // °C

#define UMIDADE_MIN                 40.0f   // %
#define UMIDADE_MAX                 80.0f   // %

#define TURBIDEZ_LIMITE_SUJEIRA     30.0f   // % - aciona TPA de limpeza

// ============================================================
// CONFIGURAÇÃO DE ALIMENTAÇÃO
// ============================================================
#define ALIMENTACAO_HORARIO_1       8       // 08:00
#define ALIMENTACAO_HORARIO_2       16      // 16:00
#define ALIMENTACAO_GRAMAS          50      // gramas por refeição
#define ALIMENTACAO_QTD_DIARIA      2       // vezes por dia

// ============================================================
// CONFIGURAÇÃO DO SISTEMA TPA (Troca Parcial de Água)
// ============================================================
// TPA por sujeira (sensor de cor detecta lodo)
#define TPA_SUJEIRA_NIVEL_MIN       25.0f   // Esvazia até 25% (5L)
#define TPA_SUJEIRA_NIVEL_MAX       90.0f   // CORRIGIDO: Enche até 90% (18L) para evitar transbordamento

// TPA por rotatividade (hidroponia) - a cada 2 horas
#define TPA_ROTATIVIDADE_INTERVALO_MS   (2 * 60 * 60 * 1000)  // 2 horas
#define TPA_ROTATIVIDADE_NIVEL_MIN  50.0f   // Esvazia até 50% (10L)
#define TPA_ROTATIVIDADE_NIVEL_MAX  90.0f   // CORRIGIDO: Enche até 90% (18L) para evitar transbordamento

// Cooldown entre TPAs
#define TPA_COOLDOWN_MS             (5 * 60 * 1000)  // 5 minutos

// ============================================================
// INTERVALOS DAS TAREFAS FREERTOS
// ============================================================
#define TASK_SENSOR_INTERVAL_MS     5000    // 5 segundos
#define TASK_TPA_CHECK_INTERVAL_MS  10000   // 10 segundos
#define TASK_FEEDING_INTERVAL_MS    1000    // 1 segundo (precisão horário)
#define TASK_DISPLAY_INTERVAL_MS    1000    // 1 segundo
#define TASK_WEBSERVER_INTERVAL_MS  100     // 100ms (resposta rápida)
#define TASK_LOG_INTERVAL_MS        60000   // 1 minuto

// Prioridades das tarefas
#define TASK_SENSOR_PRIORITY        (tskIDLE_PRIORITY + 2)
#define TASK_TPA_PRIORITY           (tskIDLE_PRIORITY + 3)
#define TASK_FEEDING_PRIORITY       (tskIDLE_PRIORITY + 3)
#define TASK_DISPLAY_PRIORITY       (tskIDLE_PRIORITY + 1)
#define TASK_WEBSERVER_PRIORITY     (tskIDLE_PRIORITY + 2)
#define TASK_LOG_PRIORITY           (tskIDLE_PRIORITY + 1)

// Tamanhos de stack
#define TASK_SENSOR_STACK           512
#define TASK_TPA_STACK              512
#define TASK_FEEDING_STACK          256
#define TASK_DISPLAY_STACK          512
#define TASK_WEBSERVER_STACK        2048
#define TASK_LOG_STACK              512

// ============================================================
// CONFIGURAÇÃO WI-FI
// ============================================================
#define WIFI_SSID                   "HydroSense_AP"
#define WIFI_PASSWORD               "hydro2024"
#define WEBSERVER_PORT              80

// ============================================================
// CONFIGURAÇÃO DE LOG/HISTÓRICO
// ============================================================
#define LOG_MAX_ENTRIES             100     // Máximo de entradas no histórico
#define LOG_SAVE_INTERVAL_MS        300000  // Salva a cada 5 minutos

// ============================================================
// ESTRUTURAS DE DADOS
// ============================================================

// Tipos de evento para log
typedef enum {
    LOG_SISTEMA_INICIO,
    LOG_SISTEMA_ERRO,
    LOG_ALIMENTACAO_PROGRAMADA,
    LOG_ALIMENTACAO_MANUAL,
    LOG_TPA_SUJEIRA_INICIO,
    LOG_TPA_SUJEIRA_FIM,
    LOG_TPA_ROTATIVIDADE_INICIO,
    LOG_TPA_ROTATIVIDADE_FIM,
    LOG_BOMBA1_LIGADA,
    LOG_BOMBA1_DESLIGADA,
    LOG_BOMBA2_LIGADA,
    LOG_BOMBA2_DESLIGADA,
    LOG_ALERTA_TEMPERATURA,
    LOG_ALERTA_NIVEL,
    LOG_ALERTA_TURBIDEZ,
    LOG_SENSOR_ERRO
} log_tipo_t;

// Entrada de log
typedef struct {
    uint32_t timestamp;
    log_tipo_t tipo;
    float valor1;
    float valor2;
    char mensagem[64];
} log_entry_t;

// Status completo do sistema
typedef struct {
    // Sensores
    float temperatura;          // °C (AHT10)
    float umidade;              // % (AHT10)
    float nivel_litros;         // Litros (VL53L0X)
    float nivel_percentual;     // % (VL53L0X)
    float turbidez;             // % (TCS3200)
    
    // Estados
    bool bomba1_ativa;          // Drenagem
    bool bomba2_ativa;          // Enchimento
    bool tpa_em_andamento;
    bool wifi_conectado;
    
    // Contadores
    uint8_t alimentacoes_hoje;
    uint32_t ultima_alimentacao;
    uint32_t ultima_tpa;
    uint32_t uptime_segundos;
    
    // Alertas
    bool alerta_temperatura;
    bool alerta_nivel;
    bool alerta_turbidez;
} system_status_t;

// Variáveis globais externas
extern system_status_t g_status;
extern SemaphoreHandle_t g_status_mutex;
extern QueueHandle_t g_log_queue;

// ============================================================
// FUNÇÕES DE CONFIGURAÇÃO
// ============================================================
void config_init(void);
void config_get_status(system_status_t* status);
void config_set_status(const system_status_t* status);

#endif // HYDROSENSE_CONFIG_H
