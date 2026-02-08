/**
 * HydroSense - Driver TCS3200 (Sensor de Cor)
 * 
 * Usado para detectar turbidez da água através da cor
 * Detecta sujeira/lodo nas paredes do aquário
 */

#ifndef TCS3200_H
#define TCS3200_H

#include "pico/stdlib.h"
#include <stdbool.h>

// Pinos do TCS3200
#define TCS3200_S0_PIN          8
#define TCS3200_S1_PIN          9
#define TCS3200_S2_PIN          10
#define TCS3200_S3_PIN          11
#define TCS3200_OUT_PIN         12
#define TCS3200_OE_PIN          13  // Output Enable (opcional, ativo LOW)

// Seleção de frequência (S0, S1)
#define TCS3200_FREQ_OFF        0   // Power down
#define TCS3200_FREQ_2          1   // 2% scaling
#define TCS3200_FREQ_20         2   // 20% scaling
#define TCS3200_FREQ_100        3   // 100% scaling

// Seleção de filtro de cor (S2, S3)
#define TCS3200_FILTER_RED      0
#define TCS3200_FILTER_BLUE     1
#define TCS3200_FILTER_CLEAR    2
#define TCS3200_FILTER_GREEN    3

// Limites de cor para água limpa (calibrar conforme aquário)
#define AGUA_LIMPA_R_MIN        180
#define AGUA_LIMPA_R_MAX        255
#define AGUA_LIMPA_G_MIN        200
#define AGUA_LIMPA_G_MAX        255
#define AGUA_LIMPA_B_MIN        200
#define AGUA_LIMPA_B_MAX        255

// Threshold para detectar sujeira
#define TURBIDEZ_THRESHOLD      30   // Diferença de cor que indica sujeira

// Estrutura de cores RGB
typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t clear;
} tcs3200_rgb_t;

// Estrutura de dados do sensor
typedef struct {
    bool inicializado;
    tcs3200_rgb_t cor_atual;
    tcs3200_rgb_t cor_referencia;   // Cor da água limpa (calibração)
    float turbidez_indice;          // 0 = limpa, 100 = muito turva
    bool agua_limpa;                // true se dentro dos parâmetros
    bool detectou_sujeira;          // true se detectou lodo/sujeira
    uint32_t ultima_leitura;
    uint16_t leituras_validas;
} tcs3200_data_t;

// Funções do driver
bool tcs3200_init(void);
void tcs3200_set_frequency(uint8_t freq);
void tcs3200_set_filter(uint8_t filter);
uint16_t tcs3200_read_frequency(void);
bool tcs3200_read_rgb(tcs3200_rgb_t* rgb);
float tcs3200_calcular_turbidez(void);
bool tcs3200_detectar_sujeira(void);
void tcs3200_calibrar_agua_limpa(void);
void tcs3200_get_data(tcs3200_data_t* data);

#endif // TCS3200_H
