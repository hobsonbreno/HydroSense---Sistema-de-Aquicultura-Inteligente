/**
 * Teste ultra básico - sem WiFi
 */

#include "pico/stdlib.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Aguarda USB
    
    printf("\n\n=== TESTE BASICO SEM WIFI ===\n");
    printf("Pico iniciado!\n\n");
    
    // Configura LED GPIO25
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    int count = 0;
    while (1) {
        count++;
        
        printf("[%d] Pico W funcionando!\n", count);
        
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
    
    return 0;
}