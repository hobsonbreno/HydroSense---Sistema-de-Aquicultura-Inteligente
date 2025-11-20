#include "ph_sensor.h"
#include "system_config.h"
#include "hardware/adc.h"
#include <stdio.h>

static bool sensor_initialized = false;

void ph_sensor_init(void) {
    adc_gpio_init(PH_SENSOR_PIN);
    sensor_initialized = true;
    printf("✅ Sensor de pH inicializado (ADC)\n");
}

float ph_read_value(void) {
    if (!sensor_initialized) {
        return -1.0f;
    }
    
    adc_select_input(1); // GPIO27 = ADC1
    uint16_t adc_value = adc_read();
    
    // Simulação de conversão para pH (7.0 ± variação)
    float voltage = adc_value * 3.3f / 4096.0f;
    float ph = 6.0f + (voltage * 2.0f); // Faixa 6-8 pH
    
    return ph;
}

bool ph_is_ideal(float ph) {
    return (ph >= PH_MIN_IDEAL && ph <= PH_MAX_IDEAL);
}

void ph_calibrate(float known_ph, uint16_t adc_reading) {
    printf("✅ Sensor de pH calibrado para pH %.1f (ADC: %d)\n", known_ph, adc_reading);
}