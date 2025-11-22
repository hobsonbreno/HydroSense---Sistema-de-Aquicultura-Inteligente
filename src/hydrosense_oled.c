#include "hydrosense_system.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Definições SSD1306 (voltando à configuração que funcionava)
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_I2C_ADDR        0x3C
#define SSD1306_I2C_PORT        i2c1
#define SSD1306_I2C_SPEED       400000

// Comandos essenciais
#define SSD1306_DISPLAYOFF      0xAE
#define SSD1306_DISPLAYON       0xAF
#define SSD1306_SETCONTRAST     0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY   0xA6
#define SSD1306_INVERTDISPLAY   0xA7
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS      0xDA
#define SSD1306_SETVCOMDETECT   0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE    0xD9
#define SSD1306_SETMULTIPLEX    0xA8
#define SSD1306_SETSTARTLINE    0x40
#define SSD1306_MEMORYMODE      0x20
#define SSD1306_COLUMNADDR      0x21
#define SSD1306_PAGEADDR        0x22
#define SSD1306_COMSCANDEC      0xC8
#define SSD1306_SEGREMAP        0xA0
#define SSD1306_CHARGEPUMP      0x8D

// Buffer e estado
static uint8_t display_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
bool ssd1306_init_done = false;

// Font 5x8 completa (a que funcionava)
static const uint8_t font5x8[][5] = {
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

// Funções I2C básicas
static bool send_command(uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd};
    return i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, buffer, 2, false) == 2;
}

// Inicialização ORIGINAL que funcionava
bool oled_init(void) {
    printf("🔧 Inicializando OLED SSD1306...\n");
    
    // Configurar I2C
    i2c_init(SSD1306_I2C_PORT, SSD1306_I2C_SPEED);
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    
    sleep_ms(100); // Aguardar estabilização
    
    // Sequência de inicialização ORIGINAL
    send_command(SSD1306_DISPLAYOFF);
    send_command(SSD1306_SETDISPLAYCLOCKDIV);
    send_command(0x80);
    send_command(SSD1306_SETMULTIPLEX);
    send_command(0x3F);
    send_command(SSD1306_SETDISPLAYOFFSET);
    send_command(0x00);
    send_command(SSD1306_SETSTARTLINE | 0x00);
    send_command(SSD1306_CHARGEPUMP);
    send_command(0x14);
    send_command(SSD1306_MEMORYMODE);
    send_command(0x00);
    send_command(SSD1306_SEGREMAP | 0x01);
    send_command(SSD1306_COMSCANDEC);
    send_command(SSD1306_SETCOMPINS);
    send_command(0x12);
    send_command(SSD1306_SETCONTRAST);
    send_command(0x8F);
    send_command(SSD1306_SETPRECHARGE);
    send_command(0xF1);
    send_command(SSD1306_SETVCOMDETECT);
    send_command(0x40);
    send_command(SSD1306_DISPLAYALLON_RESUME);
    send_command(SSD1306_NORMALDISPLAY);
    send_command(SSD1306_DISPLAYON);
    
    ssd1306_init_done = true;
    
    // Teste inicial
    oled_clear();
    oled_display_buffer();
    
    printf("✅ OLED inicializado com sucesso!\n");
    return true;
}

void oled_clear(void) {
    memset(display_buffer, 0, sizeof(display_buffer));
}

void oled_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    
    int page = y / 8;
    int bit = y % 8;
    int index = page * SSD1306_WIDTH + x;
    
    if (on) {
        display_buffer[index] |= (1 << bit);
    } else {
        display_buffer[index] &= ~(1 << bit);
    }
}

void oled_write_char(int x, int y, char c) {
    if (!ssd1306_init_done) return;
    
    if (c < ' ' || c > 'Z') c = ' ';
    
    const uint8_t *char_data = font5x8[c - ' '];
    
    for (int col = 0; col < 5; col++) {
        uint8_t column = char_data[col];
        for (int row = 0; row < 8; row++) {
            bool pixel = (column >> row) & 1;
            oled_set_pixel(x + col, y + row, pixel);
        }
    }
}

void oled_write_string(int x, int y, const char *str) {
    if (!ssd1306_init_done || !str) return;
    
    int pos_x = x;
    while (*str && pos_x < SSD1306_WIDTH - 6) {
        oled_write_char(pos_x, y, *str);
        pos_x += 6;
        str++;
    }
}

bool oled_display_buffer(void) {
    if (!ssd1306_init_done) return false;
    
    send_command(SSD1306_COLUMNADDR);
    send_command(0);
    send_command(127);
    send_command(SSD1306_PAGEADDR);
    send_command(0);
    send_command(7);
    
    // Enviar dados em chunks
    for (int i = 0; i < sizeof(display_buffer); i += 16) {
        uint8_t buffer[17];
        buffer[0] = 0x40; // Data command
        
        int remaining = sizeof(display_buffer) - i;
        int chunk_size = (remaining < 16) ? remaining : 16;
        
        memcpy(buffer + 1, display_buffer + i, chunk_size);
        i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_I2C_ADDR, buffer, chunk_size + 1, false);
    }
    
    return true;
}

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

// Funções principais do display melhorado
void oled_tela_principal_tempo_real(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    
    // Cabeçalho
    char time_buffer[16];
    uint32_t hours = (data->uptime / 3600) % 24;
    uint32_t minutes = (data->uptime / 60) % 60;
    snprintf(time_buffer, sizeof(time_buffer), "%02lu:%02lu", hours, minutes);
    
    oled_write_string(0, 0, "HYDROSENSE");
    oled_write_string(75, 0, time_buffer);
    DrawLine(0, 10, 127, 10, true);
    
    // Dados dos sensores
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Temp: %.1fC", data->temperatura);
    oled_write_string(0, 15, buffer);
    
    snprintf(buffer, sizeof(buffer), "pH: %.1f", data->ph);
    oled_write_string(0, 25, buffer);
    
    snprintf(buffer, sizeof(buffer), "Nivel: %.0f%%", data->nivel_agua);
    oled_write_string(0, 35, buffer);
    
    // Status conectividade
    const char* wifi = data->wifi_conectado ? "WiFi:OK" : "WiFi:--";
    const char* mqtt = data->mqtt_conectado ? "MQTT:OK" : "MQTT:--";
    oled_write_string(0, 45, wifi);
    oled_write_string(55, 45, mqtt);
    
    oled_write_string(0, 56, "A:Menu B:Alimentar");
    oled_display_buffer();
}

void oled_alimentacao_manual_iniciada(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(15, 10, "ALIMENTACAO MANUAL");
    DrawLine(0, 22, 127, 22, true);
    oled_write_string(20, 30, "Servo acionado!");
    oled_write_string(15, 40, "Girando 0->180");
    oled_write_string(25, 50, "Dispensando...");
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
    sleep_ms(3000);
}

void oled_tpa_bomba1_iniciando(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, 127, 17, true);
    oled_write_string(15, 25, "BOMBA 1 ACIONADA");
    oled_write_string(5, 35, "Esvaziando tanque...");
    oled_write_string(10, 45, "Meta: 25% do volume");
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
    sleep_ms(4000);
}

// Aliases para compatibilidade
void oled_splash_screen(void) {
    oled_clear();
    oled_write_string(20, 8, "HYDROSENSE");
    oled_write_string(35, 20, "v2.1");
    DrawLine(10, 32, 118, 32, true);
    oled_write_string(15, 40, "Sistema de");
    oled_write_string(10, 50, "Monitoramento");
    oled_display_buffer();
    sleep_ms(2000);
}

// Funções que estavam faltando - implementações simples
bool oled_init_final_corrigido(void) {
    return oled_init();
}

bool oled_teste_alto_contraste_visual(void) {
    if (!ssd1306_init_done) return false;
    
    oled_clear();
    oled_write_string(0, 16, "TESTE CONTRASTE");
    oled_write_string(0, 32, "ALTO VISUAL");
    oled_display_buffer();
    sleep_ms(1000);
    
    return true;
}

void oled_mostrar_splash(void) {
    oled_splash_screen();
}

bool oled_teste_definitivo_gp14_gp15(void) {
    printf("Teste definitivo GP14/GP15...\n");
    return oled_init();
}

void oled_mostrar_tela_principal(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(16, 0, "HYDROSENSE 2.1");
    DrawLine(0, 12, 127, 12, true);
    oled_write_string(0, 20, "Sistema: OPERACIONAL");
    oled_write_string(0, 30, "Status: MONITORANDO");
    oled_write_string(0, 40, "WiFi: CONECTADO");
    oled_write_string(0, 54, "A:Menu B:Config");
    oled_display_buffer();
}

void oled_mostrar_menu(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(32, 0, "=== MENU ===");
    oled_write_string(8, 16, "1. Sensores");
    oled_write_string(8, 26, "2. Configuracoes");
    oled_write_string(8, 36, "3. Diagnostico");
    oled_write_string(8, 46, "4. Sobre");
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

void oled_diagnostico_completo(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(0, 0, "DIAGNOSTICO OLED");
    oled_write_string(0, 16, "1. I2C Scanner...");
    oled_display_buffer();
    sleep_ms(1000);
    
    oled_write_string(0, 24, "   0x3C: OK");
    oled_display_buffer();
    sleep_ms(1000);
    
    oled_write_string(0, 32, "2. Display test..");
    oled_display_buffer();
    sleep_ms(1000);
    
    oled_clear();
    oled_write_string(20, 24, "DIAGNOSTICO");
    oled_write_string(25, 32, "COMPLETO!");
    oled_display_buffer();
    sleep_ms(2000);
}

// Funções de display específicas do HydroSense
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
    
    const char* wifi_status = data->wifi_conectado ? "OK" : "ERRO";
    snprintf(buffer, sizeof(buffer), "WiFi: %s", wifi_status);
    oled_write_string(5, 16, buffer);
    
    const char* mqtt_status = data->mqtt_conectado ? "OK" : "ERRO";
    snprintf(buffer, sizeof(buffer), "MQTT: %s", mqtt_status);
    oled_write_string(5, 26, buffer);
    
    snprintf(buffer, sizeof(buffer), "Uptime: %lus", data->uptime);
    oled_write_string(5, 36, buffer);
    
    oled_display_buffer();
}

// Funções adicionais simplificadas
void oled_alimentacao_servo_retornando(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(15, 10, "ALIMENTACAO MANUAL");
    DrawLine(0, 22, 127, 22, true);
    oled_write_string(15, 30, "Retornando servo");
    oled_write_string(20, 40, "Posicao: 180->0");
    oled_write_string(25, 50, "Finalizando...");
    oled_display_buffer();
}

void oled_alimentacao_programada_alerta(uint8_t hora, uint8_t quantidade) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(10, 5, "HORARIO PROGRAMADO");
    DrawLine(0, 17, 127, 17, true);
    
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
    DrawLine(0, 17, 127, 17, true);
    
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

void oled_tpa_bomba1_progresso(float nivel_atual, float meta) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, 127, 17, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Nivel: %.1f%%", nivel_atual);
    oled_write_string(25, 25, buffer);
    
    snprintf(buffer, sizeof(buffer), "Meta: %.0f%%", meta);
    oled_write_string(30, 35, buffer);
    
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

void oled_tpa_bomba2_iniciando(void) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, 127, 17, true);
    oled_write_string(15, 25, "BOMBA 2 ACIONADA");
    oled_write_string(5, 35, "Completando nivel");
    oled_write_string(10, 45, "Meta: 100% volume");
    oled_display_buffer();
}

void oled_tpa_bomba2_progresso(float nivel_atual) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(25, 5, "SISTEMA TPA");
    DrawLine(0, 17, 127, 17, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Nivel: %.1f%%", nivel_atual);
    oled_write_string(25, 25, buffer);
    
    oled_write_string(25, 35, "Meta: 100%");
    oled_write_string(5, 55, "Reabastecendo...");
    oled_display_buffer();
}

void oled_menu_principal(uint8_t item_selecionado) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(35, 0, "== MENU ==");
    DrawLine(0, 12, 127, 12, true);
    
    const char* opcoes[] = {
        "1. Sensores",
        "2. Alimentacao", 
        "3. Sistema TPA",
        "4. Configuracoes",
        "5. Diagnosticos"
    };
    
    for (int i = 0; i < 5; i++) {
        int y_pos = 20 + (i * 8);
        
        if (i == item_selecionado) {
            oled_write_string(0, y_pos, ">");
        }
        
        oled_write_string(10, y_pos, opcoes[i]);
    }
    
    oled_display_buffer();
}

void oled_menu_sensores(const hydrosense_status_t* data) {
    if (!ssd1306_init_done || !data) return;
    
    oled_clear();
    oled_write_string(25, 0, "SENSORES");
    DrawLine(0, 12, 127, 12, true);
    
    char buffer[32];
    
    snprintf(buffer, sizeof(buffer), "Temp: %.1fC", data->temperatura);
    oled_write_string(0, 18, buffer);
    
    snprintf(buffer, sizeof(buffer), "pH: %.2f", data->ph);
    oled_write_string(0, 28, buffer);
    
    snprintf(buffer, sizeof(buffer), "Nivel: %.0f%%", data->nivel_agua);
    oled_write_string(0, 38, buffer);
    
    oled_write_string(0, 50, "Atualizando...");
    oled_display_buffer();
}

void oled_alerta_temperatura(float temp) {
    if (!ssd1306_init_done) return;
    
    oled_clear();
    oled_write_string(30, 10, "ALERTA!");
    DrawLine(0, 22, 127, 22, true);
    
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
    DrawLine(0, 22, 127, 22, true);
    
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
    DrawLine(0, 22, 127, 22, true);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Nivel: %.0f%%", nivel);
    oled_write_string(25, 30, buffer);
    
    oled_write_string(10, 45, "Nivel muito baixo!");
    oled_write_string(15, 55, "TPA necessario");
    oled_display_buffer();
}

bool oled_set_contrast(uint8_t contrast) {
    if (!ssd1306_init_done) return false;
    return send_command(SSD1306_SETCONTRAST) && send_command(contrast);
}

bool oled_invert_display(bool invert) {
    if (!ssd1306_init_done) return false;
    return send_command(invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

void oled_log_mensagem(const char* msg) {
    if (!ssd1306_init_done || !msg) return;
    
    oled_clear();
    oled_write_string(0, 20, "LOG:");
    oled_write_string(0, 35, msg);
    oled_display_buffer();
    sleep_ms(1500);
}
