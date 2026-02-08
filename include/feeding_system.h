/**
 * HydroSense v3.0 - Sistema de Alimentação
 * 
 * Controla o servo SG90 para alimentar os peixes
 * Horários: 08:00 e 16:00
 * Quantidade: 50g por refeição (10 peixes)
 */

#ifndef FEEDING_SYSTEM_H
#define FEEDING_SYSTEM_H

#include "pico/stdlib.h"
#include <stdbool.h>

// Configuração do servo SG90
#define SERVO_PIN               16
#define SERVO_FREQ_HZ           50
#define SERVO_MIN_PULSE_US      500     // 0 graus
#define SERVO_MAX_PULSE_US      2400    // 180 graus

// Configuração de alimentação
#define FEEDING_HORA_1          8       // 08:00
#define FEEDING_HORA_2          16      // 16:00
#define FEEDING_GRAMAS          50      // gramas por refeição
#define FEEDING_PEIXES          10      // quantidade de peixes

// Estados do alimentador
typedef enum {
    FEEDER_IDLE,
    FEEDER_MOVING_TO_180,
    FEEDER_MOVING_TO_0,
    FEEDER_COMPLETED
} feeder_state_t;

// Estrutura de controle
typedef struct {
    bool inicializado;
    feeder_state_t estado;
    uint8_t alimentacoes_hoje;
    uint32_t ultima_alimentacao;
    uint8_t angulo_atual;
    bool horario1_executado;
    bool horario2_executado;
    uint8_t dia_atual;
} feeder_control_t;

// Funções principais
void feeder_init(void);
void feeder_task(void* pvParameters);

// Controle do servo
void servo_set_angle(uint16_t angle);
void servo_alimentar(const char* origem);
void servo_teste(void);

// Verificações
bool feeder_is_feeding_time(void);
bool feeder_can_feed(void);

// Getters
void feeder_get_control(feeder_control_t* ctrl);
uint8_t feeder_get_alimentacoes_hoje(void);

#endif // FEEDING_SYSTEM_H
