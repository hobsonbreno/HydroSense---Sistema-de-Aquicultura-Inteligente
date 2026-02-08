/**
 * Teste OLED BitDogLab - GPIO14/15
 */
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define LED_PIN 25
#define OLED_SDA 14
#define OLED_SCL 15
#define OLED_ADDR 0x3C

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    printf("\n\nTESTE OLED BITDOGLAB\n");
    printf("====================\n\n");
    
    // Init I2C direto em GPIO14/15
    printf("Init I2C em GPIO%d/%d...\n", OLED_SDA, OLED_SCL);
    i2c_init(i2c1, 100000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(100);
    printf("I2C inicializado!\n");
    
    // Scan com timeout
    printf("Scan I2C (com timeout):\n");
    int found = 0;
    for (int addr = 0x08; addr < 0x78; addr++) {
        uint8_t data;
        // Usa timeout de 1000us para não travar
        int ret = i2c_read_timeout_us(i2c1, addr, &data, 1, false, 1000);
        if (ret >= 0) {
            printf("  0x%02X encontrado\n", addr);
            found++;
        }
    }
    printf("Total: %d dispositivos\n\n", found);
    
    printf("FIM DO SCAN\n");
    
    while (1) {
        gpio_put(LED_PIN, 1); sleep_ms(500);
        gpio_put(LED_PIN, 0); sleep_ms(500);
        printf(".");
    }
}
