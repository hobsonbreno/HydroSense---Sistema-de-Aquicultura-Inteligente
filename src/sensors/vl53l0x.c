/**
 * HydroSense - Driver VL53L0X (Sensor de Distância Laser)
 * 
 * Implementação para medir nível de água do tanque de 20 litros
 */

#include "sensors/vl53l0x.h"
#include <stdio.h>
#include <string.h>

// Dados globais do sensor
static vl53l0x_data_t sensor_data = {0};

// Configuração do tanque (pode ser calibrada)
static float cfg_altura_tanque = TANQUE_ALTURA_CM;
static float cfg_dist_sensor_fundo = TANQUE_DIST_SENSOR_FUNDO;

// Funções I2C auxiliares
static bool vl53l0x_write_byte(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_write_blocking(VL53L0X_I2C_PORT, VL53L0X_I2C_ADDR, buf, 2, false) == 2;
}

static bool vl53l0x_read_byte(uint8_t reg, uint8_t* value) {
    if (i2c_write_blocking(VL53L0X_I2C_PORT, VL53L0X_I2C_ADDR, &reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(VL53L0X_I2C_PORT, VL53L0X_I2C_ADDR, value, 1, false) == 1;
}

static bool vl53l0x_read_word(uint8_t reg, uint16_t* value) {
    uint8_t buf[2];
    if (i2c_write_blocking(VL53L0X_I2C_PORT, VL53L0X_I2C_ADDR, &reg, 1, true) != 1) {
        return false;
    }
    if (i2c_read_blocking(VL53L0X_I2C_PORT, VL53L0X_I2C_ADDR, buf, 2, false) != 2) {
        return false;
    }
    *value = (buf[0] << 8) | buf[1];
    return true;
}

bool vl53l0x_init(void) {
    printf("🔧 Inicializando VL53L0X (sensor de nível)...\n");
    
    // Inicializa I2C
    i2c_init(VL53L0X_I2C_PORT, VL53L0X_I2C_SPEED);
    gpio_set_function(VL53L0X_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(VL53L0X_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(VL53L0X_I2C_SDA);
    gpio_pull_up(VL53L0X_I2C_SCL);
    
    sleep_ms(50);
    
    // Verifica ID do sensor
    uint8_t model_id;
    if (!vl53l0x_read_byte(VL53L0X_REG_MODEL_ID, &model_id)) {
        printf("❌ VL53L0X não encontrado no I2C!\n");
        sensor_data.inicializado = false;
        return false;
    }
    
    if (model_id != 0xEE) {
        printf("❌ VL53L0X ID inválido: 0x%02X (esperado: 0xEE)\n", model_id);
        sensor_data.inicializado = false;
        return false;
    }
    
    // Configuração básica do sensor (modo contínuo simplificado)
    // Sequência de inicialização recomendada pelo datasheet
    vl53l0x_write_byte(0x88, 0x00);
    vl53l0x_write_byte(0x80, 0x01);
    vl53l0x_write_byte(0xFF, 0x01);
    vl53l0x_write_byte(0x00, 0x00);
    vl53l0x_write_byte(0x00, 0x01);
    vl53l0x_write_byte(0xFF, 0x00);
    vl53l0x_write_byte(0x80, 0x00);
    
    sensor_data.inicializado = true;
    sensor_data.leituras_validas = 0;
    sensor_data.leituras_erro = 0;
    
    printf("✅ VL53L0X inicializado! (Tanque: %.0fL, Altura: %.1fcm)\n", 
           TANQUE_CAPACIDADE_LITROS, cfg_altura_tanque);
    
    return true;
}

bool vl53l0x_read_distance(uint16_t* distance_mm) {
    if (!sensor_data.inicializado) {
        return false;
    }
    
    // Inicia medição single-shot
    vl53l0x_write_byte(VL53L0X_REG_SYSRANGE_START, 0x01);
    
    // Aguarda medição (timeout de 500ms)
    uint8_t status;
    uint32_t start = to_ms_since_boot(get_absolute_time());
    
    do {
        if (!vl53l0x_read_byte(VL53L0X_REG_RESULT_RANGE_STATUS, &status)) {
            sensor_data.leituras_erro++;
            return false;
        }
        
        if (to_ms_since_boot(get_absolute_time()) - start > 500) {
            printf("⚠️ VL53L0X timeout na leitura\n");
            sensor_data.leituras_erro++;
            return false;
        }
        
        sleep_ms(5);
    } while ((status & 0x01) == 0);
    
    // Lê resultado
    uint16_t range;
    if (!vl53l0x_read_word(0x1E, &range)) {  // Range result register
        sensor_data.leituras_erro++;
        return false;
    }
    
    // Limpa interrupção
    vl53l0x_write_byte(VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    // Verifica se leitura é válida (8190mm = erro)
    if (range >= 8190) {
        sensor_data.leituras_erro++;
        return false;
    }
    
    *distance_mm = range;
    sensor_data.distancia_mm = range;
    sensor_data.distancia_cm = range / 10.0f;
    sensor_data.ultima_leitura = to_ms_since_boot(get_absolute_time());
    sensor_data.leituras_validas++;
    
    // Calcula nível
    sensor_data.nivel_litros = vl53l0x_distancia_para_litros(sensor_data.distancia_cm);
    sensor_data.nivel_percentual = vl53l0x_distancia_para_percent(sensor_data.distancia_cm);
    
    return true;
}

float vl53l0x_distancia_para_litros(float distancia_cm) {
    // Distância até a superfície da água
    // Se sensor está a 35cm do fundo e a água está a 30cm (cheio):
    // distancia_lida = 35 - 30 = 5cm da superfície
    // altura_agua = 35 - distancia_lida
    
    float altura_agua = cfg_dist_sensor_fundo - distancia_cm;
    
    // Limita valores
    if (altura_agua < 0) altura_agua = 0;
    if (altura_agua > cfg_altura_tanque) altura_agua = cfg_altura_tanque;
    
    // Converte altura para litros (proporcional)
    float litros = (altura_agua / cfg_altura_tanque) * TANQUE_CAPACIDADE_LITROS;
    
    return litros;
}

float vl53l0x_distancia_para_percent(float distancia_cm) {
    float litros = vl53l0x_distancia_para_litros(distancia_cm);
    return (litros / TANQUE_CAPACIDADE_LITROS) * 100.0f;
}

float vl53l0x_get_water_level_liters(void) {
    uint16_t dist;
    if (vl53l0x_read_distance(&dist)) {
        return sensor_data.nivel_litros;
    }
    return sensor_data.nivel_litros;  // Retorna último valor válido
}

float vl53l0x_get_water_level_percent(void) {
    uint16_t dist;
    if (vl53l0x_read_distance(&dist)) {
        return sensor_data.nivel_percentual;
    }
    return sensor_data.nivel_percentual;
}

void vl53l0x_get_data(vl53l0x_data_t* data) {
    memcpy(data, &sensor_data, sizeof(vl53l0x_data_t));
}

bool vl53l0x_is_level_critical(float target_percent) {
    return sensor_data.nivel_percentual <= target_percent;
}

void vl53l0x_calibrar_tanque(float altura_cm, float dist_sensor_fundo) {
    cfg_altura_tanque = altura_cm;
    cfg_dist_sensor_fundo = dist_sensor_fundo;
    printf("📏 Calibração VL53L0X: Altura=%.1fcm, Dist.Fundo=%.1fcm\n", 
           altura_cm, dist_sensor_fundo);
}
