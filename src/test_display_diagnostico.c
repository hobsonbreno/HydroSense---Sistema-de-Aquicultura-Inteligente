#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Definições do SSD1306
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_I2C_ADDR 0x3C

// Comandos SSD1306
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

// Buffer do display
static uint8_t display_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Função para enviar comando
bool send_command(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    int result = i2c_write_blocking(i2c1, SSD1306_I2C_ADDR, buf, 2, false);
    return result == 2;
}

// Função para enviar dados
bool send_data(uint8_t *data, size_t len) {
    uint8_t *temp_buf = malloc(len + 1);
    if (!temp_buf) return false;
    
    temp_buf[0] = 0x40; // Data mode
    memcpy(temp_buf + 1, data, len);
    
    int result = i2c_write_blocking(i2c1, SSD1306_I2C_ADDR, temp_buf, len + 1, false);
    free(temp_buf);
    return result == (len + 1);
}

// Função para limpar buffer
void clear_buffer() {
    memset(display_buffer, 0, sizeof(display_buffer));
}

// Função para definir pixel
void set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    
    int page = y / 8;
    int bit = y % 8;
    int index = x + page * SSD1306_WIDTH;
    
    if (on) {
        display_buffer[index] |= (1 << bit);
    } else {
        display_buffer[index] &= ~(1 << bit);
    }
}

// Função para atualizar display
bool update_display() {
    // Configura área de exibição
    if (!send_command(SSD1306_COLUMNADDR)) return false;
    if (!send_command(0)) return false;
    if (!send_command(SSD1306_WIDTH - 1)) return false;
    if (!send_command(SSD1306_PAGEADDR)) return false;
    if (!send_command(0)) return false;
    if (!send_command((SSD1306_HEIGHT / 8) - 1)) return false;
    
    // Envia buffer
    return send_data(display_buffer, sizeof(display_buffer));
}

// Font simples 5x7
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
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
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // 'F'
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
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
};

// Função para escrever caractere
void write_char(int x, int y, char c) {
    if (c < ' ' || c > 'Z') c = ' ';
    int char_index = c - ' ';
    
    const uint8_t *char_data = font5x7[char_index];
    
    for (int col = 0; col < 5; col++) {
        uint8_t line = char_data[col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                set_pixel(x + col, y + row, true);
            }
        }
    }
}

// Função para escrever string
void write_string(int x, int y, const char *str) {
    int pos_x = x;
    while (*str && pos_x < SSD1306_WIDTH - 6) {
        write_char(pos_x, y, *str);
        pos_x += 6;
        str++;
    }
}

// Inicialização básica do display
bool init_display_basic() {
    printf("🔧 Inicializando display SSD1306 (método básico)...\n");
    
    // Configuração I2C1 para GP14/GP15
    i2c_init(i2c1, 100000); // 100kHz para máxima compatibilidade
    gpio_set_function(14, GPIO_FUNC_I2C); // GP14 = SDA
    gpio_set_function(15, GPIO_FUNC_I2C); // GP15 = SCL
    gpio_pull_up(14);
    gpio_pull_up(15);
    
    sleep_ms(100);
    
    // Testa comunicação
    uint8_t dummy;
    int ret = i2c_read_blocking(i2c1, SSD1306_I2C_ADDR, &dummy, 1, false);
    if (ret < 0) {
        printf("❌ Display não encontrado no endereço 0x%02X\n", SSD1306_I2C_ADDR);
        return false;
    }
    
    printf("✅ Display detectado!\n");
    
    // Sequência de inicialização
    if (!send_command(SSD1306_DISPLAYOFF)) return false;
    if (!send_command(SSD1306_SETDISPLAYCLOCKDIV)) return false;
    if (!send_command(0x80)) return false;
    if (!send_command(SSD1306_SETMULTIPLEX)) return false;
    if (!send_command(0x3F)) return false;
    if (!send_command(SSD1306_SETDISPLAYOFFSET)) return false;
    if (!send_command(0x00)) return false;
    if (!send_command(SSD1306_SETSTARTLINE | 0x0)) return false;
    if (!send_command(SSD1306_CHARGEPUMP)) return false;
    if (!send_command(0x14)) return false;
    if (!send_command(SSD1306_MEMORYMODE)) return false;
    if (!send_command(0x00)) return false;
    if (!send_command(SSD1306_SEGREMAP | 0x1)) return false;
    if (!send_command(SSD1306_COMSCANDEC)) return false;
    if (!send_command(SSD1306_SETCOMPINS)) return false;
    if (!send_command(0x12)) return false;
    if (!send_command(SSD1306_SETCONTRAST)) return false;
    if (!send_command(0x7F)) return false;
    if (!send_command(SSD1306_SETPRECHARGE)) return false;
    if (!send_command(0xF1)) return false;
    if (!send_command(SSD1306_SETVCOMDETECT)) return false;
    if (!send_command(0x40)) return false;
    if (!send_command(SSD1306_NORMALDISPLAY)) return false;
    if (!send_command(SSD1306_DISPLAYON)) return false;
    
    sleep_ms(100);
    
    printf("✅ Display inicializado!\n");
    return true;
}

// Teste de orientação completo
void test_all_orientations() {
    printf("\n🔄 === TESTE COMPLETO DE ORIENTAÇÕES ===\n");
    
    struct {
        uint8_t seg_remap;
        uint8_t com_scan;
        const char* name;
    } orientations[] = {
        {0xA0, 0xC0, "Normal (A0+C0)"},
        {0xA1, 0xC0, "Espelho H (A1+C0)"},
        {0xA0, 0xC8, "Espelho V (A0+C8)"},
        {0xA1, 0xC8, "Rotacao 180 (A1+C8)"}
    };
    
    for (int i = 0; i < 4; i++) {
        printf("🧪 Teste %d: %s\n", i+1, orientations[i].name);
        
        // Aplica configuração
        send_command(orientations[i].seg_remap);
        send_command(orientations[i].com_scan);
        
        // Cria padrão de teste
        clear_buffer();
        
        // Texto de identificação
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "TESTE %d", i+1);
        write_string(5, 5, buffer);
        write_string(5, 15, orientations[i].name);
        
        // Desenha canto para orientação
        for (int x = 0; x < 20; x++) {
            set_pixel(x, 0, true);
            set_pixel(0, x, true);
        }
        
        // Desenha seta para indicar direção "cima"
        set_pixel(10, 25, true);
        set_pixel(9, 26, true);
        set_pixel(11, 26, true);
        set_pixel(8, 27, true);
        set_pixel(12, 27, true);
        write_string(5, 30, "CIMA");
        
        // Atualiza display
        update_display();
        
        printf("📺 Aguarde 5 segundos para verificar...\n");
        sleep_ms(5000);
    }
    
    printf("✅ Teste de orientações concluído!\n");
    printf("📝 Qual teste mostrou o texto LEGÍVEL na posição correta?\n");
}

// Aplica orientação específica
void apply_orientation(int orientation) {
    printf("🎯 Aplicando orientação %d...\n", orientation);
    
    switch (orientation) {
        case 1: // Normal
            send_command(0xA0);
            send_command(0xC0);
            break;
        case 2: // Espelho horizontal
            send_command(0xA1);
            send_command(0xC0);
            break;
        case 3: // Espelho vertical
            send_command(0xA0);
            send_command(0xC8);
            break;
        case 4: // Rotação 180°
            send_command(0xA1);
            send_command(0xC8);
            break;
        default:
            printf("❌ Orientação inválida! Use 1-4\n");
            return;
    }
    
    // Teste da nova orientação
    clear_buffer();
    write_string(5, 5, "HydroSense 2.1");
    write_string(5, 15, "Sistema IoT");
    write_string(5, 25, "Aquicultura");
    write_string(5, 35, "Display OK!");
    
    // Indicadores de posição
    write_string(5, 50, "TOPO");
    write_string(90, 50, "DIR");
    
    update_display();
    
    printf("✅ Orientação %d aplicada!\n", orientation);
}

// Teste visual extremo
void extreme_visual_test() {
    printf("\n🎨 === TESTE VISUAL EXTREMO ===\n");
    
    // Teste 1: Tela toda preta
    printf("🖤 1. Tela PRETA (3s)...\n");
    memset(display_buffer, 0x00, sizeof(display_buffer));
    update_display();
    sleep_ms(3000);
    
    // Teste 2: Tela toda branca
    printf("🤍 2. Tela BRANCA (3s)...\n");
    memset(display_buffer, 0xFF, sizeof(display_buffer));
    update_display();
    sleep_ms(3000);
    
    // Teste 3: Padrão xadrez
    printf("🏁 3. Padrão XADREZ (3s)...\n");
    clear_buffer();
    for (int y = 0; y < SSD1306_HEIGHT; y++) {
        for (int x = 0; x < SSD1306_WIDTH; x++) {
            if (((x / 8) + (y / 8)) % 2 == 0) {
                set_pixel(x, y, true);
            }
        }
    }
    update_display();
    sleep_ms(3000);
    
    // Teste 4: Bordas e cruz
    printf("➕ 4. BORDAS e CRUZ (3s)...\n");
    clear_buffer();
    
    // Bordas
    for (int x = 0; x < SSD1306_WIDTH; x++) {
        set_pixel(x, 0, true);
        set_pixel(x, SSD1306_HEIGHT - 1, true);
    }
    for (int y = 0; y < SSD1306_HEIGHT; y++) {
        set_pixel(0, y, true);
        set_pixel(SSD1306_WIDTH - 1, y, true);
    }
    
    // Cruz central
    int cx = SSD1306_WIDTH / 2;
    int cy = SSD1306_HEIGHT / 2;
    for (int i = -20; i <= 20; i++) {
        set_pixel(cx + i, cy, true);
        set_pixel(cx, cy + i, true);
    }
    
    update_display();
    sleep_ms(3000);
    
    // Teste 5: Texto grande
    printf("📝 5. TEXTO GRANDE (5s)...\n");
    clear_buffer();
    write_string(10, 5, "HYDROSENSE");
    write_string(15, 15, "DISPLAY");
    write_string(20, 25, "TESTE");
    write_string(25, 35, "2024");
    write_string(5, 50, "ABC123XYZ");
    update_display();
    sleep_ms(5000);
    
    printf("✅ Teste visual extremo concluído!\n");
    printf("📺 Se você NÃO viu nenhuma mudança no display,\n");
    printf("   o problema é de HARDWARE (conexão/alimentação)!\n");
}

// Diagnóstico completo
void full_diagnostic() {
    printf("\n🔍 === DIAGNÓSTICO COMPLETO DO DISPLAY ===\n");
    
    // 1. Teste I2C
    printf("1️⃣ Testando comunicação I2C...\n");
    uint8_t dummy;
    int devices = 0;
    
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        int ret = i2c_read_blocking(i2c1, addr, &dummy, 1, false);
        if (ret >= 0) {
            printf("   📍 Dispositivo encontrado: 0x%02X\n", addr);
            devices++;
        }
    }
    
    printf("   📊 Total de dispositivos I2C: %d\n", devices);
    
    if (devices == 0) {
        printf("   ❌ PROBLEMA: Nenhum dispositivo I2C detectado!\n");
        printf("   🔧 VERIFIQUE:\n");
        printf("      - VCC = 3.3V (NÃO 5V!)\n");
        printf("      - GND conectado\n");
        printf("      - SDA em GP14\n");
        printf("      - SCL em GP15\n");
        return;
    }
    
    // 2. Teste específico SSD1306
    printf("\n2️⃣ Testando SSD1306 especificamente...\n");
    int ret = i2c_read_blocking(i2c1, SSD1306_I2C_ADDR, &dummy, 1, false);
    if (ret < 0) {
        printf("   ❌ SSD1306 não encontrado em 0x%02X\n", SSD1306_I2C_ADDR);
        return;
    } else {
        printf("   ✅ SSD1306 detectado em 0x%02X\n", SSD1306_I2C_ADDR);
    }
    
    // 3. Teste de comandos básicos
    printf("\n3️⃣ Testando comandos básicos...\n");
    if (send_command(SSD1306_DISPLAYOFF)) {
        printf("   ✅ Comando OFF aceito\n");
    } else {
        printf("   ❌ Comando OFF rejeitado\n");
        return;
    }
    
    if (send_command(SSD1306_DISPLAYON)) {
        printf("   ✅ Comando ON aceito\n");
    } else {
        printf("   ❌ Comando ON rejeitado\n");
        return;
    }
    
    printf("\n✅ Diagnóstico básico passou!\n");
    printf("📋 O display está respondendo corretamente.\n");
    printf("🔄 Prosseguindo para testes visuais...\n");
}

// Menu principal
void show_menu() {
    printf("\n🔧 === DIAGNÓSTICO E CORREÇÃO DO DISPLAY OLED ===\n");
    printf("1. Diagnóstico completo\n");
    printf("2. Teste visual extremo\n");
    printf("3. Teste todas as orientações\n");
    printf("4. Aplicar orientação específica (1-4)\n");
    printf("5. Inicializar display novamente\n");
    printf("6. Sair\n");
    printf("Escolha uma opção: ");
}

int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    printf("🚀 === PROGRAMA DE DIAGNÓSTICO DO DISPLAY OLED ===\n");
    printf("📋 Este programa vai diagnosticar e corrigir problemas do display\n");
    
    // Inicialização básica
    if (!init_display_basic()) {
        printf("❌ Falha na inicialização básica do display\n");
        printf("🔧 Execute o diagnóstico completo para mais detalhes\n");
    }
    
    while (true) {
        show_menu();
        
        // Simular entrada do usuário (em um projeto real, você implementaria entrada serial)
        // Para este exemplo, vamos executar automaticamente alguns testes
        
        printf("Executando diagnóstico automático...\n");
        
        full_diagnostic();
        sleep_ms(2000);
        
        extreme_visual_test();
        sleep_ms(2000);
        
        test_all_orientations();
        sleep_ms(2000);
        
        // Aplicar a orientação mais comum (A1+C8)
        printf("Aplicando orientação padrão (4: A1+C8)...\n");
        apply_orientation(4);
        
        printf("\n🎉 Diagnóstico completo executado!\n");
        printf("💡 Se o display ainda não funcionar corretamente:\n");
        printf("   1. Verifique as conexões físicas\n");
        printf("   2. Teste com outro display\n");
        printf("   3. Verifique a alimentação (deve ser 3.3V)\n");
        
        sleep_ms(10000); // Aguarda 10 segundos antes de repetir
    }
    
    return 0;
}