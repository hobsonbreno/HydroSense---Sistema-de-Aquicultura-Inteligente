#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "pico/stdlib.h"

void ph_sensor_init(void);
float ph_read_value(void);
bool ph_is_ideal(float ph);
void ph_calibrate(float known_ph, uint16_t adc_reading);

#endif // PH_SENSOR_H