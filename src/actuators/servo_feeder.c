#include "servo_feeder.h"
#include "system_config.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include <stdio.h>

// Definições do PWM para servo SG90
#define PWM_PERIOD_US 20000  // 20ms período para 50Hz
#define SERVO_MIN_US 500     // 0.5ms para 0°
#define SERVO_MAX_US 2500    // 2.5ms para 180°

static uint slice_num;
static uint chan_num;
static servo_state_t current_state = SERVO_STATE_IDLE;
static bool button_last_state = false;
static uint32_t last_button_time = 0;
static bool feeding_completed = false;

uint32_t servo_angle(uint16_t angle) {
    // Limita o ângulo entre 0 e 180 graus
    if (angle > 180) angle = 180;
    
    // Calcula duty_us usando faixa 500-2500µs
    uint32_t duty_us = SERVO_MIN_US + (angle * (SERVO_MAX_US - SERVO_MIN_US) / 180);
    
    // Define o nível PWM
    pwm_set_chan_level(slice_num, chan_num, duty_us);
    
    printf("📍 Servo: %d° (PWM: %luµs)\n", angle, duty_us);
    return duty_us;
}

void servo_set_position(uint16_t angle) {
    servo_angle(angle);
}

void button_init(void) {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN); // Pull-up interno - botão conecta ao GND
    printf("✅ Botão A da BitDog Lab inicializado (GPIO %d)\n", BUTTON_A_PIN);
}

void button_b_init(void) {
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN); // Pull-up interno - botão conecta ao GND
    printf("✅ Botão B da BitDog Lab inicializado (GPIO %d)\n", BUTTON_B_PIN);
}

void servo_feeder_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    chan_num = pwm_gpio_to_channel(SERVO_PIN);
    
    // Configuração PWM para 50Hz (20ms período)
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 125.0f);
    pwm_config_set_wrap(&cfg, PWM_PERIOD_US - 1);
    pwm_init(slice_num, &cfg, true);
    
    // Inicializa botão A
    button_init();
    
    // Inicializa botão B
    button_b_init();
    
    servo_set_position(0); // Posição inicial (0 graus)
    current_state = SERVO_STATE_IDLE;
    feeding_completed = false;
    printf("✅ Servo alimentador inicializado (GPIO %d) + Botão B (GPIO %d)\n", SERVO_PIN, BUTTON_B_PIN);
    printf("⏰ Horários programados: 08:00, 16:00, 00:00 (de 8 em 8 horas)\n");
}

bool button_a_pressed(void) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool current_button_state = !gpio_get(BUTTON_A_PIN); // Inverte pois é pull-up
    
    // Debounce do botão
    if (current_time - last_button_time < BUTTON_DEBOUNCE_MS) {
        return false;
    }
    
    // Detecta borda de subida (botão pressionado)
    if (current_button_state && !button_last_state) {
        button_last_state = current_button_state;
        last_button_time = current_time;
        return true;
    }
    
    button_last_state = current_button_state;
    return false;
}

bool button_b_pressed(void) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool current_button_state = !gpio_get(BUTTON_B_PIN); // Inverte pois é pull-up
    
    // Debounce do botão
    if (current_time - last_button_time < BUTTON_DEBOUNCE_MS) {
        return false;
    }
    
    // Detecta borda de subida (botão pressionado)
    if (current_button_state && !button_last_state) {
        button_last_state = current_button_state;
        last_button_time = current_time;
        return true;
    }
    
    button_last_state = current_button_state;
    return false;
}

void servo_manual_feed_sequence(void) {
    if (current_state != SERVO_STATE_IDLE) {
        printf("⚠️ Servo ocupado - ignorando comando manual\n");
        return;
    }
    
    current_state = SERVO_STATE_MANUAL_FEEDING;
    printf("🔘 ALIMENTAÇÃO MANUAL ACIONADA (Botão B)!\n");
    printf("🔄 Sequência: 0° → 180° (5s) → 0° → PARAR\n");
    
    // Movimento 1: Posição inicial para 180°
    printf("   📍 Movimento 1: 0° → 180°\n");
    servo_angle(0);
    sleep_ms(500);
    servo_angle(180);
    
    // Aguarda 5 segundos na posição 180°
    printf("   ⏳ Aguardando 5 segundos na posição 180° (dispensando ração)\n");
    sleep_ms(5000);
    
    // Movimento 2: Retorna para 0°
    printf("   📍 Movimento 2: 180° → 0°\n");
    servo_angle(0);
    sleep_ms(500);
    
    current_state = SERVO_STATE_IDLE;
    feeding_completed = true;
    printf("✅ Alimentação manual concluída - Servo PARADO na posição 0°\n");
    printf("🔘 Aguardando próximo acionamento do Botão B ou horário programado\n");
}

void servo_check_manual_trigger(void) {
    if (button_b_pressed()) {
        printf("🔘 Botão B pressionado!\n");
        feeding_completed = false; // Reset do flag de alimentação
        servo_manual_feed_sequence();
    }
}

servo_state_t servo_get_state(void) {
    return current_state;
}

void servo_feed_fish(void) {
    if (current_state != SERVO_STATE_IDLE) {
        printf("⚠️ Servo ocupado - ignorando alimentação automática\n");
        return;
    }
    
    current_state = SERVO_STATE_FEEDING;
    printf("🍽️ ALIMENTAÇÃO AUTOMÁTICA PROGRAMADA!\n");
    printf("🔄 Sequência: 0° → 180° (5s) → 0° → PARAR\n");
    
    // Movimento 1: Posição inicial para 180°
    printf("   📍 Movimento 1: 0° → 180°\n");
    servo_angle(0);
    sleep_ms(500);
    servo_angle(180);
    
    // Aguarda 5 segundos na posição 180°
    printf("   ⏳ Aguardando 5 segundos na posição 180° (dispensando ração)\n");
    sleep_ms(5000);
    
    // Movimento 2: Retorna para 0°
    printf("   📍 Movimento 2: 180° → 0°\n");
    servo_angle(0);
    sleep_ms(500);
    
    current_state = SERVO_STATE_IDLE;
    feeding_completed = true;
    printf("✅ Alimentação automática concluída - Servo PARADO na posição 0°\n");
    printf("⏰ Próxima alimentação: 08:00, 16:00 ou 00:00h\n");
}

bool is_scheduled_feeding_time(void) {
    datetime_t t;
    rtc_get_datetime(&t);
    uint16_t current_time = (t.hour * 100) + t.min;
    
    // Verifica se é exatamente um dos horários programados
    return (current_time == 800 ||   // 08:00
            current_time == 1600 ||  // 16:00  
            current_time == 0);      // 00:00
}