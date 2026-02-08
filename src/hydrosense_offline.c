/**
 * HydroSense v8 - Versão OFFLINE
 * Sistema completo SEM WiFi para demonstração
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ============================================================
// PINOS - BitDogLab
// ============================================================
// I2C Sensores (via extensor I2C)
#define SENSOR_SDA      2
#define SENSOR_SCL      3

// I2C OLED (direto na BitDogLab)
#define OLED_SDA        14
#define OLED_SCL        15

// Servo
#define SERVO_PIN       16

// LED RGB
#define LED_R_PIN       11
#define LED_G_PIN       12
#define LED_B_PIN       13

// Buzzer
#define BUZZER_PIN      21

// Botões
#define BTN_A_PIN       5
#define BTN_B_PIN       6

// ============================================================
// ENDEREÇOS I2C
// ============================================================
#define AHT10_ADDR      0x38
#define VL53L0X_ADDR    0x29
#define OLED_ADDR       0x3C

// ============================================================
// CONFIGURAÇÃO DO TANQUE
// ============================================================
#define TANK_HEIGHT_CM  30.0f
#define TANK_CAPACITY_L 20.0f
#define SENSOR_OFFSET_CM 2.0f

// ============================================================
// VARIÁVEIS GLOBAIS
// ============================================================
static int current_i2c_mode = 0;
static volatile float g_temperatura = 0.0f;
static volatile float g_umidade = 0.0f;
static volatile uint16_t g_distancia_mm = 0;
static volatile float g_nivel_percent = 0.0f;
static volatile float g_volume_litros = 0.0f;
static volatile bool g_aht_ok = false;
static volatile bool g_vl53_ok = false;
static volatile bool g_oled_ok = false;
static volatile uint32_t g_leitura_count = 0;

// ============================================================
// I2C SWITCHING
// ============================================================

void i2c_switch_to_sensors(void) {
    if (current_i2c_mode == 1) return;
    
    gpio_set_function(OLED_SDA, GPIO_FUNC_NULL);
    gpio_set_function(OLED_SCL, GPIO_FUNC_NULL);
    
    i2c_deinit(i2c1);
    i2c_init(i2c1, 100000);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_us(100);
    
    current_i2c_mode = 1;
}

void i2c_switch_to_oled(void) {
    if (current_i2c_mode == 2) return;
    
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_NULL);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_NULL);
    
    i2c_deinit(i2c1);
    i2c_init(i2c1, 400000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_us(100);
    
    current_i2c_mode = 2;
}

// ============================================================
// LED RGB
// ============================================================
void led_rgb_init(void) {
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
}

void led_rgb_set(int r, int g, int b) {
    gpio_put(LED_R_PIN, r);
    gpio_put(LED_G_PIN, g);
    gpio_put(LED_B_PIN, b);
}

// ============================================================
// BUZZER
// ============================================================
void buzzer_init(void) {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

void buzzer_beep(int ms) {
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(ms);
    gpio_put(BUZZER_PIN, 0);
}

// ============================================================
// SERVO
// ============================================================
void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, 20000);
    pwm_init(slice, &config, true);
}

void servo_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    uint16_t pulse = 1000 + (angle * 1000 / 180);
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

// ============================================================
// AHT10 - Temperatura e Umidade
// ============================================================
bool aht10_init(void) {
    i2c_switch_to_sensors();
    
    uint8_t cmd[] = {0xBE, 0x08, 0x00};
    int ret = i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false);
    sleep_ms(10);
    
    return ret == 3;
}

bool aht10_read(float *temp, float *hum) {
    i2c_switch_to_sensors();
    
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) != 3) {
        return false;
    }
    
    sleep_ms(80);
    
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) != 6) {
        return false;
    }
    
    uint32_t humidity_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    *hum = (float)humidity_raw * 100.0f / 1048576.0f;
    *temp = (float)temp_raw * 200.0f / 1048576.0f - 50.0f;
    
    if (*hum > 100.0f) *hum = 100.0f;
    if (*hum < 0.0f) *hum = 0.0f;
    
    return true;
}

// ============================================================
// VL53L0X - Distância
// ============================================================
bool vl53_init(void) {
    i2c_switch_to_sensors();
    
    uint8_t test_reg = 0x00;
    uint8_t chip_id = 0;
    
    if (i2c_write_blocking(i2c1, VL53L0X_ADDR, &test_reg, 1, true) != 1) return false;
    if (i2c_read_blocking(i2c1, VL53L0X_ADDR, &chip_id, 1, false) != 1) return false;
    
    return chip_id == 0xEE;
}

uint16_t vl53_read_distance(void) {
    i2c_switch_to_sensors();
    
    uint8_t range_start = 0x00;
    uint8_t start_cmd = 0x01;
    
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &range_start, 1, true);
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &start_cmd, 1, false);
    
    sleep_ms(30);
    
    uint8_t result_reg = 0x1E;
    uint8_t distance_data[2];
    
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &result_reg, 1, true);
    if (i2c_read_blocking(i2c1, VL53L0X_ADDR, distance_data, 2, false) == 2) {
        return (distance_data[0] << 8) | distance_data[1];
    }
    
    return 8190;  // Erro
}

// ============================================================
// OLED BÁSICO
// ============================================================
bool oled_init(void) {
    i2c_switch_to_oled();
    
    uint8_t init_cmds[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81,
        0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    
    return i2c_write_blocking(i2c1, OLED_ADDR, init_cmds, sizeof(init_cmds), false) == sizeof(init_cmds);
}

void oled_clear(void) {
    i2c_switch_to_oled();
    // Implementação básica
}

void oled_print(int x, int y, const char* text) {
    // Implementação básica
    printf("OLED[%d,%d]: %s\n", x, y, text);
}

void oled_update(void) {
    // Implementação básica
}

// ============================================================
// CÁLCULOS
// ============================================================
void calculate_water_level(uint16_t distance_mm) {
    float distance_cm = distance_mm / 10.0f;
    float water_depth = TANK_HEIGHT_CM - distance_cm + SENSOR_OFFSET_CM;
    
    if (water_depth < 0) water_depth = 0;
    if (water_depth > TANK_HEIGHT_CM) water_depth = TANK_HEIGHT_CM;
    
    g_nivel_percent = (water_depth / TANK_HEIGHT_CM) * 100.0f;
    g_volume_litros = (g_nivel_percent / 100.0f) * TANK_CAPACITY_L;
}

// ============================================================
// MAIN
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    printf("\n\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     HydroSense v8 - OFFLINE MODE      ║\n");
    printf("║   Sistema de Aquicultura Inteligente   ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf(">>> Sistema OFFLINE iniciado! <<<\n\n");
    
    // Inicializa hardware
    printf("🔧 Inicializando hardware...\n");
    
    led_rgb_init();
    led_rgb_set(1, 0, 0);
    
    buzzer_init();
    buzzer_beep(200);
    
    servo_init();
    
    // Inicializa OLED
    printf("   📺 OLED: ");
    g_oled_ok = oled_init();
    printf("%s\n", g_oled_ok ? "OK" : "ERRO");
    
    // Inicializa sensores
    printf("   🌡️ AHT10: ");
    g_aht_ok = aht10_init();
    printf("%s\n", g_aht_ok ? "OK" : "ERRO");
    
    printf("   📏 VL53L0X: ");
    g_vl53_ok = vl53_init();
    printf("%s\n", g_vl53_ok ? "OK" : "ERRO");
    
    // LED verde = sistema pronto
    led_rgb_set(0, 1, 0);
    buzzer_beep(100);
    sleep_ms(100);
    buzzer_beep(100);
    
    printf("\n=== MONITORAMENTO INICIADO ===\n\n");
    
    // Loop principal
    while (true) {
        g_leitura_count++;
        
        // Lê sensores
        if (g_aht_ok) {
            aht10_read(&g_temperatura, &g_umidade);
        }
        
        if (g_vl53_ok) {
            g_distancia_mm = vl53_read_distance();
            calculate_water_level(g_distancia_mm);
        }
        
        // Atualiza display
        printf("[%lu] T=%.1f°C H=%.0f%% D=%dmm N=%.0f%% V=%.1fL\n",
               g_leitura_count, g_temperatura, g_umidade,
               g_distancia_mm, g_nivel_percent, g_volume_litros);
        
        // LED indica status
        if (g_nivel_percent < 20) {
            led_rgb_set(1, 0, 0);  // Vermelho - nível baixo
            buzzer_beep(50);
        } else if (g_nivel_percent < 50) {
            led_rgb_set(1, 1, 0);  // Amarelo - nível médio
        } else {
            led_rgb_set(0, 1, 0);  // Verde - nível OK
        }
        
        // Teste automático do servo a cada 20 leituras
        if ((g_leitura_count % 20) == 0) {
            printf("🍽️ Alimentação automática!\n");
            servo_angle(90);
            sleep_ms(1000);
            servo_angle(0);
            servo_stop();
        }
        
        sleep_ms(2000);  // Leitura a cada 2 segundos
    }
    
    return 0;
}