#include "feeding_task.h"
#include "system_config.h"
#include "actuators/servo_feeder.h"
#include "hardware/rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

extern SystemStatus_t g_system_status;
extern SemaphoreHandle_t system_data_mutex;

// Usa as variáveis definidas em system_config.c
extern const uint16_t feeding_times[];
extern const uint8_t feeding_times_count;

// Variáveis de controle
static bool last_feeding_executed = false;
static uint16_t last_feeding_hour = 25; // Valor inválido para forçar primeira verificação

void feeding_task(void *pvParameters) {
    printf("🐟 Task de alimentação iniciada\n");
    printf("⏰ Horários programados: 00:00, 08:00, 16:00 (de 8 em 8 horas)\n");
    printf("🔘 Pressione botão B (GPIO 6) para alimentação manual\n");
    
    while (1) {
        // SEMPRE verifica botão manual (máxima prioridade)
        servo_check_manual_trigger();
        
        // Verifica horários programados apenas se servo estiver idle
        if (servo_get_state() == SERVO_STATE_IDLE) {
            datetime_t t;
            rtc_get_datetime(&t);
            uint16_t current_time = (t.hour * 100) + t.min;
            uint8_t current_hour = t.hour;
            
            // Verifica se é um dos horários programados E ainda não alimentou nesta hora
            bool is_feeding_time = false;
            for (int i = 0; i < feeding_times_count; i++) {
                if (current_time == feeding_times[i] && current_hour != last_feeding_hour) {
                    is_feeding_time = true;
                    break;
                }
            }
            
            if (is_feeding_time) {
                printf("⏰ HORÁRIO PROGRAMADO DETECTADO: %02d:%02d\n", t.hour, t.min);
                servo_feed_fish();
                last_feeding_hour = current_hour;
                last_feeding_executed = true;
                
                // Atualiza último horário de alimentação
                if (xSemaphoreTake(system_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_system_status.last_feeding_time = to_ms_since_boot(get_absolute_time()) / 1000;
                    xSemaphoreGive(system_data_mutex);
                }
            } else {
                // Reset do flag quando sai do horário de alimentação
                if (current_time != feeding_times[0] && 
                    current_time != feeding_times[1] && 
                    current_time != feeding_times[2]) {
                    last_feeding_executed = false;
                }
            }
        }
        
        // Verifica a cada 1 segundo para precisão nos horários
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

uint16_t get_current_time_hhmm(void) {
    datetime_t t;
    rtc_get_datetime(&t);
    return (t.hour * 100) + t.min;
}