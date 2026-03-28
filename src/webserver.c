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
// HTML do Dashboard Completo (Unificado com Frontend Principal)
// ============================================================
static const char* HTML_FULL_DASHBOARD = 
"<!DOCTYPE html>\n"
"<html lang='pt-BR'>\n"
"<head>\n"
"<meta charset='UTF-8'>\n"
"<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
"<title>HydroSense v10</title>\n"
"<style>\n"
"*{margin:0;padding:0;box-sizing:border-box}\n"
"body{font-family:'Segoe UI',Arial,sans-serif;background:#1a1f2e;color:#fff;min-height:100vh}\n"
".header{text-align:center;padding:15px;background:linear-gradient(135deg,#1a2633,#0d1117);border-bottom:1px solid #30363d}\n"
".header h1{color:#58a6ff;font-size:1.8em}\n"
".header p{color:#8b949e;font-size:0.85em}\n"
".status-badge{display:inline-block;padding:4px 12px;border-radius:20px;font-size:0.75em;margin-top:8px}\n"
".status-online{background:#238636;color:#fff}\n"
".status-offline{background:#f85149;color:#fff}\n"
".main{display:grid;grid-template-columns:1fr 1fr;gap:15px;padding:15px;max-width:1400px;margin:0 auto}\n"
"@media(max-width:900px){.main{grid-template-columns:1fr}}\n"
".section{background:#21262d;border-radius:12px;padding:15px;border:1px solid #30363d}\n"
".section-title{color:#58a6ff;font-size:1em;margin-bottom:15px;display:flex;align-items:center;gap:8px}\n"
".sensors-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}\n"
".sensor-card{background:#161b22;border-radius:8px;padding:12px;text-align:center;border:1px solid #30363d}\n"
".sensor-icon{font-size:1.5em;margin-bottom:5px}\n"
".sensor-value{font-size:1.8em;font-weight:bold;color:#58a6ff}\n"
".sensor-label{color:#8b949e;font-size:0.75em;margin-top:3px}\n"
".nivel-bar{width:100%;height:12px;background:#30363d;border-radius:6px;overflow:hidden;margin-top:10px}\n"
".nivel-fill{height:100%;background:linear-gradient(90deg,#238636,#3fb950);transition:width 0.5s}\n"
".controls-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}\n"
".control-card{background:#161b22;border-radius:8px;padding:12px;text-align:center;border:1px solid #30363d}\n"
".control-icon{font-size:1.3em;margin-bottom:5px}\n"
".control-name{font-size:0.85em;color:#c9d1d9}\n"
".control-status{font-size:0.7em;padding:3px 8px;border-radius:10px;margin:5px 0}\n"
".status-on{background:#238636;color:#fff}\n"
".status-off{background:#484f58;color:#8b949e}\n"
".btn{width:100%;padding:8px;border:none;border-radius:6px;cursor:pointer;font-weight:bold;font-size:0.85em;transition:all 0.2s}\n"
".btn-on{background:#238636;color:#fff}\n"
".btn-on:hover{background:#2ea043}\n"
".btn-off{background:#f85149;color:#fff}\n"
".btn-off:hover{background:#da3633}\n"
".btn-activate{background:#1f6feb;color:#fff}\n"
".btn-activate:hover{background:#388bfd}\n"
".alimentacao{text-align:center;padding:15px}\n"
".relogio{font-size:2.8em;font-weight:bold;color:#3fb950;font-family:monospace}\n"
".data{color:#8b949e;font-size:0.85em;margin:5px 0}\n"
".info-alimentacao{color:#f0883e;font-size:0.8em;margin:5px 0}\n"
".btn-alimentar{display:block;margin:15px auto 0;padding:12px 30px;background:linear-gradient(135deg,#f0883e,#d29922);border:none;border-radius:25px;color:#fff;font-weight:bold;cursor:pointer;font-size:0.95em}\n"
".btn-alimentar:hover{transform:scale(1.02)}\n"
".logs{background:#161b22;border-radius:8px;padding:10px;max-height:200px;overflow-y:auto;font-family:monospace;font-size:0.75em}\n"
".log-entry{padding:4px 0;border-bottom:1px solid #30363d;display:flex;gap:8px}\n"
".log-time{color:#58a6ff;min-width:55px}\n"
".log-msg{color:#c9d1d9}\n"
".stats{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-top:15px;text-align:center}\n"
".stat-value{font-size:1.2em;font-weight:bold;color:#58a6ff}\n"
".stat-label{font-size:0.7em;color:#8b949e}\n"
".footer{text-align:center;padding:10px;color:#484f58;font-size:0.75em}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class='header'>\n"
"<h1>🐟 HydroSense v10</h1>\n"
"<p>Sistema Inteligente de Aquicultura — BitDogLab + Pico W</p>\n"
"<span id='badge' class='status-badge status-online'>● Conectado</span>\n"
"</div>\n"
"<div class='main'>\n"
"<div class='section'>\n"
"<div class='section-title'>📊 Sensores em Tempo Real</div>\n"
"<div class='sensors-grid'>\n"
"<div class='sensor-card'><div class='sensor-icon'>🌡️</div><div id='temp' class='sensor-value'>--</div><div class='sensor-label'>°C TEMPERATURA</div></div>\n"
"<div class='sensor-card'><div class='sensor-icon'>💧</div><div id='umid' class='sensor-value'>--</div><div class='sensor-label'>%% UMIDADE</div></div>\n"
"<div class='sensor-card'><div class='sensor-icon'>📏</div><div id='dist' class='sensor-value'>--</div><div class='sensor-label'>mm DISTÂNCIA</div></div>\n"
"<div class='sensor-card'><div class='sensor-icon'>🌊</div><div id='nivel' class='sensor-value'>--</div><div class='sensor-label'>%% NÍVEL DA ÁGUA</div></div>\n"
"</div>\n"
"<div class='sensor-card' style='margin-top:10px'>\n"
"<div class='sensor-icon'>🫧</div><div id='vol' class='sensor-value'>--</div><div class='sensor-label'>Litros VOLUME DO AQUÁRIO</div>\n"
"<div class='nivel-bar'><div id='nivelBar' class='nivel-fill' style='width:0%%'></div></div>\n"
"<div style='color:#8b949e;font-size:0.7em;margin-top:5px'>Nível <span id='nivelTxt'>0</span>%% - <span id='volTxt'>0</span>L de 20L</div>\n"
"</div>\n"
"<div class='sensor-card' style='margin-top:10px'>\n"
"<div class='sensor-icon'>🔬</div><span id='turb' class='sensor-value' style='font-size:1.3em'>--</span>\n"
"<div class='sensor-label'>COR / QUALIDADE DA ÁGUA</div>\n"
"</div>\n"
"</div>\n"
"<div class='section'>\n"
"<div class='section-title'>⚡ Controle de Relés — Estado em Tempo Real</div>\n"
"<div class='controls-grid'>\n"
"<div class='control-card'>\n"
"<div class='control-icon'>🌀</div><div class='control-name'>Ventilador</div><div class='control-status status-off' id='ventSt'>● OFF</div>\n"
"<button class='btn btn-on' onclick=\"cmd('rele/1/on')\">LIGAR</button>\n"
"</div>\n"
"<div class='control-card'>\n"
"<div class='control-icon'>⬇️</div><div class='control-name'>Bomba 1</div><div class='control-status status-off' id='b1St'>● OFF</div>\n"
"<button class='btn btn-on' onclick=\"cmd('rele/2/on')\">LIGAR</button>\n"
"</div>\n"
"<div class='control-card'>\n"
"<div class='control-icon'>⬆️</div><div class='control-name'>Bomba 2</div><div class='control-status status-off' id='b2St'>● OFF</div>\n"
"<button class='btn btn-on' onclick=\"cmd('rele/3/on')\">LIGAR</button>\n"
"</div>\n"
"<div class='control-card'>\n"
"<div class='control-icon'>✅</div><div class='control-name'>Ligar Tudo</div>\n"
"<button class='btn btn-on' onclick=\"cmd('rele/all/on')\" style='margin-top:22px'>LIGAR TUDO</button>\n"
"</div>\n"
"<div class='control-card'>\n"
"<div class='control-icon'>⛔</div><div class='control-name'>Desligar Tudo</div>\n"
"<button class='btn btn-off' onclick=\"cmd('rele/all/off')\" style='margin-top:22px'>DESLIGAR TUDO</button>\n"
"</div>\n"
"<div class='control-card'>\n"
"<div class='control-icon'>🔄</div><div class='control-name'>TPA Manual</div><div class='control-status status-off' id='tpaSt'>● INATIVO</div>\n"
"<button class='btn btn-activate' onclick=\"cmd('tpa/start')\">INICIAR TPA</button>\n"
"</div>\n"
"</div>\n"
"</div>\n"
"<div class='section alimentacao'>\n"
"<div class='section-title' style='justify-content:center'>🐟 Sistema de Alimentação</div>\n"
"<div id='clock' class='relogio'>--:--:--</div>\n"
"<div id='date' class='data'>-- de -------- de ----</div>\n"
"<div class='info-alimentacao'>● Alimentação automática: 08:00 e 16:00</div>\n"
"<div style='color:#8b949e;font-size:0.8em'>Servo SG90 - GPIO 16</div>\n"
"<button class='btn-alimentar' onclick=\"cmd('feed')\">🐟 Alimentar Agora</button>\n"
"<div class='stats'>\n"
"<div><div id='feeds' class='stat-value'>0</div><div class='stat-label'>Alimentações Hoje</div></div>\n"
"<div><div id='reads' class='stat-value'>0</div><div class='stat-label'>Leituras</div></div>\n"
"<div><div id='ip' class='stat-value'>--</div><div class='stat-label'>IP do Dispositivo</div></div>\n"
"</div>\n"
"</div>\n"
"<div class='section'>\n"
"<div class='section-title'>📋 Log de Eventos</div>\n"
"<div id='logs' class='logs'>\n"
"<div class='log-entry'><span class='log-time'>--:--:--</span><span class='log-msg'>Aguardando dados...</span></div>\n"
"</div>\n"
"</div>\n"
"</div>\n"
"<div class='footer'>Última atualização: <span id='lastUp'>--:--:--</span> · Fonte: Pico W Embarcado</div>\n"
"<script>\n"
"const meses=['Janeiro','Fevereiro','Marco','Abril','Maio','Junho','Julho','Agosto','Setembro','Outubro','Novembro','Dezembro'];\n"
"let logs=[],reads=0;\n"
"function pad(n){return n<10?'0'+n:n}\n"
"function updateClock(){\n"
"const d=new Date();\n"
"document.getElementById('clock').textContent=pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds());\n"
"document.getElementById('date').textContent=d.getDate()+' de '+meses[d.getMonth()]+' de '+d.getFullYear();\n"
"}\n"
"function addLog(msg){const d=new Date();logs.unshift({t:pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds()),m:msg});if(logs.length>20)logs.pop();renderLogs()}\n"
"function renderLogs(){document.getElementById('logs').innerHTML=logs.map(l=>'<div class=\"log-entry\"><span class=\"log-time\">'+l.t+'</span><span class=\"log-msg\">'+l.m+'</span></div>').join('')}\n"
"function cmd(c){fetch('/api/'+c).then(()=>addLog('Comando: '+c)).catch(()=>addLog('Erro: '+c))}\n"
"function getData(){\n"
"fetch('/api/status').then(r=>r.json()).then(d=>{\n"
"document.getElementById('temp').textContent=d.temperatura.toFixed(1);\n"
"document.getElementById('umid').textContent=d.umidade.toFixed(0);\n"
"document.getElementById('dist').textContent=Math.round(d.nivel_litros*10);\n"
"document.getElementById('nivel').textContent=d.nivel_percentual.toFixed(0);\n"
"document.getElementById('vol').textContent=d.nivel_litros.toFixed(1);\n"
"document.getElementById('nivelBar').style.width=d.nivel_percentual+'%%';\n"
"document.getElementById('nivelTxt').textContent=d.nivel_percentual.toFixed(0);\n"
"document.getElementById('volTxt').textContent=d.nivel_litros.toFixed(1);\n"
"document.getElementById('turb').textContent=d.turbidez<30?'Limpa':d.turbidez<60?'Turva':'Suja';\n"
"document.getElementById('feeds').textContent=d.alimentacoes_hoje;\n"
"reads++;document.getElementById('reads').textContent=reads;\n"
"document.getElementById('ip').textContent=location.hostname;\n"
"document.getElementById('ventSt').textContent=d.bomba1?'● ON':'● OFF';\n"
"document.getElementById('ventSt').className='control-status '+(d.bomba1?'status-on':'status-off');\n"
"document.getElementById('b1St').textContent=d.bomba1?'● ON':'● OFF';\n"
"document.getElementById('b1St').className='control-status '+(d.bomba1?'status-on':'status-off');\n"
"document.getElementById('b2St').textContent=d.bomba2?'● ON':'● OFF';\n"
"document.getElementById('b2St').className='control-status '+(d.bomba2?'status-on':'status-off');\n"
"document.getElementById('tpaSt').textContent=d.tpa_ativo?'● ATIVO':'● INATIVO';\n"
"document.getElementById('tpaSt').className='control-status '+(d.tpa_ativo?'status-on':'status-off');\n"
"const now=new Date();document.getElementById('lastUp').textContent=pad(now.getHours())+':'+pad(now.getMinutes())+':'+pad(now.getSeconds());\n"
"}).catch(()=>{document.getElementById('badge').className='status-badge status-offline';document.getElementById('badge').textContent='● Offline'});\n"
"}\n"
"setInterval(updateClock,1000);\n"
"setInterval(getData,2000);\n"
"updateClock();getData();\n"
"addLog('🚀 HydroSense v10 iniciando...');\n"
"addLog('🌐 Conectado ao Pico W');\n"
"</script>\n"
"</body></html>\n";

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
    // Copia o HTML completo para o buffer
    size_t html_len = strlen(HTML_FULL_DASHBOARD);
    if (html_len >= max_len) {
        html_len = max_len - 1;
    }
    memcpy(buffer, HTML_FULL_DASHBOARD, html_len);
    buffer[html_len] = '\0';
    return (int)html_len;
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
    // Comandos de controle de relés
    else if (strncmp(url, "/api/rele/", 10) == 0) {
        content_type = "application/json";
        extern system_status_t g_status;
        extern SemaphoreHandle_t g_status_mutex;
        
        bool success = false;
        if (strstr(url, "/1/on")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba1_ativa = true;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        } else if (strstr(url, "/1/off")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba1_ativa = false;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        } else if (strstr(url, "/2/on")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba1_ativa = true;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        } else if (strstr(url, "/2/off")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba1_ativa = false;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        } else if (strstr(url, "/3/on")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba2_ativa = true;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        } else if (strstr(url, "/3/off")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba2_ativa = false;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        } else if (strstr(url, "/all/on")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba1_ativa = true;
                g_status.bomba2_ativa = true;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        } else if (strstr(url, "/all/off")) {
            if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_status.bomba1_ativa = false;
                g_status.bomba2_ativa = false;
                xSemaphoreGive(g_status_mutex);
                success = true;
            }
        }
        content_len = snprintf(http_buffer + 200, HTTP_BUFFER_SIZE - 200,
            "{\"success\":%s}", success ? "true" : "false");
    }
    // Comando de alimentação
    else if (strcmp(url, "/api/feed") == 0) {
        content_type = "application/json";
        extern system_status_t g_status;
        extern SemaphoreHandle_t g_status_mutex;
        if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_status.alimentacoes_hoje++;
            xSemaphoreGive(g_status_mutex);
        }
        printf("🐟 Alimentação manual via web\n");
        content_len = snprintf(http_buffer + 200, HTTP_BUFFER_SIZE - 200,
            "{\"success\":true,\"message\":\"Alimentacao executada\"}");
    }
    // Comando TPA
    else if (strncmp(url, "/api/tpa/", 9) == 0) {
        content_type = "application/json";
        extern system_status_t g_status;
        extern SemaphoreHandle_t g_status_mutex;
        bool start = strstr(url, "start") != NULL;
        if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_status.tpa_em_andamento = start;
            xSemaphoreGive(g_status_mutex);
        }
        printf("🔄 TPA %s via web\n", start ? "iniciado" : "parado");
        content_len = snprintf(http_buffer + 200, HTTP_BUFFER_SIZE - 200,
            "{\"success\":true,\"tpa_active\":%s}", start ? "true" : "false");
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
