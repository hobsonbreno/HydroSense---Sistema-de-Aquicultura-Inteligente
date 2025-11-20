#ifndef TURBIDITY_SENSOR_H
#define TURBIDITY_SENSOR_H

#include "pico/stdlib.h"

void turbidity_sensor_init(void);
float turbidity_read_value(void);
bool turbidity_is_acceptable(float turbidity);

#endif // TURBIDITY_SENSOR_H