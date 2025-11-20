#ifndef WATER_LEVEL_SENSOR_H
#define WATER_LEVEL_SENSOR_H

#include "pico/stdlib.h"

void water_level_sensor_init(void);
float water_level_read_percent(void);
bool water_level_is_adequate(float level_percent);

#endif // WATER_LEVEL_SENSOR_H