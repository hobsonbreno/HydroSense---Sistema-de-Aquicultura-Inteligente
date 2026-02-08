/**
 * HydroSense v10 DIAGNÓSTICO - Versão mínima para encontrar travamento
 * Cada etapa tem um print, para sabermos exatamente onde trava
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"

// WiFi
#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"

// I2C
#define I2C_PORT i2c1
#define I2C_SDA 6
#define I2C_SCL 7

// Sensores
#define AHT10_ADDR 0x38
#define VL53L0X_ADDR 0x29
#define OLED_ADDR 0x3C

// Atuadores
#define SERVO_PIN 2
#define RELAY_LN1_PIN 14
#define RELAY_LN2_PIN 15
#define RELAY_LN3_PIN 16

// Sensor cor
#define COLOR_S0_PIN 10
#define COLOR_S1_PIN 11
#define COLOR_S2_PIN 12
#define COLOR_S3_PIN 13
#define COLOR_OUT_PIN 9

// Dados globais
static float g_temp = 0, g_hum = 0;
static uint16_t g_dist = 0;
static float g_nivel = 0, g_volume = 0;
static char g_cor[20] = "desconhecido";
static bool g_wifi = false;
static uint32_t g_count = 0;
static bool relay_ln1 = false, relay_ln2 = false, relay_ln3 = false;

// ===== FUNÇÕES DE SENSOR =====
bool read_aht10(float *temp, float *humidity) {
    uint8_t init_cmd[] = {0xBE, 0x08, 0x00};
    uint8_t measure_cmd[] = {0xAC, 0x33, 0x00};
    uint8_t data[6];
    
    if (i2c_write_blocking(I2C_PORT, AHT10_ADDR, init_cmd, 3, false) < 0) return false;
    sleep_ms(10);
    if (i2c_write_blocking(I2C_PORT, AHT10_ADDR, measure_cmd, 3, false) < 0) return false;
    sleep_ms(80);
    if (i2c_read_blocking(I2C_PORT, AHT10_ADDR, data, 6, false) < 0) return false;
    
    uint32_t hraw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t traw = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    *humidity = (float)hraw * 100.0f / 1048576.0f;
    *temp = (float)traw * 200.0f / 1048576.0f - 50.0f;
    return true;
}

uint16_t read_vl53l0x(void) {
    uint8_t data[2];
    uint8_t cmd = 0x14;
    if (i2c_write_blocking(I2C_PORT, VL53L0X_ADDR, &cmd, 1, true) < 0) return 0;
    if (i2c_read_blocking(I2C_PORT, VL53L0X_ADDR, data, 2, false) < 0) return 0;
    return (data[0] << 8) | data[1];
}

// ===== HTTP SERVER =====
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

static void http_err(void *arg, err_t err) {
    printf("[HTTP] Erro: %d\n", err);
}

static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || !newpcb) return ERR_VAL;
    tcp_setprio(newpcb, TCP_PRIO_MIN);
    tcp_recv(newpcb, http_recv);
    tcp_err(newpcb, http_err);
    tcp_sent(newpcb, http_sent);
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) { tcp_close(tpcb); return ERR_OK; }
    tcp_recved(tpcb, p->tot_len);
    
    char *req = malloc(p->tot_len + 1);
    pbuf_copy_partial(p, req, p->tot_len, 0);
    req[p->tot_len] = '\0';
    pbuf_free(p);
    
    char resp[2048];
    
    if (strstr(req, "GET /sensors")) {
        snprintf(resp, sizeof(resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n"
            "{\"temperatura\":%.2f,\"umidade\":%.2f,\"distancia\":%d,"
            "\"nivel\":%.2f,\"volume\":%.2f,\"corAgua\":\"%s\","
            "\"wifiStatus\":true,\"contadorLeituras\":%d,"
            "\"deviceIp\":\"%s\",\"timestamp\":%d}",
            g_temp, g_hum, g_dist, g_nivel, g_volume, g_cor, g_count,
            ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr),
            (int)(time_us_64()/1000000));
    }
    else if (strstr(req, "POST /relay")) {
        if (strstr(req, "\"pin\":14")) { bool s = strstr(req,"\"state\":1")!=NULL; gpio_put(RELAY_LN1_PIN,s); relay_ln1=s; }
        else if (strstr(req, "\"pin\":15")) { bool s = strstr(req,"\"state\":1")!=NULL; gpio_put(RELAY_LN2_PIN,s); relay_ln2=s; }
        else if (strstr(req, "\"pin\":16")) { bool s = strstr(req,"\"state\":1")!=NULL; gpio_put(RELAY_LN3_PIN,s); relay_ln3=s; }
        snprintf(resp, sizeof(resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n"
            "{\"success\":true,\"message\":\"Relay command executed\"}");
    }
    else if (strstr(req, "POST /servo")) {
        uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
        for (int i = 0; i < 200; i++) { pwm_set_gpio_level(SERVO_PIN, 2000); sleep_ms(10); }
        pwm_set_gpio_level(SERVO_PIN, 1500);
        snprintf(resp, sizeof(resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n"
            "{\"success\":true,\"message\":\"Feeding executed\"}");
    }
    else if (strstr(req, "GET /status")) {
        snprintf(resp, sizeof(resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n"
            "{\"system\":\"HydroSense v10\",\"wifi\":true,"
            "\"relays\":{\"LN1\":%s,\"LN2\":%s,\"LN3\":%s}}",
            relay_ln1?"true":"false", relay_ln2?"true":"false", relay_ln3?"true":"false");
    }
    else if (strstr(req, "OPTIONS")) {
        snprintf(resp, sizeof(resp),
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: close\r\n\r\n");
    }
    else {
        snprintf(resp, sizeof(resp),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
            "<html><body><h1>HydroSense v10</h1>"
            "<p>Temp: %.1fC | Umid: %.1f%%</p>"
            "<p>Nivel: %.1f%% (%.1fL)</p>"
            "<p><a href='/sensors'>API JSON</a></p>"
            "</body></html>",
            g_temp, g_hum, g_nivel, g_volume);
    }
    
    free(req);
    size_t len = strlen(resp);
    tcp_write(tpcb, resp, len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    return ERR_OK;
}

// ===== MAIN =====
int main() {
    // ETAPA 1: stdio
    stdio_init_all();
    sleep_ms(3000);
    
    printf("\n\n===========================================\n");
    printf("  HYDROSENSE v10 - DIAGNOSTICO DE BOOT\n");
    printf("===========================================\n\n");
    printf("[BOOT] stdio OK - USB CDC ativo\n");
    
    // ETAPA 2: CYW43
    printf("[BOOT] Inicializando CYW43...\n");
    if (cyw43_arch_init()) {
        printf("[ERRO] CYW43 falhou! Parando.\n");
        while(1) sleep_ms(1000);
    }
    printf("[BOOT] CYW43 OK\n");
    
    cyw43_arch_enable_sta_mode();
    printf("[BOOT] STA mode ativado\n");
    
    // ETAPA 3: I2C
    printf("[BOOT] Inicializando I2C (SDA=%d, SCL=%d)...\n", I2C_SDA, I2C_SCL);
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    printf("[BOOT] I2C OK\n");
    
    // ETAPA 4: Relés
    printf("[BOOT] Inicializando reles...\n");
    gpio_init(RELAY_LN1_PIN); gpio_set_dir(RELAY_LN1_PIN, GPIO_OUT); gpio_put(RELAY_LN1_PIN, 0);
    gpio_init(RELAY_LN2_PIN); gpio_set_dir(RELAY_LN2_PIN, GPIO_OUT); gpio_put(RELAY_LN2_PIN, 0);
    gpio_init(RELAY_LN3_PIN); gpio_set_dir(RELAY_LN3_PIN, GPIO_OUT); gpio_put(RELAY_LN3_PIN, 0);
    printf("[BOOT] Reles OK (GPIO %d, %d, %d)\n", RELAY_LN1_PIN, RELAY_LN2_PIN, RELAY_LN3_PIN);
    
    // ETAPA 5: Servo
    printf("[BOOT] Inicializando servo (GPIO %d)...\n", SERVO_PIN);
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 64.0f);
    pwm_config_set_wrap(&cfg, 39062);
    pwm_init(slice, &cfg, true);
    pwm_set_gpio_level(SERVO_PIN, 1500);
    printf("[BOOT] Servo OK\n");
    
    // ETAPA 6: Sensor cor
    printf("[BOOT] Inicializando sensor de cor...\n");
    gpio_init(COLOR_S0_PIN); gpio_set_dir(COLOR_S0_PIN, GPIO_OUT);
    gpio_init(COLOR_S1_PIN); gpio_set_dir(COLOR_S1_PIN, GPIO_OUT);
    gpio_init(COLOR_S2_PIN); gpio_set_dir(COLOR_S2_PIN, GPIO_OUT);
    gpio_init(COLOR_S3_PIN); gpio_set_dir(COLOR_S3_PIN, GPIO_OUT);
    gpio_init(COLOR_OUT_PIN); gpio_set_dir(COLOR_OUT_PIN, GPIO_IN);
    gpio_put(COLOR_S0_PIN, 1); gpio_put(COLOR_S1_PIN, 0);
    printf("[BOOT] Sensor cor OK\n");
    
    // ETAPA 7: Teste rápido sensores I2C
    printf("[BOOT] Testando AHT10 (0x%02X)...\n", AHT10_ADDR);
    float t, h;
    if (read_aht10(&t, &h)) {
        printf("[BOOT] AHT10 OK: T=%.1fC H=%.1f%%\n", t, h);
        g_temp = t; g_hum = h;
    } else {
        printf("[BOOT] AHT10 FALHOU (sem sensor?)\n");
        g_temp = 25.0; g_hum = 60.0;
    }
    
    printf("[BOOT] Testando VL53L0X (0x%02X)...\n", VL53L0X_ADDR);
    g_dist = read_vl53l0x();
    if (g_dist > 0) {
        printf("[BOOT] VL53L0X OK: %dmm\n", g_dist);
    } else {
        printf("[BOOT] VL53L0X FALHOU (sem sensor?)\n");
        g_dist = 100;
    }
    
    // Calcular nível
    float wh = 200.0f - (float)g_dist;
    if (wh < 0) wh = 0;
    g_nivel = (wh / 200.0f) * 100.0f;
    g_volume = (g_nivel / 100.0f) * 20.0f;
    strcpy(g_cor, "cristalino");
    
    // ETAPA 8: WiFi
    printf("\n[WIFI] Conectando a: %s\n", WIFI_SSID);
    printf("[WIFI] Senha: %s\n", WIFI_PASSWORD);
    
    int result = -1;
    
    printf("[WIFI] Tentativa 1/3: WPA2 AES (30s timeout)...\n");
    result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000);
    
    if (result != 0) {
        printf("[WIFI] T1 falhou (err=%d). Aguardando 2s...\n", result);
        sleep_ms(2000);
        printf("[WIFI] Tentativa 2/3: WPA2 MIXED (30s timeout)...\n");
        result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000);
    }
    
    if (result != 0) {
        printf("[WIFI] T2 falhou (err=%d). Aguardando 2s...\n", result);
        sleep_ms(2000);
        printf("[WIFI] Tentativa 3/3: WPA TKIP (30s timeout)...\n");
        result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA_TKIP_PSK, 30000);
    }
    
    if (result != 0) {
        printf("[WIFI] TODAS AS TENTATIVAS FALHARAM! (err=%d)\n", result);
        printf("[WIFI] Continuando em modo OFFLINE...\n");
        g_wifi = false;
    } else {
        g_wifi = true;
        const char *ip = ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr);
        printf("[WIFI] CONECTADO! IP: %s\n", ip);
        
        // Iniciar servidor HTTP
        printf("[HTTP] Iniciando servidor na porta 80...\n");
        struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
        if (pcb) {
            err_t err = tcp_bind(pcb, IP_ANY_TYPE, 80);
            if (err == ERR_OK) {
                pcb = tcp_listen_with_backlog(pcb, 1);
                if (pcb) {
                    tcp_accept(pcb, http_accept);
                    printf("[HTTP] Servidor OK - http://%s\n", ip);
                    printf("[HTTP] API:    http://%s/sensors\n", ip);
                } else {
                    printf("[HTTP] ERRO: listen falhou\n");
                }
            } else {
                printf("[HTTP] ERRO: bind falhou (%d)\n", err);
                tcp_close(pcb);
            }
        } else {
            printf("[HTTP] ERRO: tcp_new falhou\n");
        }
    }
    
    // ETAPA 9: Loop principal
    printf("\n==========================================\n");
    printf("  MONITORAMENTO INICIADO\n");
    printf("  WiFi: %s\n", g_wifi ? "CONECTADO" : "OFFLINE");
    printf("==========================================\n\n");
    
    while (true) {
        g_count++;
        
        // Ler sensores
        if (!read_aht10(&g_temp, &g_hum)) {
            // Manter último valor se falhar
        }
        g_dist = read_vl53l0x();
        
        // Calcular nível
        float wh2 = 200.0f - (float)g_dist;
        if (wh2 < 0) wh2 = 0;
        g_nivel = (wh2 / 200.0f) * 100.0f;
        if (g_nivel > 100) g_nivel = 100;
        g_volume = (g_nivel / 100.0f) * 20.0f;
        
        // Automação: ventilador se temp > 29°C
        if (g_temp > 29.0f && !relay_ln1) {
            gpio_put(RELAY_LN1_PIN, 1);
            relay_ln1 = true;
            printf("[AUTO] Ventilador LIGADO (temp=%.1f)\n", g_temp);
        } else if (g_temp <= 28.0f && relay_ln1) {
            gpio_put(RELAY_LN1_PIN, 0);
            relay_ln1 = false;
            printf("[AUTO] Ventilador DESLIGADO (temp=%.1f)\n", g_temp);
        }
        
        printf("[DADOS] #%d T=%.1fC H=%.1f%% D=%dmm N=%.0f%% V=%.1fL WiFi:%s\n",
               g_count, g_temp, g_hum, g_dist, g_nivel, g_volume, g_wifi?"OK":"OFF");
        
        // Poll WiFi (necessário para lwip processar)
        if (g_wifi) {
            cyw43_arch_poll();
        }
        
        sleep_ms(2000);
    }
    
    return 0;
}
