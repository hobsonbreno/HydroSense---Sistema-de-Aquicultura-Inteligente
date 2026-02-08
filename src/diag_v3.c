/**
 * HydroSense - Diagnóstico Simplificado v3
 * 
 * Configuração:
 * - Sensores (VL53L0X, AHT10): I2C1 em GPIO2/3 (extensor)
 * - OLED: I2C1 em GPIO14/15 (BitDogLab direto)
 * - Servo: GPIO16
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <string.h>

// Pinos
#define LED_PIN         25
#define SERVO_PIN       16

// I2C Sensores (extensor)
#define SENSOR_SDA      2
#define SENSOR_SCL      3

// I2C OLED (BitDogLab)
#define OLED_SDA        14
#define OLED_SCL        15

// Endereços I2C
#define VL53L0X_ADDR    0x29
#define AHT10_ADDR      0x38
#define OLED_ADDR       0x3C

// OLED config
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

// Dados dos sensores
static float temperatura = 0;
static float umidade = 0;
static uint16_t distancia_mm = 0;

// Fonte 5x7 básica para OLED
static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, // espaço
    0x00,0x00,0x5F,0x00,0x00, // !
    0x00,0x07,0x00,0x07,0x00, // "
    0x14,0x7F,0x14,0x7F,0x14, // #
    0x24,0x2A,0x7F,0x2A,0x12, // $
    0x23,0x13,0x08,0x64,0x62, // %
    0x36,0x49,0x56,0x20,0x50, // &
    0x00,0x08,0x07,0x03,0x00, // '
    0x00,0x1C,0x22,0x41,0x00, // (
    0x00,0x41,0x22,0x1C,0x00, // )
    0x2A,0x1C,0x7F,0x1C,0x2A, // *
    0x08,0x08,0x3E,0x08,0x08, // +
    0x00,0x80,0x70,0x30,0x00, // ,
    0x08,0x08,0x08,0x08,0x08, // -
    0x00,0x00,0x60,0x60,0x00, // .
    0x20,0x10,0x08,0x04,0x02, // /
    0x3E,0x51,0x49,0x45,0x3E, // 0
    0x00,0x42,0x7F,0x40,0x00, // 1
    0x72,0x49,0x49,0x49,0x46, // 2
    0x21,0x41,0x49,0x4D,0x33, // 3
    0x18,0x14,0x12,0x7F,0x10, // 4
    0x27,0x45,0x45,0x45,0x39, // 5
    0x3C,0x4A,0x49,0x49,0x31, // 6
    0x41,0x21,0x11,0x09,0x07, // 7
    0x36,0x49,0x49,0x49,0x36, // 8
    0x46,0x49,0x49,0x29,0x1E, // 9
    0x00,0x00,0x14,0x00,0x00, // :
    0x00,0x40,0x34,0x00,0x00, // ;
    0x00,0x08,0x14,0x22,0x41, // <
    0x14,0x14,0x14,0x14,0x14, // =
    0x00,0x41,0x22,0x14,0x08, // >
    0x02,0x01,0x59,0x09,0x06, // ?
    0x3E,0x41,0x5D,0x59,0x4E, // @
    0x7C,0x12,0x11,0x12,0x7C, // A
    0x7F,0x49,0x49,0x49,0x36, // B
    0x3E,0x41,0x41,0x41,0x22, // C
    0x7F,0x41,0x41,0x41,0x3E, // D
    0x7F,0x49,0x49,0x49,0x41, // E
    0x7F,0x09,0x09,0x09,0x01, // F
    0x3E,0x41,0x41,0x51,0x73, // G
    0x7F,0x08,0x08,0x08,0x7F, // H
    0x00,0x41,0x7F,0x41,0x00, // I
    0x20,0x40,0x41,0x3F,0x01, // J
    0x7F,0x08,0x14,0x22,0x41, // K
    0x7F,0x40,0x40,0x40,0x40, // L
    0x7F,0x02,0x1C,0x02,0x7F, // M
    0x7F,0x04,0x08,0x10,0x7F, // N
    0x3E,0x41,0x41,0x41,0x3E, // O
    0x7F,0x09,0x09,0x09,0x06, // P
    0x3E,0x41,0x51,0x21,0x5E, // Q
    0x7F,0x09,0x19,0x29,0x46, // R
    0x26,0x49,0x49,0x49,0x32, // S
    0x03,0x01,0x7F,0x01,0x03, // T
    0x3F,0x40,0x40,0x40,0x3F, // U
    0x1F,0x20,0x40,0x20,0x1F, // V
    0x3F,0x40,0x38,0x40,0x3F, // W
    0x63,0x14,0x08,0x14,0x63, // X
    0x03,0x04,0x78,0x04,0x03, // Y
    0x61,0x59,0x49,0x4D,0x43, // Z
};

// ============================================================
// Funções I2C
// ============================================================

void i2c_init_sensors(void) {
    i2c_init(i2c1, 100000);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
}

void i2c_switch_to_sensors(void) {
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    sleep_us(50);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_ms(5);  // Mais tempo para estabilizar
}

void i2c_switch_to_oled(void) {
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    sleep_us(50);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(5);  // Mais tempo para estabilizar
}

// ============================================================
// AHT10
// ============================================================

bool aht10_init(void) {
    i2c_switch_to_sensors();
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    return i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) >= 0;
}

bool aht10_read(void) {
    i2c_switch_to_sensors();
    
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) return false;
    sleep_ms(80);
    
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) return false;
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    umidade = (float)hum_raw / 1048576.0f * 100.0f;
    temperatura = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    
    return true;
}

// ============================================================
// VL53L0X - Simplificado para clone v2
// ============================================================

static uint8_t vl53_stop_var = 0;

bool vl53_init(void) {
    i2c_switch_to_sensors();
    
    // Lê ID
    uint8_t reg = 0xC0;
    uint8_t id;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &id, 1, false);
    printf("   VL53 ID: 0x%02X\n", id);
    
    if (id == 0xFF || id == 0x00) return false;
    
    // Soft reset
    uint8_t buf[2] = {0xBF, 0x01};
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    sleep_ms(50);
    
    // Aguarda boot
    reg = 0x00;
    int timeout = 100;
    while (timeout-- > 0) {
        i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
        uint8_t val;
        i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
        if (val & 0x01) break;
        sleep_ms(10);
    }
    
    // Configuração básica
    buf[0] = 0x80; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x00; buf[1] = 0x00;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    reg = 0x91;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &vl53_stop_var, 1, false);
    
    buf[0] = 0x00; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x00;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x80; buf[1] = 0x00;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    printf("   VL53 stop_var: 0x%02X\n", vl53_stop_var);
    return true;
}

uint16_t vl53_read_mm(void) {
    i2c_switch_to_sensors();
    
    uint8_t buf[2];
    
    // Prepara medição
    buf[0] = 0x80; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x00; buf[1] = 0x00;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x91; buf[1] = vl53_stop_var;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x00; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x00;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x80; buf[1] = 0x00;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    // Inicia medição
    buf[0] = 0x00; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    // Aguarda início
    uint8_t reg = 0x00;
    int timeout = 50;
    while (timeout-- > 0) {
        i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
        uint8_t val;
        i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
        if ((val & 0x01) == 0) break;
        sleep_ms(2);
    }
    
    // Aguarda resultado
    reg = 0x13;
    timeout = 100;
    while (timeout-- > 0) {
        i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
        uint8_t val;
        i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
        if (val & 0x07) break;
        sleep_ms(5);
    }
    
    if (timeout <= 0) return 0xFFFF;
    
    // Lê distância
    reg = 0x1E;  // 0x14 + 10
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    uint16_t distance = ((uint16_t)buf[0] << 8) | buf[1];
    
    // Limpa interrupção
    buf[0] = 0x0B; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    return (distance > 0 && distance < 2000) ? distance : 0xFFFF;
}

// ============================================================
// OLED
// ============================================================

void oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
}

void oled_init(void) {
    i2c_switch_to_oled();
    
    oled_cmd(0xAE);  // Display off
    oled_cmd(0xD5); oled_cmd(0x80);  // Clock
    oled_cmd(0xA8); oled_cmd(0x3F);  // Multiplex
    oled_cmd(0xD3); oled_cmd(0x00);  // Offset
    oled_cmd(0x40);  // Start line
    oled_cmd(0x8D); oled_cmd(0x14);  // Charge pump
    oled_cmd(0x20); oled_cmd(0x00);  // Memory mode
    oled_cmd(0xA1);  // Segment remap
    oled_cmd(0xC8);  // COM scan
    oled_cmd(0xDA); oled_cmd(0x12);  // COM pins
    oled_cmd(0x81); oled_cmd(0xCF);  // Contrast
    oled_cmd(0xD9); oled_cmd(0xF1);  // Pre-charge
    oled_cmd(0xDB); oled_cmd(0x40);  // VCOMH
    oled_cmd(0xA4);  // Resume RAM
    oled_cmd(0xA6);  // Normal display
    oled_cmd(0xAF);  // Display on
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_update(void) {
    i2c_switch_to_oled();
    
    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);  // Column range
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);    // Page range
    
    for (int i = 0; i < sizeof(oled_buffer); i += 16) {
        uint8_t buf[17];
        buf[0] = 0x40;
        memcpy(&buf[1], &oled_buffer[i], 16);
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 17, false);
    }
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    int idx = x + (y / 8) * OLED_WIDTH;
    if (on) oled_buffer[idx] |= (1 << (y % 8));
    else oled_buffer[idx] &= ~(1 << (y % 8));
}

void oled_char(int x, int y, char c) {
    if (c < 32 || c > 90) c = '?';
    int idx = (c - 32) * 5;
    for (int i = 0; i < 5; i++) {
        uint8_t col = font5x7[idx + i];
        for (int j = 0; j < 7; j++) {
            oled_pixel(x + i, y + j, col & (1 << j));
        }
    }
}

void oled_text(int x, int y, const char* str) {
    while (*str) {
        oled_char(x, y, *str++);
        x += 6;
    }
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    printf("\n\n");
    printf("===========================================\n");
    printf("  HydroSense - Diagnostico Simples v3\n");
    printf("===========================================\n\n");
    fflush(stdout);
    
    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    // Inicializa I2C
    printf("Inicializando I2C em GPIO%d/%d...\n", SENSOR_SDA, SENSOR_SCL);
    i2c_init_sensors();
    sleep_ms(100);
    
    // Scan sensores
    printf("\nScan I2C (sensores):\n");
    i2c_switch_to_sensors();
    for (int addr = 0x08; addr < 0x78; addr++) {
        uint8_t data;
        if (i2c_read_blocking(i2c1, addr, &data, 1, false) >= 0) {
            printf("  0x%02X encontrado", addr);
            if (addr == VL53L0X_ADDR) printf(" [VL53]");
            if (addr == AHT10_ADDR) printf(" [AHT10]");
            printf("\n");
        }
    }
    
    // Inicializa sensores
    printf("\nInicializando AHT10...\n");
    bool aht_ok = aht10_init();
    printf("  AHT10: %s\n", aht_ok ? "OK" : "FALHOU");
    
    printf("\nInicializando VL53L0X...\n");
    bool vl53_ok = vl53_init();
    printf("  VL53: %s\n", vl53_ok ? "OK" : "FALHOU");
    
    // Inicializa OLED
    printf("\nInicializando OLED em GPIO%d/%d...\n", OLED_SDA, OLED_SCL);
    oled_init();
    oled_clear();
    oled_text(10, 0, "HYDROSENSE");
    oled_text(0, 20, "DIAGNOSTICO V3");
    oled_text(0, 40, "INICIANDO...");
    oled_update();
    printf("  OLED: OK\n");
    
    sleep_ms(2000);
    
    // Loop principal
    printf("\n=== LEITURA CONTINUA ===\n");
    
    int ciclo = 0;
    while (1) {
        ciclo++;
        gpio_put(LED_PIN, ciclo % 2);
        
        // Lê sensores
        printf("\n[Ciclo %d]\n", ciclo);
        
        if (aht_ok && aht10_read()) {
            printf("  Temp: %.1f C  Umid: %.1f%%\n", temperatura, umidade);
        } else {
            printf("  AHT10: erro de leitura\n");
        }
        
        if (vl53_ok) {
            distancia_mm = vl53_read_mm();
            if (distancia_mm != 0xFFFF) {
                printf("  Dist: %d mm\n", distancia_mm);
            } else {
                printf("  VL53: timeout\n");
            }
        }
        
        // Atualiza OLED
        oled_clear();
        oled_text(0, 0, "HYDROSENSE V3");
        
        char buf[32];
        if (aht_ok) {
            snprintf(buf, sizeof(buf), "TEMP: %.1fC", temperatura);
            oled_text(0, 16, buf);
            snprintf(buf, sizeof(buf), "UMID: %.1f%%", umidade);
            oled_text(0, 26, buf);
        }
        
        if (vl53_ok && distancia_mm != 0xFFFF) {
            snprintf(buf, sizeof(buf), "DIST: %dmm", distancia_mm);
            oled_text(0, 40, buf);
        } else {
            oled_text(0, 40, "DIST: ---");
        }
        
        snprintf(buf, sizeof(buf), "CICLO: %d", ciclo);
        oled_text(0, 54, buf);
        
        oled_update();
        
        sleep_ms(2000);
    }
    
    return 0;
}
