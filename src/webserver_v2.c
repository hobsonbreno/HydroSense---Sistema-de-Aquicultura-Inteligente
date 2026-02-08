/**
 * HydroSense v3.0 - Servidor HTTP Corrigido
 * 
 * Implementação robusta com DHCP server para modo AP
 */

#include "webserver.h"
#include "hydrosense_config.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "dhcpserver.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Buffer para respostas HTTP
#define RESPONSE_BUFFER_SIZE 4096
static char response_buffer[RESPONSE_BUFFER_SIZE];
static bool server_running = false;
static struct tcp_pcb* server_pcb = NULL;
static dhcp_server_t dhcp_server;

// Forward declarations
static err_t http_accept_callback(void* arg, struct tcp_pcb* newpcb, err_t err);
static err_t http_recv_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
static void http_err_callback(void* arg, err_t err);

// ============================================================
// HTML Simples para Dashboard (reduzido para caber na memória)
// ============================================================
static const char HTML_HEADER[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: close\r\n"
"Cache-Control: no-cache\r\n"
"\r\n"
"<!DOCTYPE html><html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta http-equiv='refresh' content='5'>"
"<title>HydroSense</title>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:Arial;background:#1a1a2e;color:#fff;padding:15px}"
"h1{color:#00d4ff;text-align:center;margin-bottom:20px}"
".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px}"
".card{background:#2a2a4e;border-radius:10px;padding:15px;text-align:center}"
".card h3{font-size:0.9em;color:#888;margin-bottom:5px}"
".card .val{font-size:1.8em;font-weight:bold;color:#00d4ff}"
".ok{color:#0f0}.warn{color:#ff0}.err{color:#f00}"
".bar{height:8px;background:#333;border-radius:4px;margin-top:8px}"
".fill{height:100%;background:#00d4ff;border-radius:4px}"
"footer{text-align:center;margin-top:20px;color:#666;font-size:0.8em}"
"</style></head><body>"
"<h1>🐟 HydroSense v3.0</h1>"
"<div class='grid'>";

static const char HTML_FOOTER[] = 
"</div>"
"<footer>Auto-refresh: 5s | IP: 192.168.4.1</footer>"
"</body></html>";

static const char JSON_HEADER[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: application/json\r\n"
"Access-Control-Allow-Origin: *\r\n"
"Connection: close\r\n"
"\r\n";

static const char HTTP_404[] = 
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/plain\r\n"
"Connection: close\r\n"
"\r\n"
"404 Not Found";

// ============================================================
// Geração de Conteúdo
// ============================================================
static int generate_dashboard(char* buf, size_t max_len) {
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    
    system_status_t st;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        st = g_status;
        xSemaphoreGive(g_status_mutex);
    } else {
        memset(&st, 0, sizeof(st));
        st.temperatura = 25.0f;
        st.nivel_percentual = 100.0f;
    }
    
    const char* temp_class = (st.temperatura >= 22 && st.temperatura <= 28) ? "ok" : "warn";
    const char* nivel_class = (st.nivel_percentual >= 50) ? "ok" : "warn";
    const char* turb_class = (st.turbidez < 30) ? "ok" : "warn";
    
    return snprintf(buf, max_len,
        "%s"
        "<div class='card'><h3>🌡️ Temperatura</h3><div class='val %s'>%.1f°C</div></div>"
        "<div class='card'><h3>💧 Umidade</h3><div class='val'>%.1f%%</div></div>"
        "<div class='card'><h3>🌊 Nível</h3><div class='val %s'>%.1fL</div>"
        "<div class='bar'><div class='fill' style='width:%.0f%%'></div></div></div>"
        "<div class='card'><h3>🔬 Turbidez</h3><div class='val %s'>%.1f%%</div></div>"
        "<div class='card'><h3>🐟 Alimentação</h3><div class='val'>%d/2</div></div>"
        "<div class='card'><h3>⚙️ TPA</h3><div class='val'>%s</div></div>"
        "<div class='card'><h3>🔌 Bomba1</h3><div class='val'>%s</div></div>"
        "<div class='card'><h3>🔌 Bomba2</h3><div class='val'>%s</div></div>"
        "%s",
        HTML_HEADER,
        temp_class, st.temperatura,
        st.umidade,
        nivel_class, st.nivel_litros, st.nivel_percentual,
        turb_class, st.turbidez,
        st.alimentacoes_hoje,
        st.tpa_em_andamento ? "ATIVO" : "OFF",
        st.bomba1_ativa ? "ON" : "OFF",
        st.bomba2_ativa ? "ON" : "OFF",
        HTML_FOOTER
    );
}

static int generate_json(char* buf, size_t max_len) {
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    
    system_status_t st;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        st = g_status;
        xSemaphoreGive(g_status_mutex);
    } else {
        memset(&st, 0, sizeof(st));
    }
    
    return snprintf(buf, max_len,
        "%s"
        "{\"temp\":%.1f,\"umid\":%.1f,\"nivel\":%.1f,\"nivel_pct\":%.0f,"
        "\"turb\":%.1f,\"feed\":%d,\"tpa\":%s,\"b1\":%s,\"b2\":%s,\"up\":%lu}",
        JSON_HEADER,
        st.temperatura, st.umidade, st.nivel_litros, st.nivel_percentual,
        st.turbidez, st.alimentacoes_hoje,
        st.tpa_em_andamento ? "true" : "false",
        st.bomba1_ativa ? "true" : "false",
        st.bomba2_ativa ? "true" : "false",
        st.uptime_segundos
    );
}

// ============================================================
// Callbacks TCP
// ============================================================
static void tcp_server_close(struct tcp_pcb* tpcb) {
    if (tpcb == NULL) return;
    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_close(tpcb);
}

static err_t http_recv_callback(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (p == NULL) {
        tcp_server_close(tpcb);
        return ERR_OK;
    }
    
    // Copia request
    char request[128];
    size_t len = (p->len < sizeof(request) - 1) ? p->len : sizeof(request) - 1;
    memcpy(request, p->payload, len);
    request[len] = '\0';
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    // Parse simples
    int resp_len = 0;
    
    if (strstr(request, "GET /api/status") != NULL) {
        resp_len = generate_json(response_buffer, RESPONSE_BUFFER_SIZE);
    } else if (strstr(request, "GET / ") != NULL || strstr(request, "GET /index") != NULL) {
        resp_len = generate_dashboard(response_buffer, RESPONSE_BUFFER_SIZE);
    } else {
        resp_len = snprintf(response_buffer, RESPONSE_BUFFER_SIZE, "%s", HTTP_404);
    }
    
    // Envia resposta
    if (resp_len > 0) {
        err_t wr_err = tcp_write(tpcb, response_buffer, resp_len, TCP_WRITE_FLAG_COPY);
        if (wr_err == ERR_OK) {
            tcp_output(tpcb);
        }
    }
    
    tcp_server_close(tpcb);
    return ERR_OK;
}

static void http_err_callback(void* arg, err_t err) {
    // Erro de conexão - nada a fazer
}

static err_t http_accept_callback(void* arg, struct tcp_pcb* newpcb, err_t err) {
    if (err != ERR_OK || newpcb == NULL) {
        return ERR_VAL;
    }
    
    tcp_setprio(newpcb, TCP_PRIO_MIN);
    tcp_recv(newpcb, http_recv_callback);
    tcp_err(newpcb, http_err_callback);
    
    return ERR_OK;
}

// ============================================================
// Inicialização
// ============================================================
bool wifi_init_ap(void) {
    printf("🌐 Iniciando Wi-Fi em modo Access Point...\n");
    
    // Inicializa CYW43
    if (cyw43_arch_init()) {
        printf("❌ Falha ao inicializar cyw43\n");
        return false;
    }
    
    // Habilita modo AP
    cyw43_arch_enable_ap_mode(WIFI_AP_SSID, WIFI_AP_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    
    // Configura IP estático para AP
    ip4_addr_t gw, mask;
    IP4_ADDR(&gw, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    
    // Inicia servidor DHCP para clientes
    dhcp_server_init(&dhcp_server, &gw, &mask);
    
    printf("✅ Access Point criado!\n");
    printf("   📶 SSID: %s\n", WIFI_AP_SSID);
    printf("   🔑 Senha: %s\n", WIFI_AP_PASSWORD);
    printf("   🌐 IP: 192.168.4.1\n");
    
    return true;
}

const char* wifi_get_ip(void) {
    return "192.168.4.1";
}

bool webserver_init(void) {
    printf("🌐 Iniciando servidor HTTP...\n");
    
    // Inicializa Wi-Fi AP
    if (!wifi_init_ap()) {
        return false;
    }
    
    // Cria PCB TCP
    server_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (server_pcb == NULL) {
        printf("❌ Falha ao criar PCB\n");
        return false;
    }
    
    // Bind na porta 80
    err_t err = tcp_bind(server_pcb, IP_ADDR_ANY, 80);
    if (err != ERR_OK) {
        printf("❌ Falha no bind (err=%d)\n", err);
        tcp_close(server_pcb);
        return false;
    }
    
    // Listen
    server_pcb = tcp_listen_with_backlog(server_pcb, 4);
    if (server_pcb == NULL) {
        printf("❌ Falha no listen\n");
        return false;
    }
    
    tcp_accept(server_pcb, http_accept_callback);
    server_running = true;
    
    printf("✅ Servidor HTTP iniciado!\n");
    printf("   🌐 http://192.168.4.1/\n");
    printf("   📊 http://192.168.4.1/api/status\n");
    
    return true;
}

void webserver_task(void* pvParameters) {
    printf("🌐 Task do servidor web iniciada\n");
    
    // Pequeno delay para outros sistemas iniciarem
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    if (!webserver_init()) {
        printf("❌ Falha ao iniciar servidor web\n");
        vTaskDelete(NULL);
        return;
    }
    
    // Loop - processa eventos de rede
    while (server_running) {
        cyw43_arch_poll();
        vTaskDelay(pdMS_TO_TICKS(10));  // Poll mais frequente
    }
}

void webserver_stop(void) {
    server_running = false;
    if (server_pcb != NULL) {
        tcp_close(server_pcb);
        server_pcb = NULL;
    }
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
    printf("🛑 Servidor HTTP parado\n");
}
