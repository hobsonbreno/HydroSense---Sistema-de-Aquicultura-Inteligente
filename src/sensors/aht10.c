/**
 * HydroSense - Driver AHT10 (Sensor de Temperatura e Umidade)
 * 
 * Implementação para monitoramento ambiental do aquário
 */

#include "sensors/aht10.h"
#include <stdio.h>
#include <string.h>

// Dados globais do sensor
static aht10_data_t sensor_data = {0};

// Funções I2C auxiliares
static bool aht10_write_cmd(uint8_t cmd, uint8_t data1, uint8_t data2) {
    uint8_t buf[3] = {cmd, data1, data2};
    return i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDR, buf, 3, false) == 3;
}

static bool aht10_read_data(uint8_t* buf, size_t len) {
    return i2c_read_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDR, buf, len, false) == (int)len;
}

bool aht10_init(void) {
    printf("🔧 Inicializando AHT10 (temperatura/umidade)...\n");
    
    // Inicializa I2C (compartilhado com OLED no I2C1)
    i2c_init(AHT10_I2C_PORT, AHT10_I2C_SPEED);
    gpio_set_function(AHT10_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(AHT10_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(AHT10_I2C_SDA);
    gpio_pull_up(AHT10_I2C_SCL);
    
    sleep_ms(40);  // Aguarda power-up
    
    // Envia comando de inicialização
    if (!aht10_write_cmd(AHT10_CMD_INIT, 0x08, 0x00)) {
        printf("❌ AHT10 não respondeu ao comando de inicialização!\n");
        sensor_data.inicializado = false;
        return false;
    }
    
    sleep_ms(10);
    
    // Verifica status
    uint8_t status;
    if (!aht10_read_data(&status, 1)) {
        printf("❌ AHT10 não encontrado no I2C!\n");
        sensor_data.inicializado = false;
        return false;
    }
    
    // Bit 3 deve ser 1 (calibrado)
    if (!(status & 0x08)) {
        printf("⚠️ AHT10 não calibrado, tentando calibrar...\n");
        aht10_write_cmd(AHT10_CMD_INIT, 0x08, 0x00);
        sleep_ms(10);
    }
    
    sensor_data.inicializado = true;
    sensor_data.leituras_validas = 0;
    sensor_data.leituras_erro = 0;
    
    printf("✅ AHT10 inicializado!\n");
    printf("   📏 Faixa temperatura: %.0f°C - %.0f°C (ideal: %.0f°C)\n",
           TEMP_AQUARIO_MIN, TEMP_AQUARIO_MAX, TEMP_AQUARIO_IDEAL);
    
    return true;
}

bool aht10_read(float* temperatura, float* umidade) {
    if (!sensor_data.inicializado) {
        return false;
    }
    
    // Envia comando de medição
    if (!aht10_write_cmd(AHT10_CMD_TRIGGER, AHT10_TRIGGER_DATA1, AHT10_TRIGGER_DATA2)) {
        sensor_data.leituras_erro++;
        return false;
    }
    
    // Aguarda medição (típico: 75ms)
    sleep_ms(80);
    
    // Lê 6 bytes de dados
    uint8_t buf[6];
    if (!aht10_read_data(buf, 6)) {
        sensor_data.leituras_erro++;
        return false;
    }
    
    // Verifica se está ocupado (bit 7)
    if (buf[0] & 0x80) {
        sleep_ms(20);
        if (!aht10_read_data(buf, 6)) {
            sensor_data.leituras_erro++;
            return false;
        }
    }
    
    // Extrai dados de umidade (20 bits)
    uint32_t raw_humidity = ((uint32_t)(buf[1]) << 12) | 
                            ((uint32_t)(buf[2]) << 4) | 
                            ((buf[3] & 0xF0) >> 4);
    
    // Extrai dados de temperatura (20 bits)
    uint32_t raw_temp = ((uint32_t)(buf[3] & 0x0F) << 16) | 
                        ((uint32_t)(buf[4]) << 8) | 
                        buf[5];
    
    // Converte para valores físicos
    *umidade = ((float)raw_humidity / 1048576.0f) * 100.0f;
    *temperatura = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;
    
    // Atualiza dados do sensor
    sensor_data.temperatura = *temperatura;
    sensor_data.umidade = *umidade;
    sensor_data.temperatura_ok = aht10_is_temperature_ok(*temperatura);
    sensor_data.umidade_ok = aht10_is_humidity_ok(*umidade);
    sensor_data.ultima_leitura = to_ms_since_boot(get_absolute_time());
    sensor_data.leituras_validas++;
    
    return true;
}

void aht10_get_data(aht10_data_t* data) {
    memcpy(data, &sensor_data, sizeof(aht10_data_t));
}

bool aht10_soft_reset(void) {
    uint8_t cmd = AHT10_CMD_SOFT_RESET;
    if (i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDR, &cmd, 1, false) != 1) {
        return false;
    }
    sleep_ms(20);
    return aht10_init();
}

bool aht10_is_temperature_ok(float temp) {
    return (temp >= TEMP_AQUARIO_MIN && temp <= TEMP_AQUARIO_MAX);
}

bool aht10_is_humidity_ok(float hum) {
    return (hum >= UMIDADE_AMBIENTE_MIN && hum <= UMIDADE_AMBIENTE_MAX);
}
