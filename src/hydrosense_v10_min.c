/**
 * HydroSense MINIMO - Apenas USB serial + WiFi
 * Sem sensores, sem display, sem servo, sem relés
 * Propósito: diagnosticar se o USB CDC funciona
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <stdio.h>
#include <string.h>

#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"

// Dados simulados para teste
static float g_temp = 25.5f;
static float g_hum = 65.0f;
static uint16_t g_dist = 120;
static float g_nivel = 75.0f;
static float g_volume = 15.0f;
static uint32_t g_count = 0;
static char g_ip[20] = "0.0.0.0";

// === HTTP Server simples ===
static err_t http_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t http_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) { tcp_close(tpcb); return ERR_OK; }
    tcp_recved(tpcb, p->tot_len);
    
    char *req = (char*)malloc(p->tot_len + 1);
    if (!req) { pbuf_free(p); tcp_close(tpcb); return ERR_OK; }
    pbuf_copy_partial(p, req, p->tot_len, 0);
    req[p->tot_len] = '\0';
    pbuf_free(p);
    
    char resp[1024];
    
    if (strstr(req, "GET /sensors") || strstr(req, "GET /api")) {
        snprintf(resp, sizeof(resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: close\r\n\r\n"
            "{\"temperatura\":%.1f,\"umidade\":%.1f,\"distancia\":%d,"
            "\"nivel\":%.1f,\"volume\":%.1f,\"corAgua\":\"cristalino\","
            "\"wifiStatus\":true,\"contadorLeituras\":%d,"
            "\"deviceIp\":\"%s\"}",
            g_temp, g_hum, g_dist, g_nivel, g_volume, g_count, g_ip);
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
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n"
            "<html><body><h1>HydroSense v10</h1>"
            "<p>T=%.1fC H=%.1f%% N=%.1f%%</p>"
            "<p><a href='/sensors'>API JSON</a></p>"
            "</body></html>",
            g_temp, g_hum, g_nivel);
    }
    
    free(req);
    tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    return ERR_OK;
}

static err_t http_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || !newpcb) return ERR_VAL;
    tcp_setprio(newpcb, TCP_PRIO_MIN);
    tcp_recv(newpcb, http_recv_cb);
    tcp_sent(newpcb, http_sent_cb);
    return ERR_OK;
}

int main() {
    // ETAPA 1: USB Serial
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n");
    printf("========================================\n");
    printf("  HydroSense v10 - MINIMO DIAGNOSTICO\n");
    printf("========================================\n");
    printf("[OK] USB Serial funcionando!\n\n");
    
    // ETAPA 2: CYW43 init
    printf("[INIT] Inicializando WiFi chip...\n");
    if (cyw43_arch_init()) {
        printf("[ERRO] CYW43 falhou!\n");
        // Loop piscando LED
        while(1) {
            printf(".");
            sleep_ms(1000);
        }
    }
    cyw43_arch_enable_sta_mode();
    printf("[OK] CYW43 inicializado\n");
    
    // ETAPA 3: Conectar WiFi
    printf("[WIFI] Rede: %s\n", WIFI_SSID);
    printf("[WIFI] Senha: %s\n", WIFI_PASSWORD);
    
    int result = -1;
    
    // Tentativa 1: WPA2 AES
    printf("[WIFI] Tentativa 1/3 - WPA2 AES (30s)...\n");
    result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000);
    
    if (result != 0) {
        printf("[WIFI] T1 falhou (err=%d)\n", result);
        sleep_ms(2000);
        
        // Tentativa 2: WPA2 MIXED
        printf("[WIFI] Tentativa 2/3 - WPA2 MIXED (30s)...\n");
        result = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000);
    }
    
    if (result != 0) {
        printf("[WIFI] T2 falhou (err=%d)\n", result);
        sleep_ms(2000);
        
        // Tentativa 3: WPA TKIP
        printf("[WIFI] Tentativa 3/3 - WPA TKIP (30s)...\n");
        result = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA_TKIP_PSK, 30000);
    }
    
    if (result != 0) {
        printf("[WIFI] FALHOU! Erro=%d\n", result);
        printf("[WIFI] Continuando offline...\n\n");
    } else {
        const char *ip = ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr);
        strncpy(g_ip, ip, sizeof(g_ip)-1);
        printf("[WIFI] CONECTADO! IP: %s\n", g_ip);
        
        // Iniciar HTTP server
        struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
        if (pcb) {
            tcp_bind(pcb, IP_ANY_TYPE, 80);
            pcb = tcp_listen_with_backlog(pcb, 1);
            if (pcb) {
                tcp_accept(pcb, http_accept_cb);
                printf("[HTTP] Servidor: http://%s\n", g_ip);
                printf("[HTTP] API:      http://%s/sensors\n\n", g_ip);
            }
        }
    }
    
    // ETAPA 4: Loop
    printf("=== MONITORAMENTO ATIVO ===\n\n");
    
    while (true) {
        g_count++;
        
        // Dados simulados (sem sensores por enquanto)
        g_temp = 25.0f + (float)(g_count % 10) * 0.3f;
        g_hum = 60.0f + (float)(g_count % 20) * 0.5f;
        g_nivel = 75.0f;
        g_volume = 15.0f;
        
        printf("#%d T=%.1fC H=%.1f%% N=%.0f%% WiFi:%s\n",
               g_count, g_temp, g_hum, g_nivel,
               result == 0 ? "OK" : "OFF");
        
        // ESSENCIAL: poll do lwip
        cyw43_arch_poll();
        
        sleep_ms(2000);
    }
    
    return 0;
}
