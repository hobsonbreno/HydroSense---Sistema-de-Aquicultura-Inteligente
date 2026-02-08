#include "hydrosense_system.h"
#include "hardware/clocks.h"
#include <stdio.h>

// ============================================================
// MELHORIAS v2.2 - Filtro de Média Móvel para Sensores
// ============================================================

// Configuração do filtro de média móvel
#define FILTER_SAMPLES 10  // Número de amostras para média

// Buffers circulares para filtro de média móvel
static float temp_buffer[FILTER_SAMPLES] = {0};
static float ph_buffer[FILTER_SAMPLES] = {0};
static float nivel_buffer[FILTER_SAMPLES] = {0};
static float turbidez_buffer[FILTER_SAMPLES] = {0};
static uint8_t buffer_index = 0;
static bool buffer_cheio = false;

// Função auxiliar para calcular média móvel
static float calcular_media_movel(float* buffer, float novo_valor) {
    buffer[buffer_index] = novo_valor;
    
    int amostras = buffer_cheio ? FILTER_SAMPLES : (buffer_index + 1);
    float soma = 0;
    for (int i = 0; i < amostras; i++) {
        soma += buffer[i];
    }
    return soma / amostras;
}

// Atualiza índice do buffer circular
static void atualizar_buffer_index(void) {
    buffer_index = (buffer_index + 1) % FILTER_SAMPLES;
    if (buffer_index == 0) buffer_cheio = true;
}

// Cooldown para TPA (evita ativações repetidas)
#define TPA_COOLDOWN_MS (30 * 60 * 1000)  // 30 minutos entre TPAs
static uint32_t ultima_tpa_timestamp = 0;

// ============================================================
// NeoPixel (WS2812B) - implementação simplificada
// ============================================================
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

// ============================================================
// Leitura de todos os sensores com atualização do buffer
// ============================================================
void sensores_ler_todos(void) {
    extern hydrosense_status_t system_status;
    
    system_status.temperatura = sensor_ler_temperatura();
    system_status.ph = sensor_ler_ph();
    system_status.nivel_agua = sensor_ler_nivel_agua();
    
    // Atualiza índice do buffer circular após ler todos
    atualizar_buffer_index();
    
    // Log de debug a cada 10 leituras
    static uint8_t log_counter = 0;
    if (++log_counter >= 10) {
        log_counter = 0;
        printf("📈 Filtro: %d amostras | Buffer: %s\n", 
               buffer_cheio ? FILTER_SAMPLES : buffer_index,
               buffer_cheio ? "CHEIO" : "ENCHENDO");
    }
}

float sensor_ler_temperatura(void) {
    static bool adc_init_temp = false;
    if (!adc_init_temp) {
        adc_init();
        adc_gpio_init(ADC_TEMP_PIN);
        adc_init_temp = true;
    }
    
    adc_select_input(ADC_TEMP_PIN - 26);
    
    // Leitura múltipla para reduzir ruído (oversampling)
    uint32_t soma = 0;
    for (int i = 0; i < 4; i++) {
        soma += adc_read();
        sleep_us(100);
    }
    uint16_t adc_value = soma / 4;
    
    // Conversão para temperatura (calibração para NTC 10K)
    float voltage = (adc_value / 4095.0f) * 3.3f;
    
    // Fórmula Steinhart-Hart simplificada para NTC
    // R = 10000 * (3.3/V - 1), T = 1/(A + B*ln(R) + C*ln(R)^3) - 273.15
    // Simplificado para range de aquicultura:
    float temp_raw = 25.0f + ((voltage - 1.65f) * 15.0f);
    
    // Aplica filtro de média móvel
    float temp = calcular_media_movel(temp_buffer, temp_raw);
    
    // Limita ao range válido
    return (temp < 15.0f) ? 15.0f : ((temp > 35.0f) ? 35.0f : temp);
}

float sensor_ler_ph(void) {
    static bool adc_init_ph = false;
    if (!adc_init_ph) {
        adc_gpio_init(ADC_PH_PIN);
        adc_init_ph = true;
    }
    
    adc_select_input(ADC_PH_PIN - 26);
    
    // Leitura múltipla para estabilidade (pH varia lentamente)
    uint32_t soma = 0;
    for (int i = 0; i < 8; i++) {
        soma += adc_read();
        sleep_us(200);
    }
    uint16_t adc_value = soma / 8;
    
    // Conversão para pH (calibração típica de sensor E-201C)
    // pH 7 = 1.65V, sensibilidade ~0.18V/pH
    float voltage = (adc_value / 4095.0f) * 3.3f;
    float ph_raw = 7.0f - ((voltage - 1.65f) / 0.18f);
    
    // Aplica filtro de média móvel (pH muda lentamente)
    float ph = calcular_media_movel(ph_buffer, ph_raw);
    
    // Limita ao range válido para aquicultura
    return (ph < 4.0f) ? 4.0f : ((ph > 10.0f) ? 10.0f : ph);
}

float sensor_ler_nivel_agua(void) {
    static bool adc_init_nivel = false;
    if (!adc_init_nivel) {
        adc_gpio_init(ADC_NIVEL_PIN);
        adc_init_nivel = true;
    }
    
    adc_select_input(ADC_NIVEL_PIN - 26);
    
    // Leitura múltipla para estabilidade
    uint32_t soma = 0;
    for (int i = 0; i < 4; i++) {
        soma += adc_read();
        sleep_us(100);
    }
    uint16_t adc_value = soma / 4;
    
    float nivel_raw = (adc_value / 4095.0f) * 100.0f;
    
    // Aplica filtro de média móvel
    float nivel = calcular_media_movel(nivel_buffer, nivel_raw);
    
    return (nivel < 0.0f) ? 0.0f : ((nivel > 100.0f) ? 100.0f : nivel);
}

// ============================================================
// NOVO: Sensor de Turbidez
// ============================================================
float sensor_ler_turbidez(void) {
    // Usa mesmo ADC que nível, mas em momento diferente
    // Em produção, usar ADC separado ou multiplexador
    static bool primeiro_read = true;
    static float ultima_turbidez = 3.0f;
    
    adc_select_input(2);  // ADC2
    
    uint32_t soma = 0;
    for (int i = 0; i < 8; i++) {
        soma += adc_read();
        sleep_us(150);
    }
    uint16_t adc_value = soma / 8;
    
    // Conversão para NTU (sensor típico de turbidez)
    // Água limpa: ~0-5 NTU, Água turva: >10 NTU
    float voltage = (adc_value / 4095.0f) * 3.3f;
    float ntu_raw = (3.3f - voltage) * 10.0f;  // Inverso: menos luz = mais turbidez
    
    // Aplica filtro de média móvel
    float ntu = calcular_media_movel(turbidez_buffer, ntu_raw);
    
    if (primeiro_read) {
        primeiro_read = false;
        atualizar_buffer_index();
    }
    
    ultima_turbidez = ntu;
    return (ntu < 0.0f) ? 0.0f : ((ntu > 50.0f) ? 50.0f : ntu);
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
    
    // Verifica se TPA já está em andamento
    if (system_status.tpa_em_andamento) return false;
    
    // Verifica cooldown (evita TPAs muito frequentes)
    uint32_t agora = get_timestamp_ms();
    if ((agora - ultima_tpa_timestamp) < TPA_COOLDOWN_MS && ultima_tpa_timestamp != 0) {
        return false;
    }
    
    // Verifica condições para TPA
    bool ph_critico = (system_status.ph >= PH_TPA_TRIGGER) || (system_status.ph < PH_MIN);
    bool nivel_baixo = (system_status.nivel_agua < NIVEL_CRITICO);
    
    if (ph_critico) {
        printf("🚨 pH fora do range: %.2f (ideal: %.1f-%.1f)\n", 
               system_status.ph, PH_MIN, PH_MAX);
        ultima_tpa_timestamp = agora;
        return tpa_iniciar("pH CRÍTICO - AUTOMÁTICO");
    }
    
    // Verifica turbidez alta (nova condição)
    float turbidez = sensor_ler_turbidez();
    if (turbidez > 10.0f) {
        printf("🚨 Turbidez alta: %.1f NTU (máx: 10 NTU)\n", turbidez);
        ultima_tpa_timestamp = agora;
        return tpa_iniciar("TURBIDEZ ALTA - AUTOMÁTICO");
    }
    
    // Alerta de nível baixo (sem TPA, apenas notifica)
    if (nivel_baixo) {
        printf("⚠️ Nível baixo: %.1f%% - Verificar bomba de reposição\n", 
               system_status.nivel_agua);
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