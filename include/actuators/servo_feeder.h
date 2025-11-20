#ifndef SERVO_FEEDER_H
#define SERVO_FEEDER_H

#include "pico/stdlib.h"

void servo_feeder_init(void);
void servo_feed_fish(void);
void servo_set_position(uint16_t angle);

#endif // SERVO_FEEDER_H