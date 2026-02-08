/**
 * HydroSense v10 - Sistema Completo de Automação de Aquário
 * 
 * Hardware:
 * - Raspberry Pi Pico W + BitDogLab
 * - AHT10 (Temperatura/Umidade) - I2C
 * - VL53L0X (Distância/Nível) - I2C  
 * - Sensor de Cor TCS3200 - GPIO
 * - Display OLED SSD1306 - I2C
 * - Servo Motor (Alimentação) - GPIO 2
 * - 3x Relés: LN1 (GPIO 14), LN2 (GPIO 15), LN3 (GPIO 16)
 * - WiFi: Rede HydroSense
 * 
 * Funcionalidades:
 * - Sensores reais com I2C switching
 * - Controle de 3 relés via API
 * - Servo motor para alimentação
 * - HTTP Server com API REST
 * - Display OLED com informações
 * - Sistema completo de aquário
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

// ===== CONFIGURAÇÕES DE HARDWARE =====
#define I2C_PORT i2c1
#define I2C_SDA 6
#define I2C_SCL 7
#define I2C_FREQ_SENSORS 100000  // 100kHz para sensores
#define I2C_FREQ_OLED 400000     // 400kHz para OLED

// Endereços I2C
#define AHT10_ADDR 0x38
#define OLED_ADDR 0x3C
#define VL53L0X_ADDR 0x29

// GPIOs
#define SERVO_PIN 2
#define RELAY_LN1_PIN 14    // Motor/Ventilador
#define RELAY_LN2_PIN 15    // Bomba 01 (Esvaziar)
#define RELAY_LN3_PIN 16    // Bomba 02 (Encher)

// Sensor de Cor TCS3200
#define COLOR_S0_PIN 10
#define COLOR_S1_PIN 11
#define COLOR_S2_PIN 12
#define COLOR_S3_PIN 13
#define COLOR_OUT_PIN 9

// Configurações do sistema
#define AQUARIUM_CAPACITY_L 20.0f
#define SENSOR_HEIGHT_MM 200.0f
#define TEMP_THRESHOLD 29.0f

// ===== ESTRUTURAS DE DADOS =====
typedef struct {
    float temperatura;
    float umidade;
    uint16_t distancia_mm;
    float nivel_agua_percent;
    float volume_agua_litros;
    char cor_agua[20];
    bool wifi_connected;
    uint32_t contador_leituras;
} sensor_data_t;

typedef struct {
    bool ln1_state;  // Ventilador
    bool ln2_state;  // Bomba esvaziar
    bool ln3_state;  // Bomba encher
} relay_states_t;

// ===== VARIÁVEIS GLOBAIS =====
static sensor_data_t current_data = {0};
static relay_states_t relay_states = {false, false, false};
static uint32_t reading_counter = 0;

// WiFi credentials
const char* WIFI_SSID = "HydroSense";
const char* WIFI_PASSWORD = "Hb12345678";

// ===== PROTÓTIPOS DE FUNÇÕES =====
void init_hardware(void);
void init_i2c(uint32_t frequency);
void init_relays(void);
void init_servo(void);
void init_color_sensor(void);
bool read_aht10(float *temp, float *humidity);
uint16_t read_vl53l0x_distance(void);
void read_color_sensor(char *color_name);
void calculate_water_level(uint16_t distance_mm);
void update_oled_display(void);
void control_relay(uint8_t pin, bool state);
void servo_feed_rotation(void);
bool wifi_connect_simple(void);
err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void tcp_server_err(void *arg, err_t err);
err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
void send_http_response(struct tcp_pcb *tpcb, const char *response);
void handle_http_request(struct tcp_pcb *tpcb, const char *request);

// ===== IMPLEMENTAÇÃO MAIN =====
int main() {
    stdio_init_all();
    sleep_ms(3000);  // Aguarda USB CDC estabilizar (ESSENCIAL!)
    
    printf("\n\n\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     HydroSense v10 - Sistema Completo  ║\n");
    printf("║   Aquicultura Inteligente com IoT      ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf(">>> Firmware v10 iniciado! <<<\n\n");
    
    // Inicializar hardware
    printf("Inicializando hardware...\n");
    init_hardware();
    
    // Conectar WiFi (não retorna erro fatal - continua em modo offline)
    printf("Conectando ao WiFi...\n");
    bool wifi_ok = wifi_connect_simple();
    if (!wifi_ok) {
        printf("AVISO: WiFi nao conectado - modo OFFLINE\n");
        printf("O sistema continuara lendo sensores sem WiFi\n\n");
        current_data.wifi_connected = false;
    } else {
        printf("Sistema inicializado com sucesso!\n");
        printf("IP: %s\n", ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr));
        current_data.wifi_connected = true;
    }
    
    printf("\n=== MONITORAMENTO INICIADO ===\n\n");
    
    // Loop principal
    while (true) {
        // Ler sensores
        init_i2c(I2C_FREQ_SENSORS);
        
        // AHT10 - Temperatura e umidade
        if (!read_aht10(&current_data.temperatura, &current_data.umidade)) {
            printf("Aviso: Erro lendo AHT10 (sensor pode nao estar conectado)\n");
        }
        
        // VL53L0X - Distância/Nível da água
        current_data.distancia_mm = read_vl53l0x_distance();
        calculate_water_level(current_data.distancia_mm);
        
        // Sensor de cor
        read_color_sensor(current_data.cor_agua);
        
        // Atualizar dados
        current_data.contador_leituras = ++reading_counter;
        
        // Atualizar display
        init_i2c(I2C_FREQ_OLED);
        update_oled_display();
        
        printf("Leitura #%d - T:%.1fC H:%.1f%% D:%dmm N:%.1f%% V:%.1fL Cor:%s WiFi:%s\n",
               reading_counter, current_data.temperatura, current_data.umidade,
               current_data.distancia_mm, current_data.nivel_agua_percent, 
               current_data.volume_agua_litros, current_data.cor_agua,
               current_data.wifi_connected ? "OK" : "OFF");
        
        // Processar WiFi se conectado
        if (current_data.wifi_connected) {
            cyw43_arch_poll();
        }
        
        sleep_ms(2000);  // Leitura a cada 2 segundos
    }
    
    return 0;
}

// ===== INICIALIZAÇÃO DE HARDWARE =====
void init_hardware(void) {
    // Inicializar CYW43 (chip WiFi)
    printf("  [1/5] Inicializando CYW43 (WiFi chip)...\n");
    if (cyw43_arch_init()) {
        printf("  ERRO: Falha ao inicializar CYW43!\n");
        printf("  Continuando sem WiFi...\n");
    } else {
        cyw43_arch_enable_sta_mode();
        printf("  [1/5] CYW43 OK\n");
    }
    
    // Inicializar I2C
    printf("  [2/5] Inicializando I2C...\n");
    init_i2c(I2C_FREQ_SENSORS);
    printf("  [2/5] I2C OK (SDA=%d, SCL=%d)\n", I2C_SDA, I2C_SCL);
    
    // Inicializar relés
    printf("  [3/5] Inicializando reles...\n");
    init_relays();
    
    // Inicializar servo
    printf("  [4/5] Inicializando servo...\n");
    init_servo();
    
    // Inicializar sensor de cor
    printf("  [5/5] Inicializando sensor de cor...\n");
    init_color_sensor();
    
    printf("Hardware inicializado com sucesso!\n\n");
}

void init_i2c(uint32_t frequency) {
    i2c_deinit(I2C_PORT);
    i2c_init(I2C_PORT, frequency);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void init_relays(void) {
    gpio_init(RELAY_LN1_PIN);
    gpio_init(RELAY_LN2_PIN);
    gpio_init(RELAY_LN3_PIN);
    
    gpio_set_dir(RELAY_LN1_PIN, GPIO_OUT);
    gpio_set_dir(RELAY_LN2_PIN, GPIO_OUT);
    gpio_set_dir(RELAY_LN3_PIN, GPIO_OUT);
    
    // Iniciar com relés desligados
    gpio_put(RELAY_LN1_PIN, 0);
    gpio_put(RELAY_LN2_PIN, 0);
    gpio_put(RELAY_LN3_PIN, 0);
    
    printf("Relés inicializados (LN1: GPIO%d, LN2: GPIO%d, LN3: GPIO%d)\n",
           RELAY_LN1_PIN, RELAY_LN2_PIN, RELAY_LN3_PIN);
}

void init_servo(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 64.0f);
    pwm_config_set_wrap(&config, 39062);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(SERVO_PIN, 1500);  // Posição neutra
    
    printf("Servo motor inicializado (GPIO%d)\n", SERVO_PIN);
}

void init_color_sensor(void) {
    gpio_init(COLOR_S0_PIN);
    gpio_init(COLOR_S1_PIN);
    gpio_init(COLOR_S2_PIN);
    gpio_init(COLOR_S3_PIN);
    gpio_init(COLOR_OUT_PIN);
    
    gpio_set_dir(COLOR_S0_PIN, GPIO_OUT);
    gpio_set_dir(COLOR_S1_PIN, GPIO_OUT);
    gpio_set_dir(COLOR_S2_PIN, GPIO_OUT);
    gpio_set_dir(COLOR_S3_PIN, GPIO_OUT);
    gpio_set_dir(COLOR_OUT_PIN, GPIO_IN);
    
    // Configurar escala de frequência (20%)
    gpio_put(COLOR_S0_PIN, 1);
    gpio_put(COLOR_S1_PIN, 0);
    
    printf("Sensor de cor TCS3200 inicializado\n");
}

// ===== LEITURA DE SENSORES =====
bool read_aht10(float *temp, float *humidity) {
    uint8_t init_cmd[] = {0xBE, 0x08, 0x00};
    uint8_t measure_cmd[] = {0xAC, 0x33, 0x00};
    uint8_t data[6];
    
    // Inicializar AHT10
    if (i2c_write_blocking(I2C_PORT, AHT10_ADDR, init_cmd, 3, false) < 0) {
        return false;
    }
    sleep_ms(10);
    
    // Comando de medição
    if (i2c_write_blocking(I2C_PORT, AHT10_ADDR, measure_cmd, 3, false) < 0) {
        return false;
    }
    sleep_ms(80);
    
    // Ler dados
    if (i2c_read_blocking(I2C_PORT, AHT10_ADDR, data, 6, false) < 0) {
        return false;
    }
    
    // Processar dados
    uint32_t humidity_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temperature_raw = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    *humidity = (float)humidity_raw * 100.0f / 1048576.0f;
    *temp = (float)temperature_raw * 200.0f / 1048576.0f - 50.0f;
    
    return true;
}

uint16_t read_vl53l0x_distance(void) {
    // Implementação simplificada do VL53L0X
    uint8_t data[2];
    uint8_t cmd = 0x14;  // Registro de distância
    
    if (i2c_write_blocking(I2C_PORT, VL53L0X_ADDR, &cmd, 1, true) < 0) {
        return 0;
    }
    
    if (i2c_read_blocking(I2C_PORT, VL53L0X_ADDR, data, 2, false) < 0) {
        return 0;
    }
    
    return (data[0] << 8) | data[1];
}

void read_color_sensor(char *color_name) {
    uint32_t red, green, blue;
    
    // Ler vermelho
    gpio_put(COLOR_S2_PIN, 0);
    gpio_put(COLOR_S3_PIN, 0);
    sleep_ms(10);
    
    // Simular leitura de frequência (implementação simplificada)
    uint32_t start_time = time_us_32();
    uint32_t pulse_count = 0;
    while (time_us_32() - start_time < 10000) {  // 10ms
        if (gpio_get(COLOR_OUT_PIN)) pulse_count++;
        sleep_us(10);
    }
    red = pulse_count;
    
    // Ler verde
    gpio_put(COLOR_S2_PIN, 1);
    gpio_put(COLOR_S3_PIN, 1);
    sleep_ms(10);
    
    start_time = time_us_32();
    pulse_count = 0;
    while (time_us_32() - start_time < 10000) {
        if (gpio_get(COLOR_OUT_PIN)) pulse_count++;
        sleep_us(10);
    }
    green = pulse_count;
    
    // Ler azul
    gpio_put(COLOR_S2_PIN, 0);
    gpio_put(COLOR_S3_PIN, 1);
    sleep_ms(10);
    
    start_time = time_us_32();
    pulse_count = 0;
    while (time_us_32() - start_time < 10000) {
        if (gpio_get(COLOR_OUT_PIN)) pulse_count++;
        sleep_us(10);
    }
    blue = pulse_count;
    
    // Determinar cor baseado nos valores RGB
    if (red > green && red > blue) {
        if (red > 150) strcpy(color_name, "turvo");
        else strcpy(color_name, "marrom");
    } else if (green > red && green > blue) {
        strcpy(color_name, "verde");
    } else if (blue > red && blue > green) {
        strcpy(color_name, "cristalino");
    } else {
        strcpy(color_name, "outro");
    }
}

void calculate_water_level(uint16_t distance_mm) {
    if (distance_mm == 0) {
        current_data.nivel_agua_percent = 0;
        current_data.volume_agua_litros = 0;
        return;
    }
    
    // Calcular nível baseado na distância do sensor
    float water_height = SENSOR_HEIGHT_MM - (float)distance_mm;
    if (water_height < 0) water_height = 0;
    
    current_data.nivel_agua_percent = (water_height / SENSOR_HEIGHT_MM) * 100.0f;
    if (current_data.nivel_agua_percent > 100) current_data.nivel_agua_percent = 100;
    
    current_data.volume_agua_litros = (current_data.nivel_agua_percent / 100.0f) * AQUARIUM_CAPACITY_L;
}

// ===== CONTROLE DE ATUADORES =====
void control_relay(uint8_t pin, bool state) {
    gpio_put(pin, state ? 1 : 0);
    
    if (pin == RELAY_LN1_PIN) relay_states.ln1_state = state;
    else if (pin == RELAY_LN2_PIN) relay_states.ln2_state = state;
    else if (pin == RELAY_LN3_PIN) relay_states.ln3_state = state;
    
    printf("Relé GPIO%d: %s\n", pin, state ? "LIGADO" : "DESLIGADO");
}

void servo_feed_rotation(void) {
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    
    printf("Executando alimentação - Servo 360°\n");
    
    // Rotação 360° (2 segundos)
    for (int i = 0; i < 200; i++) {
        pwm_set_gpio_level(SERVO_PIN, 2000);  // Rotação rápida
        sleep_ms(10);
    }
    
    // Parar servo
    pwm_set_gpio_level(SERVO_PIN, 1500);
    printf("Alimentação concluída\n");
}

// ===== DISPLAY OLED =====
void update_oled_display(void) {
    // Implementação simplificada - apenas printf para debug
    printf("\n=== HYDROSENSE DISPLAY ===\n");
    printf("Temp: %.1f°C  Umid: %.1f%%\n", current_data.temperatura, current_data.umidade);
    printf("Nível: %.1f%% (%.1fL)\n", current_data.nivel_agua_percent, current_data.volume_agua_litros);
    printf("Dist: %dmm  Cor: %s\n", current_data.distancia_mm, current_data.cor_agua);
    printf("Relés: LN1:%s LN2:%s LN3:%s\n", 
           relay_states.ln1_state ? "ON" : "OFF",
           relay_states.ln2_state ? "ON" : "OFF", 
           relay_states.ln3_state ? "ON" : "OFF");
    printf("Leituras: %d  WiFi: %s\n", current_data.contador_leituras,
           current_data.wifi_connected ? "OK" : "ERR");
    printf("========================\n\n");
}

// ===== CONECTIVIDADE WiFi =====
bool wifi_connect_simple(void) {
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

    if (result) {
        printf("❌ ERRO: Todas as tentativas de WiFi falharam\n");
        return false;
    }
    
    printf("✅ WiFi conectado!\n");
    printf("📡 IP: %s\n", ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr));
    
    // Iniciar servidor TCP
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("ERRO: Falha ao criar PCB TCP\n");
        return false;
    }
    
    err_t err = tcp_bind(pcb, IP_ANY_TYPE, 80);
    if (err) {
        printf("ERRO: Falha no bind TCP (%d)\n", err);
        tcp_close(pcb);
        return false;
    }
    
    pcb = tcp_listen_with_backlog(pcb, 1);
    if (!pcb) {
        printf("ERRO: Falha no listen TCP\n");
        return false;
    }
    
    tcp_accept(pcb, tcp_server_accept);
    printf("Servidor HTTP iniciado na porta 80\n");
    
    return true;
}

// ===== SERVIDOR HTTP TCP =====
err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == NULL) {
        return ERR_VAL;
    }
    
    tcp_setprio(newpcb, TCP_PRIO_MIN);
    tcp_recv(newpcb, tcp_server_recv);
    tcp_err(newpcb, tcp_server_err);
    tcp_sent(newpcb, tcp_server_sent);
    
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    tcp_recved(tpcb, p->tot_len);
    
    char *request = malloc(p->tot_len + 1);
    pbuf_copy_partial(p, request, p->tot_len, 0);
    request[p->tot_len] = '\0';
    
    pbuf_free(p);
    
    handle_http_request(tpcb, request);
    free(request);
    
    return ERR_OK;
}

void tcp_server_err(void *arg, err_t err) {
    printf("Erro TCP: %d\n", err);
}

err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

void handle_http_request(struct tcp_pcb *tpcb, const char *request) {
    printf("HTTP Request: %.100s...\n", request);
    
    char response[2048];
    
    if (strstr(request, "GET /sensors")) {
        // API de sensores (JSON)
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n"
            "{\n"
            "  \"temperatura\": %.2f,\n"
            "  \"umidade\": %.2f,\n"
            "  \"distancia\": %d,\n"
            "  \"nivel\": %.2f,\n"
            "  \"volume\": %.2f,\n"
            "  \"corAgua\": \"%s\",\n"
            "  \"wifiStatus\": true,\n"
            "  \"contadorLeituras\": %d,\n"
            "  \"deviceIp\": \"%s\",\n"
            "  \"timestamp\": %d\n"
            "}",
            current_data.temperatura, current_data.umidade, current_data.distancia_mm,
            current_data.nivel_agua_percent, current_data.volume_agua_litros, current_data.cor_agua,
            current_data.contador_leituras, ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr),
            (int)time(NULL)
        );
    }
    else if (strstr(request, "POST /relay")) {
        // Controle de relé
        // Extrair parâmetros do JSON (implementação simplificada)
        if (strstr(request, "\"pin\":14")) {
            bool state = strstr(request, "\"state\":1") != NULL;
            control_relay(RELAY_LN1_PIN, state);
        }
        else if (strstr(request, "\"pin\":15")) {
            bool state = strstr(request, "\"state\":1") != NULL;
            control_relay(RELAY_LN2_PIN, state);
        }
        else if (strstr(request, "\"pin\":16")) {
            bool state = strstr(request, "\"state\":1") != NULL;
            control_relay(RELAY_LN3_PIN, state);
        }
        
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n"
            "{\"success\": true, \"message\": \"Relay command executed\"}"
        );
    }
    else if (strstr(request, "POST /servo")) {
        // Controle do servo (alimentação)
        servo_feed_rotation();
        
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n"
            "{\"success\": true, \"message\": \"Feeding executed\"}"
        );
    }
    else if (strstr(request, "GET /status")) {
        // Status geral
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n"
            "{\n"
            "  \"system\": \"HydroSense v10\",\n"
            "  \"uptime\": %d,\n"
            "  \"wifi\": true,\n"
            "  \"sensors\": true,\n"
            "  \"relays\": {\"LN1\": %s, \"LN2\": %s, \"LN3\": %s}\n"
            "}",
            (int)(time_us_64() / 1000000),
            relay_states.ln1_state ? "true" : "false",
            relay_states.ln2_state ? "true" : "false", 
            relay_states.ln3_state ? "true" : "false"
        );
    }
    else {
        // Página web principal
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n"
            "<!DOCTYPE html>\n"
            "<html><head><title>HydroSense v10</title></head>\n"
            "<body>\n"
            "<h1>HydroSense v10 - Sistema Completo</h1>\n"
            "<h2>Sensores</h2>\n"
            "<p>Temperatura: %.1f°C</p>\n"
            "<p>Umidade: %.1f%%</p>\n"
            "<p>Nível da Água: %.1f%% (%.1fL)</p>\n"
            "<p>Cor da Água: %s</p>\n"
            "<h2>Relés</h2>\n"
            "<p>LN1 (Ventilador): %s</p>\n"
            "<p>LN2 (Bomba Esvaziar): %s</p>\n"
            "<p>LN3 (Bomba Encher): %s</p>\n"
            "<h2>Sistema</h2>\n"
            "<p>Leituras: %d</p>\n"
            "<p>WiFi: Conectado</p>\n"
            "<p>Versão: v10</p>\n"
            "</body></html>",
            current_data.temperatura, current_data.umidade,
            current_data.nivel_agua_percent, current_data.volume_agua_litros,
            current_data.cor_agua,
            relay_states.ln1_state ? "LIGADO" : "DESLIGADO",
            relay_states.ln2_state ? "LIGADO" : "DESLIGADO",
            relay_states.ln3_state ? "LIGADO" : "DESLIGADO",
            current_data.contador_leituras
        );
    }
    
    send_http_response(tpcb, response);
}

void send_http_response(struct tcp_pcb *tpcb, const char *response) {
    size_t len = strlen(response);
    err_t err = tcp_write(tpcb, response, len, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        tcp_output(tpcb);
    } else {
        printf("Erro no envio HTTP: %d\n", err);
    }
}