#ifndef HYDROSENSE_SYSTEM_H
#define HYDROSENSE_SYSTEM_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

// Mapeamento de hardware baseado no código Python
#define I2C_OLED_SDA        14   // Display OLED - Corrigido para GP14 (soldado na placa)
#define I2C_OLED_SCL        15   // Display OLED - Corrigido para GP15 (soldado na placa)
#define NEOPIXEL_PIN        7    // Matriz LED 5x5 (25 LEDs)
#define NEOPIXEL_COUNT      25
#define BUTTON_A_PIN        5    // Botões interativos
#define BUTTON_B_PIN        6
#define BUZZER_PIN          21   // Buzzer para feedback
#define SERVO_PIN           16   // Servo SG90 alimentador
#define BOMBA_SUJA_PIN      17   // Sistema TPA
#define BOMBA_LIMPA_PIN     19
#define ADC_PH_PIN          26   // Sensores analógicos
#define ADC_NIVEL_PIN       27
#define ADC_TEMP_PIN        28
#define VL53L0X_SDA         2    // Sensor de distância
#define VL53L0X_SCL         3

// Parâmetros do aquário (do Python)
#define TEMP_MIN            24.0f
#define TEMP_MAX            28.0f
#define PH_MIN              6.5f
#define PH_MAX              8.0f
#define PH_TPA_TRIGGER      7.8f
#define NIVEL_CRITICO       25
#define FEED_HOURS_COUNT    2
extern const uint8_t FEED_HOURS[FEED_HOURS_COUNT];

// Estados do sistema
typedef enum {
    SISTEMA_INICIANDO,
    SISTEMA_CONECTANDO,
    SISTEMA_OPERACIONAL,
    SISTEMA_ERRO,
    SISTEMA_TPA_ATIVO
} sistema_estado_t;

// Menus do display
typedef enum {
    MENU_MAIN,
    MENU_SENSORES,
    MENU_WIFI,
    MENU_ALIMENTACAO,
    MENU_TPA,
    MENU_CONFIG,
    MENU_SOBRE
} menu_tipo_t;

// Estrutura de status do sistema
typedef struct {
    float temperatura;
    float ph;
    float nivel_agua;
    bool wifi_conectado;
    bool mqtt_conectado;
    bool tpa_em_andamento;
    bool alimentacao_auto_habilitada;
    sistema_estado_t estado;
    menu_tipo_t menu_atual;
    uint8_t menu_item_selecionado;
    uint32_t ultimo_feeding;
    uint32_t uptime;
} hydrosense_status_t;

// Cores NeoPixel (RGB)
typedef struct {
    uint8_t r, g, b;
} rgb_color_t;

#define COLOR_RED       (rgb_color_t){80, 0, 0}
#define COLOR_GREEN     (rgb_color_t){0, 80, 0}
#define COLOR_BLUE      (rgb_color_t){0, 0, 80}
#define COLOR_YELLOW    (rgb_color_t){80, 40, 0}
#define COLOR_CYAN      (rgb_color_t){0, 80, 80}
#define COLOR_MAGENTA   (rgb_color_t){80, 0, 80}
#define COLOR_BLACK     (rgb_color_t){0, 0, 0}

// Funções principais do sistema
void hydrosense_init(void);
void hydrosense_main_loop(void);

// Controle do servo (baseado no Python)
void servo_alimentar_peixes(const char* origem);
void servo_teste_movimento(void);
void servo_parar(void);

// LEDs NeoPixel
void neopixel_init(void);
void neopixel_clear(void);
void neopixel_set_color(uint8_t index, rgb_color_t color);
void neopixel_show(void);
void neopixel_show_status_leds(void);
void neopixel_animacao_loading(void);
void neopixel_seta_botao_a(void);
void neopixel_seta_botao_b(void);

// Display OLED
bool oled_init_auto_scan(void);
void oled_clear(void);
void oled_set_pixel(int x, int y, bool on);
void oled_write_char(int x, int y, char c);
void oled_write_string(int x, int y, const char* str);
bool oled_display_buffer(void);  // Agora retorna bool para indicar sucesso
void oled_mostrar_splash(void);
void oled_mostrar_tela_principal(void);
void oled_mostrar_menu(void);
void oled_log_mensagem(const char* msg);
void oled_teste_orientacao(void);
void oled_set_orientacao_manual(int orientacao);
void oled_teste_conectividade(void);  // Nova função para diagnóstico
// Variável de estado do display (definida em hydrosense_oled.c)
extern bool ssd1306_init_done;
// Novas funções de teste visual detalhado
void oled_teste_tela_branca(void);
void oled_teste_padrao_xadrez(void);
void oled_teste_texto_grande(void);
void oled_teste_contraste(void);
void oled_teste_inversao(void);
void oled_verificar_hardware(void);
void oled_diagnostico_completo(void);  // Diagnóstico completo com checklist

// Funções específicas de display OLED
void oled_display_umidade(const hydrosense_status_t* data);
void oled_display_tds(const hydrosense_status_t* data);
void oled_display_bomba_status(const hydrosense_status_t* data);
void oled_display_sistema_status(const hydrosense_status_t* data);

// Novas funções para exibição detalhada das etapas
void oled_tela_principal_tempo_real(const hydrosense_status_t* data);

// Funções de alimentação
void oled_alimentacao_manual_iniciada(void);
void oled_alimentacao_servo_retornando(void);
void oled_alimentacao_manual_concluida(void);
void oled_alimentacao_programada_alerta(uint8_t hora, uint8_t quantidade);
void oled_alimentacao_programada_executando(uint8_t hora, uint8_t porcao_atual, uint8_t total_porcoes);

// Funções de TPA (Sistema de Troca Parcial de Água)
void oled_tpa_bomba1_iniciando(void);
void oled_tpa_bomba1_progresso(float nivel_atual, float meta);
void oled_tpa_bomba1_meta_atingida(void);
void oled_tpa_bomba2_iniciando(void);
void oled_tpa_bomba2_progresso(float nivel_atual);
void oled_tpa_bomba2_concluida(void);

// Funções de menu aprimorado
void oled_menu_principal(uint8_t item_selecionado);
void oled_menu_sensores(const hydrosense_status_t* data);

// Funções de alerta
void oled_alerta_temperatura(float temp);
void oled_alerta_ph(float ph);
void oled_alerta_nivel_critico(float nivel);

// Funções inspiradas na BitDogLab
bool oled_init_bitdog_inspired(void);  // Método de inicialização BitDogLab
bool oled_display_buffer_bitdog(void);  // Display buffer otimizado BitDogLab
void oled_draw_line(int x0, int y0, int x1, int y1);  // Algoritmo de Bresenham
void oled_teste_completo_bitdog(void);  // Teste completo inspirado na BitDogLab
// Funções de diagnóstico ultra-básico (último recurso)
bool oled_init_ultra_basic(void);  // Método ultra-básico de inicialização
void oled_scan_all_addresses(void);  // Scan completo de endereços I2C
void oled_test_i2c_speeds(void);  // Teste de diferentes velocidades I2C
// Funções corrigidas para I2C1 (GP14/GP15)
bool oled_init_simplificado_corrigido(void);  // Inicialização corrigida para I2C1
void oled_scan_i2c1_addresses(void);  // Scan específico para I2C1
bool oled_teste_definitivo_gp14_gp15(void);  // Teste definitivo para GP14/GP15
// Funções FINAIS corrigidas
bool oled_init_final_corrigido(void);  // FUNÇÃO FINAL - I2C1 exclusivo
bool oled_teste_alto_contraste_visual(void);  // Teste visual extremo
// FUNÇÕES DE CORREÇÃO DE ORIENTAÇÃO (PROBLEMA DO TEXTO DEITADO)
bool oled_corrigir_orientacao_normal(void);  // Correção automática para orientação normal
void oled_testar_orientacoes(void);  // Testa todas as 4 orientações possíveis
void oled_aplicar_orientacao(int orientacao);  // Aplica orientação específica (1-4)
bool oled_auto_corrigir_orientacao(void);  // Correção automática inteligente
bool oled_corrigir_texto_deitado(void);  // Correção específica para texto deitado/rotacionado

// Sensores
float sensor_ler_temperatura(void);
float sensor_ler_ph(void);
float sensor_ler_nivel_agua(void);
void sensores_ler_todos(void);

// Sistema TPA
bool tpa_iniciar(const char* motivo);
bool tpa_verificar_necessario(void);

// Botões
void botoes_init(void);
void botoes_processar(void);
bool botao_a_pressionado(void);
bool botao_b_pressionado(void);

// WiFi e MQTT
bool wifi_conectar_inteligente(void);
bool mqtt_conectar(void);
void mqtt_publicar_dados(void);

// Buzzer
void buzzer_init(void);
void buzzer_beep(uint16_t freq, uint16_t duracao_ms);
void buzzer_feedback_botao(char botao);

// Alimentação automática
void alimentacao_verificar_horarios(void);
bool alimentacao_is_horario_programado(void);

// Utilitários
uint32_t get_timestamp_ms(void);
void delay_ms(uint32_t ms);

#endif // HYDROSENSE_SYSTEM_H