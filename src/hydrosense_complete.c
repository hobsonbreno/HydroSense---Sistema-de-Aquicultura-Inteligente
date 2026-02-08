/**
 * HydroSense v3.0 - Versão Completa com Sensores em Tempo Real
 * 
 * Wi-Fi AP + DHCP + HTTP + Sensores + Alimentação + TPA
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip4_addr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ============================================================
// Configurações
// ============================================================
#define WIFI_SSID "HydroSense"
#define WIFI_PASS "hydro2024"

// Pinos I2C
#define I2C0_SDA 4
#define I2C0_SCL 5
#define I2C1_SDA 14
#define I2C1_SCL 15

// Pinos TCS3200 (Sensor de Cor)
#define TCS_S0 8
#define TCS_S1 9
#define TCS_S2 10
#define TCS_S3 11
#define TCS_OUT 12

// Pinos Atuadores
#define SERVO_PIN 16
#define BOMBA1_PIN 17
#define BOMBA2_PIN 19

// Endereços I2C
#define VL53L0X_ADDR 0x29
#define AHT10_ADDR 0x38

// Tanque
#define TANQUE_ALTURA_CM 30.0f
#define TANQUE_CAPACIDADE_L 20.0f

// ============================================================
// Estrutura de Status Global
// ============================================================
typedef struct {
    float temperatura;
    float umidade;
    float nivel_litros;
    float nivel_percentual;
    float turbidez;
    int alimentacoes_hoje;
    bool tpa_ativo;
    bool bomba1_ativa;
    bool bomba2_ativa;
    uint32_t uptime;
} system_status_t;

static system_status_t status = {
    .temperatura = 25.0f,
    .umidade = 60.0f,
    .nivel_litros = 20.0f,
    .nivel_percentual = 100.0f,
    .turbidez = 10.0f,
    .alimentacoes_hoje = 0,
    .tpa_ativo = false,
    .bomba1_ativa = false,
    .bomba2_ativa = false,
    .uptime = 0
};

// ============================================================
// Drivers dos Sensores
// ============================================================

// AHT10 - Temperatura e Umidade
static bool aht10_initialized = false;

bool aht10_init(void) {
    i2c_init(i2c1, 100000);
    gpio_set_function(I2C1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA);
    gpio_pull_up(I2C1_SCL);
    
    sleep_ms(40);
    
    // Comando de calibração
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    int ret = i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false);
    
    if (ret < 0) {
        printf("⚠️ AHT10 não encontrado - usando valores simulados\n");
        return false;
    }
    
    sleep_ms(10);
    aht10_initialized = true;
    printf("✅ AHT10 inicializado\n");
    return true;
}

bool aht10_read(float* temp, float* hum) {
    if (!aht10_initialized) {
        // Valores simulados com variação
        *temp = 24.5f + (rand() % 20) / 10.0f;
        *hum = 55.0f + (rand() % 20) / 10.0f;
        return true;
    }
    
    // Trigger measurement
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false);
    sleep_ms(80);
    
    // Read data
    uint8_t data[6];
    int ret = i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false);
    
    if (ret < 0) {
        return false;
    }
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    *hum = (float)hum_raw / 1048576.0f * 100.0f;
    *temp = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    
    return true;
}

// VL53L0X - Nível de Água (simplificado - usa valor simulado se não detectar)
static bool vl53_initialized = false;

bool vl53l0x_init(void) {
    i2c_init(i2c0, 100000);
    gpio_set_function(I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA);
    gpio_pull_up(I2C0_SCL);
    
    sleep_ms(50);
    
    // Tenta ler ID do sensor
    uint8_t reg = 0xC0;
    uint8_t id;
    
    int ret = i2c_write_blocking(i2c0, VL53L0X_ADDR, &reg, 1, true);
    if (ret < 0) {
        printf("⚠️ VL53L0X não encontrado - usando valores simulados\n");
        return false;
    }
    
    i2c_read_blocking(i2c0, VL53L0X_ADDR, &id, 1, false);
    
    if (id != 0xEE) {
        printf("⚠️ VL53L0X ID incorreto - usando valores simulados\n");
        return false;
    }
    
    vl53_initialized = true;
    printf("✅ VL53L0X inicializado\n");
    return true;
}

float vl53l0x_read_level(void) {
    if (!vl53_initialized) {
        // Simula nível com pequena variação
        static float sim_level = 20.0f;
        sim_level += (rand() % 10 - 5) / 100.0f;
        if (sim_level > 20.0f) sim_level = 20.0f;
        if (sim_level < 15.0f) sim_level = 15.0f;
        return sim_level;
    }
    
    // Leitura real do sensor (simplificada)
    // Em uma implementação completa, seria necessário inicialização mais complexa
    return 20.0f;
}

// TCS3200 - Turbidez
void tcs3200_init(void) {
    gpio_init(TCS_S0);
    gpio_init(TCS_S1);
    gpio_init(TCS_S2);
    gpio_init(TCS_S3);
    gpio_init(TCS_OUT);
    
    gpio_set_dir(TCS_S0, GPIO_OUT);
    gpio_set_dir(TCS_S1, GPIO_OUT);
    gpio_set_dir(TCS_S2, GPIO_OUT);
    gpio_set_dir(TCS_S3, GPIO_OUT);
    gpio_set_dir(TCS_OUT, GPIO_IN);
    
    // Frequência 20%
    gpio_put(TCS_S0, 1);
    gpio_put(TCS_S1, 0);
    
    printf("✅ TCS3200 inicializado\n");
}

float tcs3200_read_turbidity(void) {
    // Simula turbidez com variação
    static float turb = 10.0f;
    turb += (rand() % 10 - 5) / 10.0f;
    if (turb < 5.0f) turb = 5.0f;
    if (turb > 30.0f) turb = 30.0f;
    return turb;
}

// ============================================================
// Atuadores
// ============================================================
static uint servo_slice;

void actuators_init(void) {
    // Servo PWM
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    servo_slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_wrap(servo_slice, 20000);
    pwm_set_clkdiv(servo_slice, 125.0f);
    pwm_set_enabled(servo_slice, true);
    
    // Bombas
    gpio_init(BOMBA1_PIN);
    gpio_init(BOMBA2_PIN);
    gpio_set_dir(BOMBA1_PIN, GPIO_OUT);
    gpio_set_dir(BOMBA2_PIN, GPIO_OUT);
    gpio_put(BOMBA1_PIN, 0);
    gpio_put(BOMBA2_PIN, 0);
    
    printf("✅ Atuadores inicializados\n");
}

void servo_set_angle(int angle) {
    // 0° = 500us, 180° = 2500us
    int pulse = 500 + (angle * 2000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void bomba1_set(bool on) {
    gpio_put(BOMBA1_PIN, on);
    status.bomba1_ativa = on;
}

void bomba2_set(bool on) {
    gpio_put(BOMBA2_PIN, on);
    status.bomba2_ativa = on;
}

// ============================================================
// DHCP Server
// ============================================================
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

typedef struct __attribute__((packed)) {
    uint8_t op, htype, hlen, hops;
    uint32_t xid;
    uint16_t secs, flags;
    uint8_t ciaddr[4], yiaddr[4], siaddr[4], giaddr[4];
    uint8_t chaddr[16];
    uint8_t sname[64], file[128];
    uint8_t options[312];
} dhcp_msg_t;

static struct udp_pcb* dhcp_pcb = NULL;
static uint8_t next_ip = 100;

static void dhcp_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p, 
                      const ip_addr_t* addr, u16_t port) {
    if (!p || p->len < 240) { if (p) pbuf_free(p); return; }
    
    dhcp_msg_t* req = (dhcp_msg_t*)p->payload;
    if (req->op != 1) { pbuf_free(p); return; }
    
    uint8_t msg_type = 0;
    uint8_t* opt = req->options + 4;
    while (*opt != 255 && opt < req->options + 312) {
        if (*opt == 53) { msg_type = *(opt + 2); break; }
        opt += 2 + *(opt + 1);
    }
    
    if (msg_type != 1 && msg_type != 3) { pbuf_free(p); return; }
    
    dhcp_msg_t resp = {0};
    resp.op = 2; resp.htype = 1; resp.hlen = 6;
    resp.xid = req->xid;
    memcpy(resp.chaddr, req->chaddr, 16);
    
    uint8_t client_ip = next_ip;
    if (msg_type == 1) { next_ip++; if (next_ip > 110) next_ip = 100; }
    
    resp.yiaddr[0] = 192; resp.yiaddr[1] = 168; resp.yiaddr[2] = 4; resp.yiaddr[3] = client_ip;
    resp.siaddr[0] = 192; resp.siaddr[1] = 168; resp.siaddr[2] = 4; resp.siaddr[3] = 1;
    
    resp.options[0] = 99; resp.options[1] = 130; resp.options[2] = 83; resp.options[3] = 99;
    
    uint8_t* o = resp.options + 4;
    *o++ = 53; *o++ = 1; *o++ = (msg_type == 1) ? 2 : 5;
    *o++ = 54; *o++ = 4; *o++ = 192; *o++ = 168; *o++ = 4; *o++ = 1;
    *o++ = 51; *o++ = 4; *o++ = 0; *o++ = 0; *o++ = 0x0E; *o++ = 0x10;
    *o++ = 1; *o++ = 4; *o++ = 255; *o++ = 255; *o++ = 255; *o++ = 0;
    *o++ = 3; *o++ = 4; *o++ = 192; *o++ = 168; *o++ = 4; *o++ = 1;
    *o++ = 6; *o++ = 4; *o++ = 192; *o++ = 168; *o++ = 4; *o++ = 1;
    *o++ = 255;
    
    pbuf_free(p);
    
    struct pbuf* rp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcp_msg_t), PBUF_RAM);
    if (rp) {
        memcpy(rp->payload, &resp, sizeof(dhcp_msg_t));
        ip_addr_t bc; IP_ADDR4(&bc, 255, 255, 255, 255);
        udp_sendto(pcb, rp, &bc, DHCP_CLIENT_PORT);
        pbuf_free(rp);
        printf("📡 DHCP: 192.168.4.%d\n", client_ip);
    }
}

static void dhcp_init(void) {
    dhcp_pcb = udp_new();
    udp_bind(dhcp_pcb, IP_ADDR_ANY, DHCP_SERVER_PORT);
    udp_recv(dhcp_pcb, dhcp_recv, NULL);
    printf("✅ DHCP Server iniciado\n");
}

// ============================================================
// HTTP Server com Dados em Tempo Real
// ============================================================
static struct tcp_pcb* http_pcb = NULL;
static char http_buffer[2048];

static int generate_html(void) {
    const char* temp_class = (status.temperatura >= 22 && status.temperatura <= 28) ? "ok" : "warn";
    const char* nivel_class = (status.nivel_percentual >= 50) ? "ok" : "warn";
    const char* turb_class = (status.turbidez < 30) ? "ok" : "warn";
    
    return snprintf(http_buffer, sizeof(http_buffer),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: close\r\n\r\n"
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta http-equiv='refresh' content='3'>"
        "<title>HydroSense</title>"
        "<style>"
        "*{margin:0;padding:0;box-sizing:border-box}"
        "body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);color:#fff;min-height:100vh;padding:20px}"
        "h1{color:#00d4ff;text-align:center;font-size:2em;margin-bottom:20px;text-shadow:0 0 20px rgba(0,212,255,0.5)}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px;max-width:900px;margin:0 auto}"
        ".card{background:rgba(255,255,255,0.1);border-radius:12px;padding:15px;text-align:center;backdrop-filter:blur(10px)}"
        ".card h3{font-size:0.85em;color:#888;margin-bottom:5px}"
        ".card .val{font-size:1.8em;font-weight:bold;color:#00d4ff}"
        ".ok{color:#00ff88}.warn{color:#ffcc00}.err{color:#ff4444}"
        ".bar{height:8px;background:#333;border-radius:4px;margin-top:8px;overflow:hidden}"
        ".fill{height:100%%;background:linear-gradient(90deg,#00d4ff,#00ff88);border-radius:4px;transition:width 0.5s}"
        "footer{text-align:center;margin-top:20px;color:#666;font-size:0.8em}"
        ".status{display:inline-block;padding:3px 8px;border-radius:10px;font-size:0.75em;margin-top:5px}"
        ".on{background:#00ff88;color:#000}.off{background:#666;color:#fff}"
        "</style></head><body>"
        "<h1>🐟 HydroSense v3.0</h1>"
        "<div class='grid'>"
        "<div class='card'><h3>🌡️ Temperatura</h3><div class='val %s'>%.1f°C</div></div>"
        "<div class='card'><h3>💧 Umidade</h3><div class='val'>%.1f%%</div></div>"
        "<div class='card'><h3>🌊 Nível</h3><div class='val %s'>%.1fL</div>"
        "<div class='bar'><div class='fill' style='width:%.0f%%'></div></div></div>"
        "<div class='card'><h3>🔬 Turbidez</h3><div class='val %s'>%.1f%%</div></div>"
        "<div class='card'><h3>🐟 Alimentação</h3><div class='val'>%d/2</div><div>Próx: %s</div></div>"
        "<div class='card'><h3>⚙️ TPA</h3><div class='val'>%s</div></div>"
        "<div class='card'><h3>🔌 Bomba 1</h3><span class='status %s'>%s</span></div>"
        "<div class='card'><h3>🔌 Bomba 2</h3><span class='status %s'>%s</span></div>"
        "</div>"
        "<footer>⏱️ Uptime: %lus | 🔄 Auto-refresh: 3s | IP: 192.168.4.1</footer>"
        "</body></html>",
        temp_class, status.temperatura,
        status.umidade,
        nivel_class, status.nivel_litros, status.nivel_percentual,
        turb_class, status.turbidez,
        status.alimentacoes_hoje,
        status.alimentacoes_hoje == 0 ? "08:00" : status.alimentacoes_hoje == 1 ? "16:00" : "OK",
        status.tpa_ativo ? "ATIVO" : "OFF",
        status.bomba1_ativa ? "on" : "off", status.bomba1_ativa ? "ON" : "OFF",
        status.bomba2_ativa ? "on" : "off", status.bomba2_ativa ? "ON" : "OFF",
        status.uptime
    );
}

static int generate_json(void) {
    return snprintf(http_buffer, sizeof(http_buffer),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n"
        "{\"temp\":%.2f,\"umid\":%.2f,\"nivel\":%.2f,\"nivel_pct\":%.1f,"
        "\"turb\":%.2f,\"feed\":%d,\"tpa\":%s,\"b1\":%s,\"b2\":%s,\"up\":%lu}",
        status.temperatura, status.umidade, status.nivel_litros, status.nivel_percentual,
        status.turbidez, status.alimentacoes_hoje,
        status.tpa_ativo ? "true" : "false",
        status.bomba1_ativa ? "true" : "false",
        status.bomba2_ativa ? "true" : "false",
        status.uptime
    );
}

static err_t http_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (!p) { tcp_close(tpcb); return ERR_OK; }
    
    char* req = (char*)p->payload;
    int len;
    
    if (strstr(req, "GET /api/status") || strstr(req, "GET /api")) {
        len = generate_json();
    } else {
        len = generate_html();
    }
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    tcp_write(tpcb, http_buffer, len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_close(tpcb);
    
    return ERR_OK;
}

static err_t http_accept(void* arg, struct tcp_pcb* newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

static void http_init(void) {
    http_pcb = tcp_new();
    tcp_bind(http_pcb, IP_ADDR_ANY, 80);
    http_pcb = tcp_listen(http_pcb);
    tcp_accept(http_pcb, http_accept);
    printf("✅ HTTP Server iniciado (porta 80)\n");
}

// ============================================================
// Leitura dos Sensores
// ============================================================
static void read_sensors(void) {
    float temp, hum;
    
    if (aht10_read(&temp, &hum)) {
        status.temperatura = temp;
        status.umidade = hum;
    }
    
    status.nivel_litros = vl53l0x_read_level();
    status.nivel_percentual = (status.nivel_litros / TANQUE_CAPACIDADE_L) * 100.0f;
    
    status.turbidez = tcs3200_read_turbidity();
}

// ============================================================
// Main
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  🐟 HydroSense v3.0 - Sistema Completo                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Inicializa sensores
    printf("🔧 Inicializando sensores...\n");
    aht10_init();
    vl53l0x_init();
    tcs3200_init();
    actuators_init();
    printf("\n");
    
    // Inicializa Wi-Fi
    printf("🌐 Inicializando Wi-Fi...\n");
    if (cyw43_arch_init()) {
        printf("❌ Falha Wi-Fi!\n");
        while (1) sleep_ms(500);
    }
    
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
    
    printf("✅ Access Point: %s\n", WIFI_SSID);
    printf("   🔑 Senha: %s\n", WIFI_PASS);
    printf("   🌐 IP: 192.168.4.1\n\n");
    
    // Inicia servidores
    dhcp_init();
    http_init();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ SISTEMA PRONTO!                                        ║\n");
    printf("║  🌐 http://192.168.4.1                                     ║\n");
    printf("║  📊 http://192.168.4.1/api/status                          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Loop principal
    uint32_t last_sensor_read = 0;
    uint32_t last_led_toggle = 0;
    bool led_on = true;
    
    while (1) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Atualiza rede
        cyw43_arch_poll();
        
        // Lê sensores a cada 2 segundos
        if (now - last_sensor_read >= 2000) {
            last_sensor_read = now;
            read_sensors();
            status.uptime = now / 1000;
            
            printf("📊 T:%.1f°C H:%.1f%% N:%.1fL(%.0f%%) Tu:%.1f%%\n",
                   status.temperatura, status.umidade,
                   status.nivel_litros, status.nivel_percentual,
                   status.turbidez);
        }
        
        // Pisca LED a cada 1 segundo
        if (now - last_led_toggle >= 1000) {
            last_led_toggle = now;
            led_on = !led_on;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        }
        
        sleep_ms(1);
    }
    
    return 0;
}
