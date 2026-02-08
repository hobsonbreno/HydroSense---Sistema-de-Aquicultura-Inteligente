/**
 * HydroSense - Driver VL53L0X (Sensor de Distância Laser)
 * 
 * Usado para medir o nível de água do tanque de 20 litros
 * Converte distância em volume/porcentagem
 */

#ifndef VL53L0X_H
#define VL53L0X_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdbool.h>

// Configuração I2C do VL53L0X
#define VL53L0X_I2C_PORT        i2c0
#define VL53L0X_I2C_SDA         2
#define VL53L0X_I2C_SCL         3
#define VL53L0X_I2C_ADDR        0x29
#define VL53L0X_I2C_SPEED       400000

// Configuração do tanque de 20 litros
#define TANQUE_CAPACIDADE_LITROS    20.0f
#define TANQUE_ALTURA_CM            30.0f   // Altura interna do tanque em cm
#define TANQUE_DIST_SENSOR_FUNDO    35.0f   // Distância do sensor até o fundo (cm)

// Níveis de referência
#define NIVEL_100_PERCENT_LITROS    20.0f
#define NIVEL_50_PERCENT_LITROS     10.0f
#define NIVEL_25_PERCENT_LITROS     5.0f
#define NIVEL_20_PERCENT_LITROS     4.0f

// Registradores VL53L0X essenciais
#define VL53L0X_REG_SYSRANGE_START              0x00
#define VL53L0X_REG_RESULT_RANGE_STATUS         0x14
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS     0x13
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR      0x0B
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS    0x8A
#define VL53L0X_REG_MODEL_ID                    0xC0

// Estrutura de dados do sensor
typedef struct {
    bool inicializado;
    uint16_t distancia_mm;          // Distância medida em mm
    float distancia_cm;             // Distância em cm
    float nivel_litros;             // Volume de água em litros
    float nivel_percentual;         // Nível em porcentagem (0-100%)
    uint32_t ultima_leitura;        // Timestamp da última leitura
    uint16_t leituras_validas;      // Contador de leituras válidas
    uint16_t leituras_erro;         // Contador de erros
} vl53l0x_data_t;

// Funções do driver
bool vl53l0x_init(void);
bool vl53l0x_read_distance(uint16_t* distance_mm);
float vl53l0x_get_water_level_liters(void);
float vl53l0x_get_water_level_percent(void);
void vl53l0x_get_data(vl53l0x_data_t* data);
bool vl53l0x_is_level_critical(float target_percent);
void vl53l0x_calibrar_tanque(float altura_cm, float dist_sensor_fundo);

// Funções auxiliares
float vl53l0x_distancia_para_litros(float distancia_cm);
float vl53l0x_distancia_para_percent(float distancia_cm);

#endif // VL53L0X_H
