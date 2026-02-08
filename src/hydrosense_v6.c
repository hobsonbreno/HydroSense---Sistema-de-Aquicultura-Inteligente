/**
 * HydroSense v6 - Sistema Completo com Servidor Web
 * 
 * HARDWARE:
 * - Sensores (VL53L0X, AHT10, TCS34725): I2C1 em GPIO2/3 (extensor)
 * - OLED SSD1306: I2C1 em GPIO14/15 (BitDogLab direto)
 * - Servo SG90: GPIO16
 * - WiFi: Pico W integrado
 * 
 * SERVIDOR WEB: http://[IP]:80
 * - Interface responsiva para celular/PC
 * - Atualização automática dos dados
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================================
// CONFIGURAÇÃO - EDITE AQUI!
// ============================================================
#define WIFI_SSID     "HOBSON-BRENO"      // Nome da sua rede WiFi 2.4GHz
#define WIFI_PASSWORD "N%T%LI%123"        // Senha da rede

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
#define TCS34725_ADDR   0x29  // Mesmo que VL53L0X? Veja nota abaixo

// Configuração do tanque
#define TANK_HEIGHT_CM  30
#define TANK_CAPACITY_L 20

// ============================================================
// VARIÁVEIS GLOBAIS
// ============================================================
static int current_i2c_mode = 0;

// Dados dos sensores
static float temperatura = 0;
static float umidade = 0;
static uint16_t distancia_mm = 0;
static float nivel_agua_percent = 0;
static float volume_litros = 0;
static bool aht_ok = false;
static bool vl53_ok = false;
static bool wifi_connected = false;
static char ip_address[20] = "0.0.0.0";

// Sensor de cor
static uint16_t cor_r = 0, cor_g = 0, cor_b = 0, cor_c = 0;
static char cor_nome[20] = "---";
static bool tcs_ok = false;

// Contador de leituras
static uint32_t leitura_count = 0;

// ============================================================
// I2C SWITCHING
// ============================================================

void i2c_init_for_sensors(void) {
    if (current_i2c_mode == 1) return;
    
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    
    i2c_deinit(i2c1);
    i2c_init(i2c1, 100000);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_ms(5);
    
    current_i2c_mode = 1;
}

void i2c_init_for_oled(void) {
    if (current_i2c_mode == 2) return;
    
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    
    i2c_deinit(i2c1);
    i2c_init(i2c1, 400000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(5);
    
    current_i2c_mode = 2;
}

// ============================================================
// AHT10 - Sensor de Temperatura e Umidade
// ============================================================

bool aht10_init(void) {
    i2c_init_for_sensors();
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    return i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) >= 0;
}

bool aht10_read(void) {
    i2c_init_for_sensors();
    
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
// VL53L0X - Sensor de Distância (simplificado para clones)
// ============================================================

#define REG_SYSRANGE_START            0x00
#define REG_RESULT_RANGE_STATUS       0x14
#define REG_RESULT_RANGE_MM           0x1E
#define REG_IDENTIFICATION_MODEL_ID   0xC0

static void vl53_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
}

static uint8_t vl53_read_reg(uint8_t reg) {
    uint8_t val = 0;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
    return val;
}

bool vl53_init(void) {
    i2c_init_for_sensors();
    sleep_ms(100);
    
    // Verifica se sensor está presente
    uint8_t dummy;
    int ret = i2c_read_blocking(i2c1, VL53L0X_ADDR, &dummy, 1, false);
    if (ret < 0) {
        printf("   VL53L0X não detectado\n");
        return false;
    }
    
    uint8_t id = vl53_read_reg(REG_IDENTIFICATION_MODEL_ID);
    printf("   VL53 ID: 0x%02X %s\n", id, id == 0xEE ? "(original)" : "(clone)");
    
    return true;
}

uint16_t vl53_read_mm(void) {
    i2c_init_for_sensors();
    
    // Inicia medição
    uint8_t cmd[2] = {REG_SYSRANGE_START, 0x01};
    if (i2c_write_blocking(i2c1, VL53L0X_ADDR, cmd, 2, false) != 2) {
        return 0xFFFF;
    }
    
    // Aguarda
    for (int i = 0; i < 100; i++) {
        uint8_t status = vl53_read_reg(REG_RESULT_RANGE_STATUS);
        if (status & 0x01) break;
        sleep_ms(5);
    }
    
    // Lê distância
    uint8_t reg = REG_RESULT_RANGE_MM;
    uint8_t buf[2];
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    uint16_t dist = (buf[0] << 8) | buf[1];
    
    // Filtra valores inválidos
    if (dist == 0 || dist > 2000 || buf[0] == 0x1E) {
        return 0xFFFF;  // Inválido
    }
    
    return dist;
}

// ============================================================
// TCS34725 - Sensor de Cor RGB
// ============================================================

// O TCS34725 usa endereço 0x29, mesmo que VL53L0X!
// Se você tem ambos, precisa usar um multiplexador I2C ou XSHUT no VL53L0X
// Por enquanto, vou implementar assumindo que estão em endereços diferentes
// ou que você tem só um dos sensores

#define TCS34725_COMMAND_BIT  0x80
#define TCS34725_ENABLE       0x00
#define TCS34725_ATIME        0x01
#define TCS34725_CONTROL      0x0F
#define TCS34725_ID           0x12
#define TCS34725_CDATAL       0x14
#define TCS34725_RDATAL       0x16
#define TCS34725_GDATAL       0x18
#define TCS34725_BDATAL       0x1A

// Se TCS34725 estiver em 0x29, mude o endereço do VL53L0X ou use este:
#define TCS_ADDR  0x29  // Endereço padrão do TCS34725

static void tcs_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {TCS34725_COMMAND_BIT | reg, val};
    i2c_write_blocking(i2c1, TCS_ADDR, buf, 2, false);
}

static uint8_t tcs_read_reg(uint8_t reg) {
    uint8_t cmd = TCS34725_COMMAND_BIT | reg;
    uint8_t val = 0;
    i2c_write_blocking(i2c1, TCS_ADDR, &cmd, 1, true);
    i2c_read_blocking(i2c1, TCS_ADDR, &val, 1, false);
    return val;
}

static uint16_t tcs_read_reg16(uint8_t reg) {
    uint8_t cmd = TCS34725_COMMAND_BIT | reg;
    uint8_t buf[2];
    i2c_write_blocking(i2c1, TCS_ADDR, &cmd, 1, true);
    i2c_read_blocking(i2c1, TCS_ADDR, buf, 2, false);
    return buf[0] | (buf[1] << 8);
}

bool tcs_init(void) {
    i2c_init_for_sensors();
    sleep_ms(50);
    
    // Verifica ID (0x44 ou 0x4D para TCS34725)
    uint8_t id = tcs_read_reg(TCS34725_ID);
    printf("   TCS34725 ID: 0x%02X ", id);
    
    if (id != 0x44 && id != 0x4D) {
        printf("(não encontrado)\n");
        return false;
    }
    printf("(OK)\n");
    
    // Configura: tempo de integração = 101ms, gain = 1x
    tcs_write_reg(TCS34725_ATIME, 0xD5);   // 101ms
    tcs_write_reg(TCS34725_CONTROL, 0x00); // Gain 1x
    
    // Habilita sensor
    tcs_write_reg(TCS34725_ENABLE, 0x01);  // Power ON
    sleep_ms(3);
    tcs_write_reg(TCS34725_ENABLE, 0x03);  // Power ON + ADC enable
    sleep_ms(101);  // Aguarda primeira integração
    
    return true;
}

void tcs_read_color(void) {
    i2c_init_for_sensors();
    
    cor_c = tcs_read_reg16(TCS34725_CDATAL);
    cor_r = tcs_read_reg16(TCS34725_RDATAL);
    cor_g = tcs_read_reg16(TCS34725_GDATAL);
    cor_b = tcs_read_reg16(TCS34725_BDATAL);
    
    // Determina cor dominante
    if (cor_c < 100) {
        strcpy(cor_nome, "Escuro");
    } else if (cor_r > cor_g && cor_r > cor_b) {
        if (cor_r > cor_g + cor_b) {
            strcpy(cor_nome, "Vermelho");
        } else if (cor_g > cor_b) {
            strcpy(cor_nome, "Amarelo");
        } else {
            strcpy(cor_nome, "Magenta");
        }
    } else if (cor_g > cor_r && cor_g > cor_b) {
        if (cor_g > cor_r + cor_b) {
            strcpy(cor_nome, "Verde");
        } else if (cor_b > cor_r) {
            strcpy(cor_nome, "Ciano");
        } else {
            strcpy(cor_nome, "Verde-Amarelo");
        }
    } else if (cor_b > cor_r && cor_b > cor_g) {
        strcpy(cor_nome, "Azul");
    } else {
        if (cor_c > 5000) {
            strcpy(cor_nome, "Branco");
        } else {
            strcpy(cor_nome, "Cinza");
        }
    }
}

// ============================================================
// OLED SSD1306
// ============================================================

static uint8_t oled_buffer[1024];

bool oled_init(void) {
    i2c_init_for_oled();
    
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, OLED_ADDR, &dummy, 1, false) < 0) {
        return false;
    }
    
    uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    
    for (int i = 0; i < sizeof(init_cmds); i++) {
        uint8_t buf[2] = {0x00, init_cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    
    memset(oled_buffer, 0, 1024);
    return true;
}

void oled_clear(void) {
    memset(oled_buffer, 0, 1024);
}

void oled_display(void) {
    i2c_init_for_oled();
    
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

void oled_char(int x, int y, char c) {
    if (c < 32 || c > 90) c = 32;
    int idx = (c - 32) * 5;
    for (int i = 0; i < 5; i++) {
        if (x + i >= 128) break;
        uint8_t col = font5x7[idx + i];
        for (int j = 0; j < 7; j++) {
            if (y + j >= 64) break;
            if (col & (1 << j)) {
                int pos = ((y + j) / 8) * 128 + (x + i);
                oled_buffer[pos] |= (1 << ((y + j) % 8));
            }
        }
    }
}

void oled_print(int x, int y, const char *str) {
    while (*str) {
        oled_char(x, y, *str++);
        x += 6;
    }
}

// ============================================================
// SERVO SG90
// ============================================================

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 125.0f);
    pwm_set_wrap(slice, 20000);
    pwm_set_enabled(slice, true);
}

void servo_set_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    uint16_t pulse = 500 + (angle * 2000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

void servo_test(void) {
    printf("  Testando servo...\n");
    servo_set_angle(0);
    sleep_ms(500);
    servo_set_angle(90);
    sleep_ms(500);
    servo_set_angle(180);
    sleep_ms(500);
    servo_set_angle(90);
    sleep_ms(500);
    servo_stop();
}

// ============================================================
// SERVIDOR WEB HTTP
// ============================================================

// HTML da página web - interface moderna e responsiva
static const char *html_page = 
"<!DOCTYPE html>\n"
"<html><head>\n"
"<meta charset='UTF-8'>\n"
"<meta name='viewport' content='width=device-width, initial-scale=1'>\n"
"<title>HydroSense</title>\n"
"<style>\n"
"*{box-sizing:border-box;margin:0;padding:0}\n"
"body{font-family:'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);min-height:100vh;color:#fff}\n"
".header{background:linear-gradient(90deg,#0f3460,#16537e);padding:20px;text-align:center;box-shadow:0 2px 10px rgba(0,0,0,0.3)}\n"
".header h1{font-size:1.8em;margin-bottom:5px}\n"
".header p{opacity:0.8;font-size:0.9em}\n"
".container{padding:15px;max-width:800px;margin:0 auto}\n"
".card{background:rgba(255,255,255,0.1);border-radius:15px;padding:20px;margin-bottom:15px;backdrop-filter:blur(10px)}\n"
".card h3{font-size:1em;opacity:0.7;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px}\n"
".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:15px}\n"
".sensor{background:linear-gradient(145deg,rgba(255,255,255,0.15),rgba(255,255,255,0.05));border-radius:12px;padding:15px;text-align:center}\n"
".sensor .value{font-size:2em;font-weight:bold;margin:10px 0}\n"
".sensor .label{font-size:0.85em;opacity:0.7}\n"
".sensor .icon{font-size:1.5em;margin-bottom:5px}\n"
".temp{border-left:4px solid #ff6b6b}\n"
".hum{border-left:4px solid #4ecdc4}\n"
".dist{border-left:4px solid #45b7d1}\n"
".nivel{border-left:4px solid #96ceb4}\n"
".vol{border-left:4px solid #ffeaa7}\n"
".cor{border-left:4px solid #dfe6e9}\n"
".status{display:flex;justify-content:space-between;align-items:center;padding:10px 15px;background:rgba(0,0,0,0.2);border-radius:8px;margin-top:10px}\n"
".status .dot{width:10px;height:10px;border-radius:50%;margin-right:8px}\n"
".online{background:#2ecc71}\n"
".offline{background:#e74c3c}\n"
".refresh{background:#3498db;color:#fff;border:none;padding:12px 25px;border-radius:8px;font-size:1em;cursor:pointer;width:100%;margin-top:15px}\n"
".refresh:hover{background:#2980b9}\n"
".rgb-bar{height:30px;border-radius:8px;margin-top:10px;display:flex}\n"
".rgb-bar div{flex:1;first-child{border-radius:8px 0 0 8px}last-child{border-radius:0 8px 8px 0}}\n"
"</style>\n"
"</head><body>\n"
"<div class='header'>\n"
"<h1>🐟 HydroSense</h1>\n"
"<p>Sistema de Monitoramento de Aquicultura</p>\n"
"</div>\n"
"<div class='container'>\n"
"<div class='card'>\n"
"<h3>📊 Sensores</h3>\n"
"<div class='grid'>\n"
"<div class='sensor temp'><div class='icon'>🌡️</div><div class='value'>%TEMP%°C</div><div class='label'>Temperatura</div></div>\n"
"<div class='sensor hum'><div class='icon'>💧</div><div class='value'>%HUM%%%</div><div class='label'>Umidade</div></div>\n"
"<div class='sensor dist'><div class='icon'>📏</div><div class='value'>%DIST%mm</div><div class='label'>Distancia</div></div>\n"
"<div class='sensor nivel'><div class='icon'>🌊</div><div class='value'>%NIVEL%%%</div><div class='label'>Nivel Agua</div></div>\n"
"<div class='sensor vol'><div class='icon'>🪣</div><div class='value'>%VOL%L</div><div class='label'>Volume</div></div>\n"
"<div class='sensor cor'><div class='icon'>🎨</div><div class='value'>%COR%</div><div class='label'>Cor Agua</div></div>\n"
"</div>\n"
"</div>\n"
"<div class='card'>\n"
"<h3>🔴🟢🔵 Sensor de Cor RGB</h3>\n"
"<div class='grid'>\n"
"<div class='sensor' style='border-left:4px solid #ff0000'><div class='value'>%R%</div><div class='label'>Vermelho</div></div>\n"
"<div class='sensor' style='border-left:4px solid #00ff00'><div class='value'>%G%</div><div class='label'>Verde</div></div>\n"
"<div class='sensor' style='border-left:4px solid #0000ff'><div class='value'>%B%</div><div class='label'>Azul</div></div>\n"
"<div class='sensor' style='border-left:4px solid #ffffff'><div class='value'>%C%</div><div class='label'>Claridade</div></div>\n"
"</div>\n"
"</div>\n"
"<div class='card'>\n"
"<h3>📡 Status do Sistema</h3>\n"
"<div class='status'><span><span class='dot %WIFI_CLASS%'></span>WiFi</span><span>%WIFI_STATUS%</span></div>\n"
"<div class='status'><span><span class='dot %AHT_CLASS%'></span>AHT10</span><span>%AHT_STATUS%</span></div>\n"
"<div class='status'><span><span class='dot %VL53_CLASS%'></span>VL53L0X</span><span>%VL53_STATUS%</span></div>\n"
"<div class='status'><span><span class='dot %TCS_CLASS%'></span>TCS34725</span><span>%TCS_STATUS%</span></div>\n"
"<div class='status'><span>📈 Leituras</span><span>#%COUNT%</span></div>\n"
"</div>\n"
"<button class='refresh' onclick='location.reload()'>🔄 Atualizar Dados</button>\n"
"</div>\n"
"<script>setTimeout(()=>location.reload(),5000);</script>\n"
"</body></html>\n";

static char http_response[4096];

static void build_http_response(void) {
    char temp_str[16], hum_str[16], dist_str[16], nivel_str[16], vol_str[16];
    char r_str[16], g_str[16], b_str[16], c_str[16], count_str[16];
    
    snprintf(temp_str, sizeof(temp_str), "%.1f", temperatura);
    snprintf(hum_str, sizeof(hum_str), "%.0f", umidade);
    snprintf(dist_str, sizeof(dist_str), "%d", distancia_mm);
    snprintf(nivel_str, sizeof(nivel_str), "%.0f", nivel_agua_percent);
    snprintf(vol_str, sizeof(vol_str), "%.1f", volume_litros);
    snprintf(r_str, sizeof(r_str), "%d", cor_r);
    snprintf(g_str, sizeof(g_str), "%d", cor_g);
    snprintf(b_str, sizeof(b_str), "%d", cor_b);
    snprintf(c_str, sizeof(c_str), "%d", cor_c);
    snprintf(count_str, sizeof(count_str), "%lu", leitura_count);
    
    // Copia template e substitui placeholders
    char *html = malloc(strlen(html_page) + 500);
    if (!html) return;
    strcpy(html, html_page);
    
    // Substitui placeholders
    char *p;
    
    if ((p = strstr(html, "%TEMP%"))) { memmove(p + strlen(temp_str), p + 6, strlen(p + 6) + 1); memcpy(p, temp_str, strlen(temp_str)); }
    if ((p = strstr(html, "%HUM%"))) { memmove(p + strlen(hum_str), p + 5, strlen(p + 5) + 1); memcpy(p, hum_str, strlen(hum_str)); }
    if ((p = strstr(html, "%DIST%"))) { memmove(p + strlen(dist_str), p + 6, strlen(p + 6) + 1); memcpy(p, dist_str, strlen(dist_str)); }
    if ((p = strstr(html, "%NIVEL%"))) { memmove(p + strlen(nivel_str), p + 7, strlen(p + 7) + 1); memcpy(p, nivel_str, strlen(nivel_str)); }
    if ((p = strstr(html, "%VOL%"))) { memmove(p + strlen(vol_str), p + 5, strlen(p + 5) + 1); memcpy(p, vol_str, strlen(vol_str)); }
    if ((p = strstr(html, "%COR%"))) { memmove(p + strlen(cor_nome), p + 5, strlen(p + 5) + 1); memcpy(p, cor_nome, strlen(cor_nome)); }
    if ((p = strstr(html, "%R%"))) { memmove(p + strlen(r_str), p + 3, strlen(p + 3) + 1); memcpy(p, r_str, strlen(r_str)); }
    if ((p = strstr(html, "%G%"))) { memmove(p + strlen(g_str), p + 3, strlen(p + 3) + 1); memcpy(p, g_str, strlen(g_str)); }
    if ((p = strstr(html, "%B%"))) { memmove(p + strlen(b_str), p + 3, strlen(p + 3) + 1); memcpy(p, b_str, strlen(b_str)); }
    if ((p = strstr(html, "%C%"))) { memmove(p + strlen(c_str), p + 3, strlen(p + 3) + 1); memcpy(p, c_str, strlen(c_str)); }
    if ((p = strstr(html, "%COUNT%"))) { memmove(p + strlen(count_str), p + 7, strlen(p + 7) + 1); memcpy(p, count_str, strlen(count_str)); }
    
    // Status classes
    if ((p = strstr(html, "%WIFI_CLASS%"))) { const char *s = wifi_connected ? "online" : "offline"; memmove(p + strlen(s), p + 12, strlen(p + 12) + 1); memcpy(p, s, strlen(s)); }
    if ((p = strstr(html, "%WIFI_STATUS%"))) { const char *s = wifi_connected ? "Conectado" : "Desconectado"; memmove(p + strlen(s), p + 13, strlen(p + 13) + 1); memcpy(p, s, strlen(s)); }
    if ((p = strstr(html, "%AHT_CLASS%"))) { const char *s = aht_ok ? "online" : "offline"; memmove(p + strlen(s), p + 11, strlen(p + 11) + 1); memcpy(p, s, strlen(s)); }
    if ((p = strstr(html, "%AHT_STATUS%"))) { const char *s = aht_ok ? "OK" : "Erro"; memmove(p + strlen(s), p + 12, strlen(p + 12) + 1); memcpy(p, s, strlen(s)); }
    if ((p = strstr(html, "%VL53_CLASS%"))) { const char *s = vl53_ok ? "online" : "offline"; memmove(p + strlen(s), p + 12, strlen(p + 12) + 1); memcpy(p, s, strlen(s)); }
    if ((p = strstr(html, "%VL53_STATUS%"))) { const char *s = vl53_ok ? "OK" : "Erro"; memmove(p + strlen(s), p + 13, strlen(p + 13) + 1); memcpy(p, s, strlen(s)); }
    if ((p = strstr(html, "%TCS_CLASS%"))) { const char *s = tcs_ok ? "online" : "offline"; memmove(p + strlen(s), p + 11, strlen(p + 11) + 1); memcpy(p, s, strlen(s)); }
    if ((p = strstr(html, "%TCS_STATUS%"))) { const char *s = tcs_ok ? "OK" : "N/A"; memmove(p + strlen(s), p + 12, strlen(p + 12) + 1); memcpy(p, s, strlen(s)); }
    
    // Monta resposta HTTP
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n%s",
        (int)strlen(html), html);
    
    free(html);
}

// API JSON para integração com outros sistemas
static const char *json_template = "{\"temperatura\":%.1f,\"umidade\":%.0f,\"distancia\":%d,\"nivel\":%.0f,\"volume\":%.1f,\"cor\":{\"r\":%d,\"g\":%d,\"b\":%d,\"c\":%d,\"nome\":\"%s\"},\"wifi\":%s,\"leituras\":%lu}";
static char json_response[512];

static void build_json_response(void) {
    char json[400];
    snprintf(json, sizeof(json), json_template,
        temperatura, umidade, distancia_mm, nivel_agua_percent, volume_litros,
        cor_r, cor_g, cor_b, cor_c, cor_nome,
        wifi_connected ? "true" : "false",
        leitura_count);
    
    snprintf(json_response, sizeof(json_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n%s",
        (int)strlen(json), json);
}

// Callback TCP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    // Verifica se é requisição para API
    char *request = (char*)p->payload;
    
    if (strstr(request, "GET /api") || strstr(request, "GET /json")) {
        build_json_response();
        tcp_write(tpcb, json_response, strlen(json_response), TCP_WRITE_FLAG_COPY);
    } else {
        build_http_response();
        tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
    }
    
    tcp_output(tpcb);
    pbuf_free(p);
    tcp_close(tpcb);
    
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

static struct tcp_pcb *server_pcb = NULL;

bool start_web_server(void) {
    server_pcb = tcp_new();
    if (!server_pcb) return false;
    
    if (tcp_bind(server_pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        tcp_close(server_pcb);
        return false;
    }
    
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, tcp_server_accept);
    
    printf("   Servidor web iniciado na porta 80\n");
    return true;
}

// ============================================================
// WiFi
// ============================================================

bool wifi_connect(void) {
    printf("\n📡 Conectando ao WiFi: %s\n", WIFI_SSID);
    
    if (cyw43_arch_init()) {
        printf("   Erro ao inicializar WiFi\n");
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    
    printf("   Tentando conectar...\n");
    
    int result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID, WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );
    
    if (result != 0) {
        printf("   ❌ Falha na conexão: %d\n", result);
        return false;
    }
    
    // Obtém IP
    struct netif *netif = netif_default;
    if (netif && netif->ip_addr.addr) {
        snprintf(ip_address, sizeof(ip_address), "%s", ip4addr_ntoa(&netif->ip_addr));
        printf("   ✅ Conectado! IP: %s\n", ip_address);
    }
    
    wifi_connected = true;
    return true;
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n");
    printf("========================================\n");
    printf("  HydroSense v6 - Servidor Web\n");
    printf("  Sensores: GPIO2/3  OLED: GPIO14/15\n");
    printf("========================================\n\n");
    
    // Inicializa OLED
    printf("Inicializando OLED...\n");
    bool oled_ok = oled_init();
    printf("  OLED: %s\n", oled_ok ? "OK" : "ERRO");
    
    if (oled_ok) {
        oled_clear();
        oled_print(10, 0, "HYDROSENSE V6");
        oled_print(10, 16, "INICIANDO...");
        oled_display();
    }
    
    // Inicializa sensores
    printf("Inicializando sensores...\n");
    
    aht_ok = aht10_init();
    printf("  AHT10: %s\n", aht_ok ? "OK" : "ERRO");
    
    vl53_ok = vl53_init();
    printf("  VL53L0X: %s\n", vl53_ok ? "OK" : "ERRO");
    
    // TCS34725 - só tenta se VL53L0X não estiver no mesmo endereço
    // NOTA: Ambos usam 0x29 por padrão! Precisa de multiplexador I2C
    // Por enquanto, desabilita TCS se VL53 está ativo
    if (!vl53_ok) {
        tcs_ok = tcs_init();
        printf("  TCS34725: %s\n", tcs_ok ? "OK" : "N/A");
    } else {
        printf("  TCS34725: Desativado (conflito 0x29)\n");
        tcs_ok = false;
    }
    
    // Inicializa servo
    servo_init();
    servo_test();
    printf("  Servo: OK\n");
    
    // Conecta WiFi
    printf("\nConectando WiFi...\n");
    
    if (wifi_connect()) {
        // Inicia servidor web
        if (start_web_server()) {
            printf("\n🌐 Acesse: http://%s\n", ip_address);
            printf("🔌 API JSON: http://%s/api\n", ip_address);
        }
        
        if (oled_ok) {
            oled_clear();
            oled_print(0, 0, "WIFI OK");
            oled_print(0, 12, ip_address);
            oled_print(0, 28, "HTTP://");
            oled_print(0, 40, ip_address);
            oled_display();
        }
    } else {
        printf("❌ WiFi falhou - modo offline\n");
        
        if (oled_ok) {
            oled_clear();
            oled_print(0, 0, "WIFI ERRO");
            oled_print(0, 16, "MODO OFFLINE");
            oled_display();
        }
    }
    
    sleep_ms(3000);
    
    // Loop principal
    printf("\n=== MONITORAMENTO INICIADO ===\n");
    
    while (true) {
        leitura_count++;
        
        // Lê sensores
        if (aht_ok) {
            aht10_read();
        }
        
        if (vl53_ok) {
            distancia_mm = vl53_read_mm();
            if (distancia_mm != 0xFFFF) {
                // Calcula nível (invertido - maior distância = menos água)
                float dist_cm = distancia_mm / 10.0f;
                nivel_agua_percent = ((TANK_HEIGHT_CM - dist_cm) / TANK_HEIGHT_CM) * 100.0f;
                if (nivel_agua_percent < 0) nivel_agua_percent = 0;
                if (nivel_agua_percent > 100) nivel_agua_percent = 100;
                volume_litros = (nivel_agua_percent / 100.0f) * TANK_CAPACITY_L;
            }
        }
        
        if (tcs_ok) {
            tcs_read_color();
        }
        
        // Atualiza OLED
        if (oled_ok) {
            oled_clear();
            
            char buf[24];
            snprintf(buf, sizeof(buf), "T:%.1fC H:%.0f%%", temperatura, umidade);
            oled_print(0, 0, buf);
            
            snprintf(buf, sizeof(buf), "D:%dmm N:%.0f%%", distancia_mm, nivel_agua_percent);
            oled_print(0, 12, buf);
            
            snprintf(buf, sizeof(buf), "VOL:%.1fL", volume_litros);
            oled_print(0, 24, buf);
            
            if (tcs_ok) {
                snprintf(buf, sizeof(buf), "COR:%s", cor_nome);
                oled_print(0, 36, buf);
            }
            
            if (wifi_connected) {
                oled_print(0, 52, ip_address);
            } else {
                oled_print(0, 52, "OFFLINE");
            }
            
            oled_display();
        }
        
        // Log serial
        printf("[%lu] T=%.1fC H=%.0f%% D=%dmm N=%.0f%% V=%.1fL",
               leitura_count, temperatura, umidade, 
               distancia_mm, nivel_agua_percent, volume_litros);
        
        if (tcs_ok) {
            printf(" COR=%s(R%d G%d B%d)", cor_nome, cor_r, cor_g, cor_b);
        }
        printf("\n");
        
        // Processa WiFi
        if (wifi_connected) {
            cyw43_arch_poll();
        }
        
        sleep_ms(2000);
    }
    
    return 0;
}
