/**
 * HydroSense v4 - Sistema Completo
 * 
 * CONFIGURAÇÃO DE HARDWARE:
 * - Sensores (VL53L0X, AHT10): I2C1 em GPIO2/3 (extensor)
 * - OLED SSD1306: I2C1 em GPIO14/15 (BitDogLab direto)
 * - Servo SG90: GPIO16
 * 
 * Como ambos usam I2C1, precisamos reinicializar ao trocar os pinos.
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Pinos
#define LED_PIN         25
#define SERVO_PIN       16

// I2C Sensores (extensor)
#define SENSOR_SDA      2
#define SENSOR_SCL      3

// I2C OLED (BitDogLab)
#define OLED_SDA        14
#define OLED_SCL        15

// Endereços I2C
#define VL53L0X_ADDR    0x29
#define AHT10_ADDR      0x38
#define OLED_ADDR       0x3C

// Estado atual do I2C
static int current_i2c_mode = 0;  // 0=nenhum, 1=sensores, 2=oled

// Dados dos sensores
static float temperatura = 0;
static float umidade = 0;
static uint16_t distancia_mm = 0;
static uint8_t vl53_stop_var = 0;
static bool aht_ok = false;
static bool vl53_ok = false;

// ============================================================
// I2C Switching - Reinicializa I2C nos pinos corretos
// ============================================================

void i2c_init_for_sensors(void) {
    if (current_i2c_mode == 1) return;  // Já está em sensores
    
    // Desliga pinos OLED
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    
    // Deinicializa e reinicializa I2C
    i2c_deinit(i2c1);
    
    // Configura para sensores
    i2c_init(i2c1, 100000);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_ms(10);
    
    current_i2c_mode = 1;
}

void i2c_init_for_oled(void) {
    if (current_i2c_mode == 2) return;  // Já está em OLED
    
    // Desliga pinos sensores
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    
    // Deinicializa e reinicializa I2C
    i2c_deinit(i2c1);
    
    // Configura para OLED
    i2c_init(i2c1, 100000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(10);
    
    current_i2c_mode = 2;
}

// ============================================================
// AHT10 - Sensor de Temperatura e Umidade
// ============================================================

bool aht10_init(void) {
    i2c_init_for_sensors();
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    return i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) >= 0;
}

bool aht10_read(void) {
    i2c_init_for_sensors();
    
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) return false;
    sleep_ms(80);
    
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) return false;
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    umidade = (float)hum_raw / 1048576.0f * 100.0f;
    temperatura = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    return true;
}

// ============================================================
// VL53L0X - Sensor de Distância (Clone v2 - Inicialização Completa)
// ============================================================

static void vl53_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
}

static uint8_t vl53_read(uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
    return val;
}

static uint16_t vl53_read16(uint8_t reg) {
    uint8_t buf[2];
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

bool vl53_init(void) {
    i2c_init_for_sensors();
    
    // Verifica ID - aceita qualquer valor diferente de 0x00/0xFF
    uint8_t id = vl53_read(0xC0);
    printf("   VL53 ID: 0x%02X\n", id);
    if (id == 0xFF || id == 0x00) return false;
    
    // Software reset
    vl53_write(0xBF, 0x01);
    sleep_ms(100);  // Tempo maior para clone
    
    // Inicialização padrão VL53L0X
    vl53_write(0x88, 0x00);
    vl53_write(0x80, 0x01);
    vl53_write(0xFF, 0x01);
    vl53_write(0x00, 0x00);
    
    vl53_stop_var = vl53_read(0x91);
    
    vl53_write(0x00, 0x01);
    vl53_write(0xFF, 0x00);
    vl53_write(0x80, 0x00);
    
    // Configurações de timing para melhor precisão
    vl53_write(0x60, 0x38);  // Final range config
    vl53_write(0x44, 0x25);
    vl53_write(0x00, 0x00);  // Sysrange start
    
    // Aguarda estabilização
    sleep_ms(100);
    
    printf("   VL53 stop_var: 0x%02X\n", vl53_stop_var);
    return true;
}

uint16_t vl53_read_mm(void) {
    i2c_init_for_sensors();
    
    // Sequência de medição para VL53L0X clone
    vl53_write(0x80, 0x01);
    vl53_write(0xFF, 0x01);
    vl53_write(0x00, 0x00);
    vl53_write(0x91, vl53_stop_var);
    vl53_write(0x00, 0x01);
    vl53_write(0xFF, 0x00);
    vl53_write(0x80, 0x00);
    
    // Inicia medição single-shot
    vl53_write(0x00, 0x01);
    
    // Espera medição iniciar (timeout 100ms)
    for (int i = 0; i < 50; i++) {
        if ((vl53_read(0x00) & 0x01) == 0) break;
        sleep_ms(2);
    }
    
    // Espera resultado (timeout 500ms)
    for (int i = 0; i < 100; i++) {
        uint8_t status = vl53_read(0x13);
        if (status & 0x07) break;
        sleep_ms(5);
    }
    
    // Lê distância (registro 0x14 + offset 10 = 0x1E)
    uint16_t dist = vl53_read16(0x1E);
    
    // Verifica se é valor válido (20mm a 1200mm típico)
    if (dist < 20 || dist > 2000) {
        // Tenta registro alternativo 0x14+12
        dist = vl53_read16(0x14 + 12);
    }
    
    // Limpa interrupção
    vl53_write(0x0B, 0x01);
    
    return (dist >= 20 && dist <= 1500) ? dist : 0xFFFF;
}

// ============================================================
// OLED SSD1306
// ============================================================

static uint8_t oled_buffer[1024];

void oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(i2c1, OLED_ADDR, buf, 2, false);
}

bool oled_init(void) {
    i2c_init_for_oled();
    
    // Verifica se OLED responde
    uint8_t dummy;
    if (i2c_read_blocking(i2c1, OLED_ADDR, &dummy, 1, false) < 0) {
        return false;
    }
    
    oled_cmd(0xAE);  // Display OFF
    oled_cmd(0xD5); oled_cmd(0x80);  // Clock
    oled_cmd(0xA8); oled_cmd(0x3F);  // Multiplex 64
    oled_cmd(0xD3); oled_cmd(0x00);  // Offset 0
    oled_cmd(0x40);  // Start line 0
    oled_cmd(0x8D); oled_cmd(0x14);  // Charge pump ON
    oled_cmd(0x20); oled_cmd(0x00);  // Horizontal addressing
    oled_cmd(0xA1);  // Segment remap (flip horizontal)
    oled_cmd(0xC8);  // COM scan dec (flip vertical)
    oled_cmd(0xDA); oled_cmd(0x12);  // COM pins
    oled_cmd(0x81); oled_cmd(0xCF);  // Contrast
    oled_cmd(0xD9); oled_cmd(0xF1);  // Pre-charge
    oled_cmd(0xDB); oled_cmd(0x40);  // VCOMH
    oled_cmd(0xA4);  // Resume RAM
    oled_cmd(0xA6);  // Normal display
    oled_cmd(0xAF);  // Display ON
    
    return true;
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_update(void) {
    i2c_init_for_oled();
    
    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);
    
    for (int i = 0; i < sizeof(oled_buffer); i += 16) {
        uint8_t buf[17];
        buf[0] = 0x40;
        memcpy(&buf[1], &oled_buffer[i], 16);
        i2c_write_blocking(i2c1, OLED_ADDR, buf, 17, false);
    }
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int idx = x + (y / 8) * 128;
    if (on) oled_buffer[idx] |= (1 << (y % 8));
    else oled_buffer[idx] &= ~(1 << (y % 8));
}

// Fonte 5x7 simples
static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, // espaço
    0x00,0x00,0x5F,0x00,0x00, // !
    0x00,0x07,0x00,0x07,0x00, // "
    0x14,0x7F,0x14,0x7F,0x14, // #
    0x24,0x2A,0x7F,0x2A,0x12, // $
    0x23,0x13,0x08,0x64,0x62, // %
    0x36,0x49,0x56,0x20,0x50, // &
    0x00,0x08,0x07,0x03,0x00, // '
    0x00,0x1C,0x22,0x41,0x00, // (
    0x00,0x41,0x22,0x1C,0x00, // )
    0x2A,0x1C,0x7F,0x1C,0x2A, // *
    0x08,0x08,0x3E,0x08,0x08, // +
    0x00,0x80,0x70,0x30,0x00, // ,
    0x08,0x08,0x08,0x08,0x08, // -
    0x00,0x00,0x60,0x60,0x00, // .
    0x20,0x10,0x08,0x04,0x02, // /
    0x3E,0x51,0x49,0x45,0x3E, // 0
    0x00,0x42,0x7F,0x40,0x00, // 1
    0x72,0x49,0x49,0x49,0x46, // 2
    0x21,0x41,0x49,0x4D,0x33, // 3
    0x18,0x14,0x12,0x7F,0x10, // 4
    0x27,0x45,0x45,0x45,0x39, // 5
    0x3C,0x4A,0x49,0x49,0x31, // 6
    0x41,0x21,0x11,0x09,0x07, // 7
    0x36,0x49,0x49,0x49,0x36, // 8
    0x46,0x49,0x49,0x29,0x1E, // 9
    0x00,0x00,0x14,0x00,0x00, // :
    0x00,0x40,0x34,0x00,0x00, // ;
    0x00,0x08,0x14,0x22,0x41, // <
    0x14,0x14,0x14,0x14,0x14, // =
    0x00,0x41,0x22,0x14,0x08, // >
    0x02,0x01,0x59,0x09,0x06, // ?
    0x3E,0x41,0x5D,0x59,0x4E, // @
    0x7C,0x12,0x11,0x12,0x7C, // A
    0x7F,0x49,0x49,0x49,0x36, // B
    0x3E,0x41,0x41,0x41,0x22, // C
    0x7F,0x41,0x41,0x41,0x3E, // D
    0x7F,0x49,0x49,0x49,0x41, // E
    0x7F,0x09,0x09,0x09,0x01, // F
    0x3E,0x41,0x41,0x51,0x73, // G
    0x7F,0x08,0x08,0x08,0x7F, // H
    0x00,0x41,0x7F,0x41,0x00, // I
    0x20,0x40,0x41,0x3F,0x01, // J
    0x7F,0x08,0x14,0x22,0x41, // K
    0x7F,0x40,0x40,0x40,0x40, // L
    0x7F,0x02,0x1C,0x02,0x7F, // M
    0x7F,0x04,0x08,0x10,0x7F, // N
    0x3E,0x41,0x41,0x41,0x3E, // O
    0x7F,0x09,0x09,0x09,0x06, // P
    0x3E,0x41,0x51,0x21,0x5E, // Q
    0x7F,0x09,0x19,0x29,0x46, // R
    0x26,0x49,0x49,0x49,0x32, // S
    0x03,0x01,0x7F,0x01,0x03, // T
    0x3F,0x40,0x40,0x40,0x3F, // U
    0x1F,0x20,0x40,0x20,0x1F, // V
    0x3F,0x40,0x38,0x40,0x3F, // W
    0x63,0x14,0x08,0x14,0x63, // X
    0x03,0x04,0x78,0x04,0x03, // Y
    0x61,0x59,0x49,0x4D,0x43, // Z
};

void oled_char(int x, int y, char c) {
    if (c < 32 || c > 90) c = '?';
    int idx = (c - 32) * 5;
    for (int i = 0; i < 5; i++) {
        uint8_t col = font5x7[idx + i];
        for (int j = 0; j < 7; j++) {
            oled_pixel(x + i, y + j, col & (1 << j));
        }
    }
}

void oled_text(int x, int y, const char* str) {
    while (*str) {
        oled_char(x, y, *str++);
        x += 6;
    }
}

// ============================================================
// Servo Motor
// ============================================================

void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 64.0f);
    pwm_set_wrap(slice, 39062);
    pwm_set_enabled(slice, true);
}

void servo_angle(int angle) {
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    uint chan = pwm_gpio_to_channel(SERVO_PIN);
    int pulse = 1953 + (angle * 1953) / 180;
    pwm_set_chan_level(slice, chan, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

// ============================================================
// MAIN
// ============================================================

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n");
    printf("========================================\n");
    printf("  HydroSense v4 - Sistema Completo\n");
    printf("  Sensores: GPIO2/3  OLED: GPIO14/15\n");
    printf("========================================\n\n");
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Inicializa OLED primeiro
    printf("Inicializando OLED...\n");
    if (oled_init()) {
        printf("  OLED: OK\n");
        oled_clear();
        oled_text(10, 0, "HYDROSENSE V4");
        oled_text(0, 20, "INICIANDO...");
        oled_update();
    } else {
        printf("  OLED: ERRO\n");
    }
    
    sleep_ms(1000);
    
    // Inicializa sensores
    printf("Inicializando sensores...\n");
    aht_ok = aht10_init();
    printf("  AHT10: %s\n", aht_ok ? "OK" : "ERRO");
    
    vl53_ok = vl53_init();
    printf("  VL53: %s\n", vl53_ok ? "OK" : "ERRO");
    
    // Servo teste
    printf("Testando servo...\n");
    servo_init();
    servo_angle(90);
    sleep_ms(300);
    servo_angle(0);
    sleep_ms(300);
    servo_stop();
    printf("  Servo: OK (parado)\n");
    
    printf("\n=== MONITORAMENTO INICIADO ===\n");
    
    int ciclo = 0;
    while (1) {
        ciclo++;
        gpio_put(LED_PIN, ciclo % 2);
        
        // Lê sensores
        if (aht_ok) aht10_read();
        if (vl53_ok) distancia_mm = vl53_read_mm();
        
        // Mostra na serial
        printf("[%d] T=%.1fC H=%.0f%% D=%dmm\n", 
               ciclo, temperatura, umidade, 
               distancia_mm == 0xFFFF ? 0 : distancia_mm);
        
        // Atualiza OLED
        oled_clear();
        oled_text(10, 0, "HYDROSENSE V4");
        
        char buf[24];
        snprintf(buf, sizeof(buf), "TEMP: %.1fC", temperatura);
        oled_text(0, 16, buf);
        
        snprintf(buf, sizeof(buf), "UMID: %.0f%%", umidade);
        oled_text(0, 26, buf);
        
        if (distancia_mm != 0xFFFF) {
            snprintf(buf, sizeof(buf), "DIST: %dmm", distancia_mm);
        } else {
            snprintf(buf, sizeof(buf), "DIST: ---");
        }
        oled_text(0, 40, buf);
        
        snprintf(buf, sizeof(buf), "CICLO: %d", ciclo);
        oled_text(0, 54, buf);
        
        oled_update();
        
        sleep_ms(2000);
    }
}
