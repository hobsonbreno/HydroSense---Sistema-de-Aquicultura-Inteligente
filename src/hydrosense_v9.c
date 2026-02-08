/**
 * HydroSense v9 - WiFi SIMPLIFICADO
 * Baseado na abordagem do MicroPython (sem scan)
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
#include <math.h>

// ============================================================
// 🔧 CONFIGURAÇÃO WiFi SIMPLES
// ============================================================
#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"
#define WIFI_TIMEOUT_MS 30000  // 30 segundos

// ============================================================
// PINOS - BitDogLab
// ============================================================
#define SENSOR_SDA      2
#define SENSOR_SCL      3
#define OLED_SDA        14
#define OLED_SCL        15
#define SERVO_PIN       16
#define LED_R_PIN       11
#define LED_G_PIN       12
#define LED_B_PIN       13
#define BUZZER_PIN      21

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
#define SENSOR_OFFSET_CM 2.0f

// ============================================================
// VARIÁVEIS GLOBAIS
// ============================================================
static int current_i2c_mode = 0;
static volatile float g_temperatura = 25.5f;    // Valores demo
static volatile float g_umidade = 65.0f;
static volatile uint16_t g_distancia_mm = 120;
static volatile float g_nivel_percent = 75.0f;
static volatile float g_volume_litros = 15.0f;
static volatile bool g_aht_ok = true;
static volatile bool g_vl53_ok = true;
static volatile bool g_oled_ok = false;
static volatile bool g_wifi_connected = false;
static volatile uint32_t g_leitura_count = 0;
static char g_ip_address[20] = "0.0.0.0";

// ============================================================
// HARDWARE BÁSICO
// ============================================================
void led_rgb_init(void) {
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
}

void led_rgb_set(int r, int g, int b) {
    gpio_put(LED_R_PIN, r);
    gpio_put(LED_G_PIN, g);
    gpio_put(LED_B_PIN, b);
}

void buzzer_init(void) {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

void buzzer_beep(int ms) {
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(ms);
    gpio_put(BUZZER_PIN, 0);
}

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, 20000);
    pwm_init(slice, &config, true);
}

void servo_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    uint16_t pulse = 1000 + (angle * 1000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

// ============================================================
// PÁGINA WEB SIMPLES (menor)
// ============================================================
static const char HTML_PAGE[] =
"<!DOCTYPE html>"
"<html><head><title>HydroSense</title>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<style>"
"body{font-family:Arial;margin:20px;background:#f0f8ff}"
".card{background:white;padding:20px;margin:10px 0;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}"
"h1{color:#2196F3;text-align:center}"
".data{display:grid;grid-template-columns:repeat(auto-fit,minmax(100px,1fr));gap:10px;margin:20px 0}"
".item{text-align:center;padding:15px;background:#f8f9fa;border-radius:5px}"
".value{font-size:24px;font-weight:bold;color:#2196F3}"
".label{font-size:12px;color:#666}"
".btn{padding:10px 20px;background:#2196F3;color:white;border:none;border-radius:5px;cursor:pointer;margin:5px}"
"</style></head><body>"
"<div class='card'>"
"<h1>🐟 HydroSense</h1>"
"<div class='data'>"
"<div class='item'><div class='value' id='temp'>--</div><div class='label'>Temperatura °C</div></div>"
"<div class='item'><div class='value' id='hum'>--</div><div class='label'>Umidade %</div></div>"
"<div class='item'><div class='value' id='dist'>--</div><div class='label'>Distância mm</div></div>"
"<div class='item'><div class='value' id='nivel'>--</div><div class='label'>Nível %</div></div>"
"<div class='item'><div class='value' id='vol'>--</div><div class='label'>Volume L</div></div>"
"</div>"
"<div style='text-align:center'>"
"<button class='btn' onclick='feed()'>🍽️ Alimentar</button>"
"<button class='btn' onclick='location.reload()'>🔄 Atualizar</button>"
"</div>"
"<div style='text-align:center;margin-top:20px;color:#666'>"
"<span id='status'>WiFi: --</span> | Leituras: <span id='count'>0</span>"
"</div>"
"</div>"
"<script>"
"function update(){"
"console.log('Atualizando dados...');"
"fetch('/api').then(r=>{"
"console.log('Resposta recebida:',r);"
"return r.json();"
"}).then(d=>{"
"console.log('Dados:',d);"
"document.getElementById('temp').textContent=d.temperatura.toFixed(1);"
"document.getElementById('hum').textContent=d.umidade.toFixed(0);"
"document.getElementById('dist').textContent=d.distancia;"
"document.getElementById('nivel').textContent=d.nivel.toFixed(0);"
"document.getElementById('vol').textContent=d.volume.toFixed(1);"
"document.getElementById('count').textContent=d.leituras;"
"document.getElementById('status').textContent='WiFi: '+(d.wifi?'OK':'OFF');"
"}).catch(e=>{console.log('Erro:',e);})}"
"function feed(){fetch('/feed').then(()=>alert('Alimentado!'))}"
"window.onload=function(){update();setInterval(update,3000);};"
"</script>"
"</body></html>";

static char http_response[4096];  // Menor buffer
static char json_buffer[256];    // Menor buffer
static uint8_t oled_buffer[1024]; // Buffer OLED

// ============================================================
// SERVIDOR WEB
// ============================================================
void build_json_response(void) {
    snprintf(json_buffer, sizeof(json_buffer),
        "{\"temperatura\":%.1f,\"umidade\":%.0f,\"distancia\":%d,\"nivel\":%.0f,\"volume\":%.1f,"
        "\"wifi\":%s,\"leituras\":%lu}",
        g_temperatura, g_umidade, g_distancia_mm, g_nivel_percent, g_volume_litros,
        g_wifi_connected ? "true" : "false",
        g_leitura_count
    );
}

static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("   ✅ Dados enviados (%d bytes)\n", len);
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    printf("🌐 Requisição recebida!\n");
    
    if (!p) {
        printf("   Fechando conexão (p=NULL)\n");
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    char *request = (char*)p->payload;
    printf("   Request: %.50s\n", request);
    
    // Configura callback para quando dados forem enviados
    tcp_sent(tpcb, tcp_sent_callback);
    
    if (strstr(request, "GET /api")) {
        printf("   -> Enviando API JSON\n");
        // API JSON
        build_json_response();
        printf("   JSON: %s\n", json_buffer);  // Debug
        char response[600];
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buffer), json_buffer);
        
        err_t write_err = tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
        printf("   tcp_write result: %d, response_len: %d\n", write_err, (int)strlen(response));
        printf("   tcp_write result: %d\n", write_err);
    }
    else if (strstr(request, "GET /feed")) {
        printf("   -> Comando alimentação\n");
        // Alimentar
        printf("🍽️ Alimentação acionada!\n");
        servo_angle(90);
        sleep_ms(500);  // Reduzido para não bloquear muito
        servo_angle(0);
        servo_stop();
        
        const char *resp = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 2\r\n\r\nOK";
        err_t write_err = tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
        printf("   tcp_write result: %d\n", write_err);
    }
    else {
        printf("   -> Enviando página HTML\n");
        // Página HTML
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(HTML_PAGE), HTML_PAGE);
        
        err_t write_err = tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
        printf("   tcp_write result: %d\n", write_err);
    }
    
    err_t output_err = tcp_output(tpcb);
    printf("   tcp_output result: %d\n", output_err);
    
    pbuf_free(p);
    
    // NÃO feche aqui - será fechado no callback tcp_sent_callback
    printf("   Aguardando confirmação de envio...\n");
    return ERR_OK;
}

static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_recv_callback);
    return ERR_OK;
}

bool start_web_server(void) {
    struct tcp_pcb *server_pcb = tcp_new();
    if (!server_pcb) {
        printf("❌ Erro ao criar PCB\n");
        return false;
    }
    
    printf("   Fazendo bind na porta 80...\n");
    if (tcp_bind(server_pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("❌ Erro no bind porta 80\n");
        tcp_close(server_pcb);
        return false;
    }
    
    printf("   Colocando em modo listen...\n");
    server_pcb = tcp_listen(server_pcb);
    if (!server_pcb) {
        printf("❌ Erro no listen\n");
        return false;
    }
    
    tcp_accept(server_pcb, tcp_accept_callback);
    
    printf("✅ Servidor web rodando na porta 80\n");
    printf("📡 Aguardando conexões...\n");
    return true;
}

// ============================================================
// WiFi SIMPLIFICADO (baseado no Python)
// ============================================================
bool wifi_connect_simple(void) {
    printf("\n📡 Inicializando WiFi SIMPLES...\n");
    
    // Inicializa chip WiFi
    if (cyw43_arch_init()) {
        printf("❌ Erro ao inicializar chip WiFi\n");
        return false;
    }
    
    // Ativa modo estação
    cyw43_arch_enable_sta_mode();
    
    printf("📶 Conectando à rede: [%s]\n", WIFI_SSID);
    printf("   Senha: [%s]\n", WIFI_PASSWORD);
    
    // Conexão DIRETA - sem scan, sem múltiplas tentativas
    printf("   Conectando diretamente...\n");
    
    int result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,  // Só WPA2 AES
        WIFI_TIMEOUT_MS
    );
    
    if (result == 0) {
        // Sucesso!
        struct netif *netif = netif_default;
        if (netif && netif->ip_addr.addr) {
            snprintf(g_ip_address, sizeof(g_ip_address), "%s", 
                     ip4addr_ntoa(&netif->ip_addr));
        }
        
        printf("✅ WiFi CONECTADO!\n");
        printf("📍 IP: %s\n", g_ip_address);
        printf("🌐 Acesse: http://%s\n", g_ip_address);
        
        g_wifi_connected = true;
        return true;
    }
    
    printf("❌ Falha na conexão (erro: %d)\n", result);
    printf("   -1 = Rede não encontrada\n");
    printf("   -2 = Senha incorreta/segurança incompatível\n");
    printf("   -3 = Timeout (%ds)\n", WIFI_TIMEOUT_MS/1000);
    
    g_wifi_connected = false;
    return false;
}

// ============================================================
// CONTROLE I2C - Baseado no v7 funcional
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
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) return false;
    sleep_ms(80);
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) return false;
    if (data[0] & 0x80) return false;
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    *hum = (float)hum_raw / 1048576.0f * 100.0f;
    *temp = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    return true;
}

// ============================================================
// VL53L0X - Sensor de Distância
// ============================================================
bool vl53_init(void) {
    i2c_switch_to_sensors();
    sleep_ms(50);
    uint8_t dummy;
    int ret = i2c_read_blocking(i2c1, VL53L0X_ADDR, &dummy, 1, false);
    if (ret < 0) return false;
    uint8_t reg = 0xC0, id = 0;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &id, 1, false);
    printf("   VL53L0X ID: 0x%02X\n", id);
    return true;
}

uint16_t vl53_read_distance(void) {
    i2c_switch_to_sensors();
    uint8_t cmd[2] = {0x00, 0x01};
    if (i2c_write_blocking(i2c1, VL53L0X_ADDR, cmd, 2, false) != 2) return 0xFFFF;
    sleep_ms(50);
    uint8_t reg = 0x1E, buf[2] = {0, 0};
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    uint16_t dist = (buf[0] << 8) | buf[1];
    if (dist == 0 || dist > 2000 || buf[0] == 0x1E) return 0xFFFF;
    return dist;
}

// ============================================================
// OLED - Font simplificada
// ============================================================
static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14, 0x24,0x2A,0x7F,0x2A,0x12,
    0x23,0x13,0x08,0x64,0x62, 0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00, 0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00,
    0x14,0x08,0x3E,0x08,0x14, 0x08,0x08,0x3E,0x08,0x08, 0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x60,0x60,0x00,0x00,
    0x20,0x10,0x08,0x04,0x02, 0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31,
    0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03, 0x36,0x49,0x49,0x49,0x36,
    0x06,0x49,0x49,0x29,0x1E, 0x00,0x36,0x36,0x00,0x00, 0x00,0x56,0x36,0x00,0x00, 0x08,0x14,0x22,0x41,0x00, 0x14,0x14,0x14,0x14,0x14,
    0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x51,0x09,0x06, 0x32,0x49,0x79,0x41,0x3E, 0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36,
    0x3E,0x41,0x41,0x41,0x22, 0x7F,0x41,0x41,0x22,0x1C, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x49,0x49,0x7A,
    0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40,
    0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E, 0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E,
    0x7F,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31, 0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F,
    0x3F,0x40,0x38,0x40,0x3F, 0x63,0x14,0x08,0x14,0x63, 0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43,
};

bool oled_init(void) {
    i2c_switch_to_oled();
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, OLED_ADDR, &dummy, 1, false) < 0) return false;
    uint8_t init_cmds[] = {0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,0x81,0xCF,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0xAF};
    for (int i = 0; i < sizeof(init_cmds); i++) {
        uint8_t buf[2] = {0x00, init_cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    memset(oled_buffer, 0, sizeof(oled_buffer));
    return true;
}

void oled_clear(void) { memset(oled_buffer, 0, sizeof(oled_buffer)); }

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
    else oled_buffer[idx] &= ~(1 << (y % 8));
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
        x += 6; str++;
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
        x += 12; str++;
    }
}

void update_display(void) {
    if (!g_oled_ok) return;
    oled_clear();
    char buf[24];
    oled_print(20, 0, "HYDROSENSE");
    for (int x = 0; x < 128; x++) oled_pixel(x, 10, true);
    snprintf(buf, sizeof(buf), "T:%.1fC", g_temperatura);
    oled_print(0, 14, buf);
    snprintf(buf, sizeof(buf), "H:%.0f%%", g_umidade);
    oled_print(70, 14, buf);
    snprintf(buf, sizeof(buf), "D:%dmm", g_distancia_mm);
    oled_print(0, 26, buf);
    snprintf(buf, sizeof(buf), "N:%.0f%%", g_nivel_percent);
    oled_print(70, 26, buf);
    snprintf(buf, sizeof(buf), "VOL:%.1fL", g_volume_litros);
    oled_print(0, 38, buf);
    if (g_wifi_connected) oled_print(0, 52, g_ip_address);
    else oled_print(0, 52, "WIFI OFF");
    snprintf(buf, sizeof(buf), "#%lu", g_leitura_count);
    oled_print(90, 52, buf);
    oled_update();
}

// ============================================================
// LEITURA DOS SENSORES
// ============================================================
void read_sensors(void) {
    g_leitura_count++;
    float temp, hum;
    if (aht10_read(&temp, &hum)) {
        g_temperatura = temp;
        g_umidade = hum;
        g_aht_ok = true;
    } else g_aht_ok = false;
    
    if (g_vl53_ok) {
        uint16_t dist = vl53_read_distance();
        if (dist != 0xFFFF && dist < 2000) {
            g_distancia_mm = dist;
            float dist_cm = dist / 10.0f;
            g_nivel_percent = ((TANK_HEIGHT_CM - dist_cm - SENSOR_OFFSET_CM) / TANK_HEIGHT_CM) * 100.0f;
            if (g_nivel_percent < 0) g_nivel_percent = 0;
            if (g_nivel_percent > 100) g_nivel_percent = 100;
            g_volume_litros = (g_nivel_percent / 100.0f) * TANK_CAPACITY_L;
        }
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(3000);  // Aguarda USB
    
    printf("\n\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║    HydroSense v9 - WiFi SIMPLES       ║\n");
    printf("║  Sistema de Aquicultura Inteligente    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // Inicializa hardware
    printf("🔧 Inicializando hardware...\n");
    led_rgb_init();
    led_rgb_set(1, 0, 0);  // Vermelho = inicializando
    
    buzzer_init();
    buzzer_beep(200);
    
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
        sleep_ms(2000);
    }
    
    // Inicializa sensores
    printf("   🌡️ AHT10: ");
    g_aht_ok = aht10_init();
    printf("%s\n", g_aht_ok ? "OK" : "ERRO");
    
    printf("   📏 VL53L0X: ");
    g_vl53_ok = vl53_init();
    printf("%s\n", g_vl53_ok ? "OK" : "ERRO");
    
    // Tenta conectar ao WiFi
    bool wifi_ok = wifi_connect_simple();
    
    if (wifi_ok) {
        led_rgb_set(0, 1, 0);  // Verde = WiFi OK
        buzzer_beep(100);
        sleep_ms(100);
        buzzer_beep(100);
        
        // Inicia servidor web
        start_web_server();
    } else {
        led_rgb_set(1, 1, 0);  // Amarelo = WiFi falhou
        buzzer_beep(500);
        
        printf("\n⚠️  WiFi falhou, mas sistema continua offline\n");
    }
    
    printf("\n=== MONITORAMENTO INICIADO ===\n\n");
    
    // Loop principal
    while (true) {
        // Lê sensores reais
        read_sensors();
        
        // Atualiza display OLED
        update_display();
        
        // Mostra dados no terminal
        printf("[%lu] T=%.1f°C H=%.0f%% D=%dmm N=%.0f%% V=%.1fL %s\n",
               g_leitura_count, g_temperatura, g_umidade,
               g_distancia_mm, g_nivel_percent, g_volume_litros,
               g_wifi_connected ? "WiFi:OK" : "WiFi:OFF");
        
        // DEBUG: A cada 10 leituras, mostra status detalhado
        if ((g_leitura_count % 10) == 0) {
            printf("📊 Status: IP=%s, Servidor=%s, Count=%lu\n", 
                   g_ip_address, 
                   g_wifi_connected ? "Ativo" : "Inativo",
                   g_leitura_count);
        }
        
        // LED indica status
        if (g_wifi_connected) {
            if (g_nivel_percent < 20) {
                led_rgb_set(1, 0, 1);  // Roxo = WiFi OK, nível baixo
            } else {
                led_rgb_set(0, 1, 0);  // Verde = tudo OK
            }
        } else {
            led_rgb_set(1, 1, 0);  // Amarelo = sem WiFi
        }
        
        // Processa requisições web (não bloqueia) - IMPORTANTE!
        cyw43_arch_poll();
        
        sleep_ms(3000);  // Leitura a cada 3 segundos
    }
    
    return 0;
}