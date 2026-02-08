/**
 * HydroSense v3.0 - Implementação do Sistema de Alimentação
 */

#include "feeding_system.h"
#include "hydrosense_config.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

// Controle global do alimentador
static feeder_control_t feeder = {0};
static uint slice_num;
static uint channel;

// Converte ângulo para duty cycle PWM
static uint16_t angle_to_duty(uint16_t angle) {
    if (angle > 180) angle = 180;
    
    // Calcula pulse width em microsegundos
    uint32_t pulse_us = SERVO_MIN_PULSE_US + 
                        ((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) / 180;
    
    // Converte para duty cycle (wrap = 20000 para 50Hz com clock de 1MHz)
    return (uint16_t)pulse_us;
}

void feeder_init(void) {
    printf("🔧 Inicializando sistema de alimentação...\n");
    
    // Configura PWM para o servo
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    channel = pwm_gpio_to_channel(SERVO_PIN);
    
    // Configura PWM: 50Hz (20ms período)
    // Clock de 125MHz, divisor de 125 = 1MHz
    // Wrap de 20000 = 20ms período
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 125.0f);
    pwm_config_set_wrap(&cfg, 20000 - 1);
    pwm_init(slice_num, &cfg, true);
    
    // Posição inicial (0 graus)
    servo_set_angle(0);
    
    // Inicializa controle
    memset(&feeder, 0, sizeof(feeder_control_t));
    feeder.inicializado = true;
    feeder.estado = FEEDER_IDLE;
    feeder.angulo_atual = 0;
    
    printf("✅ Sistema de alimentação inicializado!\n");
    printf("   🕐 Horários: %02d:00 e %02d:00\n", FEEDING_HORA_1, FEEDING_HORA_2);
    printf("   🐟 Peixes: %d | Ração: %dg/refeição\n", FEEDING_PEIXES, FEEDING_GRAMAS);
}

void servo_set_angle(uint16_t angle) {
    if (!feeder.inicializado) return;
    
    uint16_t duty = angle_to_duty(angle);
    pwm_set_chan_level(slice_num, channel, duty);
    feeder.angulo_atual = angle;
}

void servo_alimentar(const char* origem) {
    if (!feeder.inicializado) return;
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  🐟 ALIMENTAÇÃO INICIADA                                      ║\n");
    printf("║  📋 Origem: %-47s ║\n", origem);
    printf("║  📊 Quantidade: %dg para %d peixes                           ║\n", 
           FEEDING_GRAMAS, FEEDING_PEIXES);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    feeder.estado = FEEDER_MOVING_TO_180;
    
    // Movimento suave de 0° para 180°
    printf("🔄 Movendo servo: 0° → 180°\n");
    for (int angle = 0; angle <= 180; angle += 5) {
        servo_set_angle(angle);
        vTaskDelay(pdMS_TO_TICKS(30));  // 30ms entre passos
    }
    
    // Pausa no topo
    vTaskDelay(pdMS_TO_TICKS(500));
    
    feeder.estado = FEEDER_MOVING_TO_0;
    
    // Movimento suave de 180° para 0°
    printf("🔄 Movendo servo: 180° → 0°\n");
    for (int angle = 180; angle >= 0; angle -= 5) {
        servo_set_angle(angle);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    
    // Desliga PWM para evitar vibração
    pwm_set_chan_level(slice_num, channel, 0);
    
    feeder.estado = FEEDER_COMPLETED;
    feeder.alimentacoes_hoje++;
    feeder.ultima_alimentacao = to_ms_since_boot(get_absolute_time());
    
    printf("✅ ALIMENTAÇÃO CONCLUÍDA!\n");
    printf("   🍽️ Alimentações hoje: %d\n", feeder.alimentacoes_hoje);
    
    // Atualiza status global
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_status.alimentacoes_hoje = feeder.alimentacoes_hoje;
        g_status.ultima_alimentacao = feeder.ultima_alimentacao;
        xSemaphoreGive(g_status_mutex);
    }
    
    feeder.estado = FEEDER_IDLE;
}

void servo_teste(void) {
    printf("🧪 Testando servo SG90...\n");
    
    servo_set_angle(0);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    servo_set_angle(90);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    servo_set_angle(180);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    servo_set_angle(0);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    pwm_set_chan_level(slice_num, channel, 0);
    
    printf("✅ Teste do servo concluído!\n");
}

bool feeder_is_feeding_time(void) {
    datetime_t dt;
    rtc_get_datetime(&dt);
    
    // Verifica se é minuto 0 de um dos horários
    if (dt.min != 0) return false;
    
    if (dt.hour == FEEDING_HORA_1 && !feeder.horario1_executado) {
        return true;
    }
    
    if (dt.hour == FEEDING_HORA_2 && !feeder.horario2_executado) {
        return true;
    }
    
    return false;
}

bool feeder_can_feed(void) {
    return feeder.estado == FEEDER_IDLE && feeder.inicializado;
}

void feeder_task(void* pvParameters) {
    printf("🐟 Task de alimentação iniciada\n");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        datetime_t dt;
        rtc_get_datetime(&dt);
        
        // Reset diário
        if (dt.day != feeder.dia_atual) {
            feeder.dia_atual = dt.day;
            feeder.alimentacoes_hoje = 0;
            feeder.horario1_executado = false;
            feeder.horario2_executado = false;
            printf("🔄 Novo dia - Reset do contador de alimentações\n");
        }
        
        // Verifica horários programados
        if (feeder_can_feed()) {
            if (dt.hour == FEEDING_HORA_1 && dt.min == 0 && !feeder.horario1_executado) {
                feeder.horario1_executado = true;
                servo_alimentar("Horário programado 08:00");
            }
            else if (dt.hour == FEEDING_HORA_2 && dt.min == 0 && !feeder.horario2_executado) {
                feeder.horario2_executado = true;
                servo_alimentar("Horário programado 16:00");
            }
        }
        
        // Reset dos flags quando sai do minuto 0
        if (dt.min != 0) {
            // Permite nova alimentação no próximo horário
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_FEEDING_INTERVAL_MS));
    }
}

void feeder_get_control(feeder_control_t* ctrl) {
    memcpy(ctrl, &feeder, sizeof(feeder_control_t));
}

uint8_t feeder_get_alimentacoes_hoje(void) {
    return feeder.alimentacoes_hoje;
}
