/**
 * Diagnóstico Mínimo - Testando I2C switching para OLED
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define LED_PIN 25

// Sensores
#define SENSOR_SDA 2
#define SENSOR_SCL 3

// OLED
#define OLED_SDA 14
#define OLED_SCL 15
#define OLED_ADDR 0x3C

void oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
}

int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
    
    printf("\n\nDIAGNOSTICO I2C SWITCHING\n");
    printf("==========================\n\n");
    
    // Inicializa I2C em sensores primeiro
    printf("1. Init I2C em GPIO%d/%d (sensores)...\n", SENSOR_SDA, SENSOR_SCL);
    i2c_init(i2c1, 100000);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_ms(100);
    
    // Scan sensores
    printf("2. Scan I2C sensores:\n");
    for (int addr = 0x08; addr < 0x78; addr++) {
        uint8_t data;
        if (i2c_read_blocking(i2c1, addr, &data, 1, false) >= 0) {
            printf("   0x%02X OK\n", addr);
        }
    }
    
    // Agora switch para OLED
    printf("\n3. Switch para GPIO%d/%d (OLED)...\n", OLED_SDA, OLED_SCL);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    sleep_ms(10);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(100);
    
    // Scan OLED
    printf("4. Scan I2C OLED:\n");
    for (int addr = 0x08; addr < 0x78; addr++) {
        uint8_t data;
        if (i2c_read_blocking(i2c1, addr, &data, 1, false) >= 0) {
            printf("   0x%02X OK\n", addr);
        }
    }
    
    // Tenta inicializar OLED
    printf("\n5. Inicializando OLED...\n");
    oled_cmd(0xAE);  // Display off
    oled_cmd(0xD5); oled_cmd(0x80);  // Clock
    oled_cmd(0xA8); oled_cmd(0x3F);  // Multiplex
    oled_cmd(0xD3); oled_cmd(0x00);  // Offset
    oled_cmd(0x40);  // Start line
    oled_cmd(0x8D); oled_cmd(0x14);  // Charge pump
    oled_cmd(0x20); oled_cmd(0x00);  // Memory mode
    oled_cmd(0xA1);  // Segment remap
    oled_cmd(0xC8);  // COM scan
    oled_cmd(0xDA); oled_cmd(0x12);  // COM pins
    oled_cmd(0x81); oled_cmd(0xCF);  // Contrast
    oled_cmd(0xD9); oled_cmd(0xF1);  // Pre-charge
    oled_cmd(0xDB); oled_cmd(0x40);  // VCOMH
    oled_cmd(0xA4);  // Resume RAM
    oled_cmd(0xA6);  // Normal display
    oled_cmd(0xAF);  // Display on
    printf("   OLED comandos enviados!\n");
    
    // Limpa tela
    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);
    
    // Envia pixels brancos
    uint8_t buf[17];
    buf[0] = 0x40;
    for (int i = 1; i < 17; i++) buf[i] = 0xFF;  // Tudo branco
    
    for (int i = 0; i < 64; i++) {  // 64 blocos de 16 bytes = 1024 bytes
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 17, false);
    }
    printf("   Tela preenchida!\n");
    
    printf("\n6. FIM DO TESTE\n");
    
    // Loop
    while (1) {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
    
    return 0;
}
