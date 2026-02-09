/**
 * HydroSense v10 FINAL - BitDogLab Completo
 * 
 * HARDWARE BitDogLab + Pico W:
 * - Sensores I2C: GPIO2(SDA)/GPIO3(SCL) via extensor (i2c1 switching)
 * - OLED SSD1306: GPIO14(SDA)/GPIO15(SCL) direto na BitDogLab (i2c1 switching)
 * - Servo SG90:   GPIO16 (PWM)
 * - LED RGB:      GPIO11(B), GPIO12(R), GPIO13(G)
 * - Buzzer:       GPIO21
 * - Botoes:       GPIO5(A), GPIO6(B)
 * - Reles REAIS:  GPIO17(LN1), GPIO18(LN2), GPIO19(LN3) via IDC
 * - WiFi:         CYW43 integrado (lwip_poll)
 * 
 * APIs HTTP:
 *   GET  /sensors  - JSON com todos os dados
 *   GET  /status   - Status do sistema
 *   POST /relay    - Controlar reles
 *   POST /feed     - Acionar alimentador (servo)
 *   GET  /         - Pagina HTML responsiva
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ============================================================
// CONFIGURACAO WiFi
// ============================================================
#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"

// ============================================================
// PINOS - BitDogLab
// ============================================================
// I2C Sensores (via extensor)
#define SENSOR_SDA      2
#define SENSOR_SCL      3

// I2C OLED (direto na BitDogLab)
#define OLED_SDA        14
#define OLED_SCL        15

// Servo
#define SERVO_PIN       16

// LED RGB
#define LED_R_PIN       12
#define LED_G_PIN       13
#define LED_B_PIN       11

// Buzzer
#define BUZZER_PIN      21

// Botoes
#define BTN_A_PIN       5
#define BTN_B_PIN       6

// Reles (via conector IDC)
#define RELAY_LN1_PIN   17   // Aerador/Ventilador
#define RELAY_LN2_PIN   18   // Aquecedor
#define RELAY_LN3_PIN   19   // Alimentador/Bomba

// ============================================================
// ENDERECOS I2C
// ============================================================
#define AHT10_ADDR      0x38
#define VL53L0X_ADDR    0x29
#define VL53L0X_NEW_ADDR 0x30   // Novo endereco para liberar 0x29 pro TCS34725
#define TCS34725_ADDR   0x29
#define TCS34725_CMD    0x80   // Command bit
#define OLED_ADDR       0x3C

// ============================================================
// CONFIGURACAO DO TANQUE
// ============================================================
#define TANK_HEIGHT_MM   200.0f
#define TANK_CAPACITY_L  20.0f
#define TEMP_THRESHOLD   29.0f

// ============================================================
// VARIAVEIS GLOBAIS
// ============================================================
static int current_i2c_mode = 0;  // 0=nenhum, 1=sensores, 2=oled

// Dados dos sensores
static volatile float g_temp = 25.0f;
static volatile float g_hum = 60.0f;
static volatile uint16_t g_dist = 200;   // Default = TANK_HEIGHT (tanque vazio)
static volatile float g_nivel = 0.0f;
static volatile float g_volume = 0.0f;

// Cor da agua (TCS34725)
static volatile uint16_t g_cor_r = 0, g_cor_g = 0, g_cor_b = 0, g_cor_c = 0;
static char g_cor_nome[16] = "N/A";

// Status
static volatile bool g_aht_ok = false;
static volatile bool g_vl53_ok = false;
static volatile bool g_tcs_ok = false;
static volatile bool g_oled_ok = false;
static volatile bool g_wifi = false;
static volatile uint32_t g_count = 0;
static char g_ip[20] = "0.0.0.0";

// Reles
static volatile bool g_relay_ln1 = false;
static volatile bool g_relay_ln2 = false;
static volatile bool g_relay_ln3 = false;

// ============================================================
// I2C SWITCHING - Alterna entre sensores e OLED
// ============================================================

void i2c_switch_to_sensors(void) {
    if (current_i2c_mode == 1) return;
    
    // Desconecta pinos do OLED
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    
    // Reinicializa I2C para sensores
    i2c_deinit(i2c1);
    i2c_init(i2c1, 100000);  // 100kHz para sensores
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_us(100);
    
    current_i2c_mode = 1;
}

void i2c_switch_to_oled(void) {
    if (current_i2c_mode == 2) return;
    
    // Desconecta pinos dos sensores
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    
    // Reinicializa I2C para OLED
    i2c_deinit(i2c1);
    i2c_init(i2c1, 400000);  // 400kHz para OLED
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_us(100);
    
    current_i2c_mode = 2;
}

// ============================================================
// AHT10 - Sensor de Temperatura e Umidade
// ============================================================

bool aht10_init(void) {
    i2c_switch_to_sensors();
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    int ret = i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false);
    sleep_ms(10);
    return (ret >= 0);
}

bool aht10_read(float *temp, float *hum) {
    i2c_switch_to_sensors();
    
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) return false;
    sleep_ms(80);
    
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) return false;
    if (data[0] & 0x80) return false;  // Ocupado
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    *hum = (float)hum_raw / 1048576.0f * 100.0f;
    *temp = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    return true;
}

// ============================================================
// VL53L0X - Sensor de Distancia (com mudanca de endereco)
// ============================================================

static uint8_t vl53_addr = VL53L0X_ADDR;  // Endereco atual (muda para NEW_ADDR no init)
static uint8_t vl53_stop_variable = 0;

static bool vl53_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_write_blocking(i2c1, vl53_addr, buf, 2, false) == 2;
}

static bool vl53_write_reg16(uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
    return i2c_write_blocking(i2c1, vl53_addr, buf, 3, false) == 3;
}

static uint8_t vl53_read_reg(uint8_t reg) {
    uint8_t val = 0;
    i2c_write_blocking(i2c1, vl53_addr, &reg, 1, true);
    i2c_read_blocking(i2c1, vl53_addr, &val, 1, false);
    return val;
}

bool vl53_init(void) {
    i2c_switch_to_sensors();
    sleep_ms(100);
    
    vl53_addr = VL53L0X_ADDR;
    
    // Verifica presenca no endereco padrao
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, vl53_addr, &dummy, 1, false) < 0) {
        // Talvez ja esteja no novo endereco (boot anterior)
        vl53_addr = VL53L0X_NEW_ADDR;
        if (i2c_read_blocking(i2c1, vl53_addr, &dummy, 1, false) < 0) {
            printf("   VL53L0X: nao encontrado\n");
            return false;
        }
        printf("   VL53L0X ja em 0x%02X\n", vl53_addr);
    }
    
    // Verifica ID (deve ser 0xEE)
    uint8_t id = vl53_read_reg(0xC0);
    printf("   VL53L0X ID: 0x%02X\n", id);
    if (id != 0xEE) return false;
    
    // Data init (sequencia simplificada baseada na lib Pololu)
    vl53_write_reg(0x88, 0x00);  // Modo 2V8
    
    // Configuracao de GPIO/interrupcao
    vl53_write_reg(0x80, 0x01);
    vl53_write_reg(0xFF, 0x01);
    vl53_write_reg(0x00, 0x00);
    vl53_stop_variable = vl53_read_reg(0x91);
    vl53_write_reg(0x00, 0x01);
    vl53_write_reg(0xFF, 0x00);
    vl53_write_reg(0x80, 0x00);
    
    // Configura interrupcao ativa baixa
    uint8_t cfg = vl53_read_reg(0x0A);
    vl53_write_reg(0x0A, cfg & ~0x10);
    vl53_write_reg(0x0B, 0x01);  // Limpa interrupcao
    
    // Sequence config e rate limit
    vl53_write_reg(0x01, 0xE8);
    vl53_write_reg16(0x44, 0x0020);  // Signal rate limit 0.25 MCPS
    
    // Muda endereco I2C para liberar 0x29 para TCS34725
    if (vl53_addr == VL53L0X_ADDR) {
        vl53_write_reg(0x8A, VL53L0X_NEW_ADDR);
        vl53_addr = VL53L0X_NEW_ADDR;
        sleep_ms(10);
        printf("   VL53L0X addr -> 0x%02X\n", vl53_addr);
    }
    
    return true;
}

uint16_t vl53_read(void) {
    i2c_switch_to_sensors();
    
    // Sequencia de inicio de medicao single-shot (com stop_variable)
    vl53_write_reg(0x80, 0x01);
    vl53_write_reg(0xFF, 0x01);
    vl53_write_reg(0x00, 0x00);
    vl53_write_reg(0x91, vl53_stop_variable);
    vl53_write_reg(0x00, 0x01);
    vl53_write_reg(0xFF, 0x00);
    vl53_write_reg(0x80, 0x00);
    vl53_write_reg(0x00, 0x01);  // SYSRANGE_START = single shot
    
    // Aguarda conclusao da medicao (poll registro 0x13)
    for (int i = 0; i < 200; i++) {
        uint8_t status = vl53_read_reg(0x13);
        if (status & 0x07) break;
        sleep_ms(1);
        if (i == 199) return 0xFFFF;  // Timeout
    }
    
    // Le distancia (RESULT_RANGE_STATUS + 10 = 0x1E)
    uint8_t reg = 0x1E;
    uint8_t buf[2] = {0, 0};
    i2c_write_blocking(i2c1, vl53_addr, &reg, 1, true);
    i2c_read_blocking(i2c1, vl53_addr, buf, 2, false);
    
    // Limpa interrupcao
    vl53_write_reg(0x0B, 0x01);
    
    uint16_t dist = (buf[0] << 8) | buf[1];
    if (dist == 0 || dist > 2000 || dist == 8190) return 0xFFFF;
    return dist;
}

// ============================================================
// TCS34725 - Sensor de Cor RGB
// ============================================================

static bool tcs_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {(uint8_t)(TCS34725_CMD | reg), val};
    return i2c_write_blocking(i2c1, TCS34725_ADDR, buf, 2, false) == 2;
}

static uint8_t tcs_read_reg(uint8_t reg) {
    uint8_t cmd = TCS34725_CMD | reg;
    uint8_t val = 0;
    i2c_write_blocking(i2c1, TCS34725_ADDR, &cmd, 1, true);
    i2c_read_blocking(i2c1, TCS34725_ADDR, &val, 1, false);
    return val;
}

static uint16_t tcs_read_reg16(uint8_t reg) {
    uint8_t cmd = TCS34725_CMD | 0x20 | reg;  // Auto-increment
    uint8_t buf[2] = {0, 0};
    i2c_write_blocking(i2c1, TCS34725_ADDR, &cmd, 1, true);
    i2c_read_blocking(i2c1, TCS34725_ADDR, buf, 2, false);
    return (uint16_t)(buf[1] << 8) | buf[0];  // Little-endian
}

bool tcs_init(void) {
    i2c_switch_to_sensors();
    sleep_ms(50);
    
    // Verifica presenca
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, TCS34725_ADDR, &dummy, 1, false) < 0) {
        printf("   TCS34725: nao encontrado\n");
        return false;
    }
    
    // Verifica ID (0x44 = TCS34725, 0x4D = TCS34727)
    uint8_t id = tcs_read_reg(0x12);
    printf("   TCS34725 ID: 0x%02X\n", id);
    if (id != 0x44 && id != 0x4D) return false;
    
    // ATIME = 0xD5 -> ~101ms integracao (10-bit)
    tcs_write_reg(0x01, 0xD5);
    // Ganho 4x (0x01)
    tcs_write_reg(0x0F, 0x01);
    // Power ON
    tcs_write_reg(0x00, 0x01);
    sleep_ms(3);
    // Power ON + ADC Enable
    tcs_write_reg(0x00, 0x03);
    sleep_ms(154);  // Aguarda primeiro ciclo de integracao
    
    printf("   TCS34725: configurado OK\n");
    return true;
}

void tcs_read_color(void) {
    i2c_switch_to_sensors();
    
    // Verifica se dados estao prontos (AVALID bit)
    uint8_t status = tcs_read_reg(0x13);
    if (!(status & 0x01)) return;
    
    // Le valores RGBC
    g_cor_c = tcs_read_reg16(0x14);  // Clear
    g_cor_r = tcs_read_reg16(0x16);  // Red
    g_cor_g = tcs_read_reg16(0x18);  // Green
    g_cor_b = tcs_read_reg16(0x1A);  // Blue
    
    // Classifica cor para qualidade da agua
    if (g_cor_c < 200) {
        snprintf(g_cor_nome, sizeof(g_cor_nome), "Escuro");
    } else if (g_cor_c > 4000) {
        snprintf(g_cor_nome, sizeof(g_cor_nome), "Cristalino");
    } else {
        float r_ratio = (float)g_cor_r / (float)g_cor_c;
        float g_ratio = (float)g_cor_g / (float)g_cor_c;
        float b_ratio = (float)g_cor_b / (float)g_cor_c;
        
        if (g_ratio > 0.35f && g_ratio > r_ratio && g_ratio > b_ratio) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Verde");
        } else if (r_ratio > 0.35f && r_ratio > g_ratio * 1.3f) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Vermelho");
        } else if (b_ratio > 0.35f && b_ratio > r_ratio && b_ratio > g_ratio) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Azul");
        } else if (r_ratio > 0.30f && g_ratio > 0.30f && b_ratio < 0.25f) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Amarelado");
        } else if (g_cor_c > 2000) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Cristalino");
        } else {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Turvo");
        }
    }
}

// ============================================================
// OLED SSD1306 128x64
// ============================================================

static uint8_t oled_buffer[1024];

// Font 5x7 (ASCII 32-90)
static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, // Space
    0x00,0x00,0x5F,0x00,0x00, // !
    0x00,0x07,0x00,0x07,0x00, // "
    0x14,0x7F,0x14,0x7F,0x14, // #
    0x24,0x2A,0x7F,0x2A,0x12, // $
    0x23,0x13,0x08,0x64,0x62, // %
    0x36,0x49,0x55,0x22,0x50, // &
    0x00,0x05,0x03,0x00,0x00, // '
    0x00,0x1C,0x22,0x41,0x00, // (
    0x00,0x41,0x22,0x1C,0x00, // )
    0x14,0x08,0x3E,0x08,0x14, // *
    0x08,0x08,0x3E,0x08,0x08, // +
    0x00,0x50,0x30,0x00,0x00, // ,
    0x08,0x08,0x08,0x08,0x08, // -
    0x00,0x60,0x60,0x00,0x00, // .
    0x20,0x10,0x08,0x04,0x02, // /
    0x3E,0x51,0x49,0x45,0x3E, // 0
    0x00,0x42,0x7F,0x40,0x00, // 1
    0x42,0x61,0x51,0x49,0x46, // 2
    0x21,0x41,0x45,0x4B,0x31, // 3
    0x18,0x14,0x12,0x7F,0x10, // 4
    0x27,0x45,0x45,0x45,0x39, // 5
    0x3C,0x4A,0x49,0x49,0x30, // 6
    0x01,0x71,0x09,0x05,0x03, // 7
    0x36,0x49,0x49,0x49,0x36, // 8
    0x06,0x49,0x49,0x29,0x1E, // 9
    0x00,0x36,0x36,0x00,0x00, // :
    0x00,0x56,0x36,0x00,0x00, // ;
    0x08,0x14,0x22,0x41,0x00, // <
    0x14,0x14,0x14,0x14,0x14, // =
    0x00,0x41,0x22,0x14,0x08, // >
    0x02,0x01,0x51,0x09,0x06, // ?
    0x32,0x49,0x79,0x41,0x3E, // @
    0x7E,0x11,0x11,0x11,0x7E, // A
    0x7F,0x49,0x49,0x49,0x36, // B
    0x3E,0x41,0x41,0x41,0x22, // C
    0x7F,0x41,0x41,0x22,0x1C, // D
    0x7F,0x49,0x49,0x49,0x41, // E
    0x7F,0x09,0x09,0x09,0x01, // F
    0x3E,0x41,0x49,0x49,0x7A, // G
    0x7F,0x08,0x08,0x08,0x7F, // H
    0x00,0x41,0x7F,0x41,0x00, // I
    0x20,0x40,0x41,0x3F,0x01, // J
    0x7F,0x08,0x14,0x22,0x41, // K
    0x7F,0x40,0x40,0x40,0x40, // L
    0x7F,0x02,0x0C,0x02,0x7F, // M
    0x7F,0x04,0x08,0x10,0x7F, // N
    0x3E,0x41,0x41,0x41,0x3E, // O
    0x7F,0x09,0x09,0x09,0x06, // P
    0x3E,0x41,0x51,0x21,0x5E, // Q
    0x7F,0x09,0x19,0x29,0x46, // R
    0x46,0x49,0x49,0x49,0x31, // S
    0x01,0x01,0x7F,0x01,0x01, // T
    0x3F,0x40,0x40,0x40,0x3F, // U
    0x1F,0x20,0x40,0x20,0x1F, // V
    0x3F,0x40,0x38,0x40,0x3F, // W
    0x63,0x14,0x08,0x14,0x63, // X
    0x07,0x08,0x70,0x08,0x07, // Y
    0x61,0x51,0x49,0x45,0x43, // Z
};

bool oled_init(void) {
    i2c_switch_to_oled();
    
    // Verifica presenca do OLED
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, OLED_ADDR, &dummy, 1, false) < 0) return false;
    
    uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide
        0xA8, 0x3F, // Multiplex 64
        0xD3, 0x00, // Display offset
        0x40,       // Start line
        0x8D, 0x14, // Charge pump ON
        0x20, 0x00, // Horizontal memory mode
        0xA1,       // Segment remap
        0xC8,       // COM scan descending
        0xDA, 0x12, // COM pins
        0x81, 0xCF, // Contrast
        0xD9, 0xF1, // Pre-charge
        0xDB, 0x40, // VCOMH deselect
        0xA4,       // Display from RAM
        0xA6,       // Normal display
        0xAF        // Display ON
    };
    
    for (int i = 0; i < (int)sizeof(init_cmds); i++) {
        uint8_t buf[2] = {0x00, init_cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    
    memset(oled_buffer, 0, sizeof(oled_buffer));
    return true;
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_update(void) {
    i2c_switch_to_oled();
    
    uint8_t cmds[] = {0x21, 0, 127, 0x22, 0, 7};
    for (int i = 0; i < 6; i++) {
        uint8_t buf[2] = {0x00, cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    
    for (int page = 0; page < 8; page++) {
        uint8_t buf[129];
        buf[0] = 0x40;
        memcpy(&buf[1], &oled_buffer[page * 128], 128);
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 129, false);
    }
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int idx = (y / 8) * 128 + x;
    if (on) oled_buffer[idx] |= (1 << (y % 8));
    else    oled_buffer[idx] &= ~(1 << (y % 8));
}

void oled_char(int x, int y, char c) {
    if (c < 32 || c > 90) c = 32;
    int idx = (c - 32) * 5;
    for (int i = 0; i < 5; i++) {
        uint8_t col = font5x7[idx + i];
        for (int j = 0; j < 7; j++) {
            oled_pixel(x + i, y + j, col & (1 << j));
        }
    }
}

void oled_print(int x, int y, const char *str) {
    while (*str) {
        oled_char(x, y, *str);
        x += 6;
        str++;
    }
}

void oled_print_large(int x, int y, const char *str) {
    while (*str) {
        if (*str >= 32 && *str <= 90) {
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

void oled_hline(int y) {
    for (int x = 0; x < 128; x++) oled_pixel(x, y, true);
}

// ============================================================
// SERVO SG90
// ============================================================

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 125.0f);  // 1MHz
    pwm_set_wrap(slice, 20000);      // 50Hz (20ms)
    pwm_set_enabled(slice, true);
}

void servo_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    uint16_t pulse = 500 + (angle * 2000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

void servo_feed(void) {
    printf("[SERVO] Alimentando...\n");
    servo_angle(0);  sleep_ms(500);
    servo_angle(180); sleep_ms(1000);
    servo_angle(0);  sleep_ms(500);
    servo_stop();
    printf("[SERVO] Concluido\n");
}

// ============================================================
// LED RGB
// ============================================================

void led_init(void) {
    gpio_init(LED_R_PIN); gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN); gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN); gpio_set_dir(LED_B_PIN, GPIO_OUT);
    gpio_put(LED_R_PIN, 0); gpio_put(LED_G_PIN, 0); gpio_put(LED_B_PIN, 0);
}

void led_set(bool r, bool g, bool b) {
    gpio_put(LED_R_PIN, r); gpio_put(LED_G_PIN, g); gpio_put(LED_B_PIN, b);
}

// ============================================================
// BUZZER
// ============================================================

void buzzer_init(void) {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);
}

void buzzer_beep(int ms) {
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(ms);
    gpio_put(BUZZER_PIN, 0);
}

// ============================================================
// RELES (GPIO REAIS via IDC)
// ============================================================

void relay_init(void) {
    gpio_init(RELAY_LN1_PIN); gpio_set_dir(RELAY_LN1_PIN, GPIO_OUT); gpio_put(RELAY_LN1_PIN, 0);
    gpio_init(RELAY_LN2_PIN); gpio_set_dir(RELAY_LN2_PIN, GPIO_OUT); gpio_put(RELAY_LN2_PIN, 0);
    gpio_init(RELAY_LN3_PIN); gpio_set_dir(RELAY_LN3_PIN, GPIO_OUT); gpio_put(RELAY_LN3_PIN, 0);
}

void relay_set(int relay, bool state) {
    switch (relay) {
        case 1: g_relay_ln1 = state; gpio_put(RELAY_LN1_PIN, state); break;
        case 2: g_relay_ln2 = state; gpio_put(RELAY_LN2_PIN, state); break;
        case 3: g_relay_ln3 = state; gpio_put(RELAY_LN3_PIN, state); break;
    }
    printf("[RELAY] LN%d=%s (GPIO%d)\n", relay,
           state ? "ON" : "OFF",
           relay == 1 ? RELAY_LN1_PIN : relay == 2 ? RELAY_LN2_PIN : RELAY_LN3_PIN);
}

// ============================================================
// DISPLAY - Telas
// ============================================================

void display_boot(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print_large(10, 5, "HYDRO");
    oled_print_large(10, 25, "SENSE");
    oled_print(15, 50, "INICIANDO V10...");
    oled_update();
}

void display_wifi_connecting(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print(0, 0, "CONECTANDO WIFI");
    oled_hline(10);
    oled_print(0, 16, WIFI_SSID);
    oled_print(0, 30, "AGUARDE...");
    oled_update();
}

void display_wifi_ok(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print(0, 0, "WIFI CONECTADO!");
    oled_hline(10);
    oled_print(0, 20, "ACESSE:");
    oled_print(0, 35, g_ip);
    oled_update();
    sleep_ms(3000);
}

void display_wifi_fail(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print(0, 0, "WIFI ERRO!");
    oled_hline(10);
    oled_print(0, 20, "MODO OFFLINE");
    oled_update();
    sleep_ms(2000);
}

void display_main(void) {
    if (!g_oled_ok) return;
    oled_clear();
    
    char buf[24];
    
    // Titulo
    oled_print(20, 0, "HYDROSENSE");
    oled_hline(10);
    
    // Temperatura e Umidade
    snprintf(buf, sizeof(buf), "T:%.1fC", g_temp);
    oled_print(0, 14, buf);
    snprintf(buf, sizeof(buf), "H:%.0f%%", g_hum);
    oled_print(75, 14, buf);
    
    // Distancia e Nivel
    snprintf(buf, sizeof(buf), "D:%dmm", g_dist);
    oled_print(0, 26, buf);
    snprintf(buf, sizeof(buf), "N:%.0f%%", g_nivel);
    oled_print(75, 26, buf);
    
    // Volume e Reles
    snprintf(buf, sizeof(buf), "VOL:%.1fL", g_volume);
    oled_print(0, 38, buf);
    snprintf(buf, sizeof(buf), "R:%c%c%c",
        g_relay_ln1 ? '1' : '-',
        g_relay_ln2 ? '2' : '-',
        g_relay_ln3 ? '3' : '-');
    oled_print(80, 38, buf);
    
    // IP e contador
    if (g_wifi) {
        oled_print(0, 52, g_ip);
    } else {
        oled_print(0, 52, "OFFLINE");
    }
    snprintf(buf, sizeof(buf), "#%d", g_count);
    oled_print(90, 52, buf);
    
    oled_update();
}

// ============================================================
// SERVIDOR HTTP
// ============================================================

// Pagina HTML responsiva
static const char *HTML_PAGE =
"<!DOCTYPE html>"
"<html lang='pt-BR'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>HydroSense IoT</title>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:'Segoe UI',sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px}"
".c{max-width:600px;margin:0 auto}"
".h{text-align:center;color:#fff;margin-bottom:20px}"
".h h1{font-size:2em;text-shadow:2px 2px 4px rgba(0,0,0,0.3)}"
".cd{background:rgba(255,255,255,0.95);border-radius:20px;padding:20px;margin-bottom:15px;box-shadow:0 10px 40px rgba(0,0,0,0.2)}"
".cd h2{color:#667eea;font-size:1em;margin-bottom:15px;text-transform:uppercase;letter-spacing:1px}"
".g{display:grid;grid-template-columns:repeat(2,1fr);gap:15px}"
".s{background:linear-gradient(135deg,#f5f7fa,#c3cfe2);border-radius:15px;padding:15px;text-align:center}"
".s .v{font-size:2.5em;font-weight:bold;color:#333}"
".s .u{font-size:0.8em;color:#666}"
".s .l{font-size:0.75em;color:#888;margin-top:5px}"
".t{border-left:4px solid #ff6b6b}.hu{border-left:4px solid #4ecdc4}"
".d{border-left:4px solid #45b7d1}.n{border-left:4px solid #96ceb4}"
".vo{border-left:4px solid #ffeaa7}"
".st{display:flex;justify-content:space-between;align-items:center;padding:10px;background:#f8f9fa;border-radius:10px;margin-top:10px}"
".dot{width:12px;height:12px;border-radius:50%;margin-right:8px;display:inline-block}"
".on{background:#2ecc71}.off{background:#e74c3c}"
".btn{width:100%;padding:12px;border:none;border-radius:10px;font-size:1em;cursor:pointer;margin-top:8px;color:#fff}"
".b1{background:linear-gradient(135deg,#667eea,#764ba2)}"
".b2{background:#e74c3c}"
".b3{background:#2ecc71}"
".rl{display:flex;gap:10px;margin-top:10px}"
".rb{flex:1;padding:10px;border:2px solid #ddd;border-radius:10px;text-align:center;cursor:pointer;transition:0.3s}"
".rb.on{border-color:#2ecc71;background:#d4efdf}"
"</style>"
"</head>"
"<body>"
"<div class='c'>"
"<div class='h'><h1>&#x1F41F; HydroSense v10</h1><p>Sistema IoT</p></div>"
"<div class='cd'>"
"<h2>Sensores</h2>"
"<div class='g'>"
"<div class='s t'><div class='v' id='t'>--</div><div class='u'>C</div><div class='l'>Temperatura</div></div>"
"<div class='s hu'><div class='v' id='h'>--</div><div class='u'>%</div><div class='l'>Umidade</div></div>"
"<div class='s d'><div class='v' id='d'>--</div><div class='u'>mm</div><div class='l'>Distancia</div></div>"
"<div class='s n'><div class='v' id='n'>--</div><div class='u'>%</div><div class='l'>Nivel</div></div>"
"</div>"
"<div class='s vo' style='margin-top:15px'><div class='v' id='vl'>--</div><div class='u'>Litros</div><div class='l'>Volume</div></div>"
"<div class='s co' style='margin-top:10px;border-left:4px solid #9b59b6'><div class='v' id='co'>--</div><div class='u'>&#x1F3A8;</div><div class='l'>Cor da Agua</div></div>"
"</div>"
"<div class='cd'>"
"<h2>Reles</h2>"
"<div class='rl'>"
"<div class='rb' id='r1' onclick='tr(1)'>LN1<br>Aerador</div>"
"<div class='rb' id='r2' onclick='tr(2)'>LN2<br>Aquecedor</div>"
"<div class='rb' id='r3' onclick='tr(3)'>LN3<br>Bomba</div>"
"</div>"
"</div>"
"<div class='cd'>"
"<h2>Controles</h2>"
"<button class='btn b1' onclick='fd()'>Alimentar</button>"
"<button class='btn b2' onclick='location.reload()'>Atualizar</button>"
"</div>"
"<div class='cd'>"
"<h2>Status</h2>"
"<div class='st'><span><span class='dot' id='wd'></span>WiFi</span><span id='ws'>--</span></div>"
"<div class='st'><span><span class='dot' id='ad'></span>AHT10</span><span id='as'>--</span></div>"
"<div class='st'><span><span class='dot' id='vd'></span>VL53L0X</span><span id='vs'>--</span></div>"
"<div class='st'><span><span class='dot' id='td'></span>TCS34725</span><span id='ts'>--</span></div>"
"<div class='st'><span>Leituras</span><span id='ct'>0</span></div>"
"</div>"
"</div>"
"<script>"
"function u(){fetch('/sensors').then(r=>r.json()).then(j=>{"
"document.getElementById('t').textContent=j.temperatura.toFixed(1);"
"document.getElementById('h').textContent=j.umidade.toFixed(0);"
"document.getElementById('d').textContent=j.distancia;"
"document.getElementById('n').textContent=j.nivel.toFixed(0);"
"document.getElementById('vl').textContent=j.volume.toFixed(1);"
"document.getElementById('ct').textContent=j.contadorLeituras;"
"document.getElementById('wd').className='dot '+(j.wifiStatus?'on':'off');"
"document.getElementById('ws').textContent=j.wifiStatus?'Conectado':'Offline';"
"document.getElementById('ad').className='dot '+(j.sensores.aht10?'on':'off');"
"document.getElementById('as').textContent=j.sensores.aht10?'OK':'Erro';"
"document.getElementById('vd').className='dot '+(j.sensores.vl53l0x?'on':'off');"
"document.getElementById('vs').textContent=j.sensores.vl53l0x?'OK':'Erro';"
"document.getElementById('co').textContent=j.corAgua||'--';"
"if(j.sensores.tcs34725!==undefined){document.getElementById('td').className='dot '+(j.sensores.tcs34725?'on':'off');document.getElementById('ts').textContent=j.sensores.tcs34725?'OK':'N/A';}"
"var r=j.relays;"
"document.getElementById('r1').className='rb'+(r.LN1?' on':'');"
"document.getElementById('r2').className='rb'+(r.LN2?' on':'');"
"document.getElementById('r3').className='rb'+(r.LN3?' on':'');"
"}).catch(e=>console.log(e))}"
"function tr(n){fetch('/relay',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({relay:n,toggle:true})}).then(()=>u())}"
"function fd(){fetch('/feed',{method:'POST'}).then(()=>alert('Alimentacao ativada!'))}"
"setInterval(u,2000);u();"
"</script>"
"</body></html>";

// Buffers HTTP (estaticos como v7)
static char http_resp[8192];
static char json_buf[768];
static char http_header[256];

// Tracking de conexao ativa
typedef struct {
    int total_to_send;
    int total_acked;
} http_conn_t;

static http_conn_t http_conn;

// Build JSON de sensores
static void build_sensor_json(void) {
    snprintf(json_buf, sizeof(json_buf),
        "{\"temperatura\":%.2f,\"umidade\":%.2f,\"distancia\":%d,"
        "\"nivel\":%.2f,\"volume\":%.2f,"
        "\"corAgua\":\"%s\",\"corR\":%d,\"corG\":%d,\"corB\":%d,"
        "\"wifiStatus\":true,\"contadorLeituras\":%d,\"deviceIp\":\"%s\","
        "\"relays\":{\"LN1\":%s,\"LN2\":%s,\"LN3\":%s},"
        "\"sensores\":{\"aht10\":%s,\"vl53l0x\":%s,\"tcs34725\":%s}}",
        g_temp, g_hum, g_dist, g_nivel, g_volume,
        g_cor_nome, g_cor_r, g_cor_g, g_cor_b,
        g_count, g_ip,
        g_relay_ln1?"true":"false", g_relay_ln2?"true":"false", g_relay_ln3?"true":"false",
        g_aht_ok?"true":"false", g_vl53_ok?"true":"false", g_tcs_ok?"true":"false");
}

// Callback chamado quando dados sao confirmados enviados
static err_t http_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    http_conn_t *conn = (http_conn_t *)arg;
    if (conn) {
        conn->total_acked += len;
        if (conn->total_acked >= conn->total_to_send) {
            tcp_arg(tpcb, NULL);
            tcp_sent(tpcb, NULL);
            tcp_close(tpcb);
        }
    }
    return ERR_OK;
}

// Callback TCP recv
static err_t http_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    char *request = (char *)p->payload;
    int send_len = 0;

    if (strstr(request, "GET /sensors") || strstr(request, "GET /api")) {
        build_sensor_json();
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /relay")) {
        if (strstr(request, "\"toggle\":true")) {
            if (strstr(request, "\"relay\":1")) relay_set(1, !g_relay_ln1);
            else if (strstr(request, "\"relay\":2")) relay_set(2, !g_relay_ln2);
            else if (strstr(request, "\"relay\":3")) relay_set(3, !g_relay_ln3);
        } else {
            bool state = (strstr(request, "\"state\":1") != NULL) || (strstr(request, "\"estado\":true") != NULL);
            if (strstr(request, "\"relay\":1") || strstr(request, "\"tipo\":\"LN1\"")) relay_set(1, state);
            else if (strstr(request, "\"relay\":2") || strstr(request, "\"tipo\":\"LN2\"")) relay_set(2, state);
            else if (strstr(request, "\"relay\":3") || strstr(request, "\"tipo\":\"LN3\"")) relay_set(3, state);
        }
        snprintf(json_buf, sizeof(json_buf),
            "{\"success\":true,\"relays\":{\"LN1\":%s,\"LN2\":%s,\"LN3\":%s}}",
            g_relay_ln1?"true":"false", g_relay_ln2?"true":"false", g_relay_ln3?"true":"false");
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /feed") || strstr(request, "POST /servo")) {
        servo_feed();
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: 49\r\n\r\n{\"success\":true,\"message\":\"Alimentacao executada\"}");
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "GET /status")) {
        snprintf(json_buf, sizeof(json_buf),
            "{\"system\":\"HydroSense v10\",\"wifi\":true,\"ip\":\"%s\","
            "\"oled\":%s,\"sensores\":{\"aht10\":%s,\"vl53l0x\":%s},"
            "\"relays\":{\"LN1\":%s,\"LN2\":%s,\"LN3\":%s},\"leituras\":%d}",
            g_ip, g_oled_ok?"true":"false",
            g_aht_ok?"true":"false", g_vl53_ok?"true":"false",
            g_relay_ln1?"true":"false", g_relay_ln2?"true":"false", g_relay_ln3?"true":"false", g_count);
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "OPTIONS")) {
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nConnection: close\r\n\r\n");
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else {
        // Pagina HTML - envia header e body como chamadas separadas
        int html_len = strlen(HTML_PAGE);
        int hdr_len = snprintf(http_header, sizeof(http_header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n", html_len);
        err_t e1 = tcp_write(tpcb, http_header, hdr_len, 0);
        err_t e2 = tcp_write(tpcb, HTML_PAGE, html_len, 0);
        send_len = hdr_len + html_len;
    }

    // Configura tracking e envia
    http_conn.total_to_send = send_len;
    http_conn.total_acked = 0;
    tcp_arg(tpcb, &http_conn);
    tcp_sent(tpcb, http_sent_cb);
    tcp_output(tpcb);
    pbuf_free(p);
    // NÃO chama tcp_close aqui - o sent callback fecha quando tudo for confirmado
    
    return ERR_OK;
}

static err_t http_accept_cb(void *a, struct tcp_pcb *n, err_t e) {
    if (e != ERR_OK || !n) return ERR_VAL;
    tcp_recv(n, http_recv_cb);
    return ERR_OK;
}

// ============================================================
// LEITURA DOS SENSORES
// ============================================================

void read_sensors(void) {
    g_count++;
    
    // AHT10
    float temp, hum;
    if (aht10_read(&temp, &hum)) {
        g_temp = temp;
        g_hum = hum;
        g_aht_ok = true;
    }
    
    // VL53L0X
    if (g_vl53_ok) {
        uint16_t dist = vl53_read();
        if (dist != 0xFFFF && dist < 2000) {
            g_dist = dist;
        } else {
            printf("[VL53] Falha leitura (0x%04X)\n", dist);
        }
    }
    
    // TCS34725
    if (g_tcs_ok) {
        tcs_read_color();
    }
    
    // Calcula nivel e volume
    float water_h = TANK_HEIGHT_MM - (float)g_dist;
    if (water_h < 0) water_h = 0;
    g_nivel = (water_h / TANK_HEIGHT_MM) * 100.0f;
    if (g_nivel > 100) g_nivel = 100;
    g_volume = (g_nivel / 100.0f) * TANK_CAPACITY_L;
    
    // Automacao: liga ventilador se temp alta
    if (g_temp > TEMP_THRESHOLD && !g_relay_ln1) {
        relay_set(1, true);
        printf("[AUTO] Ventilador ON (T>%.0f)\n", TEMP_THRESHOLD);
    } else if (g_temp <= (TEMP_THRESHOLD - 1.0f) && g_relay_ln1) {
        relay_set(1, false);
        printf("[AUTO] Ventilador OFF\n");
    }
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Aguarda USB estabilizar
    
    printf("\n\n==========================================\n");
    printf("  HydroSense v10 - BitDogLab Completo\n");
    printf("==========================================\n\n");
    
    // === CYW43 ===
    printf("[1/7] CYW43...");
    if (cyw43_arch_init()) { printf("ERRO!\n"); while(1) sleep_ms(1000); }
    cyw43_arch_enable_sta_mode();
    printf(" OK\n");
    
    // === LED RGB ===
    printf("[2/7] LED RGB...");
    led_init();
    led_set(1, 0, 0);  // Vermelho durante init
    printf(" OK\n");
    
    // === Buzzer ===
    printf("[3/7] Buzzer...");
    buzzer_init();
    buzzer_beep(100);
    printf(" OK\n");
    
    // === OLED ===
    printf("[4/7] OLED...");
    g_oled_ok = oled_init();
    printf(" %s\n", g_oled_ok ? "OK" : "N/A");
    display_boot();
    
    // === Sensores I2C ===
    printf("[5/7] Sensores...\n");
    g_aht_ok = aht10_init();
    printf("  AHT10: %s\n", g_aht_ok ? "OK" : "N/A");
    g_vl53_ok = vl53_init();
    printf("  VL53L0X: %s\n", g_vl53_ok ? "OK" : "N/A");
    g_tcs_ok = tcs_init();
    printf("  TCS34725: %s\n", g_tcs_ok ? "OK" : "N/A");
    
    // === Servo ===
    printf("[6/7] Servo...");
    servo_init();
    servo_angle(0); sleep_ms(300);
    servo_angle(90); sleep_ms(300);
    servo_angle(0); servo_stop();
    printf(" OK\n");
    
    // === Reles ===
    printf("[7/7] Reles...");
    relay_init();
    printf(" OK (GP%d,%d,%d)\n", RELAY_LN1_PIN, RELAY_LN2_PIN, RELAY_LN3_PIN);
    
    // === WiFi ===
    printf("\n[WIFI] Conectando a [%s]...\n", WIFI_SSID);
    led_set(1, 1, 0);  // Amarelo
    display_wifi_connecting();
    
    int result = -1;
    printf("  WPA2-AES...\n");
    result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000);
    if (result) {
        printf("  err=%d, WPA2-MIXED...\n", result);
        sleep_ms(2000);
        result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000);
    }
    if (result) {
        printf("  err=%d, WPA-TKIP...\n", result);
        sleep_ms(2000);
        result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA_TKIP_PSK, 30000);
    }
    
    if (result) {
        printf("[WIFI] FALHOU (%d) - modo offline\n", result);
        g_wifi = false;
        led_set(1, 0, 0);
        display_wifi_fail();
    } else {
        g_wifi = true;
        strncpy(g_ip, ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr), 19);
        printf("[WIFI] IP: %s\n", g_ip);
        led_set(0, 1, 0);  // Verde
        buzzer_beep(50); sleep_ms(50); buzzer_beep(50);
        
        // Servidor HTTP
        struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
        if (pcb) {
            tcp_bind(pcb, IP_ANY_TYPE, 80);
            pcb = tcp_listen_with_backlog(pcb, 1);
            if (pcb) {
                tcp_accept(pcb, http_accept_cb);
                printf("[HTTP] http://%s/sensors\n", g_ip);
                printf("[HTTP] http://%s/ (pagina web)\n", g_ip);
            }
        }
        display_wifi_ok();
    }
    
    printf("\n=== MONITORAMENTO ATIVO ===\n\n");
    
    // === Loop Principal ===
    while (true) {
        read_sensors();
        display_main();
        
        printf("#%d T=%.1fC H=%.1f%% D=%dmm N=%.0f%% V=%.1fL C=%s R:%c%c%c W:%s\n",
               g_count, g_temp, g_hum, g_dist, g_nivel, g_volume, g_cor_nome,
               g_relay_ln1 ? '1' : '-', g_relay_ln2 ? '2' : '-', g_relay_ln3 ? '3' : '-',
               g_wifi ? "OK" : "OFF");
        
        // Pisca LED verde quando wifi ok
        if (g_wifi) {
            led_set(0, 1, 0); sleep_ms(100); led_set(0, 0, 0);
        }
        
        // Poll WiFi frequentemente por ~2 segundos
        // Isso garante que dados TCP sejam transmitidos/recebidos
        for (int i = 0; i < 19; i++) {
            if (g_wifi) cyw43_arch_poll();
            sleep_ms(100);
        }
    }
    return 0;
}
