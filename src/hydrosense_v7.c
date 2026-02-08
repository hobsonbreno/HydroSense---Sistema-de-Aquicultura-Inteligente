/**
 * HydroSense v7 - Sistema Completo com Servidor Web IoT
 * 
 * HARDWARE BitDogLab + Pico W:
 * - Sensores I2C: GPIO2(SDA)/GPIO3(SCL) via extensor
 * - OLED SSD1306: GPIO14(SDA)/GPIO15(SCL) direto na BitDogLab
 * - Servo SG90: GPIO16 (PWM)
 * - LED RGB: GPIO11(R), GPIO12(G), GPIO13(B)
 * - Buzzer: GPIO21
 * - WiFi: Chip CYW43 integrado
 * 
 * FUNCIONALIDADES:
 * - Servidor Web HTTP com interface responsiva
 * - API JSON para integração
 * - Atualização em tempo real via JavaScript
 * - Reconexão automática do WiFi
 * - Display OLED com status
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
// 🔧 CONFIGURAÇÃO WiFi - EDITE AQUI!
// ============================================================
// TESTE: Use um hotspot do celular com estes dados
#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"

// ============================================================
// PINOS - BitDogLab
// ============================================================
// I2C Sensores (via extensor I2C)
#define SENSOR_SDA      2
#define SENSOR_SCL      3

// I2C OLED (direto na BitDogLab)
#define OLED_SDA        14
#define OLED_SCL        15

// Servo
#define SERVO_PIN       16

// LED RGB
#define LED_R_PIN       11
#define LED_G_PIN       12
#define LED_B_PIN       13

// Buzzer
#define BUZZER_PIN      21

// Botões
#define BTN_A_PIN       5
#define BTN_B_PIN       6

// ============================================================
// ENDEREÇOS I2C
// ============================================================
#define AHT10_ADDR      0x38
#define VL53L0X_ADDR    0x29
#define OLED_ADDR       0x3C

// ============================================================
// CONFIGURAÇÃO DO TANQUE
// ============================================================
#define TANK_HEIGHT_CM  30.0f
#define TANK_CAPACITY_L 20.0f
#define SENSOR_OFFSET_CM 2.0f  // Distância do sensor ao topo

// ============================================================
// VARIÁVEIS GLOBAIS
// ============================================================
static int current_i2c_mode = 0;  // 0=nenhum, 1=sensores, 2=oled

// Dados dos sensores
static volatile float g_temperatura = 0.0f;
static volatile float g_umidade = 0.0f;
static volatile uint16_t g_distancia_mm = 0;
static volatile float g_nivel_percent = 0.0f;
static volatile float g_volume_litros = 0.0f;

// Status
static volatile bool g_aht_ok = false;
static volatile bool g_vl53_ok = false;
static volatile bool g_oled_ok = false;
static volatile bool g_wifi_connected = false;
static volatile uint32_t g_leitura_count = 0;
static char g_ip_address[20] = "0.0.0.0";

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
    i2c_init(i2c1, 100000);  // 100kHz para melhor compatibilidade
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
    
    // Comando de medição
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) {
        return false;
    }
    
    sleep_ms(80);  // Aguarda medição
    
    // Lê dados
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) {
        return false;
    }
    
    // Verifica se está ocupado
    if (data[0] & 0x80) {
        return false;
    }
    
    // Converte
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    *hum = (float)hum_raw / 1048576.0f * 100.0f;
    *temp = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    
    return true;
}

// ============================================================
// VL53L0X - Sensor de Distância (Simplificado para clones)
// ============================================================

bool vl53_init(void) {
    i2c_switch_to_sensors();
    sleep_ms(50);
    
    // Tenta detectar sensor
    uint8_t dummy;
    int ret = i2c_read_blocking(i2c1, VL53L0X_ADDR, &dummy, 1, false);
    
    if (ret < 0) {
        return false;
    }
    
    // Lê ID do sensor
    uint8_t reg = 0xC0;
    uint8_t id = 0;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &id, 1, false);
    
    printf("   VL53L0X ID: 0x%02X\n", id);
    
    return true;
}

uint16_t vl53_read_distance(void) {
    i2c_switch_to_sensors();
    
    // Inicia medição single-shot
    uint8_t cmd[2] = {0x00, 0x01};
    if (i2c_write_blocking(i2c1, VL53L0X_ADDR, cmd, 2, false) != 2) {
        return 0xFFFF;
    }
    
    // Aguarda
    sleep_ms(50);
    
    // Lê resultado (registros 0x1E-0x1F)
    uint8_t reg = 0x1E;
    uint8_t buf[2] = {0, 0};
    
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    uint16_t dist = (buf[0] << 8) | buf[1];
    
    // Filtra valores inválidos
    if (dist == 0 || dist > 2000 || buf[0] == 0x1E) {
        return 0xFFFF;
    }
    
    return dist;
}

// ============================================================
// OLED SSD1306 128x64
// ============================================================

static uint8_t oled_buffer[1024];

// Font 5x7 simplificada
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
    
    // Verifica se OLED está presente
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, OLED_ADDR, &dummy, 1, false) < 0) {
        return false;
    }
    
    // Comandos de inicialização
    uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide
        0xA8, 0x3F, // Multiplex 64
        0xD3, 0x00, // Display offset
        0x40,       // Start line
        0x8D, 0x14, // Charge pump
        0x20, 0x00, // Memory mode horizontal
        0xA1,       // Segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pins
        0x81, 0xCF, // Contrast
        0xD9, 0xF1, // Pre-charge
        0xDB, 0x40, // VCOMH deselect
        0xA4,       // Display from RAM
        0xA6,       // Normal display
        0xAF        // Display ON
    };
    
    for (int i = 0; i < sizeof(init_cmds); i++) {
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
    
    // Define área de escrita
    uint8_t cmds[] = {0x21, 0, 127, 0x22, 0, 7};
    for (int i = 0; i < 6; i++) {
        uint8_t buf[2] = {0x00, cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    
    // Envia buffer
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
    if (on) {
        oled_buffer[idx] |= (1 << (y % 8));
    } else {
        oled_buffer[idx] &= ~(1 << (y % 8));
    }
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
    // Texto 2x maior
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
    
    // 500us (0°) a 2500us (180°)
    uint16_t pulse = 500 + (angle * 2000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

// ============================================================
// LED RGB
// ============================================================

void led_rgb_init(void) {
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
}

void led_rgb_set(bool r, bool g, bool b) {
    gpio_put(LED_R_PIN, r);
    gpio_put(LED_G_PIN, g);
    gpio_put(LED_B_PIN, b);
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
// SERVIDOR WEB HTTP
// ============================================================

// Página HTML com design moderno e atualização automática
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
".container{max-width:600px;margin:0 auto}"
".header{text-align:center;color:#fff;margin-bottom:20px}"
".header h1{font-size:2em;text-shadow:2px 2px 4px rgba(0,0,0,0.3)}"
".header p{opacity:0.9}"
".card{background:rgba(255,255,255,0.95);border-radius:20px;padding:20px;margin-bottom:15px;box-shadow:0 10px 40px rgba(0,0,0,0.2)}"
".card h2{color:#667eea;font-size:1em;margin-bottom:15px;text-transform:uppercase;letter-spacing:1px}"
".grid{display:grid;grid-template-columns:repeat(2,1fr);gap:15px}"
".sensor{background:linear-gradient(135deg,#f5f7fa,#c3cfe2);border-radius:15px;padding:15px;text-align:center}"
".sensor .icon{font-size:2em;margin-bottom:5px}"
".sensor .value{font-size:2.5em;font-weight:bold;color:#333}"
".sensor .unit{font-size:0.8em;color:#666}"
".sensor .label{font-size:0.75em;color:#888;margin-top:5px}"
".temp{border-left:4px solid #ff6b6b}"
".hum{border-left:4px solid #4ecdc4}"
".dist{border-left:4px solid #45b7d1}"
".nivel{border-left:4px solid #96ceb4}"
".vol{border-left:4px solid #ffeaa7}"
".status{display:flex;justify-content:space-between;align-items:center;padding:10px;background:#f8f9fa;border-radius:10px;margin-top:10px}"
".status .dot{width:12px;height:12px;border-radius:50%;margin-right:8px}"
".online{background:#2ecc71}"
".offline{background:#e74c3c}"
".btn{width:100%;padding:15px;border:none;border-radius:10px;font-size:1em;cursor:pointer;margin-top:10px}"
".btn-primary{background:linear-gradient(135deg,#667eea,#764ba2);color:#fff}"
".btn-danger{background:#e74c3c;color:#fff}"
".update{text-align:center;color:#fff;opacity:0.7;font-size:0.8em;margin-top:10px}"
"</style>"
"</head>"
"<body>"
"<div class='container'>"
"<div class='header'>"
"<h1>🐟 HydroSense</h1>"
"<p>Sistema de Monitoramento IoT</p>"
"</div>"
"<div class='card'>"
"<h2>📊 Sensores em Tempo Real</h2>"
"<div class='grid'>"
"<div class='sensor temp'>"
"<div class='icon'>🌡️</div>"
"<div class='value' id='temp'>--</div>"
"<div class='unit'>°C</div>"
"<div class='label'>Temperatura</div>"
"</div>"
"<div class='sensor hum'>"
"<div class='icon'>💧</div>"
"<div class='value' id='hum'>--</div>"
"<div class='unit'>%</div>"
"<div class='label'>Umidade</div>"
"</div>"
"<div class='sensor dist'>"
"<div class='icon'>📏</div>"
"<div class='value' id='dist'>--</div>"
"<div class='unit'>mm</div>"
"<div class='label'>Distância</div>"
"</div>"
"<div class='sensor nivel'>"
"<div class='icon'>🌊</div>"
"<div class='value' id='nivel'>--</div>"
"<div class='unit'>%</div>"
"<div class='label'>Nível Água</div>"
"</div>"
"</div>"
"<div class='sensor vol' style='margin-top:15px'>"
"<div class='icon'>🪣</div>"
"<div class='value' id='vol'>--</div>"
"<div class='unit'>Litros</div>"
"<div class='label'>Volume Estimado</div>"
"</div>"
"</div>"
"<div class='card'>"
"<h2>📡 Status do Sistema</h2>"
"<div class='status'><span style='display:flex;align-items:center'><span class='dot' id='wifi-dot'></span>WiFi</span><span id='wifi-status'>--</span></div>"
"<div class='status'><span style='display:flex;align-items:center'><span class='dot' id='aht-dot'></span>AHT10</span><span id='aht-status'>--</span></div>"
"<div class='status'><span style='display:flex;align-items:center'><span class='dot' id='vl53-dot'></span>VL53L0X</span><span id='vl53-status'>--</span></div>"
"<div class='status'><span>📈 Leituras</span><span id='count'>0</span></div>"
"</div>"
"<div class='card'>"
"<h2>🎮 Controles</h2>"
"<button class='btn btn-primary' onclick='alimentar()'>🍽️ Alimentar Agora</button>"
"<button class='btn btn-danger' onclick='location.reload()'>🔄 Atualizar Página</button>"
"</div>"
"<div class='update'>Atualização automática a cada 2 segundos</div>"
"</div>"
"<script>"
"function update(){"
"fetch('/api').then(r=>r.json()).then(d=>{"
"document.getElementById('temp').textContent=d.temperatura.toFixed(1);"
"document.getElementById('hum').textContent=d.umidade.toFixed(0);"
"document.getElementById('dist').textContent=d.distancia;"
"document.getElementById('nivel').textContent=d.nivel.toFixed(0);"
"document.getElementById('vol').textContent=d.volume.toFixed(1);"
"document.getElementById('count').textContent=d.leituras;"
"document.getElementById('wifi-dot').className='dot '+(d.wifi?'online':'offline');"
"document.getElementById('wifi-status').textContent=d.wifi?'Conectado':'Offline';"
"document.getElementById('aht-dot').className='dot '+(d.aht_ok?'online':'offline');"
"document.getElementById('aht-status').textContent=d.aht_ok?'OK':'Erro';"
"document.getElementById('vl53-dot').className='dot '+(d.vl53_ok?'online':'offline');"
"document.getElementById('vl53-status').textContent=d.vl53_ok?'OK':'Erro';"
"}).catch(e=>console.log(e))}"
"function alimentar(){fetch('/feed').then(()=>alert('Alimentação ativada!'))}"
"setInterval(update,2000);"
"update();"
"</script>"
"</body>"
"</html>";

static char http_response[8192];
static char json_buffer[512];

void build_json_response(void) {
    snprintf(json_buffer, sizeof(json_buffer),
        "{\"temperatura\":%.1f,\"umidade\":%.0f,\"distancia\":%d,\"nivel\":%.0f,\"volume\":%.1f,"
        "\"wifi\":%s,\"aht_ok\":%s,\"vl53_ok\":%s,\"leituras\":%lu}",
        g_temperatura, g_umidade, g_distancia_mm, g_nivel_percent, g_volume_litros,
        g_wifi_connected ? "true" : "false",
        g_aht_ok ? "true" : "false",
        g_vl53_ok ? "true" : "false",
        g_leitura_count
    );
}

void build_html_response(void) {
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n%s",
        (int)strlen(HTML_PAGE), HTML_PAGE);
}

// Callbacks TCP
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    char *request = (char*)p->payload;
    
    // Verifica tipo de requisição
    if (strstr(request, "GET /api")) {
        // API JSON
        build_json_response();
        char response[600];
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buffer), json_buffer);
        tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    }
    else if (strstr(request, "GET /feed")) {
        // Comando de alimentação
        printf("🍽️ Comando de alimentação recebido!\n");
        servo_angle(90);
        sleep_ms(1000);
        servo_angle(0);
        servo_stop();
        
        const char *resp = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
        tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
    }
    else {
        // Página HTML
        build_html_response();
        tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
    }
    
    tcp_output(tpcb);
    pbuf_free(p);
    tcp_close(tpcb);
    
    return ERR_OK;
}

static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_recv_callback);
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
    tcp_accept(server_pcb, tcp_accept_callback);
    
    printf("✅ Servidor web iniciado na porta 80\n");
    return true;
}

// ============================================================
// WiFi
// ============================================================

bool wifi_connect(void) {
    printf("\n📡 Inicializando WiFi...\n");
    
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_BRAZIL)) {
        printf("❌ Erro ao inicializar chip WiFi\n");
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    
    // Aguarda estabilização do chip WiFi
    sleep_ms(3000);
    
    // Scan de redes para diagnóstico
    printf("\n🔍 Escaneando redes WiFi...\n");
    cyw43_wifi_scan_options_t scan_options = {0};
    int scan_result = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, NULL);
    sleep_ms(5000);  // Aguarda scan completar
    
    printf("📶 Conectando a: [%s]\n", WIFI_SSID);
    printf("   Senha: [%s]\n", WIFI_PASSWORD);
    printf("   Aguardando...\n");
    
    // Tenta 3 vezes com diferentes autenticações
    int result = -1;
    
    // 1. WPA2 AES (mais comum em hotspots modernos)
    printf("   Tentativa 1: WPA2 AES...\n");
    result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );
    
    if (result != 0) {
        sleep_ms(2000);
        printf("   Tentativa 2: WPA2 MIXED...\n");
        result = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_MIXED_PSK,
            30000
        );
    }
    
    if (result != 0) {
        sleep_ms(2000);
        printf("   Tentativa 3: WPA...\n");
        result = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA_TKIP_PSK,
            30000
        );
    }
    
    if (result != 0) {
        printf("❌ Falha na conexão WiFi (erro: %d)\n", result);
        printf("   -1 = Rede não encontrada\n");
        printf("   -2 = Autenticação falhou\n");
        printf("   -3 = Timeout\n");
        return false;
    }
    
    // Obtém IP
    struct netif *netif = netif_default;
    if (netif && netif->ip_addr.addr) {
        snprintf(g_ip_address, sizeof(g_ip_address), "%s", 
                 ip4addr_ntoa(&netif->ip_addr));
    }
    
    printf("✅ Conectado!\n");
    printf("📍 IP: %s\n", g_ip_address);
    
    g_wifi_connected = true;
    return true;
}

// ============================================================
// ATUALIZAÇÃO DO DISPLAY
// ============================================================

void update_display(void) {
    if (!g_oled_ok) return;
    
    oled_clear();
    
    char buf[24];
    
    // Título
    oled_print(20, 0, "HYDROSENSE");
    
    // Linha separadora
    for (int x = 0; x < 128; x++) {
        oled_pixel(x, 10, true);
    }
    
    // Temperatura e Umidade
    snprintf(buf, sizeof(buf), "T:%.1fC", g_temperatura);
    oled_print(0, 14, buf);
    snprintf(buf, sizeof(buf), "H:%.0f%%", g_umidade);
    oled_print(70, 14, buf);
    
    // Distância e Nível
    snprintf(buf, sizeof(buf), "D:%dmm", g_distancia_mm);
    oled_print(0, 26, buf);
    snprintf(buf, sizeof(buf), "N:%.0f%%", g_nivel_percent);
    oled_print(70, 26, buf);
    
    // Volume
    snprintf(buf, sizeof(buf), "VOL:%.1fL", g_volume_litros);
    oled_print(0, 38, buf);
    
    // Status WiFi
    if (g_wifi_connected) {
        oled_print(0, 52, g_ip_address);
    } else {
        oled_print(0, 52, "WIFI OFF");
    }
    
    // Contador
    snprintf(buf, sizeof(buf), "#%lu", g_leitura_count);
    oled_print(90, 52, buf);
    
    oled_update();
}

// ============================================================
// LEITURA DOS SENSORES
// ============================================================

void read_sensors(void) {
    g_leitura_count++;
    
    // Lê AHT10
    float temp, hum;
    if (aht10_read(&temp, &hum)) {
        g_temperatura = temp;
        g_umidade = hum;
        g_aht_ok = true;
    } else {
        g_aht_ok = false;
    }
    
    // Lê VL53L0X
    if (g_vl53_ok) {
        uint16_t dist = vl53_read_distance();
        if (dist != 0xFFFF && dist < 2000) {
            g_distancia_mm = dist;
            
            // Calcula nível (invertido - maior distância = menos água)
            float dist_cm = dist / 10.0f;
            g_nivel_percent = ((TANK_HEIGHT_CM - dist_cm - SENSOR_OFFSET_CM) / TANK_HEIGHT_CM) * 100.0f;
            
            if (g_nivel_percent < 0) g_nivel_percent = 0;
            if (g_nivel_percent > 100) g_nivel_percent = 100;
            
            g_volume_litros = (g_nivel_percent / 100.0f) * TANK_CAPACITY_L;
        }
    }
    
    // Log
    printf("[%lu] T=%.1f°C H=%.0f%% D=%dmm N=%.0f%% V=%.1fL\n",
           g_leitura_count, g_temperatura, g_umidade,
           g_distancia_mm, g_nivel_percent, g_volume_litros);
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();
    sleep_ms(3000);  // Aguarda USB estabilizar
    
    printf("\n\n\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     HydroSense v7 - IoT Web Server     ║\n");
    printf("║   Sistema de Aquicultura Inteligente   ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf(">>> Firmware iniciado! <<<\n\n");
    
    // Inicializa periféricos
    printf("🔧 Inicializando hardware...\n");
    
    led_rgb_init();
    led_rgb_set(1, 0, 0);  // LED vermelho durante init
    
    buzzer_init();
    buzzer_beep(100);
    
    servo_init();
    
    // Inicializa OLED
    printf("   📺 OLED: ");
    g_oled_ok = oled_init();
    printf("%s\n", g_oled_ok ? "OK" : "ERRO");
    
    if (g_oled_ok) {
        oled_clear();
        oled_print_large(10, 5, "HYDRO");
        oled_print_large(10, 25, "SENSE");
        oled_print(20, 50, "INICIANDO...");
        oled_update();
    }
    
    // Inicializa sensores
    printf("   🌡️ AHT10: ");
    g_aht_ok = aht10_init();
    printf("%s\n", g_aht_ok ? "OK" : "ERRO");
    
    printf("   📏 VL53L0X: ");
    g_vl53_ok = vl53_init();
    printf("%s\n", g_vl53_ok ? "OK" : "ERRO");
    
    // Teste do servo
    printf("   🔄 Servo: ");
    servo_angle(0);
    sleep_ms(300);
    servo_angle(90);
    sleep_ms(300);
    servo_angle(0);
    servo_stop();
    printf("OK\n");
    
    // Conecta WiFi
    led_rgb_set(1, 1, 0);  // Amarelo = conectando
    
    if (g_oled_ok) {
        oled_clear();
        oled_print(0, 0, "CONECTANDO WIFI");
        oled_print(0, 16, WIFI_SSID);
        oled_update();
    }
    
    if (wifi_connect()) {
        led_rgb_set(0, 1, 0);  // Verde = conectado
        buzzer_beep(50);
        sleep_ms(50);
        buzzer_beep(50);
        
        // Inicia servidor web
        if (start_web_server()) {
            printf("\n🌐 ═══════════════════════════════════════\n");
            printf("   Acesse: http://%s\n", g_ip_address);
            printf("   API:    http://%s/api\n", g_ip_address);
            printf("🌐 ═══════════════════════════════════════\n\n");
            
            if (g_oled_ok) {
                oled_clear();
                oled_print(0, 0, "WIFI CONECTADO!");
                oled_print(0, 20, "ACESSE:");
                oled_print(0, 35, g_ip_address);
                oled_update();
                sleep_ms(3000);
            }
        }
    } else {
        led_rgb_set(1, 0, 0);  // Vermelho = erro
        
        if (g_oled_ok) {
            oled_clear();
            oled_print(0, 0, "WIFI ERRO!");
            oled_print(0, 20, "MODO OFFLINE");
            oled_update();
            sleep_ms(2000);
        }
    }
    
    // Loop principal
    printf("\n=== MONITORAMENTO INICIADO ===\n\n");
    
    while (true) {
        // Lê sensores
        read_sensors();
        
        // Atualiza display
        update_display();
        
        // LED indica status
        if (g_wifi_connected) {
            // Pisca verde quando lendo
            led_rgb_set(0, 1, 0);
            sleep_ms(100);
            led_rgb_set(0, 0, 0);
        }
        
        // Processa WiFi
        if (g_wifi_connected) {
            cyw43_arch_poll();
        }
        
        sleep_ms(1900);  // ~2s entre leituras
    }
    
    return 0;
}
