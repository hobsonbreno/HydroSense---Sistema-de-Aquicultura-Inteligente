/**
 * HydroSense - TESTE MINIMO USB CDC
 * Apenas stdio + printf - sem WiFi, sem I2C, sem nada
 */
#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    printf("\n\n============================\n");
    printf("  TESTE USB CDC - FUNCIONA!\n");
    printf("============================\n\n");
    
    int count = 0;
    while (true) {
        printf("Alive #%d\n", ++count);
        sleep_ms(1000);
    }
    return 0;
}
