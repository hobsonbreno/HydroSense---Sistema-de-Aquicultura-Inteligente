/**
 * HydroSense - DIAGNÓSTICO COMPLETO DE HARDWARE
 * 
 * Testa TODOS os componentes físicos:
 * - Display OLED SSD1306 (I2C)
 * - Servo Motor SG90 (PWM)
 * - Sensor VL53L0X (I2C)
 * - Sensor AHT10 (I2C)
 * - Sensor TCS3200 (GPIO)
 * - Bombas
 * - Scanner I2C
 * 
 * IMPORTANTE: Verifique as conexões físicas!
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include <stdio.h>
#include <string.h>

// LED interno do Pico W - acessamos diretamente pelo GPIO 25
#define LED_PIN 25

// ============================================================
// CONFIGURAÇÃO DE PINOS - AJUSTE CONFORME SEU HARDWARE!
// ============================================================

// I2C0 - VL53L0X (Sensor de distância/nível)
#define I2C0_SDA_PIN        4
#define I2C0_SCL_PIN        5

// I2C1 - AHT10 + OLED (Compartilhados)
#define I2C1_SDA_PIN        14
#define I2C1_SCL_PIN        15

// Endereços I2C
#define VL53L0X_ADDR        0x29
#define AHT10_ADDR          0x38
#define SSD1306_ADDR        0x3C  // Pode ser 0x3D em alguns displays

// Servo Motor
#define SERVO_PIN           16

// TCS3200 (Sensor de Cor)
#define TCS_S0_PIN          8
#define TCS_S1_PIN          9
#define TCS_S2_PIN          10
#define TCS_S3_PIN          11
#define TCS_OUT_PIN         12

// Bombas
#define BOMBA1_PIN          17
#define BOMBA2_PIN          19

// ============================================================
// Variáveis Globais
// ============================================================
static bool i2c0_ok = false;
static bool i2c1_ok = false;
static bool vl53_found = false;
static bool aht10_found = false;
static bool oled_found = false;
static bool servo_ok = false;

// ============================================================
// Scanner I2C - Detecta TODOS os dispositivos
// ============================================================
void scan_i2c_bus(i2c_inst_t* i2c, const char* name) {
    printf("\n📡 Escaneando barramento %s...\n", name);
    printf("    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    
    int devices_found = 0;
    
    for (int addr = 0; addr < 128; addr++) {
        if (addr % 16 == 0) {
            printf("%02X: ", addr);
        }
        
        uint8_t data;
        int ret = i2c_read_blocking(i2c, addr, &data, 1, false);
        
        if (ret >= 0) {
            printf("%02X ", addr);
            devices_found++;
            
            // Identifica dispositivos conhecidos
            if (addr == VL53L0X_ADDR) {
                printf("[VL53L0X] ");
                vl53_found = true;
            }
            if (addr == AHT10_ADDR) {
                printf("[AHT10] ");
                aht10_found = true;
            }
            if (addr == SSD1306_ADDR || addr == 0x3D) {
                printf("[OLED] ");
                oled_found = true;
            }
        } else {
            printf("-- ");
        }
        
        if (addr % 16 == 15) {
            printf("\n");
        }
    }
    
    printf("\n✅ %d dispositivo(s) encontrado(s) no %s\n", devices_found, name);
}

// ============================================================
// Inicializa I2C
// ============================================================
bool init_i2c_buses(void) {
    printf("\n🔧 Inicializando barramentos I2C...\n");
    
    // I2C0 para VL53L0X
    i2c_init(i2c0, 100000);  // 100kHz
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
    printf("   I2C0: SDA=GPIO%d, SCL=GPIO%d (100kHz)\n", I2C0_SDA_PIN, I2C0_SCL_PIN);
    i2c0_ok = true;
    
    sleep_ms(100);
    
    // I2C1 para AHT10 + OLED
    i2c_init(i2c1, 100000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);
    printf("   I2C1: SDA=GPIO%d, SCL=GPIO%d (100kHz)\n", I2C1_SDA_PIN, I2C1_SCL_PIN);
    i2c1_ok = true;
    
    sleep_ms(100);
    
    return true;
}

// ============================================================
// Testa VL53L0X (Sensor de Distância/Nível)
// ============================================================
bool test_vl53l0x(void) {
    printf("\n🔬 Testando VL53L0X (Sensor de Nível)...\n");
    
    if (!vl53_found) {
        printf("   ❌ VL53L0X NÃO DETECTADO no I2C0!\n");
        printf("   📌 Verifique conexões:\n");
        printf("      VCC → 3.3V\n");
        printf("      GND → GND\n");
        printf("      SDA → GPIO%d\n", I2C0_SDA_PIN);
        printf("      SCL → GPIO%d\n", I2C0_SCL_PIN);
        return false;
    }
    
    // Tenta ler o ID do sensor
    uint8_t reg = 0xC0;  // Registro de identificação
    uint8_t id = 0;
    
    int ret = i2c_write_blocking(i2c0, VL53L0X_ADDR, &reg, 1, true);
    if (ret < 0) {
        printf("   ❌ Falha ao escrever no VL53L0X\n");
        return false;
    }
    
    ret = i2c_read_blocking(i2c0, VL53L0X_ADDR, &id, 1, false);
    if (ret < 0) {
        printf("   ❌ Falha ao ler ID do VL53L0X\n");
        return false;
    }
    
    printf("   📊 ID do sensor: 0x%02X ", id);
    if (id == 0xEE) {
        printf("(Correto! É um VL53L0X)\n");
        printf("   ✅ VL53L0X funcionando!\n");
        return true;
    } else {
        printf("(Esperado: 0xEE)\n");
        printf("   ⚠️ ID incorreto, mas sensor responde\n");
        return true;
    }
}

// Inicialização básica do VL53L0X para leitura
uint16_t vl53l0x_read_distance(void) {
    if (!vl53_found) return 0xFFFF;
    
    uint8_t data[2];
    uint8_t reg;
    
    // Inicia medição single-shot
    reg = 0x00;
    uint8_t cmd[] = {reg, 0x01};
    i2c_write_blocking(i2c0, VL53L0X_ADDR, cmd, 2, false);
    sleep_ms(50);
    
    // Lê resultado
    reg = 0x14 + 10;  // Registro de resultado
    i2c_write_blocking(i2c0, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c0, VL53L0X_ADDR, data, 2, false);
    
    return (data[0] << 8) | data[1];
}

// ============================================================
// Testa AHT10 (Temperatura e Umidade)
// ============================================================
bool test_aht10(void) {
    printf("\n🌡️ Testando AHT10 (Temp/Umidade)...\n");
    
    if (!aht10_found) {
        printf("   ❌ AHT10 NÃO DETECTADO no I2C1!\n");
        printf("   📌 Verifique conexões:\n");
        printf("      VCC → 3.3V\n");
        printf("      GND → GND\n");
        printf("      SDA → GPIO%d\n", I2C1_SDA_PIN);
        printf("      SCL → GPIO%d\n", I2C1_SCL_PIN);
        return false;
    }
    
    // Comando de calibração
    uint8_t init_cmd[] = {0xE1, 0x08, 0x00};
    int ret = i2c_write_blocking(i2c1, AHT10_ADDR, init_cmd, 3, false);
    if (ret < 0) {
        printf("   ❌ Falha na inicialização\n");
        return false;
    }
    sleep_ms(10);
    
    // Trigger leitura
    uint8_t measure_cmd[] = {0xAC, 0x33, 0x00};
    ret = i2c_write_blocking(i2c1, AHT10_ADDR, measure_cmd, 3, false);
    if (ret < 0) {
        printf("   ❌ Falha ao iniciar medição\n");
        return false;
    }
    sleep_ms(80);
    
    // Lê dados
    uint8_t data[6];
    ret = i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false);
    if (ret < 0) {
        printf("   ❌ Falha ao ler dados\n");
        return false;
    }
    
    // Calcula valores
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    float humidity = (float)hum_raw / 1048576.0f * 100.0f;
    float temperature = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    
    printf("   📊 Status byte: 0x%02X\n", data[0]);
    printf("   🌡️ Temperatura: %.2f °C\n", temperature);
    printf("   💧 Umidade: %.2f %%\n", humidity);
    
    if (temperature > -20 && temperature < 80 && humidity > 0 && humidity < 100) {
        printf("   ✅ AHT10 funcionando!\n");
        return true;
    } else {
        printf("   ⚠️ Valores fora do esperado\n");
        return false;
    }
}

// ============================================================
// Testa Display OLED SSD1306
// ============================================================
bool test_oled(void) {
    printf("\n📺 Testando Display OLED SSD1306...\n");
    
    if (!oled_found) {
        printf("   ❌ OLED NÃO DETECTADO no I2C1!\n");
        printf("   📌 Verifique conexões:\n");
        printf("      VCC → 3.3V\n");
        printf("      GND → GND\n");
        printf("      SDA → GPIO%d\n", I2C1_SDA_PIN);
        printf("      SCL → GPIO%d\n", I2C1_SCL_PIN);
        printf("   📌 Endereços comuns: 0x3C ou 0x3D\n");
        return false;
    }
    
    // Sequência de inicialização básica do SSD1306
    uint8_t init_cmds[] = {
        0x00,       // Command mode
        0xAE,       // Display off
        0xD5, 0x80, // Set display clock
        0xA8, 0x3F, // Set multiplex ratio (64 linhas)
        0xD3, 0x00, // Set display offset
        0x40,       // Set start line
        0x8D, 0x14, // Enable charge pump
        0x20, 0x00, // Memory mode horizontal
        0xA1,       // Segment remap
        0xC8,       // COM output scan direction
        0xDA, 0x12, // COM pins
        0x81, 0xCF, // Set contrast
        0xD9, 0xF1, // Pre-charge period
        0xDB, 0x40, // VCOMH deselect level
        0xA4,       // Display RAM content
        0xA6,       // Normal display (not inverted)
        0xAF        // Display on
    };
    
    int ret = i2c_write_blocking(i2c1, SSD1306_ADDR, init_cmds, sizeof(init_cmds), false);
    if (ret < 0) {
        printf("   ❌ Falha na inicialização do display\n");
        return false;
    }
    
    sleep_ms(100);
    
    // Limpa display (preenche com zeros)
    printf("   🧹 Limpando display...\n");
    uint8_t clear_cmd[2] = {0x00, 0x21};  // Set column address
    i2c_write_blocking(i2c1, SSD1306_ADDR, clear_cmd, 2, false);
    
    uint8_t col_range[] = {0x00, 0x21, 0, 127};  // Column 0-127
    i2c_write_blocking(i2c1, SSD1306_ADDR, col_range, 4, false);
    
    uint8_t page_range[] = {0x00, 0x22, 0, 7};   // Page 0-7
    i2c_write_blocking(i2c1, SSD1306_ADDR, page_range, 4, false);
    
    // Envia dados em branco
    uint8_t blank[17];
    blank[0] = 0x40;  // Data mode
    memset(&blank[1], 0x00, 16);
    
    for (int i = 0; i < 64; i++) {
        i2c_write_blocking(i2c1, SSD1306_ADDR, blank, 17, false);
    }
    
    sleep_ms(100);
    
    // Desenha padrão de teste (bordas)
    printf("   🎨 Desenhando padrão de teste...\n");
    
    // Preenche primeira linha com pixels
    memset(&blank[1], 0xFF, 16);  // Linha branca
    i2c_write_blocking(i2c1, SSD1306_ADDR, blank, 17, false);
    
    printf("   ✅ OLED inicializado!\n");
    printf("   👀 Verifique se o display acendeu\n");
    return true;
}

// ============================================================
// Testa Servo Motor
// ============================================================
void test_servo(void) {
    printf("\n⚙️ Testando Servo Motor SG90 (GPIO%d)...\n", SERVO_PIN);
    
    // Configura PWM para servo (50Hz, 20ms período)
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    
    // 125MHz / 125 = 1MHz, wrap 20000 = 50Hz (20ms)
    pwm_set_clkdiv(slice, 125.0f);
    pwm_set_wrap(slice, 20000);
    pwm_set_enabled(slice, true);
    
    printf("   📌 PWM configurado: 50Hz, período 20ms\n");
    printf("   🔄 Movendo servo...\n");
    
    // 0° = 500us (duty 500)
    // 90° = 1500us (duty 1500)
    // 180° = 2500us (duty 2500)
    
    printf("   ➡️ Posição 0° (500us)...\n");
    pwm_set_gpio_level(SERVO_PIN, 500);
    sleep_ms(1000);
    
    printf("   ➡️ Posição 90° (1500us)...\n");
    pwm_set_gpio_level(SERVO_PIN, 1500);
    sleep_ms(1000);
    
    printf("   ➡️ Posição 180° (2500us)...\n");
    pwm_set_gpio_level(SERVO_PIN, 2500);
    sleep_ms(1000);
    
    printf("   ➡️ Voltando para 0°...\n");
    pwm_set_gpio_level(SERVO_PIN, 500);
    sleep_ms(500);
    
    // Desabilita servo para evitar tremor
    pwm_set_gpio_level(SERVO_PIN, 0);
    
    printf("   ✅ Teste do servo concluído!\n");
    printf("   👀 O servo deveria ter girado 0° → 90° → 180° → 0°\n");
    servo_ok = true;
}

// ============================================================
// Testa Bombas
// ============================================================
void test_bombas(void) {
    printf("\n💧 Testando Bombas (GPIO%d e GPIO%d)...\n", BOMBA1_PIN, BOMBA2_PIN);
    
    gpio_init(BOMBA1_PIN);
    gpio_init(BOMBA2_PIN);
    gpio_set_dir(BOMBA1_PIN, GPIO_OUT);
    gpio_set_dir(BOMBA2_PIN, GPIO_OUT);
    
    printf("   🔌 Bomba 1 ON por 1 segundo...\n");
    gpio_put(BOMBA1_PIN, 1);
    sleep_ms(1000);
    gpio_put(BOMBA1_PIN, 0);
    
    sleep_ms(500);
    
    printf("   🔌 Bomba 2 ON por 1 segundo...\n");
    gpio_put(BOMBA2_PIN, 1);
    sleep_ms(1000);
    gpio_put(BOMBA2_PIN, 0);
    
    printf("   ✅ Teste das bombas concluído!\n");
    printf("   👀 As bombas deveriam ter ligado brevemente\n");
}

// ============================================================
// Testa TCS3200 (Sensor de Cor/Turbidez)
// ============================================================
void test_tcs3200(void) {
    printf("\n🎨 Testando TCS3200 (Sensor de Cor/Turbidez)...\n");
    printf("   📌 Pinos: S0=%d, S1=%d, S2=%d, S3=%d, OUT=%d\n",
           TCS_S0_PIN, TCS_S1_PIN, TCS_S2_PIN, TCS_S3_PIN, TCS_OUT_PIN);
    
    // Inicializa pinos
    gpio_init(TCS_S0_PIN);
    gpio_init(TCS_S1_PIN);
    gpio_init(TCS_S2_PIN);
    gpio_init(TCS_S3_PIN);
    gpio_init(TCS_OUT_PIN);
    
    gpio_set_dir(TCS_S0_PIN, GPIO_OUT);
    gpio_set_dir(TCS_S1_PIN, GPIO_OUT);
    gpio_set_dir(TCS_S2_PIN, GPIO_OUT);
    gpio_set_dir(TCS_S3_PIN, GPIO_OUT);
    gpio_set_dir(TCS_OUT_PIN, GPIO_IN);
    
    // Frequência 20% (S0=H, S1=L)
    gpio_put(TCS_S0_PIN, 1);
    gpio_put(TCS_S1_PIN, 0);
    
    // Lê cada canal de cor
    const char* cores[] = {"Vermelho", "Verde", "Azul", "Clear"};
    uint8_t s2_vals[] = {0, 1, 0, 1};
    uint8_t s3_vals[] = {0, 1, 1, 0};
    
    for (int c = 0; c < 4; c++) {
        gpio_put(TCS_S2_PIN, s2_vals[c]);
        gpio_put(TCS_S3_PIN, s3_vals[c]);
        sleep_ms(10);
        
        // Conta pulsos por 100ms
        uint32_t start = to_ms_since_boot(get_absolute_time());
        int pulsos = 0;
        bool last_state = gpio_get(TCS_OUT_PIN);
        
        while (to_ms_since_boot(get_absolute_time()) - start < 100) {
            bool state = gpio_get(TCS_OUT_PIN);
            if (state && !last_state) {  // Rising edge
                pulsos++;
            }
            last_state = state;
        }
        
        printf("   🔴 %s: %d pulsos/100ms\n", cores[c], pulsos);
    }
    
    printf("   ✅ Teste do TCS3200 concluído!\n");
    printf("   📌 Se todos os valores são 0, verifique conexões\n");
}

// ============================================================
// Relatório Final
// ============================================================
void print_final_report(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           📋 RELATÓRIO DE DIAGNÓSTICO                     ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║ Componente          │ Status                              ║\n");
    printf("╠═════════════════════╪═════════════════════════════════════╣\n");
    printf("║ I2C0 (VL53L0X)      │ %s                               ║\n", i2c0_ok ? "✅ OK" : "❌ FALHA");
    printf("║ I2C1 (AHT10+OLED)   │ %s                               ║\n", i2c1_ok ? "✅ OK" : "❌ FALHA");
    printf("║ VL53L0X (Nível)     │ %s                               ║\n", vl53_found ? "✅ DETECTADO" : "❌ NÃO ENCONTRADO");
    printf("║ AHT10 (Temp/Umid)   │ %s                               ║\n", aht10_found ? "✅ DETECTADO" : "❌ NÃO ENCONTRADO");
    printf("║ OLED SSD1306        │ %s                               ║\n", oled_found ? "✅ DETECTADO" : "❌ NÃO ENCONTRADO");
    printf("║ Servo Motor         │ %s                               ║\n", servo_ok ? "✅ TESTADO" : "⚠️ VERIFICAR");
    printf("╚═════════════════════╧═════════════════════════════════════╝\n");
    
    printf("\n📌 PINAGEM USADA:\n");
    printf("┌──────────────┬─────────┬──────────────────────┐\n");
    printf("│ Componente   │ Pino    │ Função               │\n");
    printf("├──────────────┼─────────┼──────────────────────┤\n");
    printf("│ VL53L0X SDA  │ GPIO%02d  │ I2C0 Data           │\n", I2C0_SDA_PIN);
    printf("│ VL53L0X SCL  │ GPIO%02d  │ I2C0 Clock          │\n", I2C0_SCL_PIN);
    printf("│ AHT10 SDA    │ GPIO%02d │ I2C1 Data           │\n", I2C1_SDA_PIN);
    printf("│ AHT10 SCL    │ GPIO%02d │ I2C1 Clock          │\n", I2C1_SCL_PIN);
    printf("│ OLED SDA     │ GPIO%02d │ I2C1 Data (compart.)│\n", I2C1_SDA_PIN);
    printf("│ OLED SCL     │ GPIO%02d │ I2C1 Clock (compart)│\n", I2C1_SCL_PIN);
    printf("│ Servo PWM    │ GPIO%02d │ PWM                 │\n", SERVO_PIN);
    printf("│ Bomba 1      │ GPIO%02d │ Digital OUT         │\n", BOMBA1_PIN);
    printf("│ Bomba 2      │ GPIO%02d │ Digital OUT         │\n", BOMBA2_PIN);
    printf("│ TCS3200 S0   │ GPIO%02d  │ Digital OUT         │\n", TCS_S0_PIN);
    printf("│ TCS3200 S1   │ GPIO%02d  │ Digital OUT         │\n", TCS_S1_PIN);
    printf("│ TCS3200 S2   │ GPIO%02d │ Digital OUT         │\n", TCS_S2_PIN);
    printf("│ TCS3200 S3   │ GPIO%02d │ Digital OUT         │\n", TCS_S3_PIN);
    printf("│ TCS3200 OUT  │ GPIO%02d │ Digital IN          │\n", TCS_OUT_PIN);
    printf("└──────────────┴─────────┴──────────────────────┘\n");
}

// ============================================================
// Loop de Leitura Contínua
// ============================================================
void continuous_reading_loop(void) {
    printf("\n🔄 Iniciando leitura contínua dos sensores...\n");
    printf("   Pressione Ctrl+C para parar\n\n");
    
    while (1) {
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        
        // VL53L0X
        if (vl53_found) {
            uint16_t dist = vl53l0x_read_distance();
            if (dist != 0xFFFF && dist < 8000) {
                float nivel_cm = 30.0f - (dist / 10.0f);
                if (nivel_cm < 0) nivel_cm = 0;
                if (nivel_cm > 30) nivel_cm = 30;
                float nivel_l = (nivel_cm / 30.0f) * 20.0f;
                printf("🌊 Nível: %.1f cm (%.1f L, %.0f%%) [dist: %d mm]\n", 
                       nivel_cm, nivel_l, (nivel_l/20.0f)*100, dist);
            } else {
                printf("🌊 Nível: Erro de leitura\n");
            }
        } else {
            printf("🌊 Nível: VL53L0X não conectado\n");
        }
        
        // AHT10
        if (aht10_found) {
            uint8_t measure_cmd[] = {0xAC, 0x33, 0x00};
            i2c_write_blocking(i2c1, AHT10_ADDR, measure_cmd, 3, false);
            sleep_ms(80);
            
            uint8_t data[6];
            i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false);
            
            uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
            uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
            
            float humidity = (float)hum_raw / 1048576.0f * 100.0f;
            float temperature = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
            
            printf("🌡️ Temperatura: %.2f °C\n", temperature);
            printf("💧 Umidade: %.2f %%\n", humidity);
        } else {
            printf("🌡️ Temp/Umid: AHT10 não conectado\n");
        }
        
        printf("\n");
        sleep_ms(2000);
        
        // Pisca LED
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
    }
}

// ============================================================
// Main
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(3000);  // Aguarda terminal
    
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  🔧 HydroSense - DIAGNÓSTICO DE HARDWARE v1.0             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n⏳ Aguarde enquanto testamos os componentes...\n");
    
    // Inicializa LED interno
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // 1. Inicializa I2C
    init_i2c_buses();
    
    // 2. Escaneia barramentos I2C
    scan_i2c_bus(i2c0, "I2C0");
    scan_i2c_bus(i2c1, "I2C1");
    
    // 3. Testa sensores I2C
    test_vl53l0x();
    test_aht10();
    test_oled();
    
    // 4. Testa atuadores
    test_servo();
    test_bombas();
    
    // 5. Testa sensor de cor
    test_tcs3200();
    
    // 6. Relatório final
    print_final_report();
    
    // 7. Inicia leitura contínua
    continuous_reading_loop();
    
    return 0;
}
