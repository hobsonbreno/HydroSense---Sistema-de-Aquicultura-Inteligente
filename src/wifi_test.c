/**
 * Teste simples de WiFi para Pico W
 * Diagnóstico de conexão
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"

int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    printf("\n\n=== TESTE WiFi Pico W ===\n\n");
    printf("SSID: [%s]\n", WIFI_SSID);
    printf("Senha: [%s]\n", WIFI_PASSWORD);
    
    // Inicializa WiFi
    printf("\n1. Inicializando chip WiFi...\n");
    if (cyw43_arch_init()) {
        printf("   ERRO: Falha ao inicializar WiFi!\n");
        while(1) { sleep_ms(1000); }
    }
    printf("   OK!\n");
    
    // Modo estação
    printf("2. Habilitando modo estação...\n");
    cyw43_arch_enable_sta_mode();
    printf("   OK!\n");
    
    // Pisca LED para indicar que está funcionando
    for (int i = 0; i < 5; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(200);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(200);
    }
    
    // Tenta conectar
    printf("3. Conectando...\n");
    
    int result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );
    
    if (result == 0) {
        printf("\n*** CONECTADO! ***\n");
        
        // Pega IP
        struct netif *netif = netif_default;
        if (netif) {
            printf("IP: %s\n", ip4addr_ntoa(&netif->ip_addr));
        }
        
        // LED aceso fixo
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    } else {
        printf("\n*** FALHA (erro: %d) ***\n", result);
        printf("  -1 = Rede não encontrada\n");
        printf("  -2 = Autenticação falhou\n");
        printf("  -3 = Timeout\n");
    }
    
    // Loop infinito
    while(1) {
        printf(".");
        sleep_ms(5000);
    }
    
    return 0;
}
