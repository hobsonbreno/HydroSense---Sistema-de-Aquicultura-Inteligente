/**
 * Teste básico - LED + Serial
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    
    // Inicializa o chip (necessário para o LED do Pico W)
    if (cyw43_arch_init()) {
        // Se falhar, usa LED padrão
        const uint LED_PIN = 25;
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        
        while (1) {
            printf("LED ON (fallback)\n");
            gpio_put(LED_PIN, 1);
            sleep_ms(500);
            printf("LED OFF\n");
            gpio_put(LED_PIN, 0);
            sleep_ms(500);
        }
    }
    
    printf("\n\n=== TESTE BASICO PICO W ===\n");
    printf("Chip WiFi OK!\n\n");
    
    int count = 0;
    while (1) {
        count++;
        printf("[%d] LED ON\n", count);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        
        printf("[%d] LED OFF\n", count);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);
    }
    
    return 0;
}
