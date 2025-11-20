#include "servo_feeder.h"
#include "system_config.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <stdio.h>

void servo_feeder_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_wrap(slice_num, 20000); // 20ms período
    pwm_set_clkdiv(slice_num, 125.0f); // 1MHz clock
    pwm_set_enabled(slice_num, true);
    
    servo_set_position(0); // Posição inicial
    printf("✅ Servo alimentador inicializado\n");
}

void servo_feed_fish(void) {
    printf("🐠 Alimentando peixes...\n");
    servo_set_position(90);  // Abre dispensador
    sleep_ms(2000);          // Aguarda 2 segundos
    servo_set_position(0);   // Fecha dispensador
    printf("✅ Alimentação concluída\n");
}

void servo_set_position(uint16_t angle) {
    if (angle > 180) angle = 180;
    uint32_t pulse_width = 1000 + (angle * 1000 / 180); // 1-2ms
    pwm_set_gpio_level(SERVO_PIN, pulse_width);
}