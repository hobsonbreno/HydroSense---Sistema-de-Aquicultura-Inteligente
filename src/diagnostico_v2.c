/**
 * HydroSense - DIAGNÓSTICO COM PINOS CORRETOS
 * 
 * Configuração do usuário:
 * - Extensor I2C conectado em GPIO2 (SDA) e GPIO3 (SCL)
 * - VL53L0X no terminal J3 do extensor
 * - AHT10 no terminal J2 do extensor
 * - Sensor de cor no terminal J4 do extensor
 * - Display OLED no extensor
 * - Servo motor GPIO16
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// CONFIGURAÇÃO CORRETA DOS PINOS
// ============================================================

// I2C - Sensores no extensor GPIO2/GPIO3
#define I2C_PORT        i2c1
#define I2C_SDA_PIN     2    // GPIO2 (sensores)
#define I2C_SCL_PIN     3    // GPIO3 (sensores)
#define I2C_SPEED       100000  // 100kHz

// OLED direto na BitDogLab GPIO14/15 (também i2c1)
#define OLED_SDA_PIN    14
#define OLED_SCL_PIN    15

// Endereços I2C conhecidos
#define VL53L0X_ADDR    0x29   // Sensor VL53L0X/L1X v2 (distância laser)
#define AHT10_ADDR      0x38
#define SSD1306_ADDR    0x3C  // Pode ser 0x3D

// Funções para alternar I2C entre pinos
static void i2c_switch_to_sensors(void) {
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_NULL);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    sleep_us(100);
}

static void i2c_switch_to_oled(void) {
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);
    sleep_us(100);
}

// Servo Motor
#define SERVO_PIN       16

// LED
#define LED_PIN         25

// ============================================================
// Display OLED SSD1306 - Driver Básico
// ============================================================
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      8

static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];
static uint8_t oled_addr = SSD1306_ADDR;
static bool oled_found = true;  // OLED está na BitDogLab, assumimos que existe
static bool aht10_found = false;
static bool vl53_found = false;

// Fonte 5x7 básica (ASCII 32-126)
static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, // 32 space
    0x00,0x00,0x5F,0x00,0x00, // 33 !
    0x00,0x07,0x00,0x07,0x00, // 34 "
    0x14,0x7F,0x14,0x7F,0x14, // 35 #
    0x24,0x2A,0x7F,0x2A,0x12, // 36 $
    0x23,0x13,0x08,0x64,0x62, // 37 %
    0x36,0x49,0x56,0x20,0x50, // 38 &
    0x00,0x08,0x07,0x03,0x00, // 39 '
    0x00,0x1C,0x22,0x41,0x00, // 40 (
    0x00,0x41,0x22,0x1C,0x00, // 41 )
    0x2A,0x1C,0x7F,0x1C,0x2A, // 42 *
    0x08,0x08,0x3E,0x08,0x08, // 43 +
    0x00,0x80,0x70,0x30,0x00, // 44 ,
    0x08,0x08,0x08,0x08,0x08, // 45 -
    0x00,0x00,0x60,0x60,0x00, // 46 .
    0x20,0x10,0x08,0x04,0x02, // 47 /
    0x3E,0x51,0x49,0x45,0x3E, // 48 0
    0x00,0x42,0x7F,0x40,0x00, // 49 1
    0x72,0x49,0x49,0x49,0x46, // 50 2
    0x21,0x41,0x49,0x4D,0x33, // 51 3
    0x18,0x14,0x12,0x7F,0x10, // 52 4
    0x27,0x45,0x45,0x45,0x39, // 53 5
    0x3C,0x4A,0x49,0x49,0x31, // 54 6
    0x41,0x21,0x11,0x09,0x07, // 55 7
    0x36,0x49,0x49,0x49,0x36, // 56 8
    0x46,0x49,0x49,0x29,0x1E, // 57 9
    0x00,0x00,0x14,0x00,0x00, // 58 :
    0x00,0x40,0x34,0x00,0x00, // 59 ;
    0x00,0x08,0x14,0x22,0x41, // 60 <
    0x14,0x14,0x14,0x14,0x14, // 61 =
    0x00,0x41,0x22,0x14,0x08, // 62 >
    0x02,0x01,0x59,0x09,0x06, // 63 ?
    0x3E,0x41,0x5D,0x59,0x4E, // 64 @
    0x7C,0x12,0x11,0x12,0x7C, // 65 A
    0x7F,0x49,0x49,0x49,0x36, // 66 B
    0x3E,0x41,0x41,0x41,0x22, // 67 C
    0x7F,0x41,0x41,0x41,0x3E, // 68 D
    0x7F,0x49,0x49,0x49,0x41, // 69 E
    0x7F,0x09,0x09,0x09,0x01, // 70 F
    0x3E,0x41,0x41,0x51,0x73, // 71 G
    0x7F,0x08,0x08,0x08,0x7F, // 72 H
    0x00,0x41,0x7F,0x41,0x00, // 73 I
    0x20,0x40,0x41,0x3F,0x01, // 74 J
    0x7F,0x08,0x14,0x22,0x41, // 75 K
    0x7F,0x40,0x40,0x40,0x40, // 76 L
    0x7F,0x02,0x1C,0x02,0x7F, // 77 M
    0x7F,0x04,0x08,0x10,0x7F, // 78 N
    0x3E,0x41,0x41,0x41,0x3E, // 79 O
    0x7F,0x09,0x09,0x09,0x06, // 80 P
    0x3E,0x41,0x51,0x21,0x5E, // 81 Q
    0x7F,0x09,0x19,0x29,0x46, // 82 R
    0x26,0x49,0x49,0x49,0x32, // 83 S
    0x03,0x01,0x7F,0x01,0x03, // 84 T
    0x3F,0x40,0x40,0x40,0x3F, // 85 U
    0x1F,0x20,0x40,0x20,0x1F, // 86 V
    0x3F,0x40,0x38,0x40,0x3F, // 87 W
    0x63,0x14,0x08,0x14,0x63, // 88 X
    0x03,0x04,0x78,0x04,0x03, // 89 Y
    0x61,0x59,0x49,0x4D,0x43, // 90 Z
    0x00,0x7F,0x41,0x41,0x41, // 91 [
    0x02,0x04,0x08,0x10,0x20, // 92 backslash
    0x00,0x41,0x41,0x41,0x7F, // 93 ]
    0x04,0x02,0x01,0x02,0x04, // 94 ^
    0x40,0x40,0x40,0x40,0x40, // 95 _
    0x00,0x03,0x07,0x08,0x00, // 96 `
    0x20,0x54,0x54,0x78,0x40, // 97 a
    0x7F,0x28,0x44,0x44,0x38, // 98 b
    0x38,0x44,0x44,0x44,0x28, // 99 c
    0x38,0x44,0x44,0x28,0x7F, // 100 d
    0x38,0x54,0x54,0x54,0x18, // 101 e
    0x00,0x08,0x7E,0x09,0x02, // 102 f
    0x18,0xA4,0xA4,0x9C,0x78, // 103 g
    0x7F,0x08,0x04,0x04,0x78, // 104 h
    0x00,0x44,0x7D,0x40,0x00, // 105 i
    0x20,0x40,0x40,0x3D,0x00, // 106 j
    0x7F,0x10,0x28,0x44,0x00, // 107 k
    0x00,0x41,0x7F,0x40,0x00, // 108 l
    0x7C,0x04,0x78,0x04,0x78, // 109 m
    0x7C,0x08,0x04,0x04,0x78, // 110 n
    0x38,0x44,0x44,0x44,0x38, // 111 o
    0xFC,0x18,0x24,0x24,0x18, // 112 p
    0x18,0x24,0x24,0x18,0xFC, // 113 q
    0x7C,0x08,0x04,0x04,0x08, // 114 r
    0x48,0x54,0x54,0x54,0x24, // 115 s
    0x04,0x04,0x3F,0x44,0x24, // 116 t
    0x3C,0x40,0x40,0x20,0x7C, // 117 u
    0x1C,0x20,0x40,0x20,0x1C, // 118 v
    0x3C,0x40,0x30,0x40,0x3C, // 119 w
    0x44,0x28,0x10,0x28,0x44, // 120 x
    0x4C,0x90,0x90,0x90,0x7C, // 121 y
    0x44,0x64,0x54,0x4C,0x44, // 122 z
    0x00,0x08,0x36,0x41,0x00, // 123 {
    0x00,0x00,0x77,0x00,0x00, // 124 |
    0x00,0x41,0x36,0x08,0x00, // 125 }
    0x02,0x01,0x02,0x04,0x02, // 126 ~
};

void oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(I2C_PORT, oled_addr, buf, 2, false);
}

void oled_init(void) {
    i2c_switch_to_oled();  // Muda para GPIO14/15
    
    sleep_ms(100);
    
    // Sequência de inicialização
    oled_cmd(0xAE);         // Display off
    oled_cmd(0xD5); oled_cmd(0x80);  // Clock div
    oled_cmd(0xA8); oled_cmd(0x3F);  // Multiplex 64
    oled_cmd(0xD3); oled_cmd(0x00);  // Display offset
    oled_cmd(0x40);         // Start line 0
    oled_cmd(0x8D); oled_cmd(0x14);  // Charge pump ON
    oled_cmd(0x20); oled_cmd(0x00);  // Memory mode horizontal
    oled_cmd(0xA1);         // Segment remap
    oled_cmd(0xC8);         // COM scan dec
    oled_cmd(0xDA); oled_cmd(0x12);  // COM pins
    oled_cmd(0x81); oled_cmd(0xCF);  // Contrast
    oled_cmd(0xD9); oled_cmd(0xF1);  // Pre-charge
    oled_cmd(0xDB); oled_cmd(0x40);  // VCOMH
    oled_cmd(0xA4);         // Display RAM
    oled_cmd(0xA6);         // Normal display
    oled_cmd(0xAF);         // Display ON
    
    // Limpa buffer
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_update(void) {
    i2c_switch_to_oled();  // Muda para GPIO14/15
    
    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);  // Column range
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);    // Page range
    
    // Envia buffer em blocos
    for (int i = 0; i < sizeof(oled_buffer); i += 16) {
        uint8_t buf[17];
        buf[0] = 0x40;  // Data mode
        memcpy(&buf[1], &oled_buffer[i], 16);
        i2c_write_blocking(I2C_PORT, oled_addr, buf, 17, false);
    }
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    int idx = x + (y / 8) * OLED_WIDTH;
    if (on) oled_buffer[idx] |= (1 << (y % 8));
    else oled_buffer[idx] &= ~(1 << (y % 8));
}

void oled_char(int x, int y, char c) {
    if (c < 32 || c > 126) c = '?';
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

void oled_text_large(int x, int y, const char* str) {
    // Texto 2x escala
    while (*str) {
        if (*str >= 32 && *str <= 126) {
            int idx = (*str - 32) * 5;
            for (int i = 0; i < 5; i++) {
                uint8_t col = font5x7[idx + i];
                for (int j = 0; j < 7; j++) {
                    bool on = col & (1 << j);
                    oled_pixel(x + i*2, y + j*2, on);
                    oled_pixel(x + i*2 + 1, y + j*2, on);
                    oled_pixel(x + i*2, y + j*2 + 1, on);
                    oled_pixel(x + i*2 + 1, y + j*2 + 1, on);
                }
            }
        }
        x += 12;
        str++;
    }
}

// ============================================================
// Scanner I2C
// ============================================================
static uint8_t i2c_devices[16];
static int i2c_device_count = 0;

void scan_i2c(void) {
    i2c_switch_to_sensors();  // Garante que está em GPIO2/3
    
    printf("\n📡 Escaneando I2C Sensores (GPIO%d/GPIO%d)...\n", I2C_SDA_PIN, I2C_SCL_PIN);
    printf("    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    
    i2c_device_count = 0;
    for (int addr = 0; addr < 128; addr++) {
        if (addr % 16 == 0) printf("%02X: ", addr);
        
        uint8_t data;
        int ret = i2c_read_blocking(I2C_PORT, addr, &data, 1, false);
        
        if (ret >= 0) {
            printf("%02X ", addr);
            if (i2c_device_count < 16) {
                i2c_devices[i2c_device_count++] = addr;
            }
            
            if (addr == VL53L0X_ADDR) printf("[VL53] ");
            if (addr == AHT10_ADDR) printf("[AHT10] ");
            // Outros endereços comuns
            if (addr == 0x20) printf("[PCF8574] ");
            if (addr == 0x27) printf("[LCD/PCF] ");
            if (addr == 0x3C || addr == 0x3D) printf("[OLED?] ");
            if (addr == 0x40) printf("[INA219] ");
            if (addr == 0x48) printf("[ADS1115/TMP] ");
            if (addr == 0x68) printf("[DS3231/MPU] ");
            if (addr == 0x70) printf("[TCA9548] ");
            if (addr == 0x76 || addr == 0x77) printf("[BME280] ");
        } else {
            printf("-- ");
        }
        
        if (addr % 16 == 15) printf("\n");
    }
    
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║ ✅ %d dispositivo(s) I2C encontrado(s)  ║\n", i2c_device_count);
    printf("╠════════════════════════════════════════╣\n");
    for (int i = 0; i < i2c_device_count; i++) {
        printf("║   0x%02X", i2c_devices[i]);
        if (i2c_devices[i] == VL53L0X_ADDR) printf(" - VL53L0X (Distância)");
        else if (i2c_devices[i] == AHT10_ADDR) printf(" - AHT10 (Temp/Umid)");
        else if (i2c_devices[i] == 0x3C) printf(" - OLED SSD1306");
        else if (i2c_devices[i] == 0x3D) printf(" - OLED SSD1306 alt");
        else if (i2c_devices[i] == 0x70) printf(" - TCA9548 Multiplexer");
        else printf(" - Desconhecido");
        printf("\n");
    }
    printf("╚════════════════════════════════════════╝\n");
    printf("📺 OLED: Em GPIO14/15 (BitDogLab) - separado\n");
}

// Escaneia OLED em GPIO14/15 usando i2c1 (mesma porta, pinos diferentes)
// NOTA: No RP2040, GPIO14/15 só funciona com i2c1
// DESABILITADO: Causa conflito com sensores em GPIO2/3
void scan_oled_gpio14_15(void) {
    if (oled_found) return;  // Já encontrado no outro barramento
    
    printf("\n⚠️ OLED não encontrado no extensor I2C (GPIO2/3)\n");
    printf("   Se OLED está em GPIO14/15, precisa de configuração específica.\n");
    printf("   Por agora, continuando sem OLED.\n");
    
    // NÃO busca em GPIO14/15 para não atrapalhar os sensores
    // Os sensores estão em i2c1 GPIO2/3 e precisam continuar funcionando
}


static float temperatura = 0;
static float umidade = 0;
static float nivel_cm = 0;
static float nivel_litros = 0;

// ============================================================
// AHT10 - Temperatura e Umidade
// ============================================================
bool aht10_init(void) {
    i2c_switch_to_sensors();  // Garante GPIO2/3
    
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    int ret = i2c_write_blocking(I2C_PORT, AHT10_ADDR, cmd, 3, false);
    if (ret < 0) return false;
    sleep_ms(10);
    aht10_found = true;
    return true;
}

bool aht10_read(void) {
    if (!aht10_found) return false;
    
    i2c_switch_to_sensors();  // Garante GPIO2/3
    
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    i2c_write_blocking(I2C_PORT, AHT10_ADDR, cmd, 3, false);
    sleep_ms(80);
    
    uint8_t data[6];
    int ret = i2c_read_blocking(I2C_PORT, AHT10_ADDR, data, 6, false);
    if (ret < 0) return false;
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    umidade = (float)hum_raw / 1048576.0f * 100.0f;
    temperatura = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    
    return true;
}

// ============================================================
// VL53L0X/L1X v2 - Sensor de Distância Laser ToF
// Driver simplificado para versões clone/genéricas
// ============================================================
static uint8_t stop_variable = 0;

static uint8_t vl53_read_reg(uint8_t reg) {
    uint8_t val = 0;
    i2c_write_blocking(I2C_PORT, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, VL53L0X_ADDR, &val, 1, false);
    return val;
}

static void vl53_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(I2C_PORT, VL53L0X_ADDR, buf, 2, false);
}

static void vl53_write_reg16(uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    i2c_write_blocking(I2C_PORT, VL53L0X_ADDR, buf, 3, false);
}

static uint16_t vl53_read_reg16(uint8_t reg) {
    uint8_t buf[2];
    i2c_write_blocking(I2C_PORT, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, VL53L0X_ADDR, buf, 2, false);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

bool vl53_init(void) {
    i2c_switch_to_sensors();  // Garante GPIO2/3
    
    // Testa comunicação
    uint8_t id = vl53_read_reg(0xC0);
    printf("   VL53 ID (reg 0xC0): 0x%02X\n", id);
    
    // Aceita qualquer ID (clones podem ter IDs diferentes)
    if (id == 0xFF || id == 0x00) {
        printf("   ❌ Sensor não respondeu\n");
        return false;
    }
    
    // Soft reset
    vl53_write_reg(0xBF, 0x01);
    sleep_ms(50);
    
    // Aguarda boot
    int timeout = 100;
    while ((vl53_read_reg(0x00) & 0x01) == 0 && timeout-- > 0) {
        sleep_ms(10);
    }
    
    // Configuração padrão do VL53L0X
    vl53_write_reg(0x88, 0x00);  // Sem VHV calibration
    
    vl53_write_reg(0x80, 0x01);
    vl53_write_reg(0xFF, 0x01);
    vl53_write_reg(0x00, 0x00);
    stop_variable = vl53_read_reg(0x91);
    vl53_write_reg(0x00, 0x01);
    vl53_write_reg(0xFF, 0x00);
    vl53_write_reg(0x80, 0x00);
    
    // Configurações de timing
    vl53_write_reg(0x60, 0x07);  // FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT
    vl53_write_reg16(0x44, 0x0020);  // Limite de sinal
    
    // Período de medição
    vl53_write_reg(0x50, 0x09);  // Pre-range period
    vl53_write_reg(0x70, 0x0B);  // Final range period
    
    // MSRC config
    vl53_write_reg(0x46, vl53_read_reg(0x46) | 0x12);
    
    // Timing budget ~33ms (rápido)
    vl53_write_reg16(0x51, 0x00A0);
    
    printf("   ✅ VL53L0X inicializado (stop_var=0x%02X)\n", stop_variable);
    
    vl53_found = true;
    return true;
}

uint16_t vl53_read_mm(void) {
    if (!vl53_found) return 0xFFFF;
    
    i2c_switch_to_sensors();  // Garante GPIO2/3
    
    // Inicia medição single-shot
    vl53_write_reg(0x80, 0x01);
    vl53_write_reg(0xFF, 0x01);
    vl53_write_reg(0x00, 0x00);
    vl53_write_reg(0x91, stop_variable);
    vl53_write_reg(0x00, 0x01);
    vl53_write_reg(0xFF, 0x00);
    vl53_write_reg(0x80, 0x00);
    
    // Dispara medição
    vl53_write_reg(0x00, 0x01);  // SYSRANGE_START
    
    // Aguarda início
    int timeout = 50;
    while ((vl53_read_reg(0x00) & 0x01) && timeout-- > 0) {
        sleep_ms(2);
    }
    
    // Aguarda resultado
    timeout = 100;
    while ((vl53_read_reg(0x13) & 0x07) == 0 && timeout-- > 0) {
        sleep_ms(5);
    }
    
    if (timeout <= 0) {
        return 0xFFFF;
    }
    
    // Lê distância (registros 0x14 + 10 = 0x1E)
    uint16_t distance = vl53_read_reg16(0x14 + 10);
    
    // Limpa interrupção
    vl53_write_reg(0x0B, 0x01);
    
    // Valida
    if (distance == 0 || distance > 2000) {
        return 0xFFFF;
    }
    
    return distance;
}

// ============================================================
// Servo Motor
// ============================================================
static uint servo_slice;

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    servo_slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(servo_slice, 125.0f);
    pwm_set_wrap(servo_slice, 20000);
    pwm_set_enabled(servo_slice, true);
}

void servo_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    int pulse = 500 + (angle * 2000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    // PARA o servo setando PWM para 0
    pwm_set_gpio_level(SERVO_PIN, 0);
    printf("   ⏹️ Servo PARADO\n");
}

// ============================================================
// Main
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    // Força flush do stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  🐟 HydroSense - Diagnóstico v2.0 (Pinos Corrigidos)      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    fflush(stdout);
    
    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    // I2C - PINOS CORRETOS: GPIO2 (SDA) e GPIO3 (SCL)
    printf("🔧 Configurando I2C...\n");
    printf("   SDA = GPIO%d (Pino 4)\n", I2C_SDA_PIN);
    printf("   SCL = GPIO%d (Pino 5)\n", I2C_SCL_PIN);
    
    i2c_init(I2C_PORT, I2C_SPEED);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    sleep_ms(100);
    
    // Escaneia I2C (sensores no extensor GPIO2/3)
    scan_i2c();
    
    // Se OLED não encontrado, tenta GPIO14/15 (como na main original)
    scan_oled_gpio14_15();
    
    // Inicializa sensores
    printf("\n🔧 Inicializando sensores...\n");
    
    printf("   Testando AHT10... ");
    if (aht10_init()) {
        printf("✅ OK\n");
    } else {
        printf("❌ Falhou\n");
    }
    
    printf("   Testando VL53L0X... ");
    if (vl53_init()) {
        printf("✅ OK\n");
    } else {
        printf("❌ Falhou\n");
    }
    
    // OLED
    printf("   Inicializando OLED... ");
    if (oled_found) {
        oled_init();
        printf("✅ OK (addr 0x%02X)\n", oled_addr);
        
        // Desenha tela inicial
        oled_clear();
        oled_text_large(10, 0, "HydroSense");
        oled_text(0, 20, "Sistema de Aquicultura");
        oled_text(0, 30, "Inteligente v3.0");
        oled_text(0, 50, "Iniciando...");
        oled_update();
        sleep_ms(2000);
    } else {
        printf("❌ Não encontrado\n");
    }
    
    // Servo
    printf("   Configurando Servo... ");
    servo_init();
    printf("✅ GPIO%d\n", SERVO_PIN);
    
    // Teste rápido do servo
    printf("\n⚙️ Testando Servo...\n");
    servo_angle(0);
    sleep_ms(500);
    servo_angle(90);
    sleep_ms(500);
    servo_angle(0);
    sleep_ms(300);
    servo_stop();  // PARA O SERVO!
    printf("   ✅ Servo testado e PARADO\n");
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  📊 MODO DE MONITORAMENTO CONTÍNUO                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Loop principal
    char buf[32];
    int ciclo = 0;
    
    while (1) {
        ciclo++;
        
        // Lê sensores
        bool aht_ok = aht10_read();
        uint16_t dist_mm = vl53_read_mm();
        
        // Calcula nível (tanque 30cm altura, sensor no topo)
        if (dist_mm != 0xFFFF && dist_mm < 3000) {
            nivel_cm = 30.0f - (dist_mm / 10.0f);
            if (nivel_cm < 0) nivel_cm = 0;
            if (nivel_cm > 30) nivel_cm = 30;
            nivel_litros = (nivel_cm / 30.0f) * 20.0f;
        }
        
        // Imprime no terminal
        printf("━━━ Ciclo %d ━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n", ciclo);
        
        if (aht_ok) {
            printf("🌡️ Temperatura: %.1f °C\n", temperatura);
            printf("💧 Umidade: %.1f %%\n", umidade);
        } else {
            printf("🌡️ AHT10: Não conectado\n");
        }
        
        if (vl53_found && dist_mm != 0xFFFF) {
            printf("📏 Distância: %d mm\n", dist_mm);
            printf("🌊 Nível: %.1f cm (%.1f L, %.0f%%)\n", 
                   nivel_cm, nivel_litros, (nivel_litros/20.0f)*100);
        } else {
            printf("📏 VL53L0X: Não conectado\n");
        }
        printf("\n");
        
        // Atualiza OLED
        if (oled_found) {
            oled_clear();
            
            // Título
            oled_text_large(5, 0, "HydroSense");
            
            // Linha separadora
            for (int x = 0; x < 128; x++) oled_pixel(x, 16, true);
            
            // Dados
            if (aht_ok) {
                snprintf(buf, sizeof(buf), "Temp: %.1fC", temperatura);
                oled_text(0, 20, buf);
                snprintf(buf, sizeof(buf), "Umid: %.1f%%", umidade);
                oled_text(0, 30, buf);
            } else {
                oled_text(0, 20, "Temp: ---");
                oled_text(0, 30, "Umid: ---");
            }
            
            if (vl53_found && dist_mm != 0xFFFF) {
                snprintf(buf, sizeof(buf), "Nivel: %.1fL", nivel_litros);
                oled_text(0, 42, buf);
                
                // Barra de progresso do nível
                int bar_width = (int)(nivel_litros / 20.0f * 60);
                for (int x = 64; x < 64 + bar_width && x < 124; x++) {
                    for (int y = 42; y < 50; y++) {
                        oled_pixel(x, y, true);
                    }
                }
                // Borda da barra
                for (int x = 64; x < 125; x++) {
                    oled_pixel(x, 42, true);
                    oled_pixel(x, 50, true);
                }
                for (int y = 42; y < 51; y++) {
                    oled_pixel(64, y, true);
                    oled_pixel(124, y, true);
                }
            } else {
                oled_text(0, 42, "Nivel: ---");
            }
            
            // Status
            snprintf(buf, sizeof(buf), "Ciclo: %d", ciclo);
            oled_text(0, 56, buf);
            
            oled_update();
        }
        
        // Pisca LED
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
        gpio_put(LED_PIN, 1);
        
        sleep_ms(1900);
    }
    
    return 0;
}
