#include "water_level_sensor.h"
#include "system_config.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <stdlib.h>

static bool sensor_initialized = false;

void water_level_sensor_init(void) {
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(WATER_LEVEL_SDA, GPIO_FUNC_I2C);
    gpio_set_function(WATER_LEVEL_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(WATER_LEVEL_SDA);
    gpio_pull_up(WATER_LEVEL_SCL);
    
    sensor_initialized = true;
    printf("✅ Sensor de nível da água inicializado (I2C)\n");
}

float water_level_read_percent(void) {
    if (!sensor_initialized) {
        return -1.0f;
    }
    
    // Simulação de leitura de nível (80-100%)
    static float level = 95.0f;
    level += (float)(rand() % 10 - 5) * 0.1f;
    
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    
    return level;
}

bool water_level_is_adequate(float level_percent) {
    return (level_percent >= WATER_LEVEL_MIN);
}