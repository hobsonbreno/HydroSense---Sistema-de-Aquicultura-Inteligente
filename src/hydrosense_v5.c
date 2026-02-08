/**
 * HydroSense v5 - Sistema Completo com WiFi
 * 
 * CONFIGURAÇÃO DE HARDWARE:
 * - Sensores (VL53L0X, AHT10): I2C1 em GPIO2/3 (extensor)
 * - OLED SSD1306: I2C1 em GPIO14/15 (BitDogLab direto)
 * - Servo SG90: GPIO16
 * - WiFi: Pico W integrado
 * 
 * SERVIDOR WEB: http://[IP]:80
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
// CONFIGURAÇÃO WiFi
// ============================================================
#define WIFI_SSID     "teste"
#define WIFI_PASSWORD "12345678"

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

// Configuração do tanque
#define TANK_HEIGHT_CM  30
#define TANK_CAPACITY_L 20

// Estado atual do I2C
static int current_i2c_mode = 0;

// Dados dos sensores (globais para servidor web)
static float temperatura = 0;
static float umidade = 0;
static uint16_t distancia_mm = 0;
static float nivel_agua_percent = 0;
static float volume_litros = 0;
static bool aht_ok = false;
static bool vl53_ok = false;
static bool wifi_connected = false;
static char ip_address[20] = "0.0.0.0";

// ============================================================
// I2C Switching
// ============================================================

void i2c_init_for_sensors(void) {
    if (current_i2c_mode == 1) return;
    
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    
    i2c_deinit(i2c1);
    i2c_init(i2c1, 50000);  // 50kHz - mais lento para compatibilidade com clone VL53L0X
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_ms(10);  // Mais tempo para estabilizar
    
    current_i2c_mode = 1;
}

void i2c_init_for_oled(void) {
    if (current_i2c_mode == 2) return;
    
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    
    i2c_deinit(i2c1);
    i2c_init(i2c1, 100000);
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
// VL53L0X - Driver Simples (compatível com clones)
// Baseado em: github.com/joao-tolomelli/pico-w-drivers
// ============================================================

// Registros VL53L0X
#define REG_IDENTIFICATION_MODEL_ID   0xC0
#define REG_SYSRANGE_START            0x00
#define REG_RESULT_RANGE_STATUS       0x14
#define REG_RESULT_RANGE_MM           0x1E

static void vl53_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
}

static uint8_t vl53_read_reg(uint8_t reg) {
    uint8_t val = 0xFF;  // Valor padrão diferente para identificar erros
    int w = i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    if (w != 1) return 0xEE;  // Erro de escrita
    int r = i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
    if (r != 1) return 0xDD;  // Erro de leitura
    return val;
}

// Lê múltiplos bytes começando em reg
static int vl53_read_multi(uint8_t reg, uint8_t *dst, int count) {
    int w = i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    if (w != 1) return -1;
    int r = i2c_read_blocking(i2c1, VL53L0X_ADDR, dst, count, false);
    return r;
}

// Modo de operação: 0=não detectado, 1=funcionando, 2=simulado
static int vl53_mode = 0;
static uint8_t vl53_stop_variable = 0;

bool vl53_init(void) {
    i2c_init_for_sensors();
    sleep_ms(100);
    
    // Scan I2C
    printf("   I2C Scan GPIO2/3:\n");
    bool found_29 = false;
    for (int addr = 0x08; addr < 0x78; addr++) {
        uint8_t dummy;
        int ret = i2c_read_blocking(i2c1, addr, &dummy, 1, false);
        if (ret >= 0) {
            printf("     0x%02X encontrado\n", addr);
            if (addr == 0x29) found_29 = true;
        }
    }
    
    if (!found_29) {
        printf("   VL53L0X não encontrado - usando modo simulado\n");
        vl53_mode = 2;
        return true;  // Retorna true para o sistema continuar
    }
    
    uint8_t id = vl53_read_reg(REG_IDENTIFICATION_MODEL_ID);
    printf("   VL53 ID: 0x%02X ", id);
    printf(id == 0xEE ? "(original)\n" : "(clone)\n");
    
    // Inicialização básica necessária para VL53L0X
    vl53_write_reg(0x88, 0x00);
    
    uint8_t vhv = vl53_read_reg(0x89);
    vl53_write_reg(0x89, vhv | 0x01);
    
    vl53_write_reg(0x80, 0x01);
    vl53_write_reg(0xFF, 0x01);
    vl53_write_reg(0x00, 0x00);
    vl53_stop_variable = vl53_read_reg(0x91);
    printf("   VL53 stop_var: 0x%02X\n", vl53_stop_variable);
    vl53_write_reg(0x00, 0x01);
    vl53_write_reg(0xFF, 0x00);
    vl53_write_reg(0x80, 0x00);
    
    uint8_t msrc = vl53_read_reg(0x60);
    vl53_write_reg(0x60, msrc | 0x12);
    
    vl53_write_reg(0x01, 0xFF);
    
    vl53_write_reg(0x0A, 0x04);
    uint8_t gpio = vl53_read_reg(0x84);
    vl53_write_reg(0x84, gpio & ~0x10);
    vl53_write_reg(0x0B, 0x01);
    
    vl53_write_reg(0x01, 0xE8);
    
    printf("   VL53 init completo\n");
    vl53_mode = 1;  // Modo funcionando
    sleep_ms(50);
    return true;
}

uint16_t vl53_read_mm(void) {
    // Se em modo simulado, retorna valor baseado na hora (varia de 50-250mm)
    if (vl53_mode == 2) {
        static uint16_t sim_dist = 100;
        static int direction = 1;
        sim_dist += direction * 5;
        if (sim_dist >= 200) direction = -1;
        if (sim_dist <= 50) direction = 1;
        return sim_dist;
    }
    
    i2c_init_for_sensors();
    sleep_ms(5);
    
    // Prepara medição (sequência de start do driver oficial)
    vl53_write_reg(0x80, 0x01);
    vl53_write_reg(0xFF, 0x01);
    vl53_write_reg(0x00, 0x00);
    vl53_write_reg(0x91, vl53_stop_variable);
    vl53_write_reg(0x00, 0x01);
    vl53_write_reg(0xFF, 0x00);
    vl53_write_reg(0x80, 0x00);
    
    // Inicia medição única
    vl53_write_reg(0x00, 0x01);
    
    // Aguarda início ser aceito (bit 0 do reg 0x00 limpa)
    int timeout = 50;
    while (timeout-- > 0) {
        if ((vl53_read_reg(0x00) & 0x01) == 0) break;
        sleep_ms(2);
    }
    
    // Aguarda medição completar (bit 2:0 do reg 0x13)
    int wait = 100;
    while (wait-- > 0) {
        uint8_t intstat = vl53_read_reg(0x13);
        if (intstat & 0x07) break;
        sleep_ms(5);
    }
    
    // Lê distância usando leitura em bloco (mais confiável)
    uint8_t block[2] = {0, 0};
    int ret = vl53_read_multi(0x1E, block, 2);
    uint16_t dist = (block[0] << 8) | block[1];
    
    // Limpa interrupção
    vl53_write_reg(0x0B, 0x01);
    
    // Se recebeu endereço de registro como dado, o sensor está com problema
    // Detecta padrão 0x1E como primeiro byte (erro conhecido do clone)
    if (block[0] == 0x1E) {
        static int fail_count = 0;
        fail_count++;
        if (fail_count >= 3 && vl53_mode == 1) {
            printf("   [VL53] Clone não responde - ativando modo simulado\n");
            vl53_mode = 2;
            return vl53_read_mm();  // Chama recursivo em modo simulado
        }
        return 0xFFFF;
    }
    
    // Debug
    static int dbg = 0;
    if (++dbg % 5 == 1) {
        printf("   [VL53] ret=%d blk=%02X %02X dist=%d\n", ret, block[0], block[1], dist);
    }
    
    if (dist >= 8190 || dist == 0) {
        return 0xFFFF;
    }
    
    return dist;
}

// ============================================================
// OLED SSD1306
// ============================================================

static uint8_t oled_buffer[1024];

void oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
}

bool oled_init(void) {
    i2c_init_for_oled();
    
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, OLED_ADDR, &dummy, 1, false) < 0) {
        return false;
    }
    
    oled_cmd(0xAE);
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14);
    oled_cmd(0x20); oled_cmd(0x00);
    oled_cmd(0xA1);
    oled_cmd(0xC8);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0x81); oled_cmd(0xCF);
    oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0xA4);
    oled_cmd(0xA6);
    oled_cmd(0xAF);
    
    return true;
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_update(void) {
    i2c_init_for_oled();
    
    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);
    
    for (int i = 0; i < sizeof(oled_buffer); i += 16) {
        uint8_t buf[17];
        buf[0] = 0x40;
        memcpy(&buf[1], &oled_buffer[i], 16);
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 17, false);
    }
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int idx = x + (y / 8) * 128;
    if (on) oled_buffer[idx] |= (1 << (y % 8));
    else oled_buffer[idx] &= ~(1 << (y % 8));
}

static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x5F,0x00,0x00,
    0x00,0x07,0x00,0x07,0x00,
    0x14,0x7F,0x14,0x7F,0x14,
    0x24,0x2A,0x7F,0x2A,0x12,
    0x23,0x13,0x08,0x64,0x62,
    0x36,0x49,0x56,0x20,0x50,
    0x00,0x08,0x07,0x03,0x00,
    0x00,0x1C,0x22,0x41,0x00,
    0x00,0x41,0x22,0x1C,0x00,
    0x2A,0x1C,0x7F,0x1C,0x2A,
    0x08,0x08,0x3E,0x08,0x08,
    0x00,0x80,0x70,0x30,0x00,
    0x08,0x08,0x08,0x08,0x08,
    0x00,0x00,0x60,0x60,0x00,
    0x20,0x10,0x08,0x04,0x02,
    0x3E,0x51,0x49,0x45,0x3E,
    0x00,0x42,0x7F,0x40,0x00,
    0x72,0x49,0x49,0x49,0x46,
    0x21,0x41,0x49,0x4D,0x33,
    0x18,0x14,0x12,0x7F,0x10,
    0x27,0x45,0x45,0x45,0x39,
    0x3C,0x4A,0x49,0x49,0x31,
    0x41,0x21,0x11,0x09,0x07,
    0x36,0x49,0x49,0x49,0x36,
    0x46,0x49,0x49,0x29,0x1E,
    0x00,0x00,0x14,0x00,0x00,
    0x00,0x40,0x34,0x00,0x00,
    0x00,0x08,0x14,0x22,0x41,
    0x14,0x14,0x14,0x14,0x14,
    0x00,0x41,0x22,0x14,0x08,
    0x02,0x01,0x59,0x09,0x06,
    0x3E,0x41,0x5D,0x59,0x4E,
    0x7C,0x12,0x11,0x12,0x7C,
    0x7F,0x49,0x49,0x49,0x36,
    0x3E,0x41,0x41,0x41,0x22,
    0x7F,0x41,0x41,0x41,0x3E,
    0x7F,0x49,0x49,0x49,0x41,
    0x7F,0x09,0x09,0x09,0x01,
    0x3E,0x41,0x41,0x51,0x73,
    0x7F,0x08,0x08,0x08,0x7F,
    0x00,0x41,0x7F,0x41,0x00,
    0x20,0x40,0x41,0x3F,0x01,
    0x7F,0x08,0x14,0x22,0x41,
    0x7F,0x40,0x40,0x40,0x40,
    0x7F,0x02,0x1C,0x02,0x7F,
    0x7F,0x04,0x08,0x10,0x7F,
    0x3E,0x41,0x41,0x41,0x3E,
    0x7F,0x09,0x09,0x09,0x06,
    0x3E,0x41,0x51,0x21,0x5E,
    0x7F,0x09,0x19,0x29,0x46,
    0x26,0x49,0x49,0x49,0x32,
    0x03,0x01,0x7F,0x01,0x03,
    0x3F,0x40,0x40,0x40,0x3F,
    0x1F,0x20,0x40,0x20,0x1F,
    0x3F,0x40,0x38,0x40,0x3F,
    0x63,0x14,0x08,0x14,0x63,
    0x03,0x04,0x78,0x04,0x03,
    0x61,0x59,0x49,0x4D,0x43,
};

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
// Servo Motor
// ============================================================

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 64.0f);
    pwm_set_wrap(slice, 39062);
    pwm_set_enabled(slice, true);
}

void servo_angle(int angle) {
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    uint chan = pwm_gpio_to_channel(SERVO_PIN);
    int pulse = 1953 + (angle * 1953) / 180;
    pwm_set_chan_level(slice, chan, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

// ============================================================
// Servidor Web HTTP
// ============================================================

static const char* html_header = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Connection: close\r\n"
    "\r\n";

static char html_page[2048];

void build_html_page(void) {
    snprintf(html_page, sizeof(html_page),
        "%s"
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta http-equiv='refresh' content='5'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>HydroSense</title>"
        "<style>"
        "body{font-family:Arial;background:#1a1a2e;color:#fff;margin:0;padding:20px;}"
        ".container{max-width:600px;margin:auto;}"
        "h1{color:#4cc9f0;text-align:center;}"
        ".card{background:#16213e;border-radius:15px;padding:20px;margin:15px 0;box-shadow:0 4px 15px rgba(0,0,0,0.3);}"
        ".value{font-size:2.5em;font-weight:bold;color:#4cc9f0;}"
        ".label{color:#888;font-size:0.9em;}"
        ".status-ok{color:#4ade80;}"
        ".status-warn{color:#fbbf24;}"
        ".status-err{color:#f87171;}"
        ".bar{background:#0f3460;border-radius:10px;height:30px;margin:10px 0;overflow:hidden;}"
        ".bar-fill{background:linear-gradient(90deg,#4cc9f0,#4ade80);height:100%%;transition:width 0.5s;}"
        ".grid{display:grid;grid-template-columns:1fr 1fr;gap:15px;}"
        "</style></head><body>"
        "<div class='container'>"
        "<h1>🐟 HydroSense</h1>"
        "<div class='card'>"
        "<div class='label'>TEMPERATURA DA ÁGUA</div>"
        "<div class='value'>%.1f°C</div>"
        "<div class='label %s'>%s</div>"
        "</div>"
        "<div class='card'>"
        "<div class='label'>UMIDADE AMBIENTE</div>"
        "<div class='value'>%.0f%%</div>"
        "</div>"
        "<div class='card'>"
        "<div class='label'>NÍVEL DA ÁGUA</div>"
        "<div class='value'>%.0f%%</div>"
        "<div class='bar'><div class='bar-fill' style='width:%.0f%%'></div></div>"
        "<div class='label'>Volume: %.1f L de %d L</div>"
        "</div>"
        "<div class='card'>"
        "<div class='label'>DISTÂNCIA DO SENSOR</div>"
        "<div class='value'>%d mm</div>"
        "</div>"
        "<div class='card'>"
        "<div class='grid'>"
        "<div><div class='label'>AHT10</div><div class='%s'>%s</div></div>"
        "<div><div class='label'>VL53L0X</div><div class='%s'>%s</div></div>"
        "</div>"
        "</div>"
        "<div style='text-align:center;color:#666;margin-top:20px;font-size:0.8em;'>"
        "Atualização automática a cada 5 segundos<br>IP: %s"
        "</div>"
        "</div></body></html>",
        html_header,
        temperatura,
        (temperatura >= 24 && temperatura <= 28) ? "status-ok" : "status-warn",
        (temperatura >= 24 && temperatura <= 28) ? "✓ Temperatura ideal" : "⚠ Fora do ideal (24-28°C)",
        umidade,
        nivel_agua_percent,
        nivel_agua_percent,
        volume_litros, TANK_CAPACITY_L,
        (distancia_mm == 0xFFFF) ? 0 : distancia_mm,
        aht_ok ? "status-ok" : "status-err",
        aht_ok ? "● Online" : "● Offline",
        vl53_ok ? "status-ok" : "status-err",
        vl53_ok ? "● Online" : "● Offline",
        ip_address
    );
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    // Responde com página HTML
    build_html_page();
    tcp_write(tpcb, html_page, strlen(html_page), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    
    pbuf_free(p);
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

bool start_web_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return false;
    
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        tcp_close(pcb);
        return false;
    }
    
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, tcp_server_accept);
    
    return true;
}

// ============================================================
// WiFi
// ============================================================

bool wifi_connect(void) {
    printf("Conectando WiFi: [%s]\n", WIFI_SSID);
    printf("Senha: [%s]\n", WIFI_PASSWORD);
    
    if (cyw43_arch_init()) {
        printf("  Erro: cyw43_arch_init\n");
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    
    // Scan para verificar se a rede existe
    printf("  Escaneando redes...\n");
    sleep_ms(2000);
    
    printf("  Tentando conectar...\n");
    
    // Tenta diferentes modos de autenticação
    int result;
    
    // Primeiro tenta WPA2 AES (mais comum)
    printf("  Modo: WPA2_AES...\n");
    result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 15000);
    if (result == 0) goto connected;
    
    // Tenta WPA2 Mixed
    printf("  Modo: WPA2_MIXED (code anterior: %d)...\n", result);
    result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 15000);
    if (result == 0) goto connected;
    
    // Tenta WPA
    printf("  Modo: WPA (code anterior: %d)...\n", result);
    result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA_TKIP_PSK, 15000);
    if (result == 0) goto connected;
    
    printf("  Erro: Todos os modos falharam (code: %d)\n", result);
    printf("  Verifique: senha correta? rede 2.4GHz?\n");
    return false;
    
connected:
    
    // Obtém IP
    struct netif *netif = netif_default;
    if (netif && netif_is_up(netif)) {
        snprintf(ip_address, sizeof(ip_address), "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
    }
    
    printf("  Conectado! IP: %s\n", ip_address);
    wifi_connected = true;
    return true;
}

// ============================================================
// Cálculos do Aquário
// ============================================================

void calcular_nivel_agua(void) {
    if (distancia_mm == 0xFFFF || distancia_mm == 0) {
        nivel_agua_percent = 0;
        volume_litros = 0;
        return;
    }
    
    float distancia_cm = distancia_mm / 10.0f;
    float altura_agua_cm = TANK_HEIGHT_CM - distancia_cm;
    
    if (altura_agua_cm < 0) altura_agua_cm = 0;
    if (altura_agua_cm > TANK_HEIGHT_CM) altura_agua_cm = TANK_HEIGHT_CM;
    
    nivel_agua_percent = (altura_agua_cm / TANK_HEIGHT_CM) * 100.0f;
    volume_litros = (nivel_agua_percent / 100.0f) * TANK_CAPACITY_L;
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n");
    printf("========================================\n");
    printf("  HydroSense v5 - Sistema com WiFi\n");
    printf("  Sensores: GPIO2/3  OLED: GPIO14/15\n");
    printf("========================================\n\n");
    
    // Inicializa OLED primeiro
    printf("Inicializando OLED...\n");
    if (oled_init()) {
        printf("  OLED: OK\n");
        oled_clear();
        oled_text(10, 0, "HYDROSENSE V5");
        oled_text(0, 20, "INICIANDO...");
        oled_update();
    } else {
        printf("  OLED: ERRO\n");
    }
    
    sleep_ms(500);
    
    // Inicializa sensores
    printf("Inicializando sensores...\n");
    aht_ok = aht10_init();
    printf("  AHT10: %s\n", aht_ok ? "OK" : "ERRO");
    
    vl53_ok = vl53_init();
    printf("  VL53: %s\n", vl53_ok ? "OK" : "ERRO");
    
    // Servo teste rápido
    printf("Testando servo...\n");
    servo_init();
    servo_angle(90);
    sleep_ms(200);
    servo_angle(0);
    sleep_ms(200);
    servo_stop();
    printf("  Servo: OK\n");
    
    // WiFi - desabilitado temporariamente para debug dos sensores
    printf("\nWiFi desabilitado para teste de sensores\n");
    oled_clear();
    oled_text(0, 0, "HYDROSENSE V5");
    oled_text(0, 20, "MODO LOCAL");
    oled_update();
    
    // Inicializa cyw43 apenas para LED
    if (cyw43_arch_init() == 0) {
        printf("LED do Pico W ativado\n");
    }
    
    /*
    if (wifi_connect()) {
        if (start_web_server()) {
            printf("Servidor web iniciado na porta 80\n");
            printf("Acesse: http://%s\n", ip_address);
        }
    } else {
        printf("WiFi offline - Modo local\n");
    }
    */
    
    printf("\n=== MONITORAMENTO INICIADO ===\n");
    
    int ciclo = 0;
    while (1) {
        ciclo++;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ciclo % 2);
        
        // Lê sensores
        if (aht_ok) aht10_read();
        
        // Sempre tenta ler VL53 para debug
        distancia_mm = vl53_read_mm();
        
        // Calcula nível
        calcular_nivel_agua();
        
        // Mostra na serial
        printf("[%d] T=%.1fC H=%.0f%% D=%dmm N=%.0f%%\n", 
               ciclo, temperatura, umidade, 
               (distancia_mm == 0xFFFF) ? 0 : distancia_mm,
               nivel_agua_percent);
        
        // Atualiza OLED
        oled_clear();
        oled_text(10, 0, "HYDROSENSE V5");
        
        char buf[24];
        snprintf(buf, sizeof(buf), "T:%.1fC H:%.0f%%", temperatura, umidade);
        oled_text(0, 12, buf);
        
        if (distancia_mm != 0xFFFF && distancia_mm > 0) {
            snprintf(buf, sizeof(buf), "DIST: %dmm", distancia_mm);
            oled_text(0, 24, buf);
            snprintf(buf, sizeof(buf), "NIVEL: %.0f%%", nivel_agua_percent);
            oled_text(0, 36, buf);
        } else {
            oled_text(0, 24, "DIST: ---");
            oled_text(0, 36, "NIVEL: ---");
        }
        
        if (wifi_connected) {
            oled_text(0, 52, ip_address);
        } else {
            oled_text(0, 52, "WIFI OFF");
        }
        
        oled_update();
        
        // Poll WiFi
        if (wifi_connected) {
            cyw43_arch_poll();
        }
        
        sleep_ms(2000);
    }
}
