/**
 * HydroSense v11 Beta (Anti-Sifão) - BitDogLab Completo
 * NOVIDADE: Logica Anti-Sifao para TPA (Bomba 1 para em 25%, Bomba 2 espera 23%) 
 * HARDWARE BitDogLab + Pico W:
 * - Sensores I2C: GPIO2(SDA)/GPIO3(SCL) via extensor (i2c1 switching)
 * - OLED SSD1306: GPIO14(SDA)/GPIO15(SCL) direto na BitDogLab (i2c1 switching)
 * - Servo SG90:   GPIO16 (PWM)
 * - LED RGB:      GPIO11(B), GPIO12(R), GPIO13(G)
 * - Buzzer:       GPIO21
 * - Botoes:       GPIO5(A), GPIO6(B)
 * - Reles REAIS:  GPIO17(LN1), GPIO18(LN2), GPIO19(LN3) via IDC
 * - WiFi:         CYW43 integrado (lwip_poll)
 * 
 * APIs HTTP:
 *   GET  /sensors  - JSON com todos os dados
 *   GET  /status   - Status do sistema
 *   POST /relay    - Controlar reles
 *   POST /feed     - Acionar alimentador (servo)
 *   GET  /         - Pagina HTML responsiva
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ============================================================
// CONFIGURACAO WiFi
// ============================================================
#define WIFI_SSID     "HydroSense"
#define WIFI_PASSWORD "Hb12345678"

// ============================================================
// PINOS - BitDogLab
// ============================================================
// I2C Sensores (via extensor)
#define SENSOR_SDA      2
#define SENSOR_SCL      3

// I2C OLED (direto na BitDogLab)
#define OLED_SDA        14
#define OLED_SCL        15

// Servo
#define SERVO_PIN       16

// LED RGB
#define LED_R_PIN       12
#define LED_G_PIN       13
#define LED_B_PIN       11

// Buzzer
#define BUZZER_PIN      21

// Botoes
#define BTN_A_PIN       5
#define BTN_B_PIN       6

// Reles (via conector IDC)
#define RELAY_LN1_PIN   17   // Aerador/Ventilador
#define RELAY_LN2_PIN   18   // Aquecedor
#define RELAY_LN3_PIN   19   // Alimentador/Bomba

// ============================================================
// ENDERECOS I2C
// ============================================================
#define AHT10_ADDR      0x38
#define VL53L0X_ADDR    0x29
#define VL53L0X_NEW_ADDR 0x30   // Novo endereco para liberar 0x29 pro TCS34725
#define TCS34725_ADDR   0x29
#define TCS34725_CMD    0x80   // Command bit
#define OLED_ADDR       0x3C

// ============================================================
// CONFIGURACAO DO TANQUE - CALIBRADO COM MEDIDAS REAIS
// Aquario: 50cm x 25cm x 30cm = 37,5 litros
// Sensor VL53L0X: 41cm do fundo (11cm acima do topo do aquario)
// ============================================================
#define SENSOR_DIST_FULL   110   // mm - tanque 100% cheio (agua a 30cm, sensor a 11cm)
#define SENSOR_DIST_EMPTY  410   // mm - tanque vazio (sensor a 41cm do fundo)
#define TANK_CAPACITY_L    37.5f // litros (50x25x30cm)
#define TEMP_THRESHOLD     33.0f  // Ventilador liga apenas em 33°C

// ============================================================
// VARIAVEIS GLOBAIS
// ============================================================
static int current_i2c_mode = 0;  // 0=nenhum, 1=sensores, 2=oled

// Dados dos sensores
static volatile float g_temp = 25.0f;
static volatile float g_hum = 60.0f;
static volatile uint16_t g_dist = SENSOR_DIST_EMPTY;   // Default = tanque vazio
static volatile uint16_t g_dist_raw = SENSOR_DIST_EMPTY;  // Leitura bruta (debug)
static volatile float g_nivel = 0.0f;
static volatile float g_volume = 0.0f;

// Filtro robusto para VL53L0X: MEDIANA + HISTERESE
// Mediana elimina outliers, histerese evita oscilacao
#define VL53_FILTER_SIZE 15
#define VL53_HISTERESE   5   // mm - ignora mudancas menores que 5mm
static uint16_t vl53_buffer[VL53_FILTER_SIZE] = {0};
static int vl53_buffer_idx = 0;
static int vl53_buffer_count = 0;
static uint16_t vl53_last_stable = SENSOR_DIST_EMPTY;

// Cor da agua (TCS34725)
static volatile uint16_t g_cor_r = 0, g_cor_g = 0, g_cor_b = 0, g_cor_c = 0;
static char g_cor_nome[16] = "N/A";

// Status
static volatile bool g_aht_ok = false;
static volatile bool g_vl53_ok = false;
static volatile bool g_tcs_ok = false;
static volatile bool g_oled_ok = false;
static volatile bool g_wifi = false;
static volatile uint32_t g_count = 0;
static char g_ip[20] = "0.0.0.0";

// Reles
static volatile bool g_relay_ln1 = false;
static volatile bool g_relay_ln2 = false;
static volatile bool g_relay_ln3 = false;

// Override manual: quando usuario LIGA ventilador manualmente,
// automacao NAO desliga. Mas automacao SEMPRE pode LIGAR se temp alta.
static volatile bool g_ln1_manual = false;  // true = usuario ligou manualmente

// TPA - Troca Parcial de Agua (state machine)
// Fases: 0=inativo, 1=drenando(bomba1), 2=enchendo(bomba2), 3=monitorando
// TPA - Troca Parcial de Agua (state machine)
// Fases: 0=inativo, 1=drenando(b1), 2=espera_sifao(off), 3=enchendo(b2), 4=monitorando
#define TPA_DRAIN_STOP    25.0f   // Drenar ate 25% (desliga bomba, mas sifao continua)
#define TPA_SIPHON_TARGET 23.0f   // Espera sifao baixar ate 23% antes de ligar bomba 2
#define TPA_SAFETY_LOW    20.0f   // Se cair abaixo disso, liga bomba 2 imediatamente (seguranca)
#define TPA_FILL_TARGET   75.0f   // Encher ate 75%
#define TPA_REFILL_MARGIN  3.0f   // Se cair mais que 3% do target, re-enche

static volatile int g_tpa_phase = 0;
static volatile bool g_tpa_active = false;
static volatile uint32_t g_tpa_monitor_until = 0;
#define TPA_MONITOR_TIME_MS (2 * 60 * 1000)

// ============================================================
// I2C SWITCHING - Alterna entre sensores e OLED
// ============================================================

void i2c_switch_to_sensors(void) {
    if (current_i2c_mode == 1) return;
    
    // Desconecta pinos do OLED
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    
    // Reinicializa I2C para sensores
    i2c_deinit(i2c1);
    i2c_init(i2c1, 100000);  // 100kHz para sensores
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_us(100);
    
    current_i2c_mode = 1;
}

void i2c_switch_to_oled(void) {
    if (current_i2c_mode == 2) return;
    
    // Desconecta pinos dos sensores
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    
    // Reinicializa I2C para OLED
    i2c_deinit(i2c1);
    i2c_init(i2c1, 400000);  // 400kHz para OLED
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_us(100);
    
    current_i2c_mode = 2;
}

// ============================================================
// AHT10 - Sensor de Temperatura e Umidade
// ============================================================

bool aht10_init(void) {
    i2c_switch_to_sensors();
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    int ret = i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false);
    sleep_ms(10);
    return (ret >= 0);
}

bool aht10_read(float *temp, float *hum) {
    i2c_switch_to_sensors();
    
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) return false;
    sleep_ms(80);
    
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) return false;
    if (data[0] & 0x80) return false;  // Ocupado
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    *hum = (float)hum_raw / 1048576.0f * 100.0f;
    *temp = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    return true;
}

// ============================================================
// VL53L0X - Sensor de Distancia (com mudanca de endereco)
// Baseado na implementacao de referencia:
//   github.com/joao-tolomelli/pico-w-drivers/vl53l0x_exemplo
// ============================================================

static uint8_t vl53_addr = VL53L0X_ADDR;  // Endereco atual (muda para NEW_ADDR no init)

static bool vl53_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_write_blocking(i2c1, vl53_addr, buf, 2, false) == 2;
}

static uint8_t vl53_read_reg(uint8_t reg) {
    uint8_t val = 0;
    if (i2c_write_blocking(i2c1, vl53_addr, &reg, 1, true) < 0) return 0;
    if (i2c_read_blocking(i2c1, vl53_addr, &val, 1, false) < 0) return 0;
    return val;
}

bool vl53_init(void) {
    i2c_switch_to_sensors();
    sleep_ms(100);
    
    // === CONFLITO I2C: VL53L0X e TCS34725 ambos em 0x29 ===
    // Estrategia: mudar endereco do VL53L0X primeiro, confirmar depois.
    
    // 1) Tenta no novo endereco (caso ja tenha sido mudado em boot anterior)
    vl53_addr = VL53L0X_NEW_ADDR;  // 0x30
    uint8_t id = vl53_read_reg(0xC0);
    if (id == 0xEE) {
        printf("   VL53L0X ja em 0x%02X (ID=0xEE)\n", vl53_addr);
    } else {
        // 2) VL53L0X ainda em 0x29 — muda endereco "cegamente"
        vl53_addr = VL53L0X_ADDR;  // 0x29
        vl53_write_reg(0x8A, VL53L0X_NEW_ADDR);
        sleep_ms(20);
        
        // 3) Confirma no novo endereco
        vl53_addr = VL53L0X_NEW_ADDR;  // 0x30
        id = vl53_read_reg(0xC0);
        printf("   VL53L0X addr 0x29->0x%02X ID=0x%02X\n", VL53L0X_NEW_ADDR, id);
        if (id != 0xEE) {
            printf("   VL53L0X: falha apos mudanca de endereco\n");
            return false;
        }
    }
    
    // Agora VL53L0X esta em 0x30, TCS34725 fica sozinho em 0x29
    printf("   VL53L0X: inicializado em 0x%02X OK\n", vl53_addr);
    return true;
}

uint16_t vl53_read(void) {
    i2c_switch_to_sensors();
    
    // Inicia medicao unica (single-shot) - metodo simples e comprovado
    // REG_SYSRANGE_START (0x00) = 0x01
    uint8_t cmd[2] = {0x00, 0x01};
    if (i2c_write_blocking(i2c1, vl53_addr, cmd, 2, false) != 2)
        return 0xFFFF;
    
    // Aguarda resultado - poll REG_RESULT_RANGE_STATUS (0x14) bit 0
    for (int i = 0; i < 100; i++) {
        uint8_t reg = 0x14;
        uint8_t status = 0;
        if (i2c_write_blocking(i2c1, vl53_addr, &reg, 1, true) < 0)
            return 0xFFFF;
        if (i2c_read_blocking(i2c1, vl53_addr, &status, 1, false) < 0)
            return 0xFFFF;
        if (status & 0x01) break;  // Medicao pronta
        sleep_ms(5);
        if (i == 99) return 0xFFFF;  // Timeout
    }
    
    // Le 2 bytes da distancia em REG_RESULT_RANGE_MM (0x1E)
    uint8_t reg = 0x1E;
    uint8_t buf[2] = {0, 0};
    if (i2c_write_blocking(i2c1, vl53_addr, &reg, 1, true) < 0)
        return 0xFFFF;
    if (i2c_read_blocking(i2c1, vl53_addr, buf, 2, false) < 0)
        return 0xFFFF;
    
    uint16_t dist = (buf[0] << 8) | buf[1];
    if (dist == 0 || dist > 2000 || dist == 8190) return 0xFFFF;
    return dist;
}

// ============================================================
// TCS34725 - Sensor de Cor RGB
// Baseado na implementacao de referencia:
//   github.com/joao-tolomelli/pico-w-drivers/TCS34725
// ============================================================

static bool tcs_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {(uint8_t)(TCS34725_CMD | reg), val};
    return i2c_write_blocking(i2c1, TCS34725_ADDR, buf, 2, false) == 2;
}

static uint8_t tcs_read_reg(uint8_t reg) {
    uint8_t cmd = TCS34725_CMD | reg;
    uint8_t val = 0;
    i2c_write_blocking(i2c1, TCS34725_ADDR, &cmd, 1, true);
    i2c_read_blocking(i2c1, TCS34725_ADDR, &val, 1, false);
    return val;
}

static uint16_t tcs_read_reg16(uint8_t reg) {
    uint8_t cmd = TCS34725_CMD | reg;
    uint8_t buf[2] = {0, 0};
    i2c_write_blocking(i2c1, TCS34725_ADDR, &cmd, 1, true);
    i2c_read_blocking(i2c1, TCS34725_ADDR, buf, 2, false);
    return (uint16_t)(buf[1] << 8) | buf[0];  // Little-endian (TCS34725)
}

bool tcs_init(void) {
    i2c_switch_to_sensors();
    sleep_ms(50);
    
    // Verifica presenca
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, TCS34725_ADDR, &dummy, 1, false) < 0) {
        printf("   TCS34725: nao encontrado\n");
        return false;
    }
    
    // Verifica ID (0x44 = TCS34725, 0x4D = TCS34727)
    uint8_t id = tcs_read_reg(0x12);
    printf("   TCS34725 ID: 0x%02X\n", id);
    if (id != 0x44 && id != 0x4D) return false;
    
    // Configuracao baseada na referencia (joao-tolomelli)
    // ATIME = 0xC0 -> ~154ms integracao (bom equilibrio velocidade/precisao)
    tcs_write_reg(0x01, 0xC0);
    // Ganho 16x (0x02) - igual a referencia, mais sensivel
    tcs_write_reg(0x0F, 0x02);
    // Power ON (PON)
    tcs_write_reg(0x00, 0x01);
    sleep_ms(3);  // Minimo 2.4ms apos PON
    // Power ON + ADC Enable (PON + AEN)
    tcs_write_reg(0x00, 0x03);
    sleep_ms(160);  // Aguarda primeiro ciclo de integracao (154ms)
    
    printf("   TCS34725: configurado OK (ATIME=0xC0, Gain=16x)\n");
    return true;
}

void tcs_read_color(void) {
    i2c_switch_to_sensors();
    
    // Verifica se dados estao prontos (AVALID bit no STATUS register)
    uint8_t status = tcs_read_reg(0x13);
    if (!(status & 0x01)) return;
    
    // Le valores RGBC (16-bit little-endian)
    g_cor_c = tcs_read_reg16(0x14);  // Clear
    g_cor_r = tcs_read_reg16(0x16);  // Red
    g_cor_g = tcs_read_reg16(0x18);  // Green
    g_cor_b = tcs_read_reg16(0x1A);  // Blue
    
    // Classifica cor para qualidade da agua
    if (g_cor_c < 100) {
        snprintf(g_cor_nome, sizeof(g_cor_nome), "Escuro");
    } else if (g_cor_c > 10000) {
        snprintf(g_cor_nome, sizeof(g_cor_nome), "Cristalino");
    } else {
        // Normaliza RGB para 0-255 (como na referencia)
        uint8_t r8 = (uint8_t)((uint32_t)g_cor_r * 255 / g_cor_c);
        uint8_t g8 = (uint8_t)((uint32_t)g_cor_g * 255 / g_cor_c);
        uint8_t b8 = (uint8_t)((uint32_t)g_cor_b * 255 / g_cor_c);
        if (r8 > 255) r8 = 255;
        if (g8 > 255) g8 = 255;
        if (b8 > 255) b8 = 255;
        
        // Classificacao baseada em valores normalizados
        if (r8 > g8 + 30 && r8 > b8 + 30) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Vermelho");
        } else if (g8 > r8 + 15 && g8 > b8 + 15) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Verde");
        } else if (b8 > r8 + 15 && b8 > g8 + 15) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Azul");
        } else if (r8 > 100 && g8 > 100 && b8 < 80) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Amarelado");
        } else if (r8 > 120 && g8 > 120 && b8 > 120) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Cristalino");
        } else if (g_cor_c > 3000) {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Cristalino");
        } else {
            snprintf(g_cor_nome, sizeof(g_cor_nome), "Turvo");
        }
    }
}

// ============================================================
// OLED SSD1306 128x64
// ============================================================

static uint8_t oled_buffer[1024];

// Font 5x7 (ASCII 32-90)
static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, // Space
    0x00,0x00,0x5F,0x00,0x00, // !
    0x00,0x07,0x00,0x07,0x00, // "
    0x14,0x7F,0x14,0x7F,0x14, // #
    0x24,0x2A,0x7F,0x2A,0x12, // $
    0x23,0x13,0x08,0x64,0x62, // %
    0x36,0x49,0x55,0x22,0x50, // &
    0x00,0x05,0x03,0x00,0x00, // '
    0x00,0x1C,0x22,0x41,0x00, // (
    0x00,0x41,0x22,0x1C,0x00, // )
    0x14,0x08,0x3E,0x08,0x14, // *
    0x08,0x08,0x3E,0x08,0x08, // +
    0x00,0x50,0x30,0x00,0x00, // ,
    0x08,0x08,0x08,0x08,0x08, // -
    0x00,0x60,0x60,0x00,0x00, // .
    0x20,0x10,0x08,0x04,0x02, // /
    0x3E,0x51,0x49,0x45,0x3E, // 0
    0x00,0x42,0x7F,0x40,0x00, // 1
    0x42,0x61,0x51,0x49,0x46, // 2
    0x21,0x41,0x45,0x4B,0x31, // 3
    0x18,0x14,0x12,0x7F,0x10, // 4
    0x27,0x45,0x45,0x45,0x39, // 5
    0x3C,0x4A,0x49,0x49,0x30, // 6
    0x01,0x71,0x09,0x05,0x03, // 7
    0x36,0x49,0x49,0x49,0x36, // 8
    0x06,0x49,0x49,0x29,0x1E, // 9
    0x00,0x36,0x36,0x00,0x00, // :
    0x00,0x56,0x36,0x00,0x00, // ;
    0x08,0x14,0x22,0x41,0x00, // <
    0x14,0x14,0x14,0x14,0x14, // =
    0x00,0x41,0x22,0x14,0x08, // >
    0x02,0x01,0x51,0x09,0x06, // ?
    0x32,0x49,0x79,0x41,0x3E, // @
    0x7E,0x11,0x11,0x11,0x7E, // A
    0x7F,0x49,0x49,0x49,0x36, // B
    0x3E,0x41,0x41,0x41,0x22, // C
    0x7F,0x41,0x41,0x22,0x1C, // D
    0x7F,0x49,0x49,0x49,0x41, // E
    0x7F,0x09,0x09,0x09,0x01, // F
    0x3E,0x41,0x49,0x49,0x7A, // G
    0x7F,0x08,0x08,0x08,0x7F, // H
    0x00,0x41,0x7F,0x41,0x00, // I
    0x20,0x40,0x41,0x3F,0x01, // J
    0x7F,0x08,0x14,0x22,0x41, // K
    0x7F,0x40,0x40,0x40,0x40, // L
    0x7F,0x02,0x0C,0x02,0x7F, // M
    0x7F,0x04,0x08,0x10,0x7F, // N
    0x3E,0x41,0x41,0x41,0x3E, // O
    0x7F,0x09,0x09,0x09,0x06, // P
    0x3E,0x41,0x51,0x21,0x5E, // Q
    0x7F,0x09,0x19,0x29,0x46, // R
    0x46,0x49,0x49,0x49,0x31, // S
    0x01,0x01,0x7F,0x01,0x01, // T
    0x3F,0x40,0x40,0x40,0x3F, // U
    0x1F,0x20,0x40,0x20,0x1F, // V
    0x3F,0x40,0x38,0x40,0x3F, // W
    0x63,0x14,0x08,0x14,0x63, // X
    0x07,0x08,0x70,0x08,0x07, // Y
    0x61,0x51,0x49,0x45,0x43, // Z
};

bool oled_init(void) {
    i2c_switch_to_oled();
    
    // Verifica presenca do OLED
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, OLED_ADDR, &dummy, 1, false) < 0) return false;
    
    uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide
        0xA8, 0x3F, // Multiplex 64
        0xD3, 0x00, // Display offset
        0x40,       // Start line
        0x8D, 0x14, // Charge pump ON
        0x20, 0x00, // Horizontal memory mode
        0xA1,       // Segment remap
        0xC8,       // COM scan descending
        0xDA, 0x12, // COM pins
        0x81, 0xCF, // Contrast
        0xD9, 0xF1, // Pre-charge
        0xDB, 0x40, // VCOMH deselect
        0xA4,       // Display from RAM
        0xA6,       // Normal display
        0xAF        // Display ON
    };
    
    for (int i = 0; i < (int)sizeof(init_cmds); i++) {
        uint8_t buf[2] = {0x00, init_cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    
    memset(oled_buffer, 0, sizeof(oled_buffer));
    return true;
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_update(void) {
    i2c_switch_to_oled();
    
    uint8_t cmds[] = {0x21, 0, 127, 0x22, 0, 7};
    for (int i = 0; i < 6; i++) {
        uint8_t buf[2] = {0x00, cmds[i]};
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
    }
    
    for (int page = 0; page < 8; page++) {
        uint8_t buf[129];
        buf[0] = 0x40;
        memcpy(&buf[1], &oled_buffer[page * 128], 128);
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 129, false);
    }
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int idx = (y / 8) * 128 + x;
    if (on) oled_buffer[idx] |= (1 << (y % 8));
    else    oled_buffer[idx] &= ~(1 << (y % 8));
}

void oled_char(int x, int y, char c) {
    if (c < 32 || c > 90) c = 32;
    int idx = (c - 32) * 5;
    for (int i = 0; i < 5; i++) {
        uint8_t col = font5x7[idx + i];
        for (int j = 0; j < 7; j++) {
            oled_pixel(x + i, y + j, col & (1 << j));
        }
    }
}

void oled_print(int x, int y, const char *str) {
    while (*str) {
        oled_char(x, y, *str);
        x += 6;
        str++;
    }
}

void oled_print_large(int x, int y, const char *str) {
    while (*str) {
        if (*str >= 32 && *str <= 90) {
            int idx = (*str - 32) * 5;
            for (int i = 0; i < 5; i++) {
                uint8_t col = font5x7[idx + i];
                for (int j = 0; j < 7; j++) {
                    bool on = col & (1 << j);
                    oled_pixel(x + i*2, y + j*2, on);
                    oled_pixel(x + i*2 + 1, y + j*2, on);
                    oled_pixel(x + i*2, y + j*2 + 1, on);
                    oled_pixel(x + i*2 + 1, y + j*2 + 1, on);
                }
            }
        }
        x += 12;
        str++;
    }
}

void oled_hline(int y) {
    for (int x = 0; x < 128; x++) oled_pixel(x, y, true);
}

// ============================================================
// SERVO SG90
// ============================================================

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 125.0f);  // 1MHz
    pwm_set_wrap(slice, 20000);      // 50Hz (20ms)
    pwm_set_enabled(slice, true);
}

void servo_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    uint16_t pulse = 500 + (angle * 2000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

void servo_feed(void) {
    printf("[SERVO] Alimentando - rotacao completa...\n");
    
    // Posição inicial
    servo_angle(0);
    sleep_ms(500);
    
    // Primeira meia-volta: 0° -> 180°
    for (int ang = 0; ang <= 180; ang += 5) {
        servo_angle(ang);
        sleep_ms(20);
    }
    sleep_ms(300);
    
    // Volta: 180° -> 0°
    for (int ang = 180; ang >= 0; ang -= 5) {
        servo_angle(ang);
        sleep_ms(20);
    }
    sleep_ms(300);
    
    // Segunda meia-volta: 0° -> 180° (completa ciclo)
    for (int ang = 0; ang <= 180; ang += 5) {
        servo_angle(ang);
        sleep_ms(20);
    }
    sleep_ms(300);
    
    // Retorna à posição inicial: 180° -> 0°
    for (int ang = 180; ang >= 0; ang -= 5) {
        servo_angle(ang);
        sleep_ms(20);
    }
    servo_angle(0);
    sleep_ms(500);
    
    servo_stop();
    printf("[SERVO] Concluido - rotacao completa, voltou para 0 graus\n");
}

// ============================================================
// LED RGB
// ============================================================

void led_init(void) {
    gpio_init(LED_R_PIN); gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN); gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN); gpio_set_dir(LED_B_PIN, GPIO_OUT);
    gpio_put(LED_R_PIN, 0); gpio_put(LED_G_PIN, 0); gpio_put(LED_B_PIN, 0);
}

void led_set(bool r, bool g, bool b) {
    gpio_put(LED_R_PIN, r); gpio_put(LED_G_PIN, g); gpio_put(LED_B_PIN, b);
}

// ============================================================
// BUZZER
// ============================================================

void buzzer_init(void) {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);
}

void buzzer_beep(int ms) {
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(ms);
    gpio_put(BUZZER_PIN, 0);
}

// ============================================================
// RELES (GPIO REAIS via IDC)
// ============================================================

void relay_init(void) {
    gpio_init(RELAY_LN1_PIN); gpio_set_dir(RELAY_LN1_PIN, GPIO_OUT); gpio_put(RELAY_LN1_PIN, 0);
    gpio_init(RELAY_LN2_PIN); gpio_set_dir(RELAY_LN2_PIN, GPIO_OUT); gpio_put(RELAY_LN2_PIN, 0);
    gpio_init(RELAY_LN3_PIN); gpio_set_dir(RELAY_LN3_PIN, GPIO_OUT); gpio_put(RELAY_LN3_PIN, 0);
    // Configura drive strength alto para manter estabilidade sob carga
    gpio_set_drive_strength(RELAY_LN1_PIN, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(RELAY_LN2_PIN, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(RELAY_LN3_PIN, GPIO_DRIVE_STRENGTH_12MA);
}

// Re-afirma TODOS os GPIOs dos reles para o estado desejado em software
// Corrige corrupcao por queda de tensao ao acionar multiplos reles
static void relay_refresh_all(void) {
    gpio_put(RELAY_LN1_PIN, g_relay_ln1);
    gpio_put(RELAY_LN2_PIN, g_relay_ln2);
    gpio_put(RELAY_LN3_PIN, g_relay_ln3);
}

void relay_set(int relay, bool state) {
    // Antes de mudar: desliga TODOS brevemente para evitar pico de corrente
    // simultaneo que corrompe os GPIOs
    bool was_ln1 = g_relay_ln1, was_ln2 = g_relay_ln2, was_ln3 = g_relay_ln3;
    
    // Se estamos LIGANDO um rele e ja tem outro ligado, faz escalonamento
    if (state && (was_ln1 || was_ln2 || was_ln3)) {
        // Desliga todos momentaneamente
        gpio_put(RELAY_LN1_PIN, 0);
        gpio_put(RELAY_LN2_PIN, 0);
        gpio_put(RELAY_LN3_PIN, 0);
        sleep_ms(100);  // Deixa bobinas relaxarem
        
        // Atualiza estado desejado
        switch (relay) {
            case 1: g_relay_ln1 = state; break;
            case 2: g_relay_ln2 = state; break;
            case 3: g_relay_ln3 = state; break;
        }
        
        // Re-liga um por um com delay (escalonamento)
        if (g_relay_ln1) { gpio_put(RELAY_LN1_PIN, 1); sleep_ms(300); }
        if (g_relay_ln2) { gpio_put(RELAY_LN2_PIN, 1); sleep_ms(300); }
        if (g_relay_ln3) { gpio_put(RELAY_LN3_PIN, 1); sleep_ms(300); }
    } else {
        // Caso simples: ligando o primeiro rele, ou desligando
        switch (relay) {
            case 1: g_relay_ln1 = state; break;
            case 2: g_relay_ln2 = state; break;
            case 3: g_relay_ln3 = state; break;
        }
        switch (relay) {
            case 1: gpio_put(RELAY_LN1_PIN, state); break;
            case 2: gpio_put(RELAY_LN2_PIN, state); break;
            case 3: gpio_put(RELAY_LN3_PIN, state); break;
        }
        sleep_ms(100);
        // Re-afirma todos para garantir estabilidade
        relay_refresh_all();
    }
    
    printf("[RELAY] LN%d=%s (GPIO%d) | Estado: LN1=%d LN2=%d LN3=%d\n", 
           relay, state ? "ON" : "OFF",
           relay == 1 ? RELAY_LN1_PIN : relay == 2 ? RELAY_LN2_PIN : RELAY_LN3_PIN,
           g_relay_ln1, g_relay_ln2, g_relay_ln3);
}

// ============================================================
// DISPLAY - Telas
// ============================================================

void display_boot(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print_large(10, 5, "HYDRO");
    oled_print_large(10, 25, "SENSE");
    oled_print(15, 50, "BETA v11.0...");
    oled_update();
}

void display_wifi_connecting(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print(0, 0, "CONECTANDO WIFI");
    oled_hline(10);
    oled_print(0, 16, WIFI_SSID);
    oled_print(0, 30, "AGUARDE...");
    oled_update();
}

void display_wifi_ok(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print(0, 0, "WIFI CONECTADO!");
    oled_hline(10);
    oled_print(0, 20, "ACESSE:");
    oled_print(0, 35, g_ip);
    oled_update();
    sleep_ms(3000);
}

void display_wifi_fail(void) {
    if (!g_oled_ok) return;
    oled_clear();
    oled_print(0, 0, "WIFI ERRO!");
    oled_hline(10);
    oled_print(0, 20, "MODO OFFLINE");
    oled_update();
    sleep_ms(2000);
}

void display_main(void) {
    if (!g_oled_ok) return;
    oled_clear();
    
    char buf[24];
    
    // Titulo
    oled_print(20, 0, "HYDROSENSE");
    oled_hline(10);
    
    // Temperatura e Umidade
    snprintf(buf, sizeof(buf), "T:%.1fC", g_temp);
    oled_print(0, 14, buf);
    snprintf(buf, sizeof(buf), "H:%.0f%%", g_hum);
    oled_print(75, 14, buf);
    
    // Distancia e Nivel
    snprintf(buf, sizeof(buf), "D:%dmm", g_dist);
    oled_print(0, 26, buf);
    snprintf(buf, sizeof(buf), "N:%.0f%%", g_nivel);
    oled_print(75, 26, buf);
    
    // Volume e Reles
    snprintf(buf, sizeof(buf), "VOL:%.1fL", g_volume);
    oled_print(0, 38, buf);
    snprintf(buf, sizeof(buf), "R:%c%c%c",
        g_relay_ln1 ? '1' : '-',
        g_relay_ln2 ? '2' : '-',
        g_relay_ln3 ? '3' : '-');
    oled_print(80, 38, buf);
    
    // IP e contador
    if (g_wifi) {
        oled_print(0, 52, g_ip);
    } else {
        oled_print(0, 52, "OFFLINE");
    }
    snprintf(buf, sizeof(buf), "#%d", g_count);
    oled_print(90, 52, buf);
    
    oled_update();
}

// ============================================================
// SERVIDOR HTTP
// ============================================================

// Pagina HTML completa unificada com frontend principal
static const char *HTML_PAGE =
"<!DOCTYPE html>"
"<html lang='pt-BR'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>HydroSense v11 Beta</title>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:'Segoe UI',Arial,sans-serif;background:#1a1f2e;color:#fff;min-height:100vh}"
".header{text-align:center;padding:15px;background:linear-gradient(135deg,#1a2633,#0d1117);border-bottom:1px solid #30363d}"
".header h1{color:#58a6ff;font-size:1.8em}"
".header p{color:#8b949e;font-size:0.85em}"
".status-badge{display:inline-block;padding:4px 12px;border-radius:20px;font-size:0.75em;margin-top:8px}"
".status-online{background:#238636;color:#fff}"
".status-offline{background:#f85149;color:#fff}"
".main{display:grid;grid-template-columns:1fr 1fr;gap:15px;padding:15px;max-width:1400px;margin:0 auto}"
"@media(max-width:900px){.main{grid-template-columns:1fr}}"
".section{background:#21262d;border-radius:12px;padding:15px;border:1px solid #30363d}"
".section-title{color:#58a6ff;font-size:1em;margin-bottom:15px;display:flex;align-items:center;gap:8px}"
".sensors-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
".sensor-card{background:#161b22;border-radius:8px;padding:12px;text-align:center;border:1px solid #30363d}"
".sensor-icon{font-size:1.5em;margin-bottom:5px}"
".sensor-value{font-size:1.8em;font-weight:bold;color:#58a6ff}"
".sensor-label{color:#8b949e;font-size:0.75em;margin-top:3px}"
".nivel-bar{width:100%;height:12px;background:#30363d;border-radius:6px;overflow:hidden;margin-top:10px}"
".nivel-fill{height:100%;background:linear-gradient(90deg,#238636,#3fb950);transition:width 0.5s}"
".controls-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}"
".control-card{background:#161b22;border-radius:8px;padding:12px;text-align:center;border:1px solid #30363d}"
".control-icon{font-size:1.3em;margin-bottom:5px}"
".control-name{font-size:0.85em;color:#c9d1d9}"
".control-status{font-size:0.7em;padding:3px 8px;border-radius:10px;margin:5px 0}"
".status-on{background:#238636;color:#fff}"
".status-off{background:#484f58;color:#8b949e}"
".btn{width:100%;padding:8px;border:none;border-radius:6px;cursor:pointer;font-weight:bold;font-size:0.85em;transition:all 0.2s}"
".btn-on{background:#238636;color:#fff}"
".btn-on:hover{background:#2ea043}"
".btn-off{background:#f85149;color:#fff}"
".btn-off:hover{background:#da3633}"
".btn-activate{background:#1f6feb;color:#fff}"
".btn-activate:hover{background:#388bfd}"
".alimentacao{text-align:center;padding:15px}"
".relogio{font-size:2.8em;font-weight:bold;color:#3fb950;font-family:monospace}"
".data{color:#8b949e;font-size:0.85em;margin:5px 0}"
".info-alimentacao{color:#f0883e;font-size:0.8em;margin:5px 0}"
".btn-alimentar{display:block;margin:15px auto 0;padding:12px 30px;background:linear-gradient(135deg,#f0883e,#d29922);border:none;border-radius:25px;color:#fff;font-weight:bold;cursor:pointer;font-size:0.95em}"
".btn-alimentar:hover{transform:scale(1.02)}"
".sensor-card.alert{background:rgba(231,76,60,0.3);border-color:#e74c3c;animation:pulse-alert 1.5s infinite}"
"@keyframes pulse-alert{0%,100%{box-shadow:0 0 5px #e74c3c}50%{box-shadow:0 0 20px #e74c3c,0 0 30px #e74c3c}}"
".logs{background:#161b22;border-radius:8px;padding:10px;max-height:200px;overflow-y:auto;font-family:monospace;font-size:0.75em}"
".log-entry{padding:4px 0;border-bottom:1px solid #30363d;display:flex;gap:8px}"
".log-time{color:#58a6ff;min-width:55px}"
".log-msg{color:#c9d1d9}"
".stats{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-top:15px;text-align:center}"
".stat-value{font-size:1.2em;font-weight:bold;color:#58a6ff}"
".stat-label{font-size:0.7em;color:#8b949e}"
".footer{text-align:center;padding:10px;color:#484f58;font-size:0.75em}"
"</style>"
"</head>"
"<body>"
"<div class='header'>"
"<h1>&#x1F41F; HydroSense v11 Beta</h1>"
"<p>Sistema Inteligente de Aquicultura &mdash; BitDogLab + Pico W</p>"
"<span id='badge' class='status-badge status-online'>&#x25CF; Conectado</span>"
"</div>"
"<div class='main'>"
"<div class='section'>"
"<div class='section-title'>&#x1F4CA; Sensores em Tempo Real</div>"
"<div class='sensors-grid'>"
"<div id='tempCard' class='sensor-card'><div class='sensor-icon'>&#x1F321;</div><div id='temp' class='sensor-value'>--</div><div class='sensor-label'>TEMPERATURA</div></div>"
"<div class='sensor-card'><div class='sensor-icon'>&#x1F4A7;</div><div id='umid' class='sensor-value'>--</div><div class='sensor-label'>UMIDADE</div></div>"
"<div class='sensor-card'><div class='sensor-icon'>&#x1F4CF;</div><div id='dist' class='sensor-value'>--</div><div class='sensor-label'>DISTANCIA</div></div>"
"<div class='sensor-card'><div class='sensor-icon'>&#x1F30A;</div><div id='nivel' class='sensor-value'>--</div><div class='sensor-label'>NIVEL DA AGUA</div></div>"
"</div>"
"<div class='sensor-card' style='margin-top:10px'>"
"<div class='sensor-icon'>&#x1FAE7;</div><div id='vol' class='sensor-value'>--</div><div class='sensor-label'>Litros VOLUME DO AQUARIO</div>"
"<div class='nivel-bar'><div id='nivelBar' class='nivel-fill' style='width:0%'></div></div>"
"<div style='color:#8b949e;font-size:0.7em;margin-top:5px'>Nivel <span id='nivelTxt'>0</span>% - <span id='volTxt'>0</span>L de 20L</div>"
"</div>"
"<div class='sensor-card' style='margin-top:10px'>"
"<div class='sensor-icon'>&#x1F52C;</div><span id='turb' class='sensor-value' style='font-size:1.3em'>--</span>"
"<div class='sensor-label'>COR / QUALIDADE DA AGUA</div>"
"</div>"
"</div>"
"<div class='section'>"
"<div class='section-title'>&#x26A1; Controle de Reles</div>"
"<div class='controls-grid'>"
"<div class='control-card'>"
"<div class='control-icon'>&#x1F300;</div><div class='control-name'>Ventilador</div><div class='control-status status-off' id='ventSt'>&#x25CF; OFF</div>"
"<button class='btn btn-on' onclick=\"tr(1)\">LIGAR</button>"
"</div>"
"<div class='control-card'>"
"<div class='control-icon'>&#x2B07;</div><div class='control-name'>Bomba 1</div><div class='control-status status-off' id='b1St'>&#x25CF; OFF</div>"
"<button class='btn btn-on' onclick=\"tr(2)\">LIGAR</button>"
"</div>"
"<div class='control-card'>"
"<div class='control-icon'>&#x2B06;</div><div class='control-name'>Bomba 2</div><div class='control-status status-off' id='b2St'>&#x25CF; OFF</div>"
"<button class='btn btn-on' onclick=\"tr(3)\">LIGAR</button>"
"</div>"
"<div class='control-card'>"
"<div class='control-icon'>&#x2705;</div><div class='control-name'>Ligar Tudo</div>"
"<button class='btn btn-on' onclick=\"trAll(1)\" style='margin-top:22px'>LIGAR TUDO</button>"
"</div>"
"<div class='control-card'>"
"<div class='control-icon'>&#x26D4;</div><div class='control-name'>Desligar Tudo</div>"
"<button class='btn btn-off' onclick=\"trAll(0)\" style='margin-top:22px'>DESLIGAR TUDO</button>"
"</div>"
"<div class='control-card'>"
"<div class='control-icon'>&#x1F504;</div><div class='control-name'>TPA Manual</div><div class='control-status status-off' id='tpaSt'>&#x25CF; INATIVO</div>"
"<button class='btn btn-activate' id='tpaBtn' onclick=\"tpaToggle()\">INICIAR TPA</button>"
"</div>"
"</div>"
"</div>"
"<div class='section alimentacao'>"
"<div class='section-title' style='justify-content:center'>&#x1F41F; Sistema de Alimentacao</div>"
"<div id='clock' class='relogio'>--:--:--</div>"
"<div id='date' class='data'>-- de -------- de ----</div>"
"<div class='info-alimentacao'>&#x25CF; Alimentacao automatica: 08:00 e 16:00</div>"
"<div style='color:#8b949e;font-size:0.8em'>Servo SG90 - GPIO 16</div>"
"<button class='btn-alimentar' onclick=\"fd()\">&#x1F41F; Alimentar Agora</button>"
"<div class='stats'>"
"<div><div id='feeds' class='stat-value'>0</div><div class='stat-label'>Alimentacoes Hoje</div></div>"
"<div><div id='reads' class='stat-value'>0</div><div class='stat-label'>Leituras</div></div>"
"<div><div id='ip' class='stat-value'>--</div><div class='stat-label'>IP do Dispositivo</div></div>"
"</div>"
"</div>"
"<div class='section'>"
"<div class='section-title'>&#x1F4CB; Log de Eventos</div>"
"<div id='logs' class='logs'>"
"<div class='log-entry'><span class='log-time'>--:--:--</span><span class='log-msg'>Aguardando dados...</span></div>"
"</div>"
"</div>"
"</div>"
"<div class='footer'>Ultima atualizacao: <span id='lastUp'>--:--:--</span> - Fonte: Pico W Embarcado</div>"
"<script>"
"var meses=['Janeiro','Fevereiro','Marco','Abril','Maio','Junho','Julho','Agosto','Setembro','Outubro','Novembro','Dezembro'];"
"var logs=[],reads=0,lastFeedKey='',prevRelays={LN1:false,LN2:false,LN3:false};"
"function speak(t){if('speechSynthesis'in window){window.speechSynthesis.cancel();var u=new SpeechSynthesisUtterance(t);u.lang='pt-BR';u.rate=1;u.pitch=1;window.speechSynthesis.speak(u)}}"
"function pad(n){return n<10?'0'+n:n}"
"function updateClock(){"
"var d=new Date();"
"document.getElementById('clock').textContent=pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds());"
"document.getElementById('date').textContent=d.getDate()+' de '+meses[d.getMonth()]+' de '+d.getFullYear();"
"}"
"function addLog(msg){var d=new Date();logs.unshift({t:pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds()),m:msg});if(logs.length>20)logs.pop();renderLogs()}"
"function renderLogs(){document.getElementById('logs').innerHTML=logs.map(function(l){return '<div class=\"log-entry\"><span class=\"log-time\">'+l.t+'</span><span class=\"log-msg\">'+l.m+'</span></div>'}).join('')}"
"function tr(n){fetch('/relay',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({relay:n,toggle:true})}).then(function(){var nomes=['','Ventilador','Bomba 1','Bomba 2'];addLog('Rele '+n+' acionado');speak(nomes[n]+' acionado');getData()}).catch(function(){addLog('Erro ao acionar rele')})}"
"function trAll(on){fetch(on?'/all-on':'/all-off',{method:'POST'}).then(function(){addLog(on?'Todos ligados':'Todos desligados');speak(on?'Todos os relés ligados':'Todos os relés desligados');getData()}).catch(function(){addLog('Erro')})}"
"function fd(){fetch('/feed',{method:'POST'}).then(function(){addLog('Alimentacao manual');speak('Alimentação manual iniciada. Despejando ração.')}).catch(function(){addLog('Erro alimentacao')})}"
"function tpaToggle(){var btn=document.getElementById('tpaBtn');if(btn.textContent.indexOf('PARAR')>=0){fetch('/tpa-stop',{method:'POST'}).then(function(){addLog('TPA parado');speak('Troca parcial de água interrompida');getData()}).catch(function(){addLog('Erro ao parar TPA')})}else{fetch('/tpa',{method:'POST'}).then(function(){addLog('TPA iniciado');speak('Iniciando troca parcial de água. Bomba 1 drenando até 25 por cento.');getData()}).catch(function(){addLog('Erro TPA')})}}"
"function getData(){"
"fetch('/sensors').then(function(r){return r.json()}).then(function(d){"
"document.getElementById('temp').textContent=d.temperatura.toFixed(1)+'C';"
"document.getElementById('umid').textContent=d.umidade.toFixed(0)+'%';"
"document.getElementById('dist').textContent=d.distancia+'mm';"
"document.getElementById('nivel').textContent=d.nivel.toFixed(0)+'%';"
"document.getElementById('vol').textContent=d.volume.toFixed(1);"
"document.getElementById('nivelBar').style.width=d.nivel+'%';"
"document.getElementById('nivelTxt').textContent=d.nivel.toFixed(0);"
"document.getElementById('volTxt').textContent=d.volume.toFixed(1);"
"document.getElementById('turb').textContent=d.corAgua||'--';"
"document.getElementById('feeds').textContent=d.alimentacoes||0;"
"reads++;document.getElementById('reads').textContent=reads;"
"document.getElementById('ip').textContent=d.deviceIp||location.hostname;"
"var r=d.relays||{};"
"document.getElementById('ventSt').textContent=r.LN1?'ON':'OFF';"
"document.getElementById('ventSt').className='control-status '+(r.LN1?'status-on':'status-off');"
"document.getElementById('b1St').textContent=r.LN2?'ON':'OFF';"
"document.getElementById('b1St').className='control-status '+(r.LN2?'status-on':'status-off');"
"document.getElementById('b2St').textContent=r.LN3?'ON':'OFF';"
"document.getElementById('b2St').className='control-status '+(r.LN3?'status-on':'status-off');"
"document.getElementById('tpaSt').textContent=d.tpa&&d.tpa.active?'ATIVO':'INATIVO';"
"document.getElementById('tpaSt').className='control-status '+(d.tpa&&d.tpa.active?'status-on':'status-off');"
"var tpaBtn=document.getElementById('tpaBtn');if(d.tpa&&d.tpa.active){tpaBtn.textContent='PARAR TPA';tpaBtn.className='btn btn-off';}else{tpaBtn.textContent='INICIAR TPA';tpaBtn.className='btn btn-activate';}"
"var now=new Date();document.getElementById('lastUp').textContent=pad(now.getHours())+':'+pad(now.getMinutes())+':'+pad(now.getSeconds());"
"document.getElementById('badge').className='status-badge status-online';document.getElementById('badge').innerHTML='&#x25CF; Conectado';"
"var tc=document.getElementById('tempCard');if(d.temperatura>33){tc.classList.add('alert')}else{tc.classList.remove('alert')}"
"}).catch(function(){document.getElementById('badge').className='status-badge status-offline';document.getElementById('badge').innerHTML='&#x25CF; Offline';});"
"}"
"setInterval(updateClock,1000);"
"setInterval(getData,2000);"
"setInterval(function(){var d=new Date(),h=d.getHours(),m=d.getMinutes(),k=h+':'+m;if((h===8&&m===0)||(h===16&&m===0)){if(lastFeedKey!==k){lastFeedKey=k;var p=h===8?'matutina':'vespertina';speak('Hora de alimentar os peixes! Alimentação '+p+' programada. Despejando 100 gramas de ração.');addLog('Alimentacao programada '+(h===8?'08:00':'16:00'));fd()}}},10000);"
"updateClock();getData();"
"addLog('HydroSense v11 Beta (Anti-Sifão) iniciando...');"
"addLog('Conectado ao Pico W');"
"speak('HydroSense conectado. Sistema operacional.');"
"</script>"
"</body></html>";

// Buffers HTTP (estaticos como v7)
static char http_resp[20480];  // Aumentado para frontend completo
static char json_buf[768];
static char http_header[256];

// Tracking de conexao ativa
typedef struct {
    int total_to_send;
    int total_acked;
} http_conn_t;

static http_conn_t http_conn;

// Build JSON de sensores
static void build_sensor_json(void) {
    snprintf(json_buf, sizeof(json_buf),
        "{\"temperatura\":%.2f,\"umidade\":%.2f,\"distancia\":%d,"
        "\"nivel\":%.2f,\"volume\":%.2f,"
        "\"corAgua\":\"%s\",\"corR\":%d,\"corG\":%d,\"corB\":%d,"
        "\"tpa\":{\"active\":%s,\"phase\":%d},"
        "\"wifiStatus\":true,\"contadorLeituras\":%d,\"deviceIp\":\"%s\","
        "\"relays\":{\"LN1\":%s,\"LN2\":%s,\"LN3\":%s},"
        "\"sensores\":{\"aht10\":%s,\"vl53l0x\":%s,\"tcs34725\":%s}}",
        g_temp, g_hum, g_dist, g_nivel, g_volume,
        g_cor_nome, g_cor_r, g_cor_g, g_cor_b,
        g_tpa_active?"true":"false", g_tpa_phase,
        g_count, g_ip,
        g_relay_ln1?"true":"false", g_relay_ln2?"true":"false", g_relay_ln3?"true":"false",
        g_aht_ok?"true":"false", g_vl53_ok?"true":"false", g_tcs_ok?"true":"false");
}

// Callback chamado quando dados sao confirmados enviados
static err_t http_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    http_conn_t *conn = (http_conn_t *)arg;
    if (conn) {
        conn->total_acked += len;
        if (conn->total_acked >= conn->total_to_send) {
            tcp_arg(tpcb, NULL);
            tcp_sent(tpcb, NULL);
            tcp_close(tpcb);
        }
    }
    return ERR_OK;
}

// Callback TCP recv
static err_t http_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    char *request = (char *)p->payload;
    int send_len = 0;

    // CORS Preflight (OPTIONS) - necessario para frontend externo
    if (strstr(request, "OPTIONS ")) {
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Access-Control-Max-Age: 86400\r\n"
            "Connection: close\r\n"
            "\r\n");
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "GET /sensors") || strstr(request, "GET /api")) {
        build_sensor_json();
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /relay")) {
        // Parse relay number - suporta JSON e URL-encoded
        int relay_num = 0;
        char *rp = strstr(request, "\"relay\"");
        if (rp) {
            rp = strchr(rp, ':');
            if (rp) relay_num = atoi(rp + 1);
        }
        if (!relay_num) {
            // Tenta formato URL-encoded: relay=1&state=1
            rp = strstr(request, "relay=");
            if (rp) relay_num = atoi(rp + 6);
        }
        
        if (relay_num >= 1 && relay_num <= 3) {
            if (strstr(request, "\"toggle\":true")) {
                switch (relay_num) {
                    case 1: relay_set(1, !g_relay_ln1); break;
                    case 2: relay_set(2, !g_relay_ln2); break;
                    case 3: relay_set(3, !g_relay_ln3); break;
                }
            } else {
                bool state = (strstr(request, "\"state\":1") != NULL) ||
                             (strstr(request, "\"estado\":true") != NULL) ||
                             (strstr(request, "state=1") != NULL);
                relay_set(relay_num, state);
            }
            // Marca controle manual para o rele 1 (ventilador)
            if (relay_num == 1) {
                // Se usuario LIGOU manualmente, flag fica true ate desligar
                g_ln1_manual = g_relay_ln1;
                printf("[RELAY] LN1 manual=%s\n", g_relay_ln1 ? "ON" : "OFF");
            }
        }
        snprintf(json_buf, sizeof(json_buf),
            "{\"success\":true,\"relays\":{\"LN1\":%s,\"LN2\":%s,\"LN3\":%s}}",
            g_relay_ln1?"true":"false", g_relay_ln2?"true":"false", g_relay_ln3?"true":"false");
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /feed") || strstr(request, "POST /servo")) {
        servo_feed();
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: 49\r\n\r\n{\"success\":true,\"message\":\"Alimentacao executada\"}");
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /tpa-stop")) {
        // Para TPA manualmente - DEVE vir antes de /tpa!
        g_tpa_active = false;
        g_tpa_phase = 0;
        relay_set(2, false);
        relay_set(3, false);
        printf("[TPA] PARADO manualmente\n");
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: 52\r\n\r\n{\"success\":true,\"message\":\"TPA parada com sucesso\"}");
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /tpa")) {
        // Inicia TPA - Troca Parcial de Agua
        if (!g_tpa_active && g_vl53_ok && g_nivel > 5.0f) {
            g_tpa_active = true;
            g_tpa_active = true;
            g_tpa_phase = 1;  // Fase 1: drenando
            relay_set(2, true);  // Liga bomba 1 (drenar)
            relay_set(3, false); // Garante bomba 2 desligada
            printf("[TPA] INICIADO v11 - Drenando ate %.1f%% (parada eletrica) -> Sifao ate %.1f%%\n", TPA_DRAIN_STOP, TPA_SIPHON_TARGET);
        } else if (!g_vl53_ok || g_nivel <= 5.0f) {
            printf("[TPA] REJEITADO - sensor=%s nivel=%.1f%%\n", g_vl53_ok?"OK":"FALHA", g_nivel);
        }
        snprintf(json_buf, sizeof(json_buf),
            "{\"success\":true,\"tpa\":{\"active\":%s,\"phase\":%d},\"message\":\"TPA %s\"}",
            g_tpa_active?"true":"false", g_tpa_phase, g_tpa_active?"iniciada":"rejeitada - sensor indisponivel");
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /all-on")) {
        // Escalonamento: liga um por um com relay_set (que ja tem logica interna)
        g_relay_ln1 = false; g_relay_ln2 = false; g_relay_ln3 = false;
        gpio_put(RELAY_LN1_PIN, 0); gpio_put(RELAY_LN2_PIN, 0); gpio_put(RELAY_LN3_PIN, 0);
        sleep_ms(200);
        g_relay_ln1 = true; gpio_put(RELAY_LN1_PIN, 1); sleep_ms(500);
        g_relay_ln2 = true; gpio_put(RELAY_LN2_PIN, 1); sleep_ms(500);
        g_relay_ln3 = true; gpio_put(RELAY_LN3_PIN, 1); sleep_ms(200);
        relay_refresh_all();  // Re-afirma tudo
        g_ln1_manual = true;
        printf("[RELAY] ALL-ON escalonado: LN1=%d LN2=%d LN3=%d\n", g_relay_ln1, g_relay_ln2, g_relay_ln3);
        snprintf(json_buf, sizeof(json_buf),
            "{\"success\":true,\"relays\":{\"LN1\":true,\"LN2\":true,\"LN3\":true}}");
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "POST /all-off")) {
        g_relay_ln1 = false; g_relay_ln2 = false; g_relay_ln3 = false;
        gpio_put(RELAY_LN1_PIN, 0); gpio_put(RELAY_LN2_PIN, 0); gpio_put(RELAY_LN3_PIN, 0);
        sleep_ms(100);
        relay_refresh_all();
        g_tpa_active = false; g_tpa_phase = 0;  // Cancela TPA tambem
        g_ln1_manual = false;
        printf("[RELAY] ALL-OFF: LN1=%d LN2=%d LN3=%d\n", g_relay_ln1, g_relay_ln2, g_relay_ln3);
        snprintf(json_buf, sizeof(json_buf),
            "{\"success\":true,\"relays\":{\"LN1\":false,\"LN2\":false,\"LN3\":false}}");
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "GET /status")) {
        snprintf(json_buf, sizeof(json_buf),
            "{\"system\":\"HydroSense v11 Beta\",\"wifi\":true,\"ip\":\"%s\","
            "\"oled\":%s,\"sensores\":{\"aht10\":%s,\"vl53l0x\":%s},"
            "\"relays\":{\"LN1\":%s,\"LN2\":%s,\"LN3\":%s},\"leituras\":%d}",
            g_ip, g_oled_ok?"true":"false",
            g_aht_ok?"true":"false", g_vl53_ok?"true":"false",
            g_relay_ln1?"true":"false", g_relay_ln2?"true":"false", g_relay_ln3?"true":"false", g_count);
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            (int)strlen(json_buf), json_buf);
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else if (strstr(request, "OPTIONS")) {
        snprintf(http_resp, sizeof(http_resp),
            "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nConnection: close\r\n\r\n");
        send_len = strlen(http_resp);
        tcp_write(tpcb, http_resp, send_len, 0);
    }
    else {
        // Pagina HTML - envia header e body como chamadas separadas
        int html_len = strlen(HTML_PAGE);
        int hdr_len = snprintf(http_header, sizeof(http_header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "\r\n", html_len);
        err_t e1 = tcp_write(tpcb, http_header, hdr_len, 0);
        err_t e2 = tcp_write(tpcb, HTML_PAGE, html_len, 0);
        send_len = hdr_len + html_len;
    }

    // Configura tracking e envia
    http_conn.total_to_send = send_len;
    http_conn.total_acked = 0;
    tcp_arg(tpcb, &http_conn);
    tcp_sent(tpcb, http_sent_cb);
    tcp_output(tpcb);
    pbuf_free(p);
    // NÃO chama tcp_close aqui - o sent callback fecha quando tudo for confirmado
    
    return ERR_OK;
}

static err_t http_accept_cb(void *a, struct tcp_pcb *n, err_t e) {
    if (e != ERR_OK || !n) return ERR_VAL;
    tcp_recv(n, http_recv_cb);
    return ERR_OK;
}

// ============================================================
// LEITURA DOS SENSORES
// ============================================================

void read_sensors(void) {
    g_count++;
    
    // AHT10
    float temp, hum;
    if (aht10_read(&temp, &hum)) {
        g_temp = temp;
        g_hum = hum;
        g_aht_ok = true;
    }
    
    // VL53L0X - FILTRO ROBUSTO: MEDIANA + HISTERESE
    if (g_vl53_ok) {
        uint16_t dist = vl53_read();
        if (dist != 0xFFFF && dist < 2000 && dist > 50) {
            g_dist_raw = dist;  // Salva leitura bruta para debug
            
            // Adiciona ao buffer circular
            vl53_buffer[vl53_buffer_idx] = dist;
            vl53_buffer_idx = (vl53_buffer_idx + 1) % VL53_FILTER_SIZE;
            if (vl53_buffer_count < VL53_FILTER_SIZE) vl53_buffer_count++;
            
            // Copia buffer para ordenar (calculo de mediana)
            uint16_t sorted[VL53_FILTER_SIZE];
            int valid_count = 0;
            for (int i = 0; i < vl53_buffer_count; i++) {
                if (vl53_buffer[i] > 50 && vl53_buffer[i] < 2000) {
                    sorted[valid_count++] = vl53_buffer[i];
                }
            }
            
            // Ordena (bubble sort simples - buffer pequeno)
            for (int i = 0; i < valid_count - 1; i++) {
                for (int j = 0; j < valid_count - i - 1; j++) {
                    if (sorted[j] > sorted[j + 1]) {
                        uint16_t tmp = sorted[j];
                        sorted[j] = sorted[j + 1];
                        sorted[j + 1] = tmp;
                    }
                }
            }
            
            // Pega a mediana (valor do meio)
            if (valid_count > 0) {
                uint16_t mediana = sorted[valid_count / 2];
                
                // Aplica histerese: so atualiza se mudanca > VL53_HISTERESE
                int diff = (int)mediana - (int)vl53_last_stable;
                if (diff < 0) diff = -diff;
                
                if (diff > VL53_HISTERESE || vl53_last_stable == SENSOR_DIST_EMPTY) {
                    vl53_last_stable = mediana;
                    g_dist = mediana;
                }
                // Se diferenca pequena, mantem valor anterior (estabilidade)
            }
        } else {
            printf("[VL53] Falha leitura (0x%04X) - retentando init\n", dist);
            g_vl53_ok = vl53_init();
        }
    } else {
        // Sensor nao estava OK, tenta re-iniciar
        static int vl53_retry_count = 0;
        if (++vl53_retry_count >= 5) {
            vl53_retry_count = 0;
            printf("[VL53] Retentando inicializacao...\n");
            g_vl53_ok = vl53_init();
        }
    }
    
    // TCS34725 - com retentativa
    if (g_tcs_ok) {
        tcs_read_color();
    } else {
        static int tcs_retry_count = 0;
        if (++tcs_retry_count >= 5) {
            tcs_retry_count = 0;
            printf("[TCS] Retentando inicializacao...\n");
            g_tcs_ok = tcs_init();
        }
    }
    
    // Calcula nivel e volume baseado na calibracao do sensor
    // dist=160mm -> 100% cheio, dist=360mm -> 0% vazio
    g_nivel = 100.0f * (float)(SENSOR_DIST_EMPTY - g_dist) / (float)(SENSOR_DIST_EMPTY - SENSOR_DIST_FULL);
    if (g_nivel < 0) g_nivel = 0;
    if (g_nivel > 100) g_nivel = 100;
    g_volume = (g_nivel / 100.0f) * TANK_CAPACITY_L;
    
    // ========== REGRA ABSOLUTA DE SEGURANCA ==========
    // Bomba 2 (LN3) SEMPRE desliga em 75% - SEM EXCECOES
    // Isso evita transbordamento e peixes pulando
    if (g_nivel >= 75.0f && g_relay_ln3) {
        relay_set(3, false);  // Desliga bomba 2 IMEDIATAMENTE
        printf("[SEGURANCA ABSOLUTA] Bomba 2 DESLIGADA - nivel %.1f%% >= 75%%\n", g_nivel);
        // Se estava em TPA fase 3, avanca para fase 4
        if (g_tpa_active && g_tpa_phase == 3) {
            g_tpa_phase = 4;
            g_tpa_monitor_until = to_ms_since_boot(get_absolute_time()) + TPA_MONITOR_TIME_MS;
            printf("[TPA] Avancando para fase 4 (monitoramento) por seguranca\n");
        }
    }
    
    // REABASTECIMENTO DE EMERGENCIA:
    // Aciona bomba 2 APENAS se:
    // - TPA NAO esta ativo
    // - Bomba 1 NAO esta ativa
    // - Nivel caiu abaixo de 25% (situacao critica)
    // - Bomba 2 NAO esta ligada
    // DEBUG: Log para verificar condicoes
    static int debug_counter = 0;
    if (++debug_counter >= 10) {  // A cada 10 ciclos (~20s)
        debug_counter = 0;
        printf("[DEBUG] nivel=%.1f%% TPA=%d B1=%d B2=%d CRITICO=%.0f%%\n", 
               g_nivel, g_tpa_active, g_relay_ln2, g_relay_ln3, TPA_SAFETY_LOW);
    }
    
    if (!g_tpa_active && !g_relay_ln2 && g_nivel < TPA_SAFETY_LOW && !g_relay_ln3) {
        relay_set(3, true);  // Liga bomba 2 para reabastecer
        printf("[EMERGENCIA] Bomba 2 LIGADA - nivel critico %.1f%% < %.0f%%\n", g_nivel, TPA_SAFETY_LOW);
    }
    
    // Automacao: liga ventilador se temp alta
    // SEMPRE liga se temperatura exceder o limite - independente de qualquer override
    // So desliga automaticamente se usuario NAO ligou manualmente
    if (g_temp > TEMP_THRESHOLD && !g_relay_ln1) {
        // Escalonamento: se outro rele ja esta ligado, aguarda 500ms
        // para evitar pico de corrente que causa acionamento erratico
        if (g_relay_ln2 || g_relay_ln3) {
            printf("[AUTO] Escalonamento: aguardando 500ms (outro rele ativo)\n");
            sleep_ms(500);
        }
        relay_set(1, true);
        printf("[AUTO] Ventilador ON (T=%.1f > %.0f) GPIO%d\n", g_temp, TEMP_THRESHOLD, RELAY_LN1_PIN);
    } else if (g_temp <= (TEMP_THRESHOLD - 1.0f) && g_relay_ln1 && !g_ln1_manual) {
        relay_set(1, false);
        printf("[AUTO] Ventilador OFF (T=%.1f <= %.0f)\n", g_temp, TEMP_THRESHOLD - 1.0f);
    }
    
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    
    // TPA - Troca Parcial de Agua (state machine) v11 Beta
    if (g_tpa_active) {
        switch (g_tpa_phase) {
            case 1: // Fase 1: Drenando (Bomba 1 ON ate 25%)
                if (g_nivel <= TPA_DRAIN_STOP) {
                    relay_set(2, false);  // Desliga bomba 1 Eletricamente
                    g_tpa_phase = 2; // Vai para Espera de Sifao
                    printf("[TPA] Fase 1->2: Bomba 1 OFF (%.1f%%). Aguardando Sifao ate %.1f%%\n", g_nivel, TPA_SIPHON_TARGET);
                }
                break;
            
            case 2: // Fase 2: Espera Sifao (Bombas OFF)
                // Sai se atingir o alvo do sifao (23%) OU nivel critico de seguranca (20%)
                if (g_nivel <= TPA_SIPHON_TARGET || g_nivel <= TPA_SAFETY_LOW) {
                    relay_set(2, false); // Garante bomba 1 off
                    relay_set(3, true);  // Liga bomba 2 (encher)
                    g_tpa_phase = 3;
                    printf("[TPA] Fase 2->3: Sifao acabou (%.1f%%). Enchendo...\n", g_nivel);
                }
                break;

            case 3: // Fase 3: Enchendo (Bomba 2 ON ate 75%)
                if (g_nivel >= TPA_FILL_TARGET) {
                    relay_set(3, false);  // Desliga bomba 2
                    g_tpa_phase = 4;
                    g_tpa_monitor_until = now_ms + TPA_MONITOR_TIME_MS;
                    printf("[TPA] Fase 3->4: Monitorando nivel (em 75%%) por 2min\n");
                }
                break;

            case 4: // Fase 4: Monitorando - se nivel cair, re-enche
                if (g_nivel < (TPA_FILL_TARGET - TPA_REFILL_MARGIN)) {
                    relay_set(3, true);  // Re-liga bomba 2
                    g_tpa_phase = 3;     // Volta para fase de enchimento
                    printf("[TPA] Re-enchendo (nivel caiu p/ %.1f%%)\n", g_nivel);
                } else if (now_ms > g_tpa_monitor_until) {
                    // Fim do monitoramento
                    g_tpa_active = false;
                    g_tpa_phase = 0;
                    printf("[TPA] CONCLUIDA com sucesso! (v11 Beta)\n");
                }
                break;
        }
    }
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Aguarda USB estabilizar
    
    printf("\n\n==========================================\n");
    printf("  HydroSense v11 Beta (Anti-Sifão)\n");
    printf("==========================================\n\n");
    
    // === CYW43 ===
    printf("[1/7] CYW43...");
    if (cyw43_arch_init()) { printf("ERRO!\n"); while(1) sleep_ms(1000); }
    cyw43_arch_enable_sta_mode();
    printf(" OK\n");
    
    // === LED RGB ===
    printf("[2/7] LED RGB...");
    led_init();
    led_set(1, 0, 0);  // Vermelho durante init
    printf(" OK\n");
    
    // === Buzzer ===
    printf("[3/7] Buzzer...");
    buzzer_init();
    buzzer_beep(100);
    printf(" OK\n");
    
    // === OLED ===
    printf("[4/7] OLED...");
    g_oled_ok = oled_init();
    printf(" %s\n", g_oled_ok ? "OK" : "N/A");
    display_boot();
    
    // === Sensores I2C ===
    printf("[5/7] Sensores...\n");
    g_aht_ok = aht10_init();
    printf("  AHT10: %s\n", g_aht_ok ? "OK" : "N/A");
    g_vl53_ok = vl53_init();
    printf("  VL53L0X: %s\n", g_vl53_ok ? "OK" : "N/A");
    g_tcs_ok = tcs_init();
    printf("  TCS34725: %s\n", g_tcs_ok ? "OK" : "N/A");
    
    // === Servo ===
    printf("[6/7] Servo...");
    servo_init();
    // Servo não move na inicialização - aguarda comando de alimentação
    servo_stop();
    printf(" OK (sem movimento inicial)\n");
    
    // === Reles ===
    printf("[7/7] Reles...");
    relay_init();
    printf(" OK (GP%d,%d,%d)\n", RELAY_LN1_PIN, RELAY_LN2_PIN, RELAY_LN3_PIN);
    
    // === WiFi ===
    printf("\n[WIFI] Conectando a [%s]...\n", WIFI_SSID);
    led_set(1, 1, 0);  // Amarelo
    display_wifi_connecting();
    
    int result = -1;
    printf("  WPA2-AES...\n");
    result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000);
    if (result) {
        printf("  err=%d, WPA2-MIXED...\n", result);
        sleep_ms(2000);
        result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000);
    }
    if (result) {
        printf("  err=%d, WPA-TKIP...\n", result);
        sleep_ms(2000);
        result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA_TKIP_PSK, 30000);
    }
    
    if (result) {
        printf("[WIFI] FALHOU (%d) - modo offline\n", result);
        g_wifi = false;
        led_set(1, 0, 0);
        display_wifi_fail();
    } else {
        g_wifi = true;
        strncpy(g_ip, ip4addr_ntoa(&cyw43_state.netif[CYW43_ITF_STA].ip_addr), 19);
        printf("[WIFI] IP: %s\n", g_ip);
        led_set(0, 1, 0);  // Verde
        buzzer_beep(50); sleep_ms(50); buzzer_beep(50);
        
        // Servidor HTTP
        struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
        if (pcb) {
            tcp_bind(pcb, IP_ANY_TYPE, 80);
            pcb = tcp_listen_with_backlog(pcb, 1);
            if (pcb) {
                tcp_accept(pcb, http_accept_cb);
                printf("[HTTP] http://%s/sensors\n", g_ip);
                printf("[HTTP] http://%s/ (pagina web)\n", g_ip);
            }
        }
        display_wifi_ok();
    }
    
    printf("\n=== MONITORAMENTO ATIVO ===\n\n");
    
    // === Loop Principal ===
    while (true) {
        read_sensors();
        display_main();
        
        printf("#%d T=%.1fC H=%.1f%% D=%dmm N=%.0f%% V=%.1fL C=%s R:%c%c%c W:%s\n",
               g_count, g_temp, g_hum, g_dist, g_nivel, g_volume, g_cor_nome,
               g_relay_ln1 ? '1' : '-', g_relay_ln2 ? '2' : '-', g_relay_ln3 ? '3' : '-',
               g_wifi ? "OK" : "OFF");
        
        // Pisca LED verde quando wifi ok
        if (g_wifi) {
            led_set(0, 1, 0); sleep_ms(100); led_set(0, 0, 0);
        }
        
        // Refresh periodico dos GPIOs dos reles
        // Corrige qualquer corrupcao por instabilidade eletrica
        relay_refresh_all();
        
        // Poll WiFi frequentemente por ~2 segundos
        // Isso garante que dados TCP sejam transmitidos/recebidos
        for (int i = 0; i < 19; i++) {
            if (g_wifi) cyw43_arch_poll();
            sleep_ms(100);
        }
    }
    return 0;
}
