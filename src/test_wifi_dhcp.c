/**
 * HydroSense v3.0 - Versão com DHCP Server
 * 
 * Wi-Fi AP + DHCP + Servidor HTTP
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip4_addr.h"
#include <stdio.h>
#include <string.h>

#define WIFI_SSID "HydroSense"
#define WIFI_PASS "hydro2024"

// ============================================================
// DHCP Server Simples
// ============================================================
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

typedef struct __attribute__((packed)) {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint8_t ciaddr[4];
    uint8_t yiaddr[4];
    uint8_t siaddr[4];
    uint8_t giaddr[4];
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint8_t options[312];
} dhcp_msg_t;

static struct udp_pcb* dhcp_pcb = NULL;
static uint8_t next_client_ip = 100;

static void dhcp_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p, 
                      const ip_addr_t* addr, u16_t port) {
    if (p == NULL || p->len < 240) {
        if (p) pbuf_free(p);
        return;
    }
    
    dhcp_msg_t* req = (dhcp_msg_t*)p->payload;
    
    // Só processa BOOTREQUEST
    if (req->op != 1) {
        pbuf_free(p);
        return;
    }
    
    // Encontra tipo da mensagem
    uint8_t msg_type = 0;
    uint8_t* opt = req->options + 4; // Skip magic cookie
    while (*opt != 255 && opt < req->options + 312) {
        if (*opt == 53) { // Message type
            msg_type = *(opt + 2);
            break;
        }
        opt += 2 + *(opt + 1);
    }
    
    if (msg_type != 1 && msg_type != 3) { // DISCOVER ou REQUEST
        pbuf_free(p);
        return;
    }
    
    // Prepara resposta
    dhcp_msg_t resp;
    memset(&resp, 0, sizeof(resp));
    
    resp.op = 2; // BOOTREPLY
    resp.htype = 1;
    resp.hlen = 6;
    resp.xid = req->xid;
    memcpy(resp.chaddr, req->chaddr, 16);
    
    // IP do cliente
    uint8_t client_ip = next_client_ip;
    if (msg_type == 1) { // DISCOVER - incrementa IP
        next_client_ip++;
        if (next_client_ip > 110) next_client_ip = 100;
    }
    
    resp.yiaddr[0] = 192;
    resp.yiaddr[1] = 168;
    resp.yiaddr[2] = 4;
    resp.yiaddr[3] = client_ip;
    
    // Server IP
    resp.siaddr[0] = 192;
    resp.siaddr[1] = 168;
    resp.siaddr[2] = 4;
    resp.siaddr[3] = 1;
    
    // Magic cookie
    resp.options[0] = 99;
    resp.options[1] = 130;
    resp.options[2] = 83;
    resp.options[3] = 99;
    
    uint8_t* o = resp.options + 4;
    
    // Message type (OFFER ou ACK)
    *o++ = 53; *o++ = 1;
    *o++ = (msg_type == 1) ? 2 : 5;
    
    // Server ID
    *o++ = 54; *o++ = 4;
    *o++ = 192; *o++ = 168; *o++ = 4; *o++ = 1;
    
    // Lease time (1 hora)
    *o++ = 51; *o++ = 4;
    *o++ = 0; *o++ = 0; *o++ = 0x0E; *o++ = 0x10;
    
    // Subnet mask
    *o++ = 1; *o++ = 4;
    *o++ = 255; *o++ = 255; *o++ = 255; *o++ = 0;
    
    // Router
    *o++ = 3; *o++ = 4;
    *o++ = 192; *o++ = 168; *o++ = 4; *o++ = 1;
    
    // DNS
    *o++ = 6; *o++ = 4;
    *o++ = 192; *o++ = 168; *o++ = 4; *o++ = 1;
    
    // End
    *o++ = 255;
    
    pbuf_free(p);
    
    // Envia resposta
    struct pbuf* resp_p = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcp_msg_t), PBUF_RAM);
    if (resp_p) {
        memcpy(resp_p->payload, &resp, sizeof(dhcp_msg_t));
        
        ip_addr_t broadcast;
        IP_ADDR4(&broadcast, 255, 255, 255, 255);
        
        udp_sendto(pcb, resp_p, &broadcast, DHCP_CLIENT_PORT);
        pbuf_free(resp_p);
        
        printf("📡 DHCP: Cliente -> 192.168.4.%d (%s)\n", 
               client_ip, msg_type == 1 ? "OFFER" : "ACK");
    }
}

static void dhcp_server_init(void) {
    printf("🔧 Iniciando servidor DHCP...\n");
    
    dhcp_pcb = udp_new();
    if (!dhcp_pcb) {
        printf("❌ Falha ao criar UDP DHCP\n");
        return;
    }
    
    if (udp_bind(dhcp_pcb, IP_ADDR_ANY, DHCP_SERVER_PORT) != ERR_OK) {
        printf("❌ Falha no bind DHCP\n");
        udp_remove(dhcp_pcb);
        return;
    }
    
    udp_recv(dhcp_pcb, dhcp_recv, NULL);
    printf("✅ Servidor DHCP iniciado (pool: 192.168.4.100-110)\n");
}

// ============================================================
// HTTP Server
// ============================================================
static struct tcp_pcb* http_pcb = NULL;

static const char HTTP_RESPONSE[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: close\r\n"
"\r\n"
"<!DOCTYPE html><html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta http-equiv='refresh' content='5'>"
"<title>HydroSense</title>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);color:#fff;min-height:100vh;padding:20px}"
"h1{color:#00d4ff;text-align:center;font-size:2.5em;margin-bottom:30px;text-shadow:0 0 20px rgba(0,212,255,0.5)}"
".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px;max-width:800px;margin:0 auto}"
".card{background:rgba(255,255,255,0.1);border-radius:15px;padding:20px;text-align:center;backdrop-filter:blur(10px)}"
".card h3{font-size:0.9em;color:#888;margin-bottom:8px}"
".card .val{font-size:2em;font-weight:bold;color:#00d4ff}"
".ok{color:#00ff88}.warn{color:#ffcc00}.err{color:#ff4444}"
".bar{height:10px;background:#333;border-radius:5px;margin-top:10px;overflow:hidden}"
".fill{height:100%;background:linear-gradient(90deg,#00d4ff,#00ff88);border-radius:5px}"
"footer{text-align:center;margin-top:30px;color:#666;font-size:0.9em}"
"</style></head><body>"
"<h1>🐟 HydroSense v3.0</h1>"
"<div class='grid'>"
"<div class='card'><h3>🌡️ Temperatura</h3><div class='val ok'>25.0°C</div></div>"
"<div class='card'><h3>💧 Umidade</h3><div class='val'>60%%</div></div>"
"<div class='card'><h3>🌊 Nível</h3><div class='val ok'>20.0L</div><div class='bar'><div class='fill' style='width:100%%'></div></div></div>"
"<div class='card'><h3>🔬 Turbidez</h3><div class='val ok'>10%%</div></div>"
"<div class='card'><h3>🐟 Alimentação</h3><div class='val'>0/2</div></div>"
"<div class='card'><h3>⚙️ TPA</h3><div class='val'>OFF</div></div>"
"</div>"
"<footer>✅ Sistema Online | Auto-refresh: 5s | IP: 192.168.4.1</footer>"
"</body></html>";

static err_t http_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    tcp_write(tpcb, HTTP_RESPONSE, strlen(HTTP_RESPONSE), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_close(tpcb);
    
    printf("🌐 HTTP: Página enviada\n");
    
    return ERR_OK;
}

static err_t http_accept(void* arg, struct tcp_pcb* newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

static void http_server_init(void) {
    printf("🔧 Iniciando servidor HTTP...\n");
    
    http_pcb = tcp_new();
    if (!http_pcb) {
        printf("❌ Falha ao criar TCP HTTP\n");
        return;
    }
    
    if (tcp_bind(http_pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("❌ Falha no bind HTTP\n");
        return;
    }
    
    http_pcb = tcp_listen(http_pcb);
    tcp_accept(http_pcb, http_accept);
    
    printf("✅ Servidor HTTP iniciado na porta 80\n");
}

// ============================================================
// Main
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  🐟 HydroSense v3.0 - Sistema de Aquicultura Inteligente  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Inicializa Wi-Fi
    printf("🌐 Inicializando Wi-Fi...\n");
    
    if (cyw43_arch_init()) {
        printf("❌ Falha ao inicializar cyw43!\n");
        while (1) sleep_ms(500);
    }
    
    printf("✅ CYW43 inicializado\n");
    
    // Liga LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    printf("💡 LED ligado\n");
    
    // Habilita modo AP
    printf("📶 Criando Access Point...\n");
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
    
    printf("✅ Access Point criado!\n");
    printf("   📶 SSID: %s\n", WIFI_SSID);
    printf("   🔑 Senha: %s\n", WIFI_PASS);
    printf("   🌐 IP: 192.168.4.1\n\n");
    
    // Inicia servidores
    dhcp_server_init();
    http_server_init();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ SISTEMA PRONTO!                                        ║\n");
    printf("║                                                            ║\n");
    printf("║  📶 Conecte ao Wi-Fi: HydroSense                           ║\n");
    printf("║  🔑 Senha: hydro2024                                       ║\n");
    printf("║  🌐 Acesse: http://192.168.4.1                             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Loop principal
    uint32_t counter = 0;
    while (1) {
        cyw43_arch_poll();
        sleep_ms(1);
        
        // Pisca LED a cada 1 segundo
        counter++;
        if (counter >= 1000) {
            counter = 0;
            static bool led_on = true;
            led_on = !led_on;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        }
    }
    
    return 0;
}
