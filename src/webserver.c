/**
 * HydroSense v3.0 - Implementação do Servidor HTTP
 */

#include "webserver.h"
#include "hydrosense_config.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Buffer para respostas HTTP
static char http_buffer[HTTP_BUFFER_SIZE];
static bool server_running = false;
static struct tcp_pcb* server_pcb = NULL;

// Forward declarations
static err_t http_accept(void* arg, struct tcp_pcb* newpcb, err_t err);
static err_t http_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
static void http_close(struct tcp_pcb* tpcb);

// ============================================================
// HTML do Dashboard
// ============================================================
static const char* HTML_DASHBOARD_HEADER = 
"<!DOCTYPE html>\n"
"<html lang='pt-BR'>\n"
"<head>\n"
"    <meta charset='UTF-8'>\n"
"    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
"    <meta http-equiv='refresh' content='5'>\n"
"    <title>HydroSense - Dashboard</title>\n"
"    <style>\n"
"        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
"        body {\n"
"            font-family: 'Segoe UI', Arial, sans-serif;\n"
"            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);\n"
"            color: #fff;\n"
"            min-height: 100vh;\n"
"            padding: 20px;\n"
"        }\n"
"        .header {\n"
"            text-align: center;\n"
"            padding: 20px;\n"
"            margin-bottom: 30px;\n"
"        }\n"
"        .header h1 {\n"
"            font-size: 2.5em;\n"
"            color: #00d4ff;\n"
"            text-shadow: 0 0 20px rgba(0,212,255,0.5);\n"
"        }\n"
"        .header .version {\n"
"            color: #888;\n"
"            font-size: 0.9em;\n"
"        }\n"
"        .dashboard {\n"
"            display: grid;\n"
"            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));\n"
"            gap: 20px;\n"
"            max-width: 1400px;\n"
"            margin: 0 auto;\n"
"        }\n"
"        .card {\n"
"            background: rgba(255,255,255,0.1);\n"
"            border-radius: 15px;\n"
"            padding: 25px;\n"
"            backdrop-filter: blur(10px);\n"
"            border: 1px solid rgba(255,255,255,0.1);\n"
"            transition: transform 0.3s;\n"
"        }\n"
"        .card:hover {\n"
"            transform: translateY(-5px);\n"
"        }\n"
"        .card-title {\n"
"            font-size: 1em;\n"
"            color: #888;\n"
"            margin-bottom: 10px;\n"
"            display: flex;\n"
"            align-items: center;\n"
"            gap: 10px;\n"
"        }\n"
"        .card-value {\n"
"            font-size: 2.5em;\n"
"            font-weight: bold;\n"
"            margin-bottom: 5px;\n"
"        }\n"
"        .card-status {\n"
"            font-size: 0.9em;\n"
"            padding: 5px 10px;\n"
"            border-radius: 20px;\n"
"            display: inline-block;\n"
"        }\n"
"        .status-ok { background: #00c853; color: #fff; }\n"
"        .status-warning { background: #ffc107; color: #000; }\n"
"        .status-critical { background: #ff1744; color: #fff; }\n"
"        .status-active { background: #2196f3; color: #fff; }\n"
"        .nivel-bar {\n"
"            width: 100%;\n"
"            height: 20px;\n"
"            background: rgba(255,255,255,0.2);\n"
"            border-radius: 10px;\n"
"            overflow: hidden;\n"
"            margin-top: 10px;\n"
"        }\n"
"        .nivel-fill {\n"
"            height: 100%;\n"
"            background: linear-gradient(90deg, #00d4ff, #00ff88);\n"
"            border-radius: 10px;\n"
"            transition: width 0.5s;\n"
"        }\n"
"        .logs {\n"
"            background: rgba(0,0,0,0.3);\n"
"            border-radius: 10px;\n"
"            padding: 15px;\n"
"            max-height: 300px;\n"
"            overflow-y: auto;\n"
"            font-family: monospace;\n"
"            font-size: 0.85em;\n"
"        }\n"
"        .log-entry {\n"
"            padding: 5px 0;\n"
"            border-bottom: 1px solid rgba(255,255,255,0.1);\n"
"        }\n"
"        .timestamp { color: #00d4ff; }\n"
"        .icon { font-size: 1.5em; }\n"
"        .footer {\n"
"            text-align: center;\n"
"            padding: 30px;\n"
"            color: #666;\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class='header'>\n"
"        <h1>🐟 HydroSense Dashboard</h1>\n"
"        <p class='version'>v3.0.0 - Sistema de Aquicultura Inteligente</p>\n"
"    </div>\n"
"    <div class='dashboard'>\n";

static const char* HTML_DASHBOARD_FOOTER =
"    </div>\n"
"    <div class='footer'>\n"
"        <p>Atualização automática a cada 5 segundos</p>\n"
"        <p>HydroSense &copy; 2024 - Aquicultura + Hidroponia</p>\n"
"    </div>\n"
"</body>\n"
"</html>\n";

// ============================================================
// Inicialização Wi-Fi
// ============================================================
bool wifi_init_ap(void) {
    printf("🌐 Iniciando Wi-Fi em modo Access Point...\n");
    
    if (cyw43_arch_init()) {
        printf("❌ Falha ao inicializar cyw43\n");
        return false;
    }
    
    cyw43_arch_enable_ap_mode(WIFI_AP_SSID, WIFI_AP_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    
    printf("✅ Access Point criado!\n");
    printf("   📶 SSID: %s\n", WIFI_AP_SSID);
    printf("   🔑 Senha: %s\n", WIFI_AP_PASSWORD);
    printf("   🌐 IP: 192.168.4.1\n");
    
    return true;
}

const char* wifi_get_ip(void) {
    return "192.168.4.1";
}

// ============================================================
// Geração de Conteúdo HTML
// ============================================================
static int generate_dashboard_html(char* buffer, size_t max_len) {
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    
    system_status_t status;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        status = g_status;
        xSemaphoreGive(g_status_mutex);
    } else {
        memset(&status, 0, sizeof(status));
    }
    
    // Determina status de cada parâmetro
    const char* temp_status = (status.temperatura >= TEMP_MIN_IDEAL && 
                               status.temperatura <= TEMP_MAX_IDEAL) ? "status-ok" : "status-warning";
    const char* nivel_status = (status.nivel_percentual >= 80) ? "status-ok" : 
                               (status.nivel_percentual >= 50) ? "status-warning" : "status-critical";
    const char* turbidez_status = (status.turbidez < 30) ? "status-ok" : 
                                  (status.turbidez < 60) ? "status-warning" : "status-critical";
    
    int len = snprintf(buffer, max_len,
        "%s"
        
        // Card Temperatura
        "<div class='card'>\n"
        "    <div class='card-title'><span class='icon'>🌡️</span> Temperatura</div>\n"
        "    <div class='card-value'>%.1f°C</div>\n"
        "    <span class='card-status %s'>%s</span>\n"
        "</div>\n"
        
        // Card Umidade
        "<div class='card'>\n"
        "    <div class='card-title'><span class='icon'>💧</span> Umidade</div>\n"
        "    <div class='card-value'>%.1f%%</div>\n"
        "    <span class='card-status status-ok'>Normal</span>\n"
        "</div>\n"
        
        // Card Nível de Água
        "<div class='card'>\n"
        "    <div class='card-title'><span class='icon'>🌊</span> Nível da Água</div>\n"
        "    <div class='card-value'>%.1fL</div>\n"
        "    <span class='card-status %s'>%.0f%%</span>\n"
        "    <div class='nivel-bar'><div class='nivel-fill' style='width: %.0f%%;'></div></div>\n"
        "</div>\n"
        
        // Card Turbidez
        "<div class='card'>\n"
        "    <div class='card-title'><span class='icon'>🔬</span> Turbidez</div>\n"
        "    <div class='card-value'>%.1f%%</div>\n"
        "    <span class='card-status %s'>%s</span>\n"
        "</div>\n"
        
        // Card Alimentação
        "<div class='card'>\n"
        "    <div class='card-title'><span class='icon'>🐟</span> Alimentação</div>\n"
        "    <div class='card-value'>%d/2</div>\n"
        "    <span class='card-status status-ok'>Próx: %s</span>\n"
        "</div>\n"
        
        // Card Bombas
        "<div class='card'>\n"
        "    <div class='card-title'><span class='icon'>⚙️</span> Sistema TPA</div>\n"
        "    <div class='card-value'>%s</div>\n"
        "    <span class='card-status %s'>Bomba1: %s | Bomba2: %s</span>\n"
        "</div>\n"
        
        "%s",
        
        HTML_DASHBOARD_HEADER,
        
        // Temperatura
        status.temperatura,
        temp_status,
        (status.temperatura >= TEMP_MIN_IDEAL && status.temperatura <= TEMP_MAX_IDEAL) ? "Ideal" : "Atenção",
        
        // Umidade
        status.umidade,
        
        // Nível
        status.nivel_litros,
        nivel_status,
        status.nivel_percentual,
        status.nivel_percentual,
        
        // Turbidez
        status.turbidez,
        turbidez_status,
        (status.turbidez < 30) ? "Água Limpa" : (status.turbidez < 60) ? "Atenção" : "Suja",
        
        // Alimentação
        status.alimentacoes_hoje,
        (status.alimentacoes_hoje == 0) ? "08:00" : (status.alimentacoes_hoje == 1) ? "16:00" : "Concluído",
        
        // TPA
        status.tpa_em_andamento ? "ATIVO" : "Inativo",
        status.tpa_em_andamento ? "status-active" : "status-ok",
        status.bomba1_ativa ? "ON" : "OFF",
        status.bomba2_ativa ? "ON" : "OFF",
        
        HTML_DASHBOARD_FOOTER
    );
    
    return len;
}

static int generate_json_status(char* buffer, size_t max_len) {
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    
    system_status_t status;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        status = g_status;
        xSemaphoreGive(g_status_mutex);
    } else {
        memset(&status, 0, sizeof(status));
    }
    
    return snprintf(buffer, max_len,
        "{"
        "\"temperatura\":%.2f,"
        "\"umidade\":%.2f,"
        "\"nivel_litros\":%.2f,"
        "\"nivel_percentual\":%.2f,"
        "\"turbidez\":%.2f,"
        "\"bomba1\":%s,"
        "\"bomba2\":%s,"
        "\"tpa_ativo\":%s,"
        "\"alimentacoes_hoje\":%d,"
        "\"uptime\":%lu,"
        "\"wifi\":%s"
        "}",
        status.temperatura,
        status.umidade,
        status.nivel_litros,
        status.nivel_percentual,
        status.turbidez,
        status.bomba1_ativa ? "true" : "false",
        status.bomba2_ativa ? "true" : "false",
        status.tpa_em_andamento ? "true" : "false",
        status.alimentacoes_hoje,
        status.uptime_segundos,
        status.wifi_conectado ? "true" : "false"
    );
}

// ============================================================
// Handlers HTTP
// ============================================================
static err_t http_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (p == NULL) {
        http_close(tpcb);
        return ERR_OK;
    }
    
    // Lê requisição
    char request[256];
    size_t len = (p->len < sizeof(request) - 1) ? p->len : sizeof(request) - 1;
    memcpy(request, p->payload, len);
    request[len] = '\0';
    
    pbuf_free(p);
    
    // Parse da URL
    char* method = strtok(request, " ");
    char* url = strtok(NULL, " ");
    
    if (method == NULL || url == NULL) {
        http_close(tpcb);
        return ERR_OK;
    }
    
    // Gera resposta
    int content_len = 0;
    const char* content_type = "text/html";
    
    if (strcmp(url, "/") == 0 || strcmp(url, "/dashboard") == 0) {
        content_len = generate_dashboard_html(http_buffer + 200, HTTP_BUFFER_SIZE - 200);
    } 
    else if (strcmp(url, "/api/status") == 0) {
        content_type = "application/json";
        content_len = generate_json_status(http_buffer + 200, HTTP_BUFFER_SIZE - 200);
    }
    else {
        // 404
        content_len = snprintf(http_buffer + 200, HTTP_BUFFER_SIZE - 200,
            "<html><body><h1>404 Not Found</h1></body></html>");
    }
    
    // Header HTTP
    int header_len = snprintf(http_buffer, 200,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        content_type, content_len);
    
    // Move conteúdo para após o header
    memmove(http_buffer + header_len, http_buffer + 200, content_len);
    
    // Envia resposta
    tcp_write(tpcb, http_buffer, header_len + content_len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    
    http_close(tpcb);
    return ERR_OK;
}

static void http_close(struct tcp_pcb* tpcb) {
    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_close(tpcb);
}

static err_t http_accept(void* arg, struct tcp_pcb* newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

// ============================================================
// Funções Públicas
// ============================================================
bool webserver_init(void) {
    printf("🌐 Iniciando servidor HTTP...\n");
    
    // Inicializa Wi-Fi em modo AP
    if (!wifi_init_ap()) {
        return false;
    }
    
    // Cria servidor TCP
    server_pcb = tcp_new();
    if (server_pcb == NULL) {
        printf("❌ Falha ao criar PCB TCP\n");
        return false;
    }
    
    err_t err = tcp_bind(server_pcb, IP_ADDR_ANY, WEBSERVER_PORT);
    if (err != ERR_OK) {
        printf("❌ Falha ao fazer bind na porta %d\n", WEBSERVER_PORT);
        return false;
    }
    
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, http_accept);
    
    server_running = true;
    
    printf("✅ Servidor HTTP iniciado!\n");
    printf("   🌐 Acesse: http://192.168.4.1/\n");
    printf("   📊 API: http://192.168.4.1/api/status\n");
    
    // Atualiza status global
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_status.wifi_conectado = true;
        xSemaphoreGive(g_status_mutex);
    }
    
    return true;
}

void webserver_task(void* pvParameters) {
    printf("🌐 Task do servidor web iniciada\n");
    
    // Inicializa servidor
    if (!webserver_init()) {
        printf("❌ Falha ao iniciar servidor web\n");
        vTaskDelete(NULL);
        return;
    }
    
    // Loop principal - processa eventos de rede
    while (server_running) {
        cyw43_arch_poll();
        vTaskDelay(pdMS_TO_TICKS(TASK_WEBSERVER_INTERVAL_MS));
    }
}

void webserver_stop(void) {
    if (server_pcb != NULL) {
        tcp_close(server_pcb);
        server_pcb = NULL;
    }
    server_running = false;
    cyw43_arch_deinit();
    printf("🛑 Servidor HTTP parado\n");
}
