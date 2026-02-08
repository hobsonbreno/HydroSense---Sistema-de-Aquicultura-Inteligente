#include "hydrosense_system.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"
#include <stdio.h>

// Horários de alimentação (8:00 e 16:00 como no Python)
const uint8_t FEED_HOURS[FEED_HOURS_COUNT] = {8, 16};

// Variáveis do servo
static uint slice_num;
static uint chan_num;
static bool servo_inicializado = false;

// Configurações do servo (baseadas no Python)
#define SERVO_MIN_US    1000    // 1ms para 0°
#define SERVO_MAX_US    2000    // 2ms para 180°
#define SERVO_FREQ      50      // 50Hz
#define PWM_PERIOD_US   20000   // 20ms

// Status global do sistema
static hydrosense_status_t system_status = {0};

uint32_t servo_angle(uint16_t angle) {
    if (angle > 180) angle = 180;
    
    // Cálculo PWM para servo SG90: 1ms-2ms (1000-2000 us)
    uint32_t pulse_width = 1000 + (angle * 1000 / 180); // 1000us + proporção
    
    if (servo_inicializado) {
        pwm_set_chan_level(slice_num, chan_num, pulse_width);
        printf("Servo: %d° (PWM: %luus)\n", angle, pulse_width);
    }
    
    return pulse_width;
}

void servo_stop(void) {
    if (servo_inicializado) {
        pwm_set_chan_level(slice_num, chan_num, 0);
        printf("⏹️ Servo parado\n");
    }
}

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    chan_num = pwm_gpio_to_channel(SERVO_PIN);
    
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 125.0f);  // Para 1MHz
    pwm_config_set_wrap(&cfg, 20000 - 1); // Para 20ms (50Hz)
    pwm_init(slice_num, &cfg, true);
    
    servo_inicializado = true;
    servo_angle(0); // Posição inicial
    printf("✅ Servo SG90 inicializado (GPIO %d)\n", SERVO_PIN);
}

void servo_alimentar_peixes(const char* origem) {
    printf("🐟 Alimentando peixes - %s\n", origem);
    printf("📦 Dispensando 500g de ração para tilápias e camarões\n");
    printf("🔄 Acionando Micro Servo SG90...\n");
    
    // Limpa LEDs e inicia animação
    neopixel_clear();
    
    // Posição inicial (igual ao Python)
    printf("🔧 Posicionando servo na posição inicial (0°)...\n");
    servo_angle(0);
    sleep_ms(1000);
    
    // Rotação gradual 0° → 180° (36 passos como no Python)
    printf("🔄 Iniciando rotação para dispensar 500g...\n");
    const int steps = 36;
    const float total_rotation_time = 3.0f; // 3 segundos
    const uint32_t step_delay = (total_rotation_time * 1000) / steps;
    
    for (int step = 0; step <= steps; step++) {
        uint16_t angle = (step * 180) / steps;
        servo_angle(angle);
        printf("   Passo %d/%d: %d°\n", step + 1, steps + 1, angle);
        
        // Animação LED sincronizada (como no Python)
        uint8_t led_idx = step % NEOPIXEL_COUNT;
        neopixel_set_color(led_idx, COLOR_YELLOW);
        if (step > 0) {
            rgb_color_t dim_yellow = {20, 10, 0};
            neopixel_set_color((step - 1) % NEOPIXEL_COUNT, dim_yellow);
        }
        neopixel_show();
        
        sleep_ms(step_delay);
    }
    
    // Retorna à posição inicial (igual ao Python)
    printf("🔄 Retornando à posição inicial...\n");
    for (int angle = 180; angle >= 0; angle -= 10) {
        servo_angle(angle);
        sleep_ms(100);
    }
    
    // Para o servo e finaliza
    printf("⏹️ Parando servo...\n");
    servo_stop();
    
    // LEDs de conclusão (como no Python)
    rgb_color_t feed_color = {30, 15, 0};
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        neopixel_set_color(i, feed_color);
    }
    neopixel_show();
    
    // Beep de conclusão
    buzzer_beep(1500, 300);
    sleep_ms(2000);
    
    neopixel_clear();
    printf("✅ Alimentação concluída!\n");
    printf("📊 500g de ração dispensados com sucesso\n");
    
    // Atualiza timestamp e contador de alimentações
    system_status.ultimo_feeding = get_timestamp_ms();
    system_status.alimentacoes_hoje++;
    
    printf("🍽️ Alimentações hoje: %d\n", system_status.alimentacoes_hoje);
}

void servo_teste_movimento(void) {
    printf("🧪 Testando Micro Servo SG90...\n");
    
    servo_angle(90);
    sleep_ms(1000);
    servo_angle(0);
    sleep_ms(1000);
    servo_stop();
    
    printf("✅ Servo SG90 testado!\n");
}

void alimentacao_verificar_horarios(void) {
    static bool horarios_executados[FEED_HOURS_COUNT] = {false};
    static uint8_t dia_anterior = 255; // Valor inválido para reset diário
    
    datetime_t dt;
    rtc_get_datetime(&dt);
    
    // Reset diário dos horários executados e contador
    if (dt.day != dia_anterior) {
        for (int i = 0; i < FEED_HOURS_COUNT; i++) {
            horarios_executados[i] = false;
        }
        dia_anterior = dt.day;
        system_status.alimentacoes_hoje = 0;  // Reset do contador diário
        printf("🔄 Novo dia - Reset dos horários e contador de alimentação\n");
    }
    
    // Verifica se é hora de alimentar
    if (dt.min == 0 && system_status.alimentacao_auto_habilitada) {
        for (int i = 0; i < FEED_HOURS_COUNT; i++) {
            if (dt.hour == FEED_HOURS[i] && !horarios_executados[i]) {
                horarios_executados[i] = true;
                char origem[48];
                snprintf(origem, sizeof(origem), "⏰ PROGRAMADO %02d:00 (#%d do dia)", 
                        dt.hour, system_status.alimentacoes_hoje + 1);
                servo_alimentar_peixes(origem);
                return;
            }
        }
    }
}

bool alimentacao_is_horario_programado(void) {
    datetime_t dt;
    rtc_get_datetime(&dt);
    
    for (int i = 0; i < FEED_HOURS_COUNT; i++) {
        if (dt.hour == FEED_HOURS[i] && dt.min == 0) {
            return true;
        }
    }
    return false;
}