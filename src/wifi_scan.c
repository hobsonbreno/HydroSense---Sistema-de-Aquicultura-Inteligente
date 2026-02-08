/**
 * Scan de redes WiFi
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"

static int scan_result_count = 0;

static int scan_callback(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        scan_result_count++;
        printf("  [%d] SSID: %-20s  CH: %d  RSSI: %d dBm  Auth: %d\n",
               scan_result_count,
               result->ssid,
               result->channel,
               result->rssi,
               result->auth_mode);
    }
    return 0;
}

int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    printf("\n\n");
    printf("================================\n");
    printf("   SCAN DE REDES WiFi\n");
    printf("================================\n\n");
    
    printf("Procurando por: [%s]\n\n", WIFI_SSID);
    
    // Inicializa WiFi
    printf("1. Inicializando chip WiFi...\n");
    if (cyw43_arch_init()) {
        printf("   ERRO!\n");
        while(1) sleep_ms(1000);
    }
    printf("   OK!\n");
    
    cyw43_arch_enable_sta_mode();
    sleep_ms(1000);
    
    // LED aceso indica que está funcionando
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    
    // Scan
    printf("\n2. Escaneando redes...\n\n");
    
    cyw43_wifi_scan_options_t scan_options = {0};
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_callback);
    
    if (err != 0) {
        printf("   Erro no scan: %d\n", err);
    }
    
    // Aguarda scan terminar
    while (cyw43_wifi_scan_active(&cyw43_state)) {
        sleep_ms(100);
    }
    
    printf("\n   Total: %d redes encontradas\n", scan_result_count);
    
    // Tenta conectar
    printf("\n3. Tentando conectar a [%s]...\n", WIFI_SSID);
    
    int result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );
    
    if (result == 0) {
        printf("\n*** CONECTADO! ***\n");
        struct netif *netif = netif_default;
        if (netif) {
            printf("IP: %s\n", ip4addr_ntoa(&netif->ip_addr));
        }
        
        // LED pisca rápido = sucesso
        while(1) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(100);
        }
    } else {
        printf("\n*** FALHA (erro: %d) ***\n", result);
        printf("  -1 = Rede nao encontrada\n");
        printf("  -2 = Autenticacao falhou (senha/seguranca)\n");
        printf("  -3 = Timeout\n");
        
        // LED pisca lento = erro
        while(1) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(1000);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(1000);
        }
    }
    
    return 0;
}
