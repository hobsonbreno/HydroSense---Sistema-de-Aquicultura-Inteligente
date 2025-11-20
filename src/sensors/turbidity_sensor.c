#include "turbidity_sensor.h"
#include "system_config.h"
#include "hardware/adc.h"
#include <stdio.h>

static bool sensor_initialized = false;

void turbidity_sensor_init(void) {
    adc_gpio_init(TURBIDITY_SENSOR_PIN);
    sensor_initialized = true;
    printf("✅ Sensor de turbidez inicializado (ADC)\n");
}

float turbidity_read_value(void) {
    if (!sensor_initialized) {
        return -1.0f;
    }
    
    adc_select_input(2); // GPIO28 = ADC2
    uint16_t adc_value = adc_read();
    
    // Simulação de conversão para turbidez (0-20 NTU)
    float voltage = adc_value * 3.3f / 4096.0f;
    float turbidity = voltage * 6.0f; // Faixa 0-20 NTU
    
    return turbidity;
}

bool turbidity_is_acceptable(float turbidity) {
    return (turbidity <= TURBIDITY_MAX_ACCEPTABLE);
}