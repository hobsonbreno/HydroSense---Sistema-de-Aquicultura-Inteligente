#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SSD1306_I2C_ADDR 0x3C
#define SDA_PIN 14
#define SCL_PIN 15

// Função para enviar comando
bool send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_write_blocking(i2c1, SSD1306_I2C_ADDR, buf, 2, false) == 2;
}

// Função para enviar buffer de dados
bool send_buffer(uint8_t *data, size_t len) {
    uint8_t *buf = malloc(len + 1);
    if (!buf) return false;
    buf[0] = 0x40;
    memcpy(buf + 1, data, len);
    bool result = i2c_write_blocking(i2c1, SSD1306_I2C_ADDR, buf, len + 1, false) == (len + 1);
    free(buf);
    return result;
}

// Inicialização com orientação CORRIGIDA
bool init_display_corrigido() {
    printf("🔧 Inicializando display com orientação CORRIGIDA...\n");
    
    // Configura I2C1 para GP14/GP15
    i2c_init(i2c1, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    sleep_ms(200);
    
    // Testa comunicação
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, SSD1306_I2C_ADDR, &dummy, 1, false) < 0) {
        printf("❌ Display não encontrado!\n");
        return false;
    }
    printf("✅ Display detectado!\n");
    
    // Sequência de inicialização com ORIENTAÇÃO CORRETA
    send_cmd(0xAE); // Display OFF
    send_cmd(0x20); send_cmd(0x00); // Horizontal addressing
    send_cmd(0x40); // Start line 0
    
    // ** ORIENTAÇÃO CORRIGIDA **
    send_cmd(0xA1); // Segment remap ON (espelha horizontalmente)
    send_cmd(0xC8); // COM scan direction invertido (espelha verticalmente)
    
    send_cmd(0xA8); send_cmd(0x3F); // MUX ratio 64
    send_cmd(0xD3); send_cmd(0x00); // Display offset 0
    send_cmd(0xDA); send_cmd(0x12); // COM pins configuration
    send_cmd(0x8D); send_cmd(0x14); // Charge pump ON
    send_cmd(0x81); send_cmd(0xFF); // Max contrast
    send_cmd(0xD9); send_cmd(0xF1); // Pre-charge
    send_cmd(0xDB); send_cmd(0x40); // VCOM deselect
    send_cmd(0xA4); // Resume to RAM content
    send_cmd(0xA6); // Normal display
    send_cmd(0xAF); // Display ON
    
    printf("✅ Display inicializado com orientação CORRETA!\n");
    return true;
}

// Font 5x7 simples para teste
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
};

// Função para desenhar caractere
void draw_char(uint8_t *buffer, int x, int y, char c) {
    if (x < 0 || x > 123 || y < 0 || y > 56) return;
    
    int char_index;
    if (c >= '0' && c <= '9') {
        char_index = c - '0' + 1;  // Offset por causa do espaço
    } else if (c >= 'A' && c <= 'Z') {
        char_index = c - 'A' + 11;
    } else if (c >= 'a' && c <= 'z') {
        char_index = c - 'a' + 11;  // Mesmo que maiúscula
    } else {
        char_index = 0;  // Espaço
    }
    
    const uint8_t *char_data = font5x7[char_index];
    
    for (int col = 0; col < 5; col++) {
        uint8_t column = char_data[col];
        for (int row = 0; row < 7; row++) {
            if (column & (1 << row)) {
                int pixel_x = x + col;
                int pixel_y = y + row;
                if (pixel_x < 128 && pixel_y < 64) {
                    int page = pixel_y / 8;
                    int bit = pixel_y % 8;
                    int index = pixel_x + page * 128;
                    buffer[index] |= (1 << bit);
                }
            }
        }
    }
}

// Função para desenhar string
void draw_string(uint8_t *buffer, int x, int y, const char *str) {
    int pos_x = x;
    while (*str && pos_x < 128) {
        draw_char(buffer, pos_x, y, *str);
        pos_x += 6;  // 5 pixels + 1 espaço
        str++;
    }
}

// Teste completo de orientação
void teste_orientacao_completo() {
    printf("🎯 === TESTE COMPLETO DE ORIENTAÇÃO ===\n");
    
    uint8_t buffer[1024];
    
    // Teste 1: Texto de referência com orientação corrigida
    printf("📝 Teste 1: Texto de referência...\n");
    memset(buffer, 0, 1024);
    
    draw_string(buffer, 5, 5, "HYDROSENSE 2024");
    draw_string(buffer, 10, 15, "ORIENTACAO OK");
    draw_string(buffer, 15, 25, "TEXTO NORMAL");
    draw_string(buffer, 20, 35, "AQUICULTURA");
    draw_string(buffer, 25, 45, "SISTEMA IOT");
    draw_string(buffer, 30, 55, "123456789");
    
    send_buffer(buffer, 1024);
    printf("📺 Texto deve aparecer HORIZONTAL e LEGÍVEL!\n");
    sleep_ms(5000);
    
    // Teste 2: Padrão visual de orientação
    printf("🎨 Teste 2: Padrão visual...\n");
    memset(buffer, 0, 1024);
    
    // Desenha seta apontando para cima (indicando orientação correta)
    for (int x = 60; x < 68; x++) {
        buffer[x + 2*128] |= 0x18;  // Corpo da seta
        buffer[x + 3*128] |= 0x18;
        buffer[x + 4*128] |= 0x18;
    }
    buffer[64 + 1*128] |= 0x08;  // Ponta da seta
    buffer[63 + 1*128] |= 0x04;
    buffer[65 + 1*128] |= 0x04;
    buffer[62 + 1*128] |= 0x02;
    buffer[66 + 1*128] |= 0x02;
    
    draw_string(buffer, 45, 50, "CIMA");
    
    send_buffer(buffer, 1024);
    printf("📺 Seta deve apontar para CIMA!\n");
    sleep_ms(3000);
    
    // Teste 3: Bordas para verificar orientação
    printf("🔲 Teste 3: Bordas de referência...\n");
    memset(buffer, 0, 1024);
    
    // Borda superior
    for (int x = 0; x < 128; x++) {
        buffer[x] |= 0x01;
    }
    
    // Borda inferior  
    for (int x = 0; x < 128; x++) {
        buffer[x + 7*128] |= 0x80;
    }
    
    // Borda esquerda
    for (int page = 0; page < 8; page++) {
        buffer[0 + page*128] |= 0xFF;
    }
    
    // Borda direita
    for (int page = 0; page < 8; page++) {
        buffer[127 + page*128] |= 0xFF;
    }
    
    draw_string(buffer, 35, 25, "BORDAS");
    draw_string(buffer, 30, 35, "CORRETAS");
    
    send_buffer(buffer, 1024);
    printf("📺 Bordas devem formar retângulo correto!\n");
    sleep_ms(3000);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("🚀 === TESTE FINAL DE ORIENTAÇÃO DO DISPLAY ===\n");
    printf("🎯 Objetivo: Validar se a correção de orientação funcionou\n\n");
    
    if (!init_display_corrigido()) {
        printf("❌ Falha na inicialização do display!\n");
        printf("🔧 Verifique:\n");
        printf("   - VCC conectado em 3.3V\n");
        printf("   - GND conectado\n");
        printf("   - SDA conectado em GP14\n");
        printf("   - SCL conectado em GP15\n");
        while(1) sleep_ms(1000);
    }
    
    // Executa teste completo
    teste_orientacao_completo();
    
    // Loop final com status
    printf("✅ TESTE DE ORIENTAÇÃO CONCLUÍDO!\n");
    printf("📊 RESULTADO ESPERADO:\n");
    printf("   ✓ Texto deve aparecer HORIZONTAL (não deitado)\n");
    printf("   ✓ Seta deve apontar para CIMA\n");
    printf("   ✓ Bordas devem formar retângulo normal\n");
    printf("   ✓ Tudo deve estar LEGÍVEL e CORRETO\n\n");
    
    printf("💡 Se ainda estiver com problema de orientação:\n");
    printf("   1. Verifique se o display físico está na posição correta\n");
    printf("   2. Pode ser necessário ajustar A0/A1 e C0/C8 manualmente\n");
    printf("   3. Alguns displays têm orientação diferente de fábrica\n\n");
    
    // Tela final contínua
    uint8_t final_buffer[1024];
    while(1) {
        memset(final_buffer, 0, 1024);
        
        draw_string(final_buffer, 20, 10, "HYDROSENSE");
        draw_string(final_buffer, 15, 25, "DISPLAY OK");
        draw_string(final_buffer, 25, 40, "SISTEMA");
        draw_string(final_buffer, 30, 50, "ATIVO");
        
        send_buffer(final_buffer, 1024);
        
        printf("📺 Display mostrando: HYDROSENSE DISPLAY OK SISTEMA ATIVO\n");
        sleep_ms(5000);
    }
    
    return 0;
}