#include "pump_controller.h"
#include "system_config.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <stdio.h>

static bool pump1_running = false;
static bool pump2_running = false;

void pump_controller_init(void) {
    // Configura PWM para bomba 1
    gpio_set_function(PUMP1_PIN, GPIO_FUNC_PWM);
    uint slice_num1 = pwm_gpio_to_slice_num(PUMP1_PIN);
    pwm_set_wrap(slice_num1, 65535);
    pwm_set_enabled(slice_num1, true);
    
    // Configura PWM para bomba 2
    gpio_set_function(PUMP2_PIN, GPIO_FUNC_PWM);
    uint slice_num2 = pwm_gpio_to_slice_num(PUMP2_PIN);
    pwm_set_wrap(slice_num2, 65535);
    pwm_set_enabled(slice_num2, true);
    
    // Inicializa bombas desligadas
    pwm_set_gpio_level(PUMP1_PIN, 0);
    pwm_set_gpio_level(PUMP2_PIN, 0);
    
    printf("✅ Controlador de bombas inicializado\n");
}

void pump1_start(void) {
    pwm_set_gpio_level(PUMP1_PIN, 32768); // 50% duty cycle
    pump1_running = true;
    printf("🚰 Bomba 1 (TPA) ligada\n");
}

void pump1_stop(void) {
    pwm_set_gpio_level(PUMP1_PIN, 0);
    pump1_running = false;
    printf("⏹️ Bomba 1 (TPA) desligada\n");
}

bool pump1_is_running(void) {
    return pump1_running;
}

void pump2_start(void) {
    pwm_set_gpio_level(PUMP2_PIN, 32768); // 50% duty cycle
    pump2_running = true;
    printf("💧 Bomba 2 (reabastecimento) ligada\n");
}

void pump2_stop(void) {
    pwm_set_gpio_level(PUMP2_PIN, 0);
    pump2_running = false;
    printf("⏹️ Bomba 2 (reabastecimento) desligada\n");
}

bool pump2_is_running(void) {
    return pump2_running;
}