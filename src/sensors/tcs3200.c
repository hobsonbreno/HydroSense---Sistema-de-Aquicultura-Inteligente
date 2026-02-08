/**
 * HydroSense - Driver TCS3200 (Sensor de Cor)
 * 
 * Implementação para detectar turbidez/sujeira no aquário
 */

#include "sensors/tcs3200.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Dados globais do sensor
static tcs3200_data_t sensor_data = {0};

// Timeout para leitura de frequência (ms)
#define FREQ_READ_TIMEOUT_MS    100
#define FREQ_SAMPLE_COUNT       10

void tcs3200_init_gpio(void) {
    // Configura pinos de controle como saída
    gpio_init(TCS3200_S0_PIN);
    gpio_init(TCS3200_S1_PIN);
    gpio_init(TCS3200_S2_PIN);
    gpio_init(TCS3200_S3_PIN);
    
    gpio_set_dir(TCS3200_S0_PIN, GPIO_OUT);
    gpio_set_dir(TCS3200_S1_PIN, GPIO_OUT);
    gpio_set_dir(TCS3200_S2_PIN, GPIO_OUT);
    gpio_set_dir(TCS3200_S3_PIN, GPIO_OUT);
    
    // Configura pino de saída como entrada
    gpio_init(TCS3200_OUT_PIN);
    gpio_set_dir(TCS3200_OUT_PIN, GPIO_IN);
    
    // Configura OE (Output Enable) se disponível
    gpio_init(TCS3200_OE_PIN);
    gpio_set_dir(TCS3200_OE_PIN, GPIO_OUT);
    gpio_put(TCS3200_OE_PIN, 0);  // Ativo LOW - habilita saída
}

bool tcs3200_init(void) {
    printf("🔧 Inicializando TCS3200 (sensor de cor/turbidez)...\n");
    
    tcs3200_init_gpio();
    
    // Configura frequência de saída para 20% (melhor para Pico)
    tcs3200_set_frequency(TCS3200_FREQ_20);
    
    sleep_ms(100);
    
    // Faz leitura de teste
    tcs3200_rgb_t rgb;
    if (!tcs3200_read_rgb(&rgb)) {
        printf("⚠️ TCS3200: Leitura inicial falhou, verificar conexões\n");
        // Não retorna false pois sensor pode estar ok
    }
    
    sensor_data.inicializado = true;
    sensor_data.leituras_validas = 0;
    
    // Calibra cor de referência (água limpa)
    printf("📏 Calibrando cor de referência (água limpa)...\n");
    tcs3200_calibrar_agua_limpa();
    
    printf("✅ TCS3200 inicializado!\n");
    printf("   🎨 Referência: R=%d G=%d B=%d\n", 
           sensor_data.cor_referencia.red,
           sensor_data.cor_referencia.green,
           sensor_data.cor_referencia.blue);
    
    return true;
}

void tcs3200_set_frequency(uint8_t freq) {
    switch (freq) {
        case TCS3200_FREQ_OFF:
            gpio_put(TCS3200_S0_PIN, 0);
            gpio_put(TCS3200_S1_PIN, 0);
            break;
        case TCS3200_FREQ_2:
            gpio_put(TCS3200_S0_PIN, 0);
            gpio_put(TCS3200_S1_PIN, 1);
            break;
        case TCS3200_FREQ_20:
            gpio_put(TCS3200_S0_PIN, 1);
            gpio_put(TCS3200_S1_PIN, 0);
            break;
        case TCS3200_FREQ_100:
            gpio_put(TCS3200_S0_PIN, 1);
            gpio_put(TCS3200_S1_PIN, 1);
            break;
    }
}

void tcs3200_set_filter(uint8_t filter) {
    switch (filter) {
        case TCS3200_FILTER_RED:
            gpio_put(TCS3200_S2_PIN, 0);
            gpio_put(TCS3200_S3_PIN, 0);
            break;
        case TCS3200_FILTER_BLUE:
            gpio_put(TCS3200_S2_PIN, 0);
            gpio_put(TCS3200_S3_PIN, 1);
            break;
        case TCS3200_FILTER_CLEAR:
            gpio_put(TCS3200_S2_PIN, 1);
            gpio_put(TCS3200_S3_PIN, 0);
            break;
        case TCS3200_FILTER_GREEN:
            gpio_put(TCS3200_S2_PIN, 1);
            gpio_put(TCS3200_S3_PIN, 1);
            break;
    }
    sleep_us(100);  // Aguarda estabilização
}

uint16_t tcs3200_read_frequency(void) {
    uint32_t pulse_count = 0;
    uint32_t start_time = to_us_since_boot(get_absolute_time());
    uint32_t timeout = FREQ_READ_TIMEOUT_MS * 1000;
    bool last_state = gpio_get(TCS3200_OUT_PIN);
    
    // Conta pulsos por 100ms
    while ((to_us_since_boot(get_absolute_time()) - start_time) < timeout) {
        bool current_state = gpio_get(TCS3200_OUT_PIN);
        if (current_state && !last_state) {
            pulse_count++;
        }
        last_state = current_state;
    }
    
    // Converte para frequência (Hz)
    return (uint16_t)((pulse_count * 1000) / FREQ_READ_TIMEOUT_MS);
}

bool tcs3200_read_rgb(tcs3200_rgb_t* rgb) {
    if (!sensor_data.inicializado && rgb == NULL) {
        return false;
    }
    
    // Lê cada componente de cor
    tcs3200_set_filter(TCS3200_FILTER_RED);
    rgb->red = tcs3200_read_frequency();
    
    tcs3200_set_filter(TCS3200_FILTER_GREEN);
    rgb->green = tcs3200_read_frequency();
    
    tcs3200_set_filter(TCS3200_FILTER_BLUE);
    rgb->blue = tcs3200_read_frequency();
    
    tcs3200_set_filter(TCS3200_FILTER_CLEAR);
    rgb->clear = tcs3200_read_frequency();
    
    // Atualiza dados do sensor
    sensor_data.cor_atual = *rgb;
    sensor_data.ultima_leitura = to_ms_since_boot(get_absolute_time());
    sensor_data.leituras_validas++;
    
    return true;
}

void tcs3200_calibrar_agua_limpa(void) {
    // Faz múltiplas leituras e calcula média
    uint32_t soma_r = 0, soma_g = 0, soma_b = 0, soma_c = 0;
    
    for (int i = 0; i < 5; i++) {
        tcs3200_rgb_t rgb;
        tcs3200_read_rgb(&rgb);
        soma_r += rgb.red;
        soma_g += rgb.green;
        soma_b += rgb.blue;
        soma_c += rgb.clear;
        sleep_ms(50);
    }
    
    sensor_data.cor_referencia.red = soma_r / 5;
    sensor_data.cor_referencia.green = soma_g / 5;
    sensor_data.cor_referencia.blue = soma_b / 5;
    sensor_data.cor_referencia.clear = soma_c / 5;
}

float tcs3200_calcular_turbidez(void) {
    // Calcula diferença entre cor atual e referência
    int diff_r = abs((int)sensor_data.cor_atual.red - (int)sensor_data.cor_referencia.red);
    int diff_g = abs((int)sensor_data.cor_atual.green - (int)sensor_data.cor_referencia.green);
    int diff_b = abs((int)sensor_data.cor_atual.blue - (int)sensor_data.cor_referencia.blue);
    
    // Calcula índice de turbidez (0-100)
    float max_diff = (float)(diff_r + diff_g + diff_b) / 3.0f;
    
    // Normaliza para 0-100 baseado no threshold
    float turbidez = (max_diff / (float)TURBIDEZ_THRESHOLD) * 100.0f;
    if (turbidez > 100.0f) turbidez = 100.0f;
    
    sensor_data.turbidez_indice = turbidez;
    sensor_data.agua_limpa = (turbidez < 30.0f);  // < 30% = água limpa
    sensor_data.detectou_sujeira = (turbidez >= TURBIDEZ_THRESHOLD);
    
    return turbidez;
}

bool tcs3200_detectar_sujeira(void) {
    tcs3200_rgb_t rgb;
    tcs3200_read_rgb(&rgb);
    tcs3200_calcular_turbidez();
    
    if (sensor_data.detectou_sujeira) {
        printf("🚨 SUJEIRA DETECTADA! Turbidez: %.1f%% (R=%d G=%d B=%d)\n",
               sensor_data.turbidez_indice,
               rgb.red, rgb.green, rgb.blue);
    }
    
    return sensor_data.detectou_sujeira;
}

void tcs3200_get_data(tcs3200_data_t* data) {
    memcpy(data, &sensor_data, sizeof(tcs3200_data_t));
}
