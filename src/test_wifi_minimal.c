/**
 * HydroSense v3.0 - Versão Mínima de Teste
 * 
 * Testa apenas Wi-Fi AP e servidor HTTP básico
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <stdio.h>
#include <string.h>

#define WIFI_SSID "HydroSense"
#define WIFI_PASS "hydro2024"

static struct tcp_pcb* server_pcb = NULL;

// HTML simples
static const char HTTP_RESPONSE[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<!DOCTYPE html><html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>HydroSense</title>"
"<style>"
"body{font-family:Arial;background:#1a1a2e;color:#fff;text-align:center;padding:50px}"
"h1{color:#00d4ff;font-size:3em}"
".card{background:#2a2a4e;border-radius:15px;padding:30px;margin:20px auto;max-width:400px}"
".ok{color:#0f0;font-size:2em}"
"</style></head><body>"
"<h1>🐟 HydroSense</h1>"
"<div class='card'>"
"<p class='ok'>✅ Sistema Funcionando!</p>"
"<p>Temperatura: 25.0°C</p>"
"<p>Nivel: 20.0L (100%%)</p>"
"<p>Turbidez: 10%%</p>"
"</div>"
"<p>IP: 192.168.4.1</p>"
"</body></html>";

static err_t http_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    // Envia resposta
    tcp_write(tpcb, HTTP_RESPONSE, strlen(HTTP_RESPONSE), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_close(tpcb);
    
    printf("📨 Requisição recebida e respondida\n");
    
    return ERR_OK;
}

static err_t http_accept(void* arg, struct tcp_pcb* newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  🐟 HydroSense v3.0 - TESTE MINIMO    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // Inicializa Wi-Fi
    printf("🌐 Inicializando Wi-Fi...\n");
    
    if (cyw43_arch_init()) {
        printf("❌ Falha ao inicializar cyw43!\n");
        while (1) {
            sleep_ms(500);
        }
    }
    
    printf("✅ CYW43 inicializado\n");
    
    // Liga LED para indicar que está funcionando
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    printf("💡 LED ligado\n");
    
    // Habilita modo AP
    printf("📶 Criando Access Point...\n");
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
    
    printf("✅ Access Point criado!\n");
    printf("   📶 SSID: %s\n", WIFI_SSID);
    printf("   🔑 Senha: %s\n", WIFI_PASS);
    printf("   🌐 IP: 192.168.4.1\n\n");
    
    // Cria servidor HTTP
    printf("🔧 Iniciando servidor HTTP...\n");
    
    server_pcb = tcp_new();
    if (server_pcb == NULL) {
        printf("❌ Falha ao criar PCB\n");
        while (1) sleep_ms(500);
    }
    
    err_t err = tcp_bind(server_pcb, IP_ADDR_ANY, 80);
    if (err != ERR_OK) {
        printf("❌ Falha no bind porta 80\n");
        while (1) sleep_ms(500);
    }
    
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, http_accept);
    
    printf("✅ Servidor HTTP iniciado na porta 80\n\n");
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║  ✅ PRONTO!                            ║\n");
    printf("║  Conecte ao Wi-Fi: HydroSense          ║\n");
    printf("║  Acesse: http://192.168.4.1            ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // Loop principal
    uint32_t counter = 0;
    while (1) {
        cyw43_arch_poll();
        sleep_ms(10);
        
        // Pisca LED a cada 2 segundos
        counter++;
        if (counter >= 200) {
            counter = 0;
            static bool led_on = true;
            led_on = !led_on;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
        }
    }
    
    return 0;
}
