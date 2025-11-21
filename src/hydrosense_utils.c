#include "hydrosense_system.h"
#include "hardware/clocks.h"
#include <stdio.h>

// Implementações básicas para compilação

// NeoPixel (WS2812B) - implementação simplificada
static uint32_t neopixel_buffer[NEOPIXEL_COUNT];

void neopixel_init(void) {
    printf("✅ NeoPixel inicializado (GPIO %d, %d LEDs)\n", NEOPIXEL_PIN, NEOPIXEL_COUNT);
}

void neopixel_clear(void) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        neopixel_buffer[i] = 0;
    }
}

void neopixel_set_color(uint8_t index, rgb_color_t color) {
    if (index >= NEOPIXEL_COUNT) return;
    neopixel_buffer[index] = (color.g << 16) | (color.r << 8) | color.b;
}

void neopixel_show(void) {
    // Implementação básica - seria necessário PIO para WS2812B real
    printf("🔆 LEDs atualizados\n");
}

void neopixel_show_status_leds(void) {
    extern hydrosense_status_t system_status;
    
    neopixel_clear();
    
    // LED central (12) para status geral
    if (system_status.temperatura >= TEMP_MIN && system_status.temperatura <= TEMP_MAX &&
        system_status.ph >= PH_MIN && system_status.ph <= PH_MAX &&
        system_status.nivel_agua > NIVEL_CRITICO) {
        neopixel_set_color(12, COLOR_GREEN);
    } else if (system_status.temperatura < 20 || system_status.temperatura > 32 ||
               system_status.ph < 6.0 || system_status.ph > 8.5 ||
               system_status.nivel_agua < 15) {
        neopixel_set_color(12, COLOR_RED);
    } else {
        neopixel_set_color(12, COLOR_YELLOW);
    }
    
    // LEDs específicos para cada sensor
    neopixel_set_color(11, (system_status.temperatura >= TEMP_MIN && 
                           system_status.temperatura <= TEMP_MAX) ? COLOR_GREEN : COLOR_RED);
    neopixel_set_color(13, (system_status.ph >= PH_MIN && 
                           system_status.ph <= PH_MAX) ? COLOR_GREEN : COLOR_RED);
    neopixel_set_color(7, (system_status.nivel_agua > NIVEL_CRITICO) ? COLOR_GREEN : COLOR_RED);
    neopixel_set_color(24, system_status.wifi_conectado ? COLOR_GREEN : COLOR_RED);
    
    neopixel_show();
}

void neopixel_animacao_loading(void) {
    rgb_color_t colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA};
    
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            neopixel_clear();
            neopixel_set_color(i, colors[cycle % 6]);
            neopixel_show();
            sleep_ms(50);
        }
    }
    neopixel_clear();
}

// Buzzer
static uint buzzer_slice;

void buzzer_init(void) {
    // gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM); // DESABILITADO
    // buzzer_slice = pwm_gpio_to_slice_num(BUZZER_PIN); // DESABILITADO
    
    printf("🔇 Buzzer DESABILITADO (modo silencioso)\n");
}

void buzzer_beep(uint16_t freq, uint16_t duracao_ms) {
    // FUNÇÃO COMPLETAMENTE DESABILITADA - SEM SOM
    // printf("🔇 Buzzer silencioso: %dHz por %dms\n", freq, duracao_ms);
    return; // Não faz nada
}

void buzzer_feedback_botao(char botao) {
    // FUNÇÃO COMPLETAMENTE DESABILITADA - SEM SOM
    return; // Não faz nada
}

// Sensores
void sensores_ler_todos(void) {
    extern hydrosense_status_t system_status;
    
    system_status.temperatura = sensor_ler_temperatura();
    system_status.ph = sensor_ler_ph();
    system_status.nivel_agua = sensor_ler_nivel_agua();
}

float sensor_ler_temperatura(void) {
    // Simulação baseada no código Python
    static bool adc_init_temp = false;
    if (!adc_init_temp) {
        adc_init();
        adc_gpio_init(ADC_TEMP_PIN);
        adc_init_temp = true;
    }
    
    adc_select_input(ADC_TEMP_PIN - 26);
    uint16_t adc_value = adc_read();
    float voltage = (adc_value / 65535.0f) * 3.3f;
    float temp = 20.0f + (voltage * 10.0f) + (get_timestamp_ms() % 1000 - 500) / 10000.0f;
    return (temp < 20.0f) ? 20.0f : ((temp > 32.0f) ? 32.0f : temp);
}

float sensor_ler_ph(void) {
    static bool adc_init_ph = false;
    if (!adc_init_ph) {
        adc_gpio_init(ADC_PH_PIN);
        adc_init_ph = true;
    }
    
    adc_select_input(ADC_PH_PIN - 26);
    uint16_t adc_value = adc_read();
    float voltage = (adc_value / 65535.0f) * 3.3f;
    float ph = 7.0f - ((voltage - 1.65f) / 0.18f) + (get_timestamp_ms() % 500 - 250) / 50000.0f;
    return (ph < 5.0f) ? 5.0f : ((ph > 9.0f) ? 9.0f : ph);
}

float sensor_ler_nivel_agua(void) {
    static bool adc_init_nivel = false;
    if (!adc_init_nivel) {
        adc_gpio_init(ADC_NIVEL_PIN);
        adc_init_nivel = true;
    }
    
    adc_select_input(ADC_NIVEL_PIN - 26);
    uint16_t adc_value = adc_read();
    float nivel = (adc_value / 65535.0f) * 100.0f;
    return (nivel < 0.0f) ? 0.0f : ((nivel > 100.0f) ? 100.0f : nivel);
}

// Sistema TPA
bool tpa_iniciar(const char* motivo) {
    extern hydrosense_status_t system_status;
    
    if (system_status.tpa_em_andamento) {
        printf("⚠️ TPA já em andamento\n");
        return false;
    }
    
    system_status.tpa_em_andamento = true;
    printf("💧 Iniciando TPA - Motivo: %s\n", motivo);
    
    // Inicializa bombas
    gpio_init(BOMBA_SUJA_PIN);
    gpio_set_dir(BOMBA_SUJA_PIN, GPIO_OUT);
    gpio_init(BOMBA_LIMPA_PIN);
    gpio_set_dir(BOMBA_LIMPA_PIN, GPIO_OUT);
    
    // Fase 1: Drenar água suja
    printf("🚰 FASE 1: Drenando água suja até 25%%\n");
    gpio_put(BOMBA_SUJA_PIN, 1);
    
    // Simulação de drenagem
    for (int i = 0; i < 8; i++) {
        neopixel_clear();
        for (int j = 0; j < 25; j++) {
            neopixel_set_color(j, COLOR_CYAN);
        }
        neopixel_show();
        sleep_ms(250);
        
        if (system_status.nivel_agua <= NIVEL_CRITICO) break;
    }
    
    gpio_put(BOMBA_SUJA_PIN, 0);
    buzzer_beep(800, 300);
    
    // Fase 2: Encher com água limpa
    printf("💧 FASE 2: Adicionando água limpa até 90%%\n");
    gpio_put(BOMBA_LIMPA_PIN, 1);
    
    for (int i = 0; i < 10; i++) {
        neopixel_clear();
        for (int j = 0; j < 25; j++) {
            neopixel_set_color(j, COLOR_BLUE);
        }
        neopixel_show();
        sleep_ms(180);
        
        if (system_status.nivel_agua >= 90) break;
    }
    
    gpio_put(BOMBA_LIMPA_PIN, 0);
    buzzer_beep(1200, 300);
    
    // LEDs de conclusão
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        neopixel_set_color(i, COLOR_GREEN);
    }
    neopixel_show();
    buzzer_beep(1500, 500);
    sleep_ms(2000);
    neopixel_clear();
    
    system_status.tpa_em_andamento = false;
    printf("✅ TPA concluído!\n");
    return true;
}

bool tpa_verificar_necessario(void) {
    extern hydrosense_status_t system_status;
    
    if (system_status.tpa_em_andamento) return false;
    
    if (system_status.ph >= PH_TPA_TRIGGER) {
        printf("🚨 pH alto: %.1f >= %.1f\n", system_status.ph, PH_TPA_TRIGGER);
        return tpa_iniciar("pH ALTO AUTOMÁTICO");
    }
    
    return false;
}

// WiFi e MQTT (stubs)
bool wifi_conectar_inteligente(void) {
    printf("🌐 WiFi: Modo offline (stub)\n");
    return false;
}

bool mqtt_conectar(void) {
    printf("📡 MQTT: Não conectado (stub)\n");
    return false;
}

void mqtt_publicar_dados(void) {
    // Stub - implementação futura
}