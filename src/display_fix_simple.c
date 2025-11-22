#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SSD1306_I2C_ADDR 0x3C

// Função para enviar comando
bool send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_write_blocking(i2c1, SSD1306_I2C_ADDR, buf, 2, false) == 2;
}

// Função para enviar dados
bool send_data_simple(uint8_t *data, size_t len) {
    uint8_t *buf = malloc(len + 1);
    if (!buf) return false;
    buf[0] = 0x40;
    memcpy(buf + 1, data, len);
    bool result = i2c_write_blocking(i2c1, SSD1306_I2C_ADDR, buf, len + 1, false) == (len + 1);
    free(buf);
    return result;
}

// Inicialização super simples
bool init_display_simple() {
    printf("🔧 Inicializando display (modo simples)...\n");
    
    // I2C1 para GP14/GP15
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    sleep_ms(200);
    
    // Teste comunicação
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, SSD1306_I2C_ADDR, &dummy, 1, false) < 0) {
        printf("❌ Display não encontrado!\n");
        return false;
    }
    printf("✅ Display detectado!\n");
    
    // Comandos essenciais
    send_cmd(0xAE); // Display OFF
    send_cmd(0x8D); send_cmd(0x14); // Charge pump ON
    send_cmd(0x20); send_cmd(0x00); // Horizontal addressing
    send_cmd(0x81); send_cmd(0xFF); // Max contrast
    send_cmd(0xAF); // Display ON
    
    printf("✅ Display inicializado!\n");
    return true;
}

// Teste de orientação com padrão visual claro
void test_orientation_simple(int orient) {
    printf("🔄 Testando orientação %d...\n", orient);
    
    // Aplica orientação
    switch(orient) {
        case 1: send_cmd(0xA0); send_cmd(0xC0); break; // Normal
        case 2: send_cmd(0xA1); send_cmd(0xC0); break; // Espelho H
        case 3: send_cmd(0xA0); send_cmd(0xC8); break; // Espelho V  
        case 4: send_cmd(0xA1); send_cmd(0xC8); break; // 180°
    }
    
    // Cria padrão super claro
    uint8_t pattern[1024];
    memset(pattern, 0, 1024);
    
    // Escreve "TESTE X" em padrão de pixels grande e visível
    // Letra T (teste visual)
    for (int x = 10; x < 30; x++) {
        pattern[x] |= 0x01; // Linha horizontal superior
    }
    for (int y = 0; y < 6; y++) {
        pattern[20 + y * 128] |= 0x3F; // Linha vertical
    }
    
    // Número da orientação
    int num_x = 40;
    if (orient == 1) { // "1"
        for (int y = 0; y < 6; y++) {
            pattern[num_x + y * 128] |= 0x3F;
        }
    } else if (orient == 2) { // "2"
        pattern[num_x] |= 0x3F;
        pattern[num_x + 1] |= 0x3F;
        pattern[num_x + 2 * 128] |= 0x3F;
        pattern[num_x + 2 * 128 + 1] |= 0x3F;
        pattern[num_x + 4 * 128] |= 0x3F;
        pattern[num_x + 4 * 128 + 1] |= 0x3F;
    } else if (orient == 3) { // "3"
        pattern[num_x] |= 0x3F;
        pattern[num_x + 1] |= 0x3F;
        pattern[num_x + 2 * 128] |= 0x1F;
        pattern[num_x + 2 * 128 + 1] |= 0x1F;
        pattern[num_x + 4 * 128] |= 0x3F;
        pattern[num_x + 4 * 128 + 1] |= 0x3F;
    } else if (orient == 4) { // "4"
        for (int y = 0; y < 3; y++) {
            pattern[num_x + y * 128] |= 0x20;
        }
        pattern[num_x + 2 * 128] |= 0x3F;
        pattern[num_x + 2 * 128 + 1] |= 0x3F;
        for (int y = 2; y < 6; y++) {
            pattern[num_x + 1 + y * 128] |= 0x20;
        }
    }
    
    // Seta indicando "CIMA"
    int arrow_x = 80;
    pattern[arrow_x + 1 * 128] |= 0x08;     // Ponta da seta
    pattern[arrow_x + 1 * 128 - 1] |= 0x04;
    pattern[arrow_x + 1 * 128 + 1] |= 0x04;
    pattern[arrow_x + 2 * 128] |= 0x18;     // Corpo da seta
    pattern[arrow_x + 3 * 128] |= 0x18;
    
    // Envia padrão
    send_data_simple(pattern, 1024);
    
    printf("📺 Orientação %d aplicada - observe o display por 5s\n", orient);
    sleep_ms(5000);
}

// Programa principal super simples
int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("🚀 === CORRETOR DE ORIENTAÇÃO DO DISPLAY ===\n");
    
    if (!init_display_simple()) {
        printf("❌ Falha na inicialização!\n");
        printf("🔧 Verifique conexões:\n");
        printf("   VCC -> 3.3V\n");
        printf("   GND -> GND\n");
        printf("   SDA -> GP14\n");
        printf("   SCL -> GP15\n");
        while(1) sleep_ms(1000);
    }
    
    // Testa todas as orientações automaticamente
    for (int i = 1; i <= 4; i++) {
        test_orientation_simple(i);
    }
    
    printf("\n🎯 Qual orientação mostrou o texto na posição CORRETA?\n");
    printf("Aplicando orientação padrão (4)...\n");
    
    // Aplica orientação mais comum
    send_cmd(0xA1); // Segment remap
    send_cmd(0xC8); // COM scan direction
    
    // Tela final com HydroSense
    uint8_t final_pattern[1024];
    memset(final_pattern, 0, 1024);
    
    // Desenha "HYDROSENSE" em pixels grandes
    const char* text = "HYDROSENSE OK";
    for (int i = 0; i < 13 && i < strlen(text); i++) {
        int x = i * 9 + 5;
        for (int y = 0; y < 3; y++) {
            final_pattern[x + y * 128] |= 0xFF;
        }
    }
    
    // Linha inferior com status
    for (int x = 0; x < 128; x++) {
        final_pattern[x + 7 * 128] |= (x % 4 < 2) ? 0xFF : 0x00;
    }
    
    send_data_simple(final_pattern, 1024);
    
    printf("✅ Display configurado! Se o texto estiver legível, está correto.\n");
    
    while(1) {
        printf("💡 Programa em loop - display deve mostrar 'HYDROSENSE OK'\n");
        sleep_ms(10000);
    }
    
    return 0;
}