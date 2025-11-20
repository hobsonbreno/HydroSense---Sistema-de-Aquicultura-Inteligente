#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "pico/stdlib.h"

// Inicializa o sensor de temperatura
void temperature_sensor_init(void);

// Lê a temperatura da água em Celsius
float temperature_read_value(void);

// Verifica se a temperatura está na faixa ideal
bool temperature_is_normal(float temp);

#endif // TEMPERATURE_SENSOR_H