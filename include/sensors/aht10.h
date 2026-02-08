/**
 * HydroSense - Driver AHT10 (Sensor de Temperatura e Umidade)
 * 
 * Sensor digital I2C de alta precisão
 * Temperatura: -40°C a +85°C (±0.3°C)
 * Umidade: 0-100% RH (±2%)
 */

#ifndef AHT10_H
#define AHT10_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdbool.h>

// Configuração I2C do AHT10
#define AHT10_I2C_PORT          i2c1
#define AHT10_I2C_SDA           14
#define AHT10_I2C_SCL           15
#define AHT10_I2C_ADDR          0x38
#define AHT10_I2C_SPEED         400000

// Comandos AHT10
#define AHT10_CMD_INIT          0xE1
#define AHT10_CMD_TRIGGER       0xAC
#define AHT10_CMD_SOFT_RESET    0xBA

// Parâmetros de medição
#define AHT10_TRIGGER_DATA1     0x33
#define AHT10_TRIGGER_DATA2     0x00

// Limites para aquicultura
#define TEMP_AQUARIO_MIN        22.0f
#define TEMP_AQUARIO_MAX        28.0f
#define TEMP_AQUARIO_IDEAL      25.0f
#define UMIDADE_AMBIENTE_MIN    40.0f
#define UMIDADE_AMBIENTE_MAX    80.0f

// Estrutura de dados do sensor
typedef struct {
    bool inicializado;
    float temperatura;              // Temperatura em °C
    float umidade;                  // Umidade relativa em %
    uint32_t ultima_leitura;        // Timestamp da última leitura
    bool temperatura_ok;            // Temperatura dentro da faixa ideal
    bool umidade_ok;                // Umidade dentro da faixa ideal
    uint16_t leituras_validas;
    uint16_t leituras_erro;
} aht10_data_t;

// Funções do driver
bool aht10_init(void);
bool aht10_read(float* temperatura, float* umidade);
void aht10_get_data(aht10_data_t* data);
bool aht10_soft_reset(void);
bool aht10_is_temperature_ok(float temp);
bool aht10_is_humidity_ok(float hum);

#endif // AHT10_H
