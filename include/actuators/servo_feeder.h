#ifndef SERVO_FEEDER_H
#define SERVO_FEEDER_H

#include "pico/stdlib.h"

// Configurações do servo
#define SERVO_PIN 16
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180
#define SERVO_CENTER_ANGLE 90

// Configuração do botão A da BitDog Lab
#define BUTTON_A_PIN 5  // GPIO 5 para botão A da BitDog Lab
#define BUTTON_DEBOUNCE_MS 50

// Configuração do botão B da BitDog Lab
#define BUTTON_B_PIN 6  // GPIO 6 para botão B da BitDog Lab
#define BUTTON_DEBOUNCE_MS 50

// Funções básicas do servo alimentador
void servo_feeder_init(void);
void servo_feed_fish(void);
void servo_set_position(uint16_t angle);

// Funções avançadas para controle preciso
void servo_test_movement(void);
void servo_calibrate(void);
void servo_emergency_stop(void);
bool servo_is_initialized(void);

// Novas funções para controle manual
void servo_manual_feed_sequence(void);
void button_init(void);
bool button_a_pressed(void);
void servo_check_manual_trigger(void);

// Novas funções para controle manual com botão B
void servo_manual_feed_sequence(void);
void button_b_init(void);
bool button_b_pressed(void);
void servo_check_manual_trigger(void);

// Função para verificar horários programados
bool is_scheduled_feeding_time(void);

// Estados do servo
typedef enum {
    SERVO_STATE_IDLE,
    SERVO_STATE_FEEDING,
    SERVO_STATE_MOVING,
    SERVO_STATE_MANUAL_FEEDING,
    SERVO_STATE_ERROR
} servo_state_t;

// Obter estado atual do servo
servo_state_t servo_get_state(void);

#endif // SERVO_FEEDER_H