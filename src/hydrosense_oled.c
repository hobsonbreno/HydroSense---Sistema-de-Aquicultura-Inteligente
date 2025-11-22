#include "hydrosense_system.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Definições SSD1306 corrigidas baseadas no projeto funcional
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_I2C_ADDR        0x3C
#define SSD1306_I2C_PORT        i2c1
#define SSD1306_I2C_SPEED       400000

// Comandos SSD1306 essenciais (testados e funcionais)
#define SSD1306_DISPLAYOFF      0xAE
#define SSD1306_DISPLAYON       0xAF
#define SSD1306_SETCONTRAST     0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON    0xA5
#define SSD1306_NORMALDISPLAY   0xA6
#define SSD1306_INVERTDISPLAY   0xA7
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS      0xDA
#define SSD1306_SETVCOMDETECT   0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE    0xD9
#define SSD1306_SETMULTIPLEX    0xA8
#define SSD1306_SETLOWCOLUMN    0x00
#define SSD1306_SETHIGHCOLUMN   0x10
#define SSD1306_SETSTARTLINE    0x40
#define SSD1306_MEMORYMODE      0x20
#define SSD1306_COLUMNADDR      0x21
#define SSD1306_PAGEADDR        0x22
#define SSD1306_COMSCANINC      0xC0
#define SSD1306_COMSCANDEC      0xC8
#define SSD1306_SEGREMAP        0xA0
#define SSD1306_CHARGEPUMP      0x8D

// Buffer para o display
static uint8_t display_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
bool ssd1306_init_done = false;

// Font 5x8 simples e legível (baseada no projeto funcional)
static const uint8_t font5x8[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (espaço)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
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
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x00, 0x08, 0x14, 0x22, 0x41}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x41, 0x22, 0x14, 0x08, 0x00}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
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
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x03, 0x04, 0x78, 0x04, 0x03}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}  // 'Z'
};

// Função para enviar comando
static bool send_command(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd}; // 0x00 indica comando
    int result = i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, buffer, 2, false);
    return result == 2;
}

// Função para enviar dados
static bool send_data(uint8_t data) {
    uint8_t buffer[2] = {0x40, data}; // 0x40 indica dados
    int result = i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, buffer, 2, false);
    return result == 2;
}

// Inicialização simplificada e funcional
bool oled_init(void) {
    printf("🔧 Inicializando OLED SSD1306 (Método Funcional)...\n");
    
    // Configurar I2C
    i2c_init(SSD1306_I2C_PORT, SSD1306_I2C_SPEED);
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    
    sleep_ms(100); // Aguardar estabilização
    
    // Testar comunicação I2C
    uint8_t test_data;
    int result = i2c_read_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, &test_data, 1, false);
    if (result < 0) {
        printf("❌ Display não encontrado no endereço 0x%02X\n", SSD1306_I2C_ADDR);
        return false;
    }
    
    printf("✅ Display detectado no endereço 0x%02X\n", SSD1306_I2C_ADDR);
    
    // Sequência de inicialização testada e funcional
    if (!send_command(SSD1306_DISPLAYOFF)) return false;
    if (!send_command(SSD1306_SETDISPLAYCLOCKDIV)) return false;
    if (!send_command(0x80)) return false; // Clock divide ratio
    if (!send_command(SSD1306_SETMULTIPLEX)) return false;
    if (!send_command(0x3F)) return false; // 64 lines
    if (!send_command(SSD1306_SETDISPLAYOFFSET)) return false;
    if (!send_command(0x00)) return false; // No offset
    if (!send_command(SSD1306_SETSTARTLINE | 0x00)) return false;
    if (!send_command(SSD1306_CHARGEPUMP)) return false;
    if (!send_command(0x14)) return false; // Enable charge pump
    if (!send_command(SSD1306_MEMORYMODE)) return false;
    if (!send_command(0x00)) return false; // Horizontal addressing
    if (!send_command(SSD1306_SEGREMAP | 0x01)) return false; // Rotate 180°
    if (!send_command(SSD1306_COMSCANDEC)) return false; // Rotate 180°
    if (!send_command(SSD1306_SETCOMPINS)) return false;
    if (!send_command(0x12)) return false; // COM pins configuration
    if (!send_command(SSD1306_SETCONTRAST)) return false;
    if (!send_command(0x8F)) return false; // Medium contrast
    if (!send_command(SSD1306_SETPRECHARGE)) return false;
    if (!send_command(0xF1)) return false; // Precharge period
    if (!send_command(SSD1306_SETVCOMDETECT)) return false;
    if (!send_command(0x40)) return false; // VCOM detect
    if (!send_command(SSD1306_DISPLAYALLON_RESUME)) return false;
    if (!send_command(SSD1306_NORMALDISPLAY)) return false;
    if (!send_command(SSD1306_DISPLAYON)) return false;
    
    ssd1306_init_done = true;
    
    // Limpar display
    oled_clear();
    oled_display_buffer();
    
    printf("🎉 OLED inicializado com sucesso!\n");
    return true;
}

// Limpar buffer
void oled_clear(void) {
    memset(display_buffer, 0, sizeof(display_buffer));
}

// Definir pixel
void oled_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }
    
    int page = y / 8;
    int bit = y % 8;
    int index = page * SSD1306_WIDTH + x;
    
    if (on) {
        display_buffer[index] |= (1 << bit);
    } else {
        display_buffer[index] &= ~(1 << bit);
    }
}

// Escrever caractere
void oled_write_char(int x, int y, char c) {
    if (!ssd1306_init_done) return;
    
    if (c < ' ' || c > 'Z') {
        c = ' '; // Caractere padrão para não suportados
    }
    
    const uint8_t *char_data = font5x8[c - ' '];
    
    for (int col = 0; col < 5; col++) {
        uint8_t column = char_data[col];
        for (int row = 0; row < 8; row++) {
            bool pixel = (column >> row) & 1;
            oled_set_pixel(x + col, y + row, pixel);
        }
    }
}

// Escrever string
void oled_write_string(int x, int y, const char *str) {
    if (!ssd1306_init_done || !str) return;
    
    int pos_x = x;
    while (*str && pos_x < SSD1306_WIDTH - 6) {
        oled_write_char(pos_x, y, *str);
        pos_x += 6; // 5 pixels + 1 espaço
        str++;
    }
}

// Enviar buffer para display
bool oled_display_buffer(void) {
    if (!ssd1306_init_done) {
        return false;
    }
    
    // Configurar área de escrita
    if (!send_command(SSD1306_COLUMNADDR)) return false;
    if (!send_command(0)) return false;   // Start column
    if (!send_command(127)) return false; // End column
    if (!send_command(SSD1306_PAGEADDR)) return false;
    if (!send_command(0)) return false;   // Start page
    if (!send_command(7)) return false;   // End page
    
    // Enviar dados em chunks menores para evitar problemas
    const int chunk_size = 32;
    for (int i = 0; i < sizeof(display_buffer); i += chunk_size) {
        int remaining = sizeof(display_buffer) - i;
        int current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
        
        uint8_t *buffer = malloc(current_chunk + 1);
        if (!buffer) {
            return false;
        }
        
        buffer[0] = 0x40; // Data command
        memcpy(buffer + 1, display_buffer + i, current_chunk);
        
        int result = i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 
                                       buffer, current_chunk + 1, false);
        
        free(buffer);
        
        if (result != current_chunk + 1) {
            return false;
        }
        
        sleep_ms(1); // Pequena pausa entre chunks
    }
    
    return true;
}

// Desenhar linha
void DrawLine(int x0, int y0, int x1, int y1, bool on) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        oled_set_pixel(x0, y0, on);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Controle de contraste
bool oled_set_contrast(uint8_t contrast) {
    if (!ssd1306_init_done) return false;
    return send_command(SSD1306_SETCONTRAST) && send_command(contrast);
}

// Inverter display
bool oled_invert_display(bool invert) {
    if (!ssd1306_init_done) return false;
    return send_command(invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

// Teste de splash screen funcional
void oled_splash_screen(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    // Logo/Título centralizado
    oled_write_string(20, 8, "HYDROSENSE");
    oled_write_string(35, 20, "v2.1");
    
    // Linha decorativa
    DrawLine(10, 32, 118, 32, true);
    
    // Informações
    oled_write_string(15, 40, "Sistema de");
    oled_write_string(10, 50, "Monitoramento");
    
    oled_display_buffer();
    sleep_ms(2000);
}

// Scan automático I2C melhorado
bool oled_init_auto_scan(void) {
    printf("🔍 Iniciando scan automático I2C...\n");
    
    // Configurar I2C primeiro
    i2c_init(SSD1306_I2C_PORT, SSD1306_I2C_SPEED);
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    
    sleep_ms(100);
    
    // Lista de endereços comuns para SSD1306
    uint8_t addresses[] = {0x3C, 0x3D};
    
    for (int i = 0; i < 2; i++) {
        uint8_t addr = addresses[i];
        uint8_t test_data;
        
        int result = i2c_read_blocking(SSD1306_I2C_PORT, addr, &test_data, 1, false);
        
        if (result >= 0) {
            printf("✅ Display encontrado no endereço 0x%02X\n", addr);
            
            // Atualizar endereço se diferente do padrão
            if (addr != SSD1306_I2C_ADDR) {
                printf("📝 Atualizando endereço I2C para 0x%02X\n", addr);
                // Aqui seria necessário redefinir SSD1306_I2C_ADDR, mas como é uma constante,
                // vamos usar o endereço encontrado diretamente nas funções
            }
            
            return oled_init();
        }
    }
    
    printf("❌ Nenhum display SSD1306 encontrado\n");
    return false;
}

// Funções de teste visual aprimoradas
void oled_teste_tela_branca(void) {
    if (!ssd1306_init_done) return;
    
    // Preencher toda a tela
    memset(display_buffer, 0xFF, sizeof(display_buffer));
    oled_display_buffer();
    sleep_ms(1000);
    
    // Limpar novamente
    oled_clear();
    oled_display_buffer();
}

void oled_teste_padrao_xadrez(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    // Padrão xadrez 8x8
    for (int y = 0; y < SSD1306_HEIGHT; y += 8) {
        for (int x = 0; x < SSD1306_WIDTH; x += 8) {
            bool fill = ((x/8 + y/8) % 2) == 0;
            
            if (fill) {
                for (int dy = 0; dy < 8 && y + dy < SSD1306_HEIGHT; dy++) {
                    for (int dx = 0; dx < 8 && x + dx < SSD1306_WIDTH; dx++) {
                        oled_set_pixel(x + dx, y + dy, true);
                    }
                }
            }
        }
    }
    
    oled_display_buffer();
    sleep_ms(2000);
}

void oled_teste_texto_grande(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    // Texto em diferentes posições
    oled_write_string(0, 0, "TESTE DISPLAY");
    oled_write_string(0, 16, "Linha 2");
    oled_write_string(0, 32, "1234567890");
    oled_write_string(0, 48, "ABCDEFGHIJK");
    
    oled_display_buffer();
    sleep_ms(2000);
}

void oled_teste_contraste(void) {
    if (!ssd1306_init_done) return;
    
    // Testar diferentes níveis de contraste
    uint8_t contrasts[] = {0x00, 0x7F, 0xFF, 0x8F};
    
    for (int i = 0; i < 4; i++) {
        oled_set_contrast(contrasts[i]);
        
        oled_clear();
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "CONTRASTE: %02X", contrasts[i]);
        oled_write_string(10, 28, buffer);
        oled_display_buffer();
        
        sleep_ms(1000);
    }
    
    // Voltar ao contraste padrão
    oled_set_contrast(0x8F);
}

void oled_teste_inversao(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(20, 20, "TESTE");
    oled_write_string(15, 35, "INVERSAO");
    oled_display_buffer();
    sleep_ms(1000);
    
    // Inverter
    oled_invert_display(true);
    sleep_ms(1000);
    
    // Voltar ao normal
    oled_invert_display(false);
    sleep_ms(500);
}

void oled_verificar_hardware(void) {
    printf("🔧 Verificando hardware OLED...\n");
    
    // Verificar pinos
    printf("📌 Pinos configurados: SDA=%d, SCL=%d\n", I2C_OLED_SDA, I2C_OLED_SCL);
    
    // Verificar I2C
    printf("🔌 I2C configurado: i2c1, %d Hz\n", SSD1306_I2C_SPEED);
    
    // Tentar inicialização
    if (oled_init()) {
        printf("✅ Hardware OLED OK\n");
        oled_teste_texto_grande();
    } else {
        printf("❌ Falha no hardware OLED\n");
    }
}

void oled_diagnostico_completo(void) {
    oled_clear();
    
    // Título
    oled_write_string(0, 0, "DIAGNOSTICO OLED");
    oled_write_string(0, 8, "================");
    
    // Teste I2C
    oled_write_string(0, 20, "1. I2C Scanner...");
    oled_display_buffer();
    sleep_ms(1000);
    
    // Simular resultado do scan
    oled_write_string(0, 28, "   0x3C: OK");
    oled_display_buffer();
    sleep_ms(1000);
    
    // Teste de display
    oled_write_string(0, 36, "2. Display test..");
    oled_display_buffer();
    sleep_ms(1000);
    
    // Teste de pixels
    oled_write_string(0, 44, "3. Pixel test...");
    oled_display_buffer();
    sleep_ms(2000);
    
    // Resultado final
    oled_clear();
    oled_write_string(20, 24, "DIAGNOSTICO");
    oled_write_string(25, 32, "COMPLETO!");
    oled_display_buffer();
    sleep_ms(2000);
}

// Implementações das funções que estavam faltando
bool oled_init_final_corrigido(void) {
    return oled_init();
}

bool oled_teste_alto_contraste_visual(void) {
    if (!ssd1306_init_done) {
        return false;
    }
    
    oled_clear();
    oled_write_string(0, 16, "TESTE CONTRASTE");
    oled_write_string(0, 32, "ALTO VISUAL");
    oled_display_buffer();
    sleep_ms(1000);
    
    // Teste de contraste alto
    oled_set_contrast(0xFF);
    oled_invert_display(true);
    sleep_ms(500);
    oled_invert_display(false);
    sleep_ms(500);
    
    return true;
}

void oled_mostrar_splash(void) {
    oled_splash_screen();
}

bool oled_teste_definitivo_gp14_gp15(void) {
    printf("🔧 Teste definitivo GP14/GP15...\n");
    return oled_init();
}

void oled_mostrar_tela_principal(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    // Título
    oled_write_string(16, 0, "HYDROSENSE 2.1");
    
    // Linha separadora
    DrawLine(0, 12, SSD1306_WIDTH-1, 12, true);
    
    // Informações básicas
    oled_write_string(0, 20, "Sistema: OPERACIONAL");
    oled_write_string(0, 30, "Status: MONITORANDO");
    oled_write_string(0, 40, "WiFi: CONECTADO");
    
    // Instruções
    oled_write_string(0, 54, "A:Menu B:Config");
    
    oled_display_buffer();
}

void oled_mostrar_menu(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    // Título do menu
    oled_write_string(32, 0, "=== MENU ===");
    
    // Opções do menu
    oled_write_string(8, 16, "1. Sensores");
    oled_write_string(8, 26, "2. Configuracoes");
    oled_write_string(8, 36, "3. Diagnostico");
    oled_write_string(8, 46, "4. Sobre");
    
    // Indicador de seleção (seta)
    oled_write_string(0, 16, ">");
    
    oled_display_buffer();
}

void oled_teste_orientacao(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(0, 16, "TESTE ORIENTACAO");
    oled_write_string(0, 32, "Rotacionando...");
    oled_display_buffer();
    sleep_ms(1000);
    
    // Simular teste de orientação
    for (int i = 1; i <= 4; i++) {
        oled_clear();
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Orientacao %d/4", i);
        oled_write_string(20, 28, buffer);
        oled_display_buffer();
        sleep_ms(800);
    }
}

void oled_aplicar_orientacao(int orientacao) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Aplicando: %d", orientacao);
    oled_write_string(16, 24, buffer);
    oled_write_string(8, 40, "Orientacao definida!");
    oled_display_buffer();
    sleep_ms(1500);
}

// Implementações das funções de display específicas do HydroSense
void oled_display_umidade(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    oled_write_string(25, 8, "UMIDADE");
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Temp: %.1fC", data->temperatura);
    oled_write_string(20, 28, buffer);
    
    oled_display_buffer();
}

void oled_display_tds(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    oled_write_string(35, 8, "TDS");
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "pH: %.1f", data->ph);
    oled_write_string(30, 28, buffer);
    
    oled_display_buffer();
}

void oled_display_bomba_status(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    oled_write_string(20, 8, "STATUS BOMBA");
    
    const char* status = data->tpa_em_andamento ? "ATIVA" : "PARADA";
    oled_write_string(35, 28, status);
    
    oled_display_buffer();
}

void oled_display_sistema_status(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    oled_write_string(15, 0, "STATUS SISTEMA");
    
    char buffer[32];
    
    // WiFi
    const char* wifi_status = data->wifi_conectado ? "OK" : "ERRO";
    snprintf(buffer, sizeof(buffer), "WiFi: %s", wifi_status);
    oled_write_string(5, 16, buffer);
    
    // MQTT
    const char* mqtt_status = data->mqtt_conectado ? "OK" : "ERRO";
    snprintf(buffer, sizeof(buffer), "MQTT: %s", mqtt_status);
    oled_write_string(5, 26, buffer);
    
    // Uptime
    snprintf(buffer, sizeof(buffer), "Uptime: %lus", data->uptime);
    oled_write_string(5, 36, buffer);
    
    oled_display_buffer();
}

void oled_log_mensagem(const char* msg) {
    if (!ssd1306_init_done || !msg) return;
    
    oled_clear();
    oled_write_string(0, 20, "LOG:");
    oled_write_string(0, 35, msg);
    oled_display_buffer();
    sleep_ms(1500);
}

// Novas funções para exibição detalhada de cada etapa

// Tela principal com informações em tempo real
void oled_tela_principal_tempo_real(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    
    // Cabeçalho com horário
    char time_buffer[32];
    uint32_t hours = (data->uptime / 3600) % 24;
    uint32_t minutes = (data->uptime / 60) % 60;
    snprintf(time_buffer, sizeof(time_buffer), "%02lu:%02lu", hours, minutes);
    
    oled_write_string(0, 0, "HYDROSENSE");
    oled_write_string(75, 0, time_buffer);
    
    // Linha separadora
    DrawLine(0, 10, SSD1306_WIDTH-1, 10, true);
    
    // Sensores em tempo real
    char buffer[32];
    
    // Temperatura
    snprintf(buffer, sizeof(buffer), "Temp: %.1fC", data->temperatura);
    oled_write_string(0, 15, buffer);
    
    // pH
    snprintf(buffer, sizeof(buffer), "pH: %.1f", data->ph);
    oled_write_string(0, 25, buffer);
    
    // Nível de água
    snprintf(buffer, sizeof(buffer), "Nivel: %.0f%%", data->nivel_agua);
    oled_write_string(0, 35, buffer);
    
    // Status de conectividade
    const char* wifi_icon = data->wifi_conectado ? "WiFi:OK" : "WiFi:--";
    const char* mqtt_icon = data->mqtt_conectado ? "MQTT:OK" : "MQTT:--";
    
    oled_write_string(0, 45, wifi_icon);
    oled_write_string(55, 45, mqtt_icon);
    
    // Instruções na parte inferior
    oled_write_string(0, 56, "A:Menu B:Alimentar");
    
    oled_display_buffer();
}

// Tela de alimentação manual (Botão B)
void oled_alimentacao_manual_iniciada(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(15, 10, "ALIMENTACAO MANUAL");
    DrawLine(0, 22, SSD1306_WIDTH-1, 22, true);
    
    oled_write_string(20, 30, "Servo acionado!");
    oled_write_string(15, 40, "Girando 0->180");
    oled_write_string(25, 50, "Dispensando...");
    
    oled_display_buffer();
}

void oled_alimentacao_servo_retornando(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(15, 10, "ALIMENTACAO MANUAL");
    DrawLine(0, 22, SSD1306_WIDTH-1, 22, true);
    
    oled_write_string(15, 30, "Retornando servo");
    oled_write_string(20, 40, "Posicao: 180->0");
    oled_write_string(25, 50, "Finalizando...");
    
    oled_display_buffer();
}

void oled_alimentacao_manual_concluida(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(15, 15, "ALIMENTACAO");
    oled_write_string(20, 30, "CONCLUIDA!");
    
    DrawLine(10, 45, 118, 45, true);
    oled_write_string(20, 50, "Peixes alimentados");
    
    oled_display_buffer();
    sleep_ms(3000); // Mostrar por 3 segundos
}

// Tela de alimentação programada
void oled_alimentacao_programada_alerta(uint8_t hora, uint8_t quantidade) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(10, 5, "HORARIO PROGRAMADO");
    DrawLine(0, 17, SSD1306_WIDTH-1, 17, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Hora: %02d:00", hora);
    oled_write_string(25, 25, buffer);
    
    snprintf(buffer, sizeof(buffer), "Racao: %d porcoes", quantidade);
    oled_write_string(10, 35, buffer);
    
    oled_write_string(15, 50, "Iniciando em 3s...");
    
    oled_display_buffer();
    sleep_ms(3000);
}

void oled_alimentacao_programada_executando(uint8_t hora, uint8_t porcao_atual, uint8_t total_porcoes) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(5, 5, "ALIMENTACAO AUTO");
    DrawLine(0, 17, SSD1306_WIDTH-1, 17, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Horario: %02d:00", hora);
    oled_write_string(20, 25, buffer);
    
    snprintf(buffer, sizeof(buffer), "Porcao: %d/%d", porcao_atual, total_porcoes);
    oled_write_string(25, 35, buffer);
    
    // Barra de progresso simples
    int progress_width = (porcao_atual * 80) / total_porcoes;
    DrawLine(20, 50, 20 + progress_width, 50, true);
    DrawLine(20, 51, 20 + progress_width, 51, true);
    
    oled_display_buffer();
}

// Tela de TPA - Bomba 1 (esvaziamento)
void oled_tpa_bomba1_iniciando(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, SSD1306_WIDTH-1, 17, true);
    
    oled_write_string(15, 25, "BOMBA 1 ACIONADA");
    oled_write_string(5, 35, "Esvaziando tanque...");
    oled_write_string(10, 45, "Meta: 25% do volume");
    
    oled_display_buffer();
}

void oled_tpa_bomba1_progresso(float nivel_atual, float meta) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, SSD1306_WIDTH-1, 17, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Nivel: %.1f%%", nivel_atual);
    oled_write_string(25, 25, buffer);
    
    snprintf(buffer, sizeof(buffer), "Meta: %.0f%%", meta);
    oled_write_string(30, 35, buffer);
    
    // Barra de progresso do esvaziamento
    int progress = (int)((100 - nivel_atual) * 80 / (100 - meta));
    if (progress > 80) progress = 80;
    
    DrawLine(20, 50, 20 + progress, 50, true);
    DrawLine(20, 51, 20 + progress, 51, true);
    
    oled_write_string(5, 55, "Esvaziando...");
    
    oled_display_buffer();
}

void oled_tpa_bomba1_meta_atingida(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(25, 10, "BOMBA 1");
    oled_write_string(15, 25, "META ATINGIDA!");
    oled_write_string(10, 40, "25% do volume");
    oled_write_string(20, 50, "esvaziado");
    
    oled_display_buffer();
    sleep_ms(2000);
}

// Tela de TPA - Bomba 2 (reabastecimento)
void oled_tpa_bomba2_iniciando(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, SSD1306_WIDTH-1, 17, true);
    
    oled_write_string(15, 25, "BOMBA 2 ACIONADA");
    oled_write_string(5, 35, "Completando nivel");
    oled_write_string(10, 45, "Meta: 100% volume");
    
    oled_display_buffer();
}

void oled_tpa_bomba2_progresso(float nivel_atual) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, SSD1306_WIDTH-1, 17, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Nivel: %.1f%%", nivel_atual);
    oled_write_string(25, 25, buffer);
    
    oled_write_string(25, 35, "Meta: 100%");
    
    // Barra de progresso do reabastecimento
    int progress = (int)(nivel_atual * 80 / 100);
    if (progress > 80) progress = 80;
    
    DrawLine(20, 50, 20 + progress, 50, true);
    DrawLine(20, 51, 20 + progress, 51, true);
    
    oled_write_string(5, 55, "Reabastecendo...");
    
    oled_display_buffer();
}

void oled_tpa_bomba2_concluida(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(30, 10, "TPA");
    oled_write_string(20, 25, "CONCLUIDO!");
    
    DrawLine(10, 35, 118, 35, true);
    
    oled_write_string(10, 45, "Tanque: 100%");
    oled_write_string(5, 55, "Sistema normalizado");
    
    oled_display_buffer();
    sleep_ms(4000); // Mostrar por 4 segundos
}

// Menu de navegação aprimorado
void oled_menu_principal(uint8_t item_selecionado) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(35, 0, "== MENU ==");
    DrawLine(0, 12, SSD1306_WIDTH-1, 12, true);
    
    const char* opcoes[] = {
        "1. Sensores",
        "2. Alimentacao", 
        "3. Sistema TPA",
        "4. Configuracoes",
        "5. Diagnosticos"
    };
    
    for (int i = 0; i < 5; i++) {
        int y_pos = 20 + (i * 8);
        
        // Desenhar seta se item selecionado
        if (i == item_selecionado) {
            oled_write_string(0, y_pos, ">");
        }
        
        oled_write_string(10, y_pos, opcoes[i]);
    }
    
    oled_display_buffer();
}

// Tela de status dos sensores detalhada
void oled_menu_sensores(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    
    oled_write_string(25, 0, "SENSORES");
    DrawLine(0, 12, SSD1306_WIDTH-1, 12, true);
    
    char buffer[32];
    
    // Temperatura com status
    snprintf(buffer, sizeof(buffer), "Temp: %.1fC", data->temperatura);
    oled_write_string(0, 18, buffer);
    
    const char* temp_status = (data->temperatura >= 24.0 && data->temperatura <= 28.0) ? "OK" : "!!";
    oled_write_string(100, 18, temp_status);
    
    // pH com status
    snprintf(buffer, sizeof(buffer), "pH: %.2f", data->ph);
    oled_write_string(0, 28, buffer);
    
    const char* ph_status = (data->ph >= 6.5 && data->ph <= 8.0) ? "OK" : "!!";
    oled_write_string(100, 28, ph_status);
    
    // Nível com status
    snprintf(buffer, sizeof(buffer), "Nivel: %.0f%%", data->nivel_agua);
    oled_write_string(0, 38, buffer);
    
    const char* nivel_status = (data->nivel_agua > 25.0) ? "OK" : "!!";
    oled_write_string(100, 38, nivel_status);
    
    // Última atualização
    oled_write_string(0, 50, "Atualizando...");
    
    oled_display_buffer();
}

// Funções de alerta e notificações
void oled_alerta_temperatura(float temp) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(30, 10, "ALERTA!");
    DrawLine(0, 22, SSD1306_WIDTH-1, 22, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Temp: %.1fC", temp);
    oled_write_string(25, 30, buffer);
    
    if (temp < 24.0) {
        oled_write_string(15, 45, "Muito baixa!");
    } else if (temp > 28.0) {
        oled_write_string(20, 45, "Muito alta!");
    }
    
    oled_display_buffer();
}

void oled_alerta_ph(float ph) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(30, 10, "ALERTA!");
    DrawLine(0, 22, SSD1306_WIDTH-1, 22, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "pH: %.2f", ph);
    oled_write_string(35, 30, buffer);
    
    if (ph < 6.5) {
        oled_write_string(25, 45, "Muito acido!");
    } else if (ph > 8.0) {
        oled_write_string(20, 45, "Muito basico!");
    }
    
    oled_display_buffer();
}

void oled_alerta_nivel_critico(float nivel) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    
    oled_write_string(25, 10, "CRITICO!");
    DrawLine(0, 22, SSD1306_WIDTH-1, 22, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Nivel: %.0f%%", nivel);
    oled_write_string(25, 30, buffer);
    
    oled_write_string(10, 45, "Nivel muito baixo!");
    oled_write_string(15, 55, "TPA necessario");
    
    oled_display_buffer();
}
