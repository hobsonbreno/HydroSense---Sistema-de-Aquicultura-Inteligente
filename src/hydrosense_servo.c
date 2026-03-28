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
    
    // CORREÇÃO: Posiciona suavemente em 0° e depois desabilita PWM
    // para evitar impulso/movimento indesejado ao ligar
    printf("🔧 Posicionando servo em 0° suavemente...\n");
    servo_angle(0); // Posição inicial
    sleep_ms(500);  // Aguarda servo estabilizar
    servo_stop();   // Desabilita PWM para evitar vibração
    
    printf("✅ Servo SG90 inicializado (GPIO %d) - posição 0° estável\n", SERVO_PIN);
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
    
    // CORREÇÃO: Retorna à posição inicial com movimento suave e estabilização
    printf("🔄 Retornando à posição inicial (0°)...\n");
    
    // Movimento suave de 180° para 0° em passos menores para precisão
    for (int angle = 180; angle >= 0; angle -= 5) {
        servo_angle(angle);
        sleep_ms(50);
    }
    
    // Garante posição final exata em 0°
    servo_angle(0);
    sleep_ms(500);  // Aguarda servo estabilizar na posição 0°
    
    // IMPORTANTE: Desabilita PWM somente após estabilização completa
    printf("⏹️ Servo estabilizado em 0° - desabilitando PWM...\n");
    servo_stop();
    sleep_ms(100);  // Pequeno delay adicional
    
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
    // CORREÇÃO: Removido movimento de teste ao inicializar para evitar desperdício de ração
    // O servo já foi posicionado em 0° na inicialização
    
    printf("🧪 Verificando Micro Servo SG90...\n");
    printf("   ✓ PWM configurado: 50Hz (20ms período)\n");
    printf("   ✓ Range: 0° - 180° (1000us - 2000us)\n");
    printf("   ✓ GPIO: %d\n", SERVO_PIN);
    printf("   ✓ Posição atual: 0° (repouso)\n");
    printf("   ⚠️ Teste de movimento DESABILITADO para evitar desperdício de ração\n");
    printf("   💡 Use comando serial 'FEED' ou botão para testar alimentação\n");
    printf("✅ Servo SG90 pronto para operação!\n");
}

void alimentacao_verificar_horarios(void) {
    static bool horarios_executados[FEED_HOURS_COUNT] = {false};
    static uint8_t dia_anterior = 255; // Valor inválido para reset diário
    static uint32_t ultimo_check_log = 0;
    
    datetime_t dt;
    rtc_get_datetime(&dt);
    
    // Reset diário dos horários executados e contador
    if (dt.day != dia_anterior) {
        for (int i = 0; i < FEED_HOURS_COUNT; i++) {
            horarios_executados[i] = false;
        }
        dia_anterior = dt.day;
        system_status.alimentacoes_hoje = 0;  // Reset do contador diário
        printf("🔄 Novo dia (%02d/%02d) - Reset dos horários e contador de alimentação\n", dt.day, dt.month);
    }
    
    // Log periódico de verificação (a cada 60 segundos)
    uint32_t agora = get_timestamp_ms();
    if (agora - ultimo_check_log > 60000) {
        printf("⏰ Verificação alimentação: RTC=%02d:%02d:%02d | Horários: 08:00(%s) 16:00(%s) | Auto=%s\n",
               dt.hour, dt.min, dt.sec,
               horarios_executados[0] ? "✓" : "○",
               horarios_executados[1] ? "✓" : "○",
               system_status.alimentacao_auto_habilitada ? "ON" : "OFF");
        ultimo_check_log = agora;
    }
    
    // Verifica se é hora de alimentar (aceita minuto 0 OU minuto 1 para não perder por delay)
    if ((dt.min == 0 || dt.min == 1) && system_status.alimentacao_auto_habilitada) {
        for (int i = 0; i < FEED_HOURS_COUNT; i++) {
            if (dt.hour == FEED_HOURS[i] && !horarios_executados[i]) {
                printf("🔔 HORÁRIO DE ALIMENTAÇÃO DETECTADO: %02d:%02d (Programado: %02d:00)\n",
                       dt.hour, dt.min, FEED_HOURS[i]);
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