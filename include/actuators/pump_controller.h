#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include "pico/stdlib.h"

// Inicializa o controlador de bombas
void pump_controller_init(void);

// Controla bomba 1 (TPA - drenagem)
void pump1_start(void);
void pump1_stop(void);
bool pump1_is_running(void);

// Controla bomba 2 (reabastecimento)
void pump2_start(void);
void pump2_stop(void);
bool pump2_is_running(void);

#endif // PUMP_CONTROLLER_H