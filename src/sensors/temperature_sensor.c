#include "temperature_sensor.h"
#include "system_config.h"
#include "hardware/adc.h"
#include <stdio.h>

static bool sensor_initialized = false;

void temperature_sensor_init(void) {
    adc_gpio_init(TEMP_SENSOR_PIN);
    sensor_initialized = true;
    printf("✅ Sensor de temperatura inicializado (ADC)\n");
}

float temperature_read_value(void) {
    if (!sensor_initialized) {
        printf("❌ Sensor de temperatura não inicializado\n");
        return -1.0f;
    }
    
    adc_select_input(0); // GPIO26 = ADC0
    uint16_t adc_value = adc_read();
    
    // Simulação de conversão para temperatura (25°C ± variação)
    float voltage = adc_value * 3.3f / 4096.0f;
    float temperature = 22.0f + (voltage * 8.0f); // Faixa 22-30°C
    
    return temperature;
}

bool temperature_is_normal(float temp) {
    return (temp >= TEMP_MIN_IDEAL && temp <= TEMP_MAX_IDEAL);
}