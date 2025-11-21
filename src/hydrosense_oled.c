#include "hydrosense_system.h"
#include "hardware/i2c.h"
#include "hardware/rtc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>  // Para cos() e sin()

// Baseado nos exemplos oficiais do Pico + BitDogLab
#define SSD1306_HEIGHT              64
#define SSD1306_WIDTH               128
#define SSD1306_I2C_ADDR_PRIMARY    0x3C  // Endereço mais comum
#define SSD1306_I2C_ADDR_SECONDARY  0x3D  // Endereço alternativo

// Comandos SSD1306 (dos exemplos oficiais)
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_SET_ENTIRE_ON       0xA4
#define SSD1306_SET_NORM_INV        0xA6
#define SSD1306_SET_DISP            0xAE
#define SSD1306_SET_MEM_ADDR        0x20
#define SSD1306_SET_COL_ADDR        0x21
#define SSD1306_SET_PAGE_ADDR       0x22
#define SSD1306_SET_DISP_START_LINE 0x40
#define SSD1306_SET_SEG_REMAP       0xA0
#define SSD1306_SET_MUX_RATIO       0xA8
#define SSD1306_SET_COM_OUT_DIR     0xC0
#define SSD1306_SET_DISP_OFFSET     0xD3
#define SSD1306_SET_COM_PIN_CFG     0xDA
#define SSD1306_SET_DISP_CLK_DIV    0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_VCOM_DESEL      0xDB
#define SSD1306_SET_CHARGE_PUMP     0x8D

// Estrutura para área de renderização (inspirada na BitDogLab)
typedef struct {
    uint start_column;
    uint end_column; 
    uint start_page;
    uint end_page;
    size_t buffer_length;
} render_area_t;

// Estado do driver melhorado
static bool ssd1306_init_done = false;
static i2c_inst_t *ssd1306_i2c = i2c1;  // CORRIGIDO: usar i2c1 para GP14/GP15
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint8_t ssd1306_detected_addr = 0;  // Endereço detectado
static render_area_t frame_area;

// Font 8x8 dos exemplos oficiais do Pico
static const uint8_t font_8x8[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0x20 ' '
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // 0x21 '!'
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0x22 '"'
    {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // 0x23 '#'
    {0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // 0x24 '$'
    {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // 0x25 '%'
    {0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // 0x26 '&'
    {0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0x27 '''
    {0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // 0x28 '('
    {0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // 0x29 ')'
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // 0x2A '*'
    {0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // 0x2B '+'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x06, 0x00}, // 0x2C ','
    {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // 0x2D '-'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // 0x2E '.'
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // 0x2F '/'
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // 0x30 '0'
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // 0x31 '1'
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // 0x32 '2'
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // 0x33 '3'
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // 0x34 '4'
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // 0x35 '5'
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // 0x36 '6'
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // 0x37 '7'
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // 0x38 '8'
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // 0x39 '9'
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // 0x3A ':'
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x06, 0x00}, // 0x3B ';'
    {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // 0x3C '<'
    {0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // 0x3D '='
    {0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // 0x3E '>'
    {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // 0x3F '?'
    {0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // 0x40 '@'
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // 0x41 'A'
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // 0x42 'B' 
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // 0x43 'C'
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // 0x44 'D'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // 0x45 'E'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // 0x46 'F'
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // 0x47 'G'
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // 0x48 'H'
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 0x49 'I'
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // 0x4A 'J'
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // 0x4B 'K'
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // 0x4C 'L'
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // 0x4D 'M'
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // 0x4E 'N'
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // 0x4F 'O'
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // 0x50 'P'
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // 0x51 'Q'
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // 0x52 'R'
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // 0x53 'S'
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 0x54 'T'
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // 0x55 'U'
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // 0x56 'V'
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // 0x57 'W'
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // 0x58 'X'
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // 0x59 'Y'
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // 0x5A 'Z'
    {0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00}, // 0x5B '['
    {0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00}, // 0x5C '\'
    {0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00}, // 0x5D ']'
    {0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // 0x5E '^'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // 0x5F '_'
    {0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0x60 '`'
    {0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00}, // 0x61 'a'
    {0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00}, // 0x62 'b'
    {0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00}, // 0x63 'c'
    {0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00}, // 0x64 'd'
    {0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00}, // 0x65 'e'
    {0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00}, // 0x66 'f'
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // 0x67 'g'
    {0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00}, // 0x68 'h'
    {0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 0x69 'i'
    {0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E}, // 0x6A 'j'
    {0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00}, // 0x6B 'k'
    {0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 0x6C 'l'
    {0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00}, // 0x6D 'm'
    {0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00}, // 0x6E 'n'
    {0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00}, // 0x6F 'o'
    {0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F}, // 0x70 'p'
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78}, // 0x71 'q'
    {0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00}, // 0x72 'r'
    {0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00}, // 0x73 's'
    {0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00}, // 0x74 't'
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00}, // 0x75 'u'
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // 0x76 'v'
    {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}, // 0x77 'w'
    {0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00}, // 0x78 'x'
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // 0x79 'y'
    {0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00}, // 0x7A 'z'
    {0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00}, // 0x7B '{'
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // 0x7C '|'
    {0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00}, // 0x7D '}'
    {0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 0x7E '~'
};

// Função para detectar dispositivos I2C
static bool i2c_device_scan(uint8_t addr) {
    uint8_t dummy;
    int ret = i2c_read_blocking(ssd1306_i2c, addr, &dummy, 1, false);
    return ret >= 0;
}

// Funções melhoradas com tratamento de erro
static bool ssd1306_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    int ret = i2c_write_blocking(i2c1, ssd1306_detected_addr, buf, 2, false);  // i2c1
    if (ret < 0) {
        printf("❌ Erro I2C ao enviar comando 0x%02X (ret=%d)\n", cmd, ret);
        return false;
    }
    return true;
}

static bool ssd1306_write_buf(uint8_t buf[], int buflen) {
    uint8_t *temp_buf = malloc(buflen + 1);
    if (!temp_buf) {
        printf("❌ Erro de memória ao alocar buffer\n");
        return false;
    }
    
    temp_buf[0] = 0x40;
    memcpy(temp_buf + 1, buf, buflen);
    int ret = i2c_write_blocking(i2c1, ssd1306_detected_addr, temp_buf, buflen + 1, false);  // i2c1
    free(temp_buf);
    
    if (ret < 0) {
        printf("❌ Erro I2C ao enviar buffer (%d bytes, ret=%d)\n", buflen, ret);
        return false;
    }
    return true;
}

// Função inspirada no algoritmo de Bresenham da BitDogLab
void oled_draw_line(int x0, int y0, int x1, int y1) {
    if (!ssd1306_init_done) return;
    
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    int error_2;

    while (true) {
        oled_set_pixel(x0, y0, true);
        if (x0 == x1 && y0 == y1) {
            break;
        }

        error_2 = 2 * error;

        if (error_2 >= dy) {
            error += dy;
            x0 += sx;
        }
        if (error_2 <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

// Função ULTRA-ESPECÍFICA para GP14/GP15 - teste definitivo
bool oled_teste_definitivo_gp14_gp15(void) {
    printf("🎯 === TESTE DEFINITIVO DISPLAY GP14/GP15 ===\n");
    printf("🔧 Servo funcionou = pinos corretos! Agora testando display...\n");
    
    // TESTE 1: I2C1 com GP14/GP15 (padrão correto)
    printf("\n📡 TESTE 1: I2C1 com GP14=SDA, GP15=SCL\n");
    
    i2c_deinit(i2c0);  // Garante que i2c0 está limpo
    i2c_deinit(i2c1);  // Garante que i2c1 está limpo
    sleep_ms(100);
    
    // Configura I2C1 super conservador
    i2c_init(i2c1, 50000);  // 50kHz - extremamente lento e confiável
    gpio_set_function(14, GPIO_FUNC_I2C);  // GP14 = SDA
    gpio_set_function(15, GPIO_FUNC_I2C);  // GP15 = SCL
    gpio_pull_up(14);
    gpio_pull_up(15);
    sleep_ms(500);  // Aguarda muito tempo
    
    // Testa comunicação básica em 0x3C
    uint8_t dummy;
    int ret_3c = i2c_read_blocking(i2c1, 0x3C, &dummy, 1, false);
    printf("   0x3C: %s (ret=%d)\n", ret_3c >= 0 ? "✅ RESPONDE" : "❌ NÃO RESPONDE", ret_3c);
    
    // Testa comunicação básica em 0x3D  
    int ret_3d = i2c_read_blocking(i2c1, 0x3D, &dummy, 1, false);
    printf("   0x3D: %s (ret=%d)\n", ret_3d >= 0 ? "✅ RESPONDE" : "❌ NÃO RESPONDE", ret_3d);
    
    uint8_t found_addr = 0;
    if (ret_3c >= 0) {
        found_addr = 0x3C;
    } else if (ret_3d >= 0) {
        found_addr = 0x3D;
    }
    
    if (found_addr > 0) {
        printf("🎉 DISPOSITIVO ENCONTRADO em 0x%02X!\n", found_addr);
        
        // Teste comando simples
        uint8_t cmd_test[] = {0x00, 0xAE};  // Display OFF
        int cmd_ret = i2c_write_blocking(i2c1, found_addr, cmd_test, 2, false);
        printf("   Comando teste: %s (ret=%d)\n", cmd_ret >= 0 ? "✅ OK" : "❌ FALHOU", cmd_ret);
        
        if (cmd_ret >= 0) {
            // Sequência ultra-simples de inicialização
            printf("⚙️ Enviando inicialização ultra-simples...\n");
            
            uint8_t init[] = {
                0x00, 0xAE,  // Display OFF
                0x00, 0x8D,  // Charge Pump  
                0x00, 0x14,  // Enable Charge Pump
                0x00, 0x20,  // Memory Mode
                0x00, 0x00,  // Horizontal
                0x00, 0x81,  // Contrast
                0x00, 0xFF,  // Max contrast
                0x00, 0xAF   // Display ON
            };
            
            int ret = i2c_write_blocking(i2c1, found_addr, init, sizeof(init), false);
            printf("   Inicialização: %s (ret=%d)\n", ret >= 0 ? "✅ OK" : "❌ FALHOU", ret);
            
            if (ret >= 0) {
                sleep_ms(200);
                
                // Teste visual extremo - tela toda branca
                printf("🎨 Enviando tela BRANCA extrema...\n");
                
                uint8_t white[1025];
                white[0] = 0x40;  // Data mode
                memset(&white[1], 0xFF, 1024);  // Todos pixels brancos
                
                ret = i2c_write_blocking(i2c1, found_addr, white, sizeof(white), false);
                printf("   Tela branca: %s (ret=%d)\n", ret >= 0 ? "✅ ENVIADO" : "❌ FALHOU", ret);
                
                if (ret >= 0) {
                    printf("📺 DISPLAY DEVE ESTAR COMPLETAMENTE BRANCO AGORA!\n");
                    printf("⏱️ Aguardando 10 segundos para verificação...\n");
                    
                    for (int i = 10; i > 0; i--) {
                        printf("   %d... ", i);
                        fflush(stdout);
                        sleep_ms(1000);
                    }
                    printf("\n");
                    
                    // Teste piscar display
                    printf("💡 Testando piscar display (3x)...\n");
                    for (int i = 0; i < 3; i++) {
                        // OFF
                        uint8_t off[] = {0x00, 0xAE};
                        i2c_write_blocking(i2c1, found_addr, off, 2, false);
                        printf("   OFF %d\n", i+1);
                        sleep_ms(1000);
                        
                        // ON
                        uint8_t on[] = {0x00, 0xAF}; 
                        i2c_write_blocking(i2c1, found_addr, on, 2, false);
                        printf("   ON %d\n", i+1);
                        sleep_ms(1000);
                    }
                    
                    ssd1306_init_done = true;
                    ssd1306_detected_addr = found_addr;
                    ssd1306_i2c = i2c1;
                    
                    printf("✅ TESTE DEFINITIVO CONCLUÍDO COM SUCESSO!\n");
                    return true;
                }
            }
        }
    }
    
    // TESTE 2: Scan completo I2C1 para ver todos os dispositivos
    printf("\n🔍 TESTE 2: Scan completo I2C1 (GP14/GP15)\n");
    int devices = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        int scan_ret = i2c_read_blocking(i2c1, addr, &dummy, 1, false);
        if (scan_ret >= 0) {
            printf("   0x%02X: ✅ DISPOSITIVO ENCONTRADO\n", addr);
            devices++;
        }
    }
    printf("📊 Total dispositivos I2C1: %d\n", devices);
    
    // TESTE 3: Tentar I2C0 mesmo com GP14/GP15 (teste de compatibilidade)
    printf("\n🔄 TESTE 3: I2C0 com GP14/GP15 (teste compatibilidade)\n");
    
    i2c_deinit(i2c1);
    sleep_ms(100);
    
    i2c_init(i2c0, 50000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    sleep_ms(500);
    
    ret_3c = i2c_read_blocking(i2c0, 0x3C, &dummy, 1, false);
    ret_3d = i2c_read_blocking(i2c0, 0x3D, &dummy, 1, false);
    
    printf("   I2C0+GP14/15 -> 0x3C: %s, 0x3D: %s\n", 
           ret_3c >= 0 ? "OK" : "FAIL", 
           ret_3d >= 0 ? "OK" : "FAIL");
    
    // TESTE 4: Verificação de alimentação e hardware
    printf("\n🔌 TESTE 4: Diagnóstico de hardware\n");
    printf("📍 Configuração atual:\n");
    printf("   - GP14 função: %d (deve ser 2 para I2C)\n", gpio_get_function(14));
    printf("   - GP15 função: %d (deve ser 2 para I2C)\n", gpio_get_function(15));
    printf("   - Servo funciona: ✅ (confirma pinos corretos)\n");
    
    printf("\n🔧 CHECKLIST FÍSICO:\n");
    printf("   □ Display VCC conectado em 3.3V (NÃO 5V!)\n");
    printf("   □ Display GND conectado\n");
    printf("   □ Display SDA conectado em GP14\n");
    printf("   □ Display SCL conectado em GP15\n");
    printf("   □ Jumpers/fios bem conectados (sem falso contato)\n");
    printf("   □ Display não está fisicamente danificado\n");
    
    if (devices == 0) {
        printf("\n❌ CONCLUSÃO: PROBLEMA FÍSICO CONFIRMADO\n");
        printf("   Nenhum dispositivo I2C encontrado em GP14/GP15\n");
        printf("   Como o servo funciona, os pinos estão corretos\n");
        printf("   Logo, é problema de conexão/alimentação do display\n");
    }
    
    printf("===============================================\n");
    return false;
}

// Inicialização melhorada inspirada na BitDogLab
bool oled_init_bitdog_inspired(void) {
    printf("🔍 Inicializando display SSD1306 (método BitDogLab)...\n");
    
    // Configuração I2C mais robusta
    i2c_init(i2c0, 400000);  // 400kHz exato
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    
    printf("📡 I2C configurado: SDA=GPIO%d, SCL=GPIO%d, Speed=400kHz\n", 
           I2C_OLED_SDA, I2C_OLED_SCL);
    
    // Aguarda estabilização maior
    sleep_ms(200);
    
    // Detecta dispositivo
    ssd1306_detected_addr = SSD1306_I2C_ADDR_PRIMARY;
    uint8_t dummy;
    int ret = i2c_read_blocking(i2c0, ssd1306_detected_addr, &dummy, 1, false);
    if (ret < 0) {
        printf("❌ Display não encontrado no endereço 0x%02X\n", ssd1306_detected_addr);
        return false;
    }
    
    printf("✅ Display SSD1306 detectado no endereço 0x%02X\n", ssd1306_detected_addr);
    
    // Sequência de inicialização otimizada (inspirada na BitDogLab)
    printf("⚙️ Configurando display SSD1306 (sequência BitDogLab)...\n");
    
    // Reset via comando (mais confiável)
    ssd1306_write_cmd(SSD1306_SET_DISP | 0x00); // display off
    sleep_ms(10);
    
    // Configuração de memória
    ssd1306_write_cmd(SSD1306_SET_MEM_ADDR);
    ssd1306_write_cmd(0x00); // horizontal addressing
    
    // Configuração de display
    ssd1306_write_cmd(SSD1306_SET_DISP_START_LINE | 0x00);
    ssd1306_write_cmd(SSD1306_SET_SEG_REMAP | 0x01);
    ssd1306_write_cmd(SSD1306_SET_MUX_RATIO);
    ssd1306_write_cmd(0x3F); // 64-1
    ssd1306_write_cmd(SSD1306_SET_COM_OUT_DIR | 0x08);
    ssd1306_write_cmd(SSD1306_SET_DISP_OFFSET);
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(SSD1306_SET_COM_PIN_CFG);
    ssd1306_write_cmd(0x12);
    
    // Configuração de contraste e charge pump
    ssd1306_write_cmd(SSD1306_SET_CONTRAST);
    ssd1306_write_cmd(0x7F); // Contraste médio inicialmente
    ssd1306_write_cmd(SSD1306_SET_PRECHARGE);
    ssd1306_write_cmd(0xF1);
    ssd1306_write_cmd(SSD1306_SET_VCOM_DESEL);
    ssd1306_write_cmd(0x40);
    ssd1306_write_cmd(SSD1306_SET_ENTIRE_ON | 0x00);
    ssd1306_write_cmd(SSD1306_SET_NORM_INV | 0x00);
    ssd1306_write_cmd(SSD1306_SET_DISP_CLK_DIV);
    ssd1306_write_cmd(0x80);
    ssd1306_write_cmd(SSD1306_SET_CHARGE_PUMP);
    ssd1306_write_cmd(0x14); // Enable charge pump
    
    sleep_ms(100);
    
    // Liga o display
    ssd1306_write_cmd(SSD1306_SET_DISP | 0x01);
    sleep_ms(100);
    
    // Configura área de renderização (como na BitDogLab)
    frame_area.start_column = 0;
    frame_area.end_column = SSD1306_WIDTH - 1;
    frame_area.start_page = 0;
    frame_area.end_page = (SSD1306_HEIGHT / 8) - 1;
    frame_area.buffer_length = SSD1306_WIDTH * SSD1306_HEIGHT / 8;
    
    // Limpa display completamente
    memset(ssd1306_buffer, 0, sizeof(ssd1306_buffer));
    oled_display_buffer_bitdog();
    
    sleep_ms(100);
    
    // Teste visual inspirado na BitDogLab
    printf("🧪 Teste visual inspirado na BitDogLab...\n");
    
    // Desenha retângulo usando linhas
    oled_draw_line(10, 10, 110, 10);   // top
    oled_draw_line(110, 10, 110, 50);  // right  
    oled_draw_line(110, 50, 10, 50);   // bottom
    oled_draw_line(10, 50, 10, 10);    // left
    
    // Desenha X no centro
    oled_draw_line(30, 20, 90, 40);    // diagonal \
    oled_draw_line(30, 40, 90, 20);    // diagonal /
    
    // Texto de teste
    oled_write_string(15, 55, "BitDog Test");
    
    if (oled_display_buffer_bitdog()) {
        printf("✅ Teste visual BitDogLab executado com sucesso!\n");
        sleep_ms(3000);
    }
    
    ssd1306_init_done = true;
    printf("✅ SSD1306 inicializado com método BitDogLab!\n");
    
    return true;
}

// Função de display buffer melhorada (inspirada na BitDogLab)
bool oled_display_buffer_bitdog(void) {
    if (!ssd1306_init_done) {
        printf("⚠️ Display não inicializado\n");
        return false;
    }
    
    // Configura área de renderização como na BitDogLab
    ssd1306_write_cmd(SSD1306_SET_COL_ADDR);
    ssd1306_write_cmd(frame_area.start_column);
    ssd1306_write_cmd(frame_area.end_column);
    
    ssd1306_write_cmd(SSD1306_SET_PAGE_ADDR);
    ssd1306_write_cmd(frame_area.start_page);
    ssd1306_write_cmd(frame_area.end_page);
    
    // Envia dados como na BitDogLab (sem o byte de comando extra)
    int ret = i2c_write_blocking(i2c0, ssd1306_detected_addr, ssd1306_buffer, sizeof(ssd1306_buffer), false);
    
    if (ret < 0) {
        printf("❌ Erro ao enviar buffer para display\n");
        return false;
    }
    
    return true;
}

// Teste completo inspirado na BitDogLab
void oled_teste_completo_bitdog(void) {
    if (!ssd1306_init_done) {
        printf("❌ Display não inicializado para teste BitDogLab\n");
        return;
    }
    
    printf("🧪 === TESTE COMPLETO INSPIRADO NA BITDOGLAB ===\n");
    
    // Teste 1: Texto simples
    printf("📝 Teste 1: Texto simples...\n");
    oled_clear();
    oled_write_string(10, 10, "Bem-vindos!");
    oled_write_string(15, 25, "HydroSense");
    oled_write_string(20, 40, "Sistema");
    oled_display_buffer_bitdog();
    sleep_ms(3000);
    
    // Teste 2: Linhas geométricas  
    printf("📐 Teste 2: Linhas geométricas...\n");
    oled_clear();
    // Triângulo
    oled_draw_line(64, 10, 40, 50);  // esquerda
    oled_draw_line(64, 10, 88, 50);  // direita
    oled_draw_line(40, 50, 88, 50);  // base
    oled_write_string(45, 55, "Triangulo");
    oled_display_buffer_bitdog();
    sleep_ms(3000);
    
    // Teste 3: Padrão de pixels
    printf("🎨 Teste 3: Padrão de pixels...\n");
    oled_clear();
    for (int y = 0; y < 64; y += 4) {
        for (int x = 0; x < 128; x += 4) {
            oled_set_pixel(x, y, true);
        }
    }
    oled_write_string(25, 30, "Padrao Dots");
    oled_display_buffer_bitdog();
    sleep_ms(3000);
    
    // Teste 4: Animação simples
    printf("🎬 Teste 4: Animação simples...\n");
    for (int frame = 0; frame < 20; frame++) {
        oled_clear();
        
        // Círculo que se expande
        int radius = frame;
        for (int angle = 0; angle < 360; angle += 10) {
            int x = 64 + (radius * cos(angle * 3.14159 / 180));
            int y = 32 + (radius * sin(angle * 3.14159 / 180));
            if (x >= 0 && x < 128 && y >= 0 && y < 64) {
                oled_set_pixel(x, y, true);
            }
        }
        
        oled_write_string(30, 55, "Animacao");
        oled_display_buffer_bitdog();
        sleep_ms(100);
    }
    
    printf("✅ Teste completo BitDogLab concluído!\n");
    
    // Volta para tela normal
    oled_clear();
    oled_write_string(20, 20, "HydroSense");
    oled_write_string(25, 35, "Ativo!");
    oled_display_buffer_bitdog();
}

// Inicialização melhorada com detecção automática
bool oled_init_auto_scan(void) {
    printf("🔍 Iniciando detecção do display SSD1306...\n");
    
    // Configuração I2C robusta
    i2c_init(i2c0, 400 * 1000);  // 400kHz padrão
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    
    printf("📡 I2C configurado: SDA=GPIO%d, SCL=GPIO%d, Speed=400kHz\n", 
           I2C_OLED_SDA, I2C_OLED_SCL);
    
    sleep_ms(100);  // Aguarda estabilização
    
    // Scan automático de dispositivos I2C
    printf("🔍 Escaneando dispositivos I2C...\n");
    bool found = false;
    
    // Testa endereços comuns do SSD1306
    uint8_t test_addresses[] = {SSD1306_I2C_ADDR_PRIMARY, SSD1306_I2C_ADDR_SECONDARY};
    for (int i = 0; i < 2; i++) {
        printf("   Testando endereço 0x%02X... ", test_addresses[i]);
        if (i2c_device_scan(test_addresses[i])) {
            ssd1306_detected_addr = test_addresses[i];
            found = true;
            printf("✅ ENCONTRADO!\n");
            break;
        } else {
            printf("❌ Não encontrado\n");
        }
    }
    
    if (!found) {
        // Scan completo se não encontrou nos endereços comuns
        printf("🔍 Fazendo scan completo do barramento I2C...\n");
        for (uint8_t addr = 0x08; addr < 0x78; addr++) {
            if (i2c_device_scan(addr)) {
                printf("   Dispositivo encontrado em 0x%02X\n", addr);
                // Se encontrou algo, tenta usar como SSD1306
                if (addr == SSD1306_I2C_ADDR_PRIMARY || addr == SSD1306_I2C_ADDR_SECONDARY) {
                    ssd1306_detected_addr = addr;
                    found = true;
                }
            }
        }
    }
    
    if (!found) {
        printf("❌ Nenhum display SSD1306 encontrado no barramento I2C\n");
        printf("🔧 Verifique as conexões:\n");
        printf("   - VCC → 3.3V\n");
        printf("   - GND → GND\n");
        printf("   - SDA → GPIO%d\n", I2C_OLED_SDA);
        printf("   - SCL → GPIO%d\n", I2C_OLED_SCL);
        return false;
    }
    
    printf("🎯 Usando SSD1306 no endereço 0x%02X\n", ssd1306_detected_addr);
    
    // Sequência de inicialização melhorada
    printf("⚙️ Configurando display SSD1306...\n");
    
    bool init_ok = true;
    init_ok &= ssd1306_write_cmd(SSD1306_SET_DISP | 0x00); // display off
    init_ok &= ssd1306_write_cmd(SSD1306_SET_MEM_ADDR);    // set memory address mode
    init_ok &= ssd1306_write_cmd(0x00);                    // horizontal addressing mode
    init_ok &= ssd1306_write_cmd(SSD1306_SET_DISP_START_LINE); // set display start line to 0
    init_ok &= ssd1306_write_cmd(SSD1306_SET_SEG_REMAP | 0x01); // set segment re-map
    init_ok &= ssd1306_write_cmd(SSD1306_SET_MUX_RATIO);   // set multiplex ratio
    init_ok &= ssd1306_write_cmd(SSD1306_HEIGHT - 1);
    init_ok &= ssd1306_write_cmd(SSD1306_SET_COM_OUT_DIR | 0x08); // set COM scan direction
    init_ok &= ssd1306_write_cmd(SSD1306_SET_DISP_OFFSET); // set display offset
    init_ok &= ssd1306_write_cmd(0x00);
    init_ok &= ssd1306_write_cmd(SSD1306_SET_COM_PIN_CFG);  // set COM pins hardware configuration
    init_ok &= ssd1306_write_cmd(0x12);
    init_ok &= ssd1306_write_cmd(SSD1306_SET_CONTRAST);     // set contrast control
    init_ok &= ssd1306_write_cmd(0xFF);
    init_ok &= ssd1306_write_cmd(SSD1306_SET_ENTIRE_ON);    // disable entire display on
    init_ok &= ssd1306_write_cmd(SSD1306_SET_NORM_INV);     // set normal display
    init_ok &= ssd1306_write_cmd(SSD1306_SET_DISP_CLK_DIV); // set osc frequency
    init_ok &= ssd1306_write_cmd(0x80);
    init_ok &= ssd1306_write_cmd(SSD1306_SET_CHARGE_PUMP);  // enable charge pump
    init_ok &= ssd1306_write_cmd(0x14);
    
    if (!init_ok) {
        printf("❌ Erro na sequência de inicialização do SSD1306\n");
        return false;
    }
    
    sleep_ms(100);
    
    // Liga o display
    if (!ssd1306_write_cmd(SSD1306_SET_DISP | 0x01)) {
        printf("❌ Erro ao ligar o display\n");
        return false;
    }
    
    sleep_ms(100);
    
    // Teste de funcionamento - padrão de teste
    printf("🧪 Testando display com padrão de teste...\n");
    memset(ssd1306_buffer, 0, sizeof(ssd1306_buffer));
    
    // Desenha bordas para teste
    for (int x = 0; x < SSD1306_WIDTH; x++) {
        oled_set_pixel(x, 0, true);                    // borda superior
        oled_set_pixel(x, SSD1306_HEIGHT - 1, true);  // borda inferior
    }
    for (int y = 0; y < SSD1306_HEIGHT; y++) {
        oled_set_pixel(0, y, true);                    // borda esquerda  
        oled_set_pixel(SSD1306_WIDTH - 1, y, true);   // borda direita
    }
    
    // Desenha X no centro
    for (int i = 0; i < 20; i++) {
        oled_set_pixel(54 + i, 22 + i, true);  // diagonal \
        oled_set_pixel(54 + i, 42 - i, true);  // diagonal /
    }
    
    if (!oled_display_buffer()) {
        printf("❌ Erro ao enviar buffer de teste\n");
        return false;
    }
    
    sleep_ms(2000);  // Mostra o teste por 2 segundos
    
    ssd1306_init_done = true;
    printf("✅ SSD1306 inicializado e testado com sucesso!\n");
    printf("📺 Display: %dx%d pixels, I2C 0x%02X\n", 
           SSD1306_WIDTH, SSD1306_HEIGHT, ssd1306_detected_addr);
    
    return true;
}

void oled_clear(void) {
    memset(ssd1306_buffer, 0, sizeof(ssd1306_buffer));
}

void oled_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    
    // Baseado nos exemplos oficiais do Pico
    int page = y / 8;
    int bit = y % 8;
    int index = x + page * SSD1306_WIDTH;
    
    if (on) {
        ssd1306_buffer[index] |= (1 << bit);
    } else {
        ssd1306_buffer[index] &= ~(1 << bit);
    }
}

bool oled_display_buffer(void) {
    if (!ssd1306_init_done) {
        printf("⚠️ Display não inicializado\n");
        return false;
    }
    
    bool cmd_ok = true;
    cmd_ok &= ssd1306_write_cmd(SSD1306_SET_COL_ADDR);
    cmd_ok &= ssd1306_write_cmd(0);
    cmd_ok &= ssd1306_write_cmd(SSD1306_WIDTH - 1);
    cmd_ok &= ssd1306_write_cmd(SSD1306_SET_PAGE_ADDR);
    cmd_ok &= ssd1306_write_cmd(0);
    cmd_ok &= ssd1306_write_cmd(SSD1306_HEIGHT / 8 - 1);
    
    if (!cmd_ok) {
        printf("❌ Erro nos comandos de endereçamento\n");
        return false;
    }
    
    return ssd1306_write_buf(ssd1306_buffer, sizeof(ssd1306_buffer));
}

void oled_write_char(int x, int y, char c) {
    if (c < 32 || c > 126) c = 32;
    
    int char_index = c - 32;
    const uint8_t *char_data = font_8x8[char_index];
    
    for (int i = 0; i < 8; i++) {
        uint8_t line = char_data[i];
        for (int j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                oled_set_pixel(x + i, y + j, true);
            }
        }
    }
}

void oled_write_string(int x, int y, const char* str) {
    int pos_x = x;
    while (*str && pos_x < SSD1306_WIDTH - 8) {
        oled_write_char(pos_x, y, *str);
        pos_x += 8;
        str++;
    }
}

void oled_mostrar_splash(void) {
    if (!ssd1306_init_done) {
        printf("⚠️ Display não inicializado para splash\n");
        return;
    }
    
    printf("🎨 Mostrando splash screen...\n");
    
    oled_clear();
    oled_write_string(20, 8, "HydroSense");
    oled_write_string(35, 20, "v2.1");
    oled_write_string(5, 35, "Sistema IoT");
    oled_write_string(10, 50, "Aquicultura");
    
    if (oled_display_buffer()) {
        printf("✅ Splash screen exibido\n");
    } else {
        printf("❌ Erro ao exibir splash screen\n");
    }
}

void oled_mostrar_tela_principal(void) {
    if (!ssd1306_init_done) return;
    
    extern hydrosense_status_t system_status;
    datetime_t dt;
    rtc_get_datetime(&dt);
    
    char buffer[32];
    
    oled_clear();
    
    // Cabeçalho com hora
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", dt.hour, dt.min, dt.sec);
    oled_write_string(70, 0, buffer);
    
    oled_write_string(0, 0, "HydroSense");
    
    // Dados dos sensores
    snprintf(buffer, sizeof(buffer), "T:%.1fC", system_status.temperatura);
    oled_write_string(0, 15, buffer);
    
    snprintf(buffer, sizeof(buffer), "pH:%.1f", system_status.ph);
    oled_write_string(0, 28, buffer);
    
    snprintf(buffer, sizeof(buffer), "N:%.0f%%", system_status.nivel_agua);
    oled_write_string(0, 41, buffer);
    
    // Status
    oled_write_string(70, 15, (system_status.temperatura >= TEMP_MIN && 
                               system_status.temperatura <= TEMP_MAX) ? "OK" : "!!");
    oled_write_string(70, 28, (system_status.ph >= PH_MIN && 
                               system_status.ph <= PH_MAX) ? "OK" : "!!");
    oled_write_string(70, 41, (system_status.nivel_agua > NIVEL_CRITICO) ? "OK" : "!!");
    
    // Controles
    oled_write_string(15, 54, "A:Menu B:Feed");
    
    oled_display_buffer();
}

void oled_mostrar_menu(void) {
    if (!ssd1306_init_done) return;
    
    extern hydrosense_status_t system_status;
    
    oled_clear();
    
    switch (system_status.menu_atual) {
        case MENU_SENSORES:
            oled_write_string(0, 0, "SENSORES");
            oled_write_string(0, 15, "Temperatura");
            oled_write_string(0, 28, "pH");
            oled_write_string(0, 41, "Nivel Agua");
            break;
            
        case MENU_ALIMENTACAO:
            oled_write_string(0, 0, "ALIMENTACAO");
            oled_write_string(0, 15, "Manual");
            oled_write_string(0, 28, "Automatico");
            break;
            
        case MENU_CONFIG:
            oled_write_string(0, 0, "CONFIG");
            oled_write_string(0, 15, "WiFi");
            oled_write_string(0, 28, "Sistema");
            break;
            
        default:
            oled_write_string(0, 0, "MENU");
            break;
    }
    
    oled_display_buffer();
}

void oled_log_mensagem(const char* msg) {
    if (!ssd1306_init_done) {
        printf("⚠️ Display não disponível para log: %s\n", msg);
        return;
    }
    
    printf("📝 Log no display: %s\n", msg);
    
    oled_clear();
    oled_write_string(0, 0, "LOG:");
    oled_write_string(0, 15, msg);
    
    if (oled_display_buffer()) {
        printf("✅ Log exibido no display\n");
    } else {
        printf("❌ Erro ao exibir log no display\n");
    }
    
    sleep_ms(2000);
}

// Nova função para teste de conectividade
void oled_teste_conectividade(void) {
    printf("🔧 Testando conectividade do display...\n");
    
    if (!ssd1306_init_done) {
        printf("❌ Display não foi inicializado\n");
        return;
    }
    
    // Teste simples de escrita
    if (ssd1306_write_cmd(SSD1306_SET_CONTRAST) && ssd1306_write_cmd(0x7F)) {
        printf("✅ Display respondendo aos comandos\n");
    } else {
        printf("❌ Display não está respondendo\n");
    }
    
    // Teste de buffer
    oled_clear();
    oled_write_string(0, 0, "TESTE");
    oled_write_string(0, 15, "12345");
    
    if (oled_display_buffer()) {
        printf("✅ Buffer sendo enviado corretamente\n");
    } else {
        printf("❌ Erro no envio do buffer\n");
    }
}

void oled_teste_orientacao(void) {
    printf("🔄 Teste de orientacao simplificado\n");
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(0, 0, "TESTE OK");
    oled_write_string(0, 15, "Se legivel");
    oled_write_string(0, 30, "funciona!");
    oled_display_buffer();
}

void oled_set_orientacao_manual(int orientacao) {
    printf("🔄 Orientacao: %d\n", orientacao);
    
    if (!ssd1306_init_done) return;
    
    oled_clear();
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Orient: %d", orientacao);
    oled_write_string(0, 0, buffer);
    oled_write_string(0, 15, "Teste ABC 123");
    oled_display_buffer();
}

// Novas funções de teste visual para diagnóstico
void oled_teste_tela_branca(void) {
    if (!ssd1306_init_done) {
        printf("❌ Display não inicializado para teste branco\n");
        return;
    }
    
    printf("🎨 Preenchendo tela com pixels brancos...\n");
    
    // Preenche todo o buffer com 0xFF (todos os pixels ligados)
    memset(ssd1306_buffer, 0xFF, sizeof(ssd1306_buffer));
    
    if (oled_display_buffer()) {
        printf("✅ Tela branca enviada - deve estar completamente acesa\n");
    } else {
        printf("❌ Erro ao enviar tela branca\n");
    }
}

void oled_teste_padrao_xadrez(void) {
    if (!ssd1306_init_done) {
        printf("❌ Display não inicializado para teste xadrez\n");
        return;
    }
    
    printf("🎨 Criando padrão xadrez...\n");
    
    oled_clear();
    
    // Padrão xadrez - quadrados 8x8
    for (int y = 0; y < SSD1306_HEIGHT; y += 8) {
        for (int x = 0; x < SSD1306_WIDTH; x += 8) {
            // Alterna entre quadrados cheios e vazios
            bool fill = ((x/8 + y/8) % 2) == 0;
            
            if (fill) {
                for (int dy = 0; dy < 8 && (y + dy) < SSD1306_HEIGHT; dy++) {
                    for (int dx = 0; dx < 8 && (x + dx) < SSD1306_WIDTH; dx++) {
                        oled_set_pixel(x + dx, y + dy, true);
                    }
                }
            }
        }
    }
    
    if (oled_display_buffer()) {
        printf("✅ Padrão xadrez enviado - deve mostrar quadrados alternados\n");
    } else {
        printf("❌ Erro ao enviar padrão xadrez\n");
    }
}

void oled_teste_texto_grande(void) {
    if (!ssd1306_init_done) {
        printf("❌ Display não inicializado para teste texto\n");
        return;
    }
    
    printf("🎨 Escrevendo texto grande...\n");
    
    oled_clear();
    
    // Texto em várias posições
    oled_write_string(0, 0, "TESTE");
    oled_write_string(0, 16, "DISPLAY");  
    oled_write_string(0, 32, "OLED");
    oled_write_string(0, 48, "123456");
    
    // Desenha bordas para referência
    for (int x = 0; x < SSD1306_WIDTH; x++) {
        oled_set_pixel(x, 0, true);
        oled_set_pixel(x, SSD1306_HEIGHT - 1, true);
    }
    for (int y = 0; y < SSD1306_HEIGHT; y++) {
        oled_set_pixel(0, y, true);
        oled_set_pixel(SSD1306_WIDTH - 1, y, true);
    }
    
    if (oled_display_buffer()) {
        printf("✅ Texto grande enviado - deve mostrar 'TESTE DISPLAY OLED 123456'\n");
    } else {
        printf("❌ Erro ao enviar texto grande\n");
    }
}

// Função de teste de contraste
void oled_teste_contraste(void) {
    if (!ssd1306_init_done) {
        printf("❌ Display não inicializado para teste contraste\n");
        return;
    }
    
    printf("🎨 Testando diferentes níveis de contraste...\n");
    
    // Testa vários níveis de contraste
    uint8_t contrastes[] = {0x00, 0x7F, 0xFF, 0x40, 0xCF};
    const char* nomes[] = {"MIN", "MED", "MAX", "BAI", "ALT"};
    
    for (int i = 0; i < 5; i++) {
        printf("   Contraste %s (0x%02X)...\n", nomes[i], contrastes[i]);
        
        // Define contraste
        ssd1306_write_cmd(SSD1306_SET_CONTRAST);
        ssd1306_write_cmd(contrastes[i]);
        
        // Mostra texto de teste
        oled_clear();
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "CONT:%s", nomes[i]);
        oled_write_string(20, 20, buffer);
        oled_write_string(10, 35, "Se ve isto?");
        
        oled_display_buffer();
        sleep_ms(2000);
    }
    
    // Volta ao contraste padrão
    ssd1306_write_cmd(SSD1306_SET_CONTRAST);
    ssd1306_write_cmd(0xFF);
    
    printf("✅ Teste de contraste concluído\n");
}

// Função de teste de inversão
void oled_teste_inversao(void) {
    printf("==============================================\n\n");
}

// Função de inicialização ULTRA BÁSICA - último recurso
bool oled_init_ultra_basic(void) {
    printf("🔥 MÉTODO ULTRA BÁSICO - Último recurso!\n");
    
    // Configuração I2C mais simples possível
    i2c_init(i2c0, 100000);  // 100kHz - velocidade mais lenta e confiável
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    
    printf("📡 I2C BÁSICO: 100kHz, SDA=%d, SCL=%d\n", I2C_OLED_SDA, I2C_OLED_SCL);
    
    // Aguarda MUITO mais tempo
    sleep_ms(500);
    
    // Detecta no endereço mais comum
    ssd1306_detected_addr = 0x3C;
    
    printf("🧪 Testando comunicação básica...\n");
    
    // Comando mais simples possível - só desliga display
    uint8_t cmd_off[] = {0x00, 0xAE};  // Control byte + Display OFF
    int ret = i2c_write_blocking(i2c0, 0x3C, cmd_off, 2, false);
    printf("   Display OFF: %s\n", ret >= 0 ? "✅ OK" : "❌ FALHOU");
    
    if (ret < 0) {
        printf("❌ Nem comando básico funciona!\n");
        return false;
    }
    
    sleep_ms(100);
    
    // Sequência MÍNIMA de comandos (apenas o essencial)
    uint8_t init_cmds[] = {
        0x00, 0xAE,  // Display OFF
        0x00, 0x20, 0x00, 0x00,  // Memory addressing mode: horizontal
        0x00, 0x8D, 0x00, 0x14,  // Charge pump ON
        0x00, 0xAF   // Display ON
    };
    
    printf("⚙️ Enviando comandos mínimos...\n");
    ret = i2c_write_blocking(i2c0, 0x3C, init_cmds, sizeof(init_cmds), false);
    printf("   Comandos básicos: %s\n", ret >= 0 ? "✅ OK" : "❌ FALHOU");
    
    if (ret < 0) {
        printf("❌ Comandos básicos falharam!\n");
        return false;
    }
    
    sleep_ms(100);
    
    // Teste EXTREMAMENTE simples - só pixels brancos
    printf("🎨 Teste de pixels extremamente simples...\n");
    
    // Buffer mínimo - só 1024 bytes de 0xFF (tela toda branca)
    uint8_t simple_buffer[1025];  // +1 para control byte
    simple_buffer[0] = 0x40;      // Data mode
    memset(&simple_buffer[1], 0xFF, 1024);  // Todos pixels ligados
    
    ret = i2c_write_blocking(i2c0, 0x3C, simple_buffer, sizeof(simple_buffer), false);
    printf("   Tela branca: %s\n", ret >= 0 ? "✅ ENVIADO" : "❌ FALHOU");
    
    if (ret >= 0) {
        printf("🎉 TESTE BÁSICO CONCLUÍDO!\n");
        printf("   Se o display não acender agora, é problema físico!\n");
        ssd1306_init_done = true;
        return true;
    }
    
    printf("❌ Mesmo teste ultra básico falhou!\n");
    return false;
}

// Função para testar diferentes endereços I2C
void oled_scan_all_addresses(void) {
    printf("🔍 SCAN COMPLETO DE ENDEREÇOS I2C\n");
    printf("==================================\n");
    
    // Configuração I2C super básica
    i2c_init(i2c0, 100000);
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    sleep_ms(200);
    
    printf("Testando todos os endereços de 0x08 a 0x77...\n");
    
    int devices_found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        uint8_t dummy;
        int ret = i2c_read_blocking(i2c0, addr, &dummy, 1, false);
        
        if (ret >= 0) {
            printf("   0x%02X: ✅ DISPOSITIVO ENCONTRADO!\n", addr);
            devices_found++;
            
            // Se encontrou 0x3C ou 0x3D, testa como SSD1306
            if (addr == 0x3C || addr == 0x3D) {
                printf("      └─ Pode ser SSD1306! Testando...\n");
                
                // Teste básico de comando
                uint8_t test_cmd[] = {0x00, 0xAE};  // Display OFF
                int cmd_ret = i2c_write_blocking(i2c0, addr, test_cmd, 2, false);
                printf("         Comando teste: %s\n", 
                       cmd_ret >= 0 ? "✅ RESPONDE" : "❌ NÃO RESPONDE");
            }
        }
    }
    
    printf("\n📊 Resultado do scan:\n");
    printf("   Total de dispositivos: %d\n", devices_found);
    
    if (devices_found == 0) {
        printf("❌ NENHUM dispositivo I2C encontrado!\n");
        printf("🔧 PROBLEMA DE HARDWARE CONFIRMADO:\n");
        printf("   - Verifique conexões SDA/SCL\n");
        printf("   - Verifique alimentação 3.3V\n");
        printf("   - Verifique GND comum\n");
        printf("   - Teste com outro display\n");
    } else {
        printf("✅ Dispositivos I2C funcionais encontrados\n");
    }
    
    printf("==================================\n");
}

// Função para testar diferentes velocidades I2C
void oled_test_i2c_speeds(void) {
    printf("⚡ TESTE DE VELOCIDADES I2C\n");
    printf("============================\n");
    
    uint32_t speeds[] = {50000, 100000, 200000, 400000};  // 50kHz a 400kHz
    const char* speed_names[] = {"50kHz", "100kHz", "200kHz", "400kHz"};
    
    for (int i = 0; i < 4; i++) {
        printf("🧪 Testando %s...\n", speed_names[i]);
        
        // Reconfigura I2C com nova velocidade
        i2c_deinit(i2c0);
        sleep_ms(50);
        
        i2c_init(i2c0, speeds[i]);
        gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
        gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
        gpio_pull_up(I2C_OLED_SDA);
        gpio_pull_up(I2C_OLED_SCL);
        sleep_ms(100);
        
        // Teste simples no endereço 0x3C
        uint8_t dummy;
        int ret = i2c_read_blocking(i2c0, 0x3C, &dummy, 1, false);
        
        printf("   %s: %s\n", speed_names[i], ret >= 0 ? "✅ OK" : "❌ FALHOU");
        
        if (ret >= 0) {
            // Se funcionou, testa comando
            uint8_t cmd[] = {0x00, 0xAE};
            int cmd_ret = i2c_write_blocking(i2c0, 0x3C, cmd, 2, false);
            printf("      Comando: %s\n", cmd_ret >= 0 ? "✅ OK" : "❌ FALHOU");
        }
    }
    
    printf("============================\n");
}

// Função de inicialização SIMPLIFICADA E CORRIGIDA
bool oled_init_simplificado_corrigido(void) {
    printf("🔧 === INICIALIZAÇÃO SIMPLIFICADA E CORRIGIDA ===\n");
    
    // CORREÇÃO CRÍTICA: GP14/GP15 devem usar i2c1, não i2c0
    printf("📡 Configurando I2C1 para GP14/GP15...\n");
    
    // Inicializa I2C1 com velocidade baixa e confiável
    i2c_init(i2c1, 100000);  // 100kHz - mais confiável
    
    // Configura os pinos corretos
    gpio_set_function(14, GPIO_FUNC_I2C);  // GP14 = SDA
    gpio_set_function(15, GPIO_FUNC_I2C);  // GP15 = SCL
    gpio_pull_up(14);
    gpio_pull_up(15);
    
    printf("✅ I2C1 configurado: GP14=SDA, GP15=SCL, 100kHz\n");
    
    // Aguarda estabilização
    sleep_ms(500);
    
    // Teste de comunicação básica
    printf("🔍 Testando comunicação no endereço 0x3C...\n");
    
    uint8_t dummy;
    int ret = i2c_read_blocking(i2c1, 0x3C, &dummy, 1, false);
    
    if (ret >= 0) {
        printf("✅ Dispositivo encontrado em 0x3C!\n");
        ssd1306_detected_addr = 0x3C;
    } else {
        printf("❌ Dispositivo não encontrado em 0x3C (ret=%d)\n", ret);
        
        // Testa 0x3D
        ret = i2c_read_blocking(i2c1, 0x3D, &dummy, 1, false);
        if (ret >= 0) {
            printf("✅ Dispositivo encontrado em 0x3D!\n");
            ssd1306_detected_addr = 0x3D;
        } else {
            printf("❌ Dispositivo não encontrado em 0x3D (ret=%d)\n", ret);
            return false;
        }
    }
    
    // Sequência de inicialização MÍNIMA e CONFIÁVEL
    printf("⚙️ Enviando comandos de inicialização mínimos...\n");
    
    // Array de comandos essenciais
    uint8_t init_sequence[] = {
        0x00, 0xAE,        // Display OFF
        0x00, 0x20,        // Set Memory Addressing Mode
        0x00, 0x00,        // Horizontal Addressing Mode
        0x00, 0x8D,        // Charge Pump Setting
        0x00, 0x14,        // Enable Charge Pump
        0x00, 0x81,        // Set Contrast Control
        0x00, 0x7F,        // Contrast = 127 (médio)
        0x00, 0xA1,        // Set Segment Re-map (A1h)
        0x00, 0xC8,        // Set COM Output Scan Direction
        0x00, 0xAF         // Display ON
    };
    
    ret = i2c_write_blocking(i2c1, ssd1306_detected_addr, init_sequence, sizeof(init_sequence), false);
    
    if (ret < 0) {
        printf("❌ Falha ao enviar comandos de inicialização (ret=%d)\n", ret);
        return false;
    }
    
    printf("✅ Comandos de inicialização enviados com sucesso!\n");
    
    sleep_ms(100);
    
    // Teste SUPER SIMPLES - tela toda branca
    printf("🎨 Teste visual: tela toda branca...\n");
    
    // Prepara buffer com todos os pixels ligados
    uint8_t white_buffer[1025];  // 1024 + 1 control byte
    white_buffer[0] = 0x40;      // Data mode
    memset(&white_buffer[1], 0xFF, 1024);  // Todos os pixels brancos
    
    ret = i2c_write_blocking(i2c1, ssd1306_detected_addr, white_buffer, sizeof(white_buffer), false);
    
    if (ret < 0) {
        printf("❌ Falha ao enviar buffer de pixels (ret=%d)\n", ret);
        return false;
    }
    
    printf("✅ Buffer de pixels enviado com sucesso!\n");
    printf("📺 O display deve estar COMPLETAMENTE BRANCO agora!\n");
    
    ssd1306_init_done = true;
    return true;
}

// Função para scan I2C em i2c1
void oled_scan_i2c1_addresses(void) {
    printf("🔍 === SCAN COMPLETO I2C1 (GP14/GP15) ===\n");
    
    // Configura I2C1 com velocidade baixa
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    sleep_ms(200);
    
    printf("Testando endereços de 0x08 a 0x77...\n");
    
    int devices_found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        uint8_t dummy;
        int ret = i2c_read_blocking(i2c1, addr, &dummy, 1, false);
        
        if (ret >= 0) {
            printf("   0x%02X: ✅ DISPOSITIVO ENCONTRADO!\n", addr);
            devices_found++;
        }
    }
    
    printf("\n📊 Total de dispositivos I2C1: %d\n", devices_found);
    
    if (devices_found == 0) {
        printf("❌ NENHUM dispositivo I2C1 encontrado!\n");
        printf("🔧 Possíveis problemas:\n");
        printf("   - Display não conectado em GP14/GP15\n");
        printf("   - Alimentação VCC não conectada (3.3V)\n");
        printf("   - GND não conectado\n");
        printf("   - Display defeituoso\n");
    }
    
    printf("========================================\n");
}