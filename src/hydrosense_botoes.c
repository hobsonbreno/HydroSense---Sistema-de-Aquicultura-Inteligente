#include "hydrosense_system.h"
#include <stdio.h>

// Protótipos das funções
void processar_botao_a(void);
void processar_botao_b(void);

// Variáveis de controle dos botões
static uint32_t last_button_time = 0;
static const uint32_t button_debounce = 300; // 300ms como no Python

// Estado dos botões anterior
static bool button_a_last_state = false;
static bool button_b_last_state = false;

extern hydrosense_status_t system_status;

void botoes_init(void) {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    
    printf("✅ Botões A (GPIO %d) e B (GPIO %d) inicializados\n", BUTTON_A_PIN, BUTTON_B_PIN);
}

bool botao_a_pressionado(void) {
    return !gpio_get(BUTTON_A_PIN); // Inverte porque é pull-up
}

bool botao_b_pressionado(void) {
    return !gpio_get(BUTTON_B_PIN); // Inverte porque é pull-up
}

void neopixel_seta_botao_a(void) {
    // Padrão de seta esquerda (como no Python)
    neopixel_clear();
    uint8_t pattern[5][5] = {
        {0,0,1,0,0},
        {0,1,1,0,0},
        {1,1,1,1,1},
        {0,1,1,0,0},
        {0,0,1,0,0}
    };
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (pattern[row][col]) {
                uint8_t led_idx = row * 5 + col;
                neopixel_set_color(led_idx, COLOR_CYAN);
            }
        }
    }
    neopixel_show();
}

void neopixel_seta_botao_b(void) {
    // Padrão de seta direita (como no Python)
    neopixel_clear();
    uint8_t pattern[5][5] = {
        {0,0,1,0,0},
        {0,0,1,1,0},
        {1,1,1,1,1},
        {0,0,1,1,0},
        {0,0,1,0,0}
    };
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (pattern[row][col]) {
                uint8_t led_idx = row * 5 + col;
                neopixel_set_color(led_idx, COLOR_YELLOW);
            }
        }
    }
    neopixel_show();
}

void neopixel_ambos_botoes(void) {
    // Padrão quando ambos botões são pressionados (como no Python)
    neopixel_clear();
    uint8_t pattern[5][5] = {
        {1,0,1,0,1},
        {0,1,1,1,0},
        {1,1,1,1,1},
        {0,1,1,1,0},
        {1,0,1,0,1}
    };
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            if (pattern[row][col]) {
                uint8_t led_idx = row * 5 + col;
                neopixel_set_color(led_idx, COLOR_MAGENTA);
            }
        }
    }
    neopixel_show();
}

void led_button_feedback(char button_type, uint32_t duration_ms) {
    if (button_type == 'A') {
        neopixel_seta_botao_a();
    } else if (button_type == 'B') {
        neopixel_seta_botao_b();
    } else if (button_type == 'X') { // Ambos
        neopixel_ambos_botoes();
    }
    
    sleep_ms(duration_ms);
    neopixel_clear();
}

void botoes_processar(void) {
    uint32_t current_time = get_timestamp_ms();
    
    // Debounce
    if (current_time - last_button_time < button_debounce) {
        return;
    }
    
    bool button_a_pressed = botao_a_pressionado();
    bool button_b_pressed = botao_b_pressionado();
    
    // Nenhum botão pressionado
    if (!button_a_pressed && !button_b_pressed) {
        return;
    }
    
    last_button_time = current_time;
    
    // Ambos botões pressionados (volta ao menu principal)
    if (button_a_pressed && button_b_pressed) {
        led_button_feedback('X', 200);
        // buzzer_beep(1500, 150); // REMOVIDO - SEM SOM
        system_status.menu_atual = MENU_MAIN;
        system_status.menu_item_selecionado = 0;
        printf("🔄 Voltando ao menu principal\n");
        return;
    }
    
    // Botão A pressionado
    if (button_a_pressed) {
        led_button_feedback('A', 200);
        // buzzer_beep(1000, 100); // DESABILITADO
        processar_botao_a();
    }
    // Botão B pressionado
    else if (button_b_pressed) {
        led_button_feedback('B', 200);
        // buzzer_beep(800, 100); // DESABILITADO
        processar_botao_b();
    }
}

void processar_botao_a(void) {
    switch (system_status.menu_atual) {
        case MENU_MAIN:
            // A = Entrar no menu OU testar orientação se pressionado por mais tempo
            system_status.menu_atual = MENU_SENSORES;
            system_status.menu_item_selecionado = 0;
            printf("📊 Entrando no menu de sensores\n");
            break;
            
        case MENU_SENSORES:
            // A = Próximo item do menu OU teste de orientação
            system_status.menu_item_selecionado++;
            if (system_status.menu_item_selecionado >= 6) { // 6 itens no menu
                system_status.menu_item_selecionado = 0;
            }
            
            // TESTE ESPECIAL: Se item 5 (último), testa orientações
            if (system_status.menu_item_selecionado == 5) {
                printf("🔄 Modo teste orientação ativado!\n");
                oled_teste_orientacao();
            }
            break;
            
        case MENU_ALIMENTACAO:
            // A = Alimentar manualmente
            led_button_feedback('A', 400);
            servo_alimentar_peixes("MANUAL - Menu");
            break;
            
        case MENU_TPA:
            // A = Iniciar TPA manual
            if (!system_status.tpa_em_andamento) {
                led_button_feedback('A', 400);
                tpa_iniciar("MANUAL - Menu");
            }
            break;
            
        case MENU_CONFIG:
            // A = Toggle alimentação automática
            system_status.alimentacao_auto_habilitada = !system_status.alimentacao_auto_habilitada;
            if (system_status.alimentacao_auto_habilitada) {
                // LEDs verdes indicam alimentação automática ON
                for (int i = 0; i < NEOPIXEL_COUNT; i++) {
                    neopixel_set_color(i, COLOR_GREEN);
                }
                neopixel_show();
                sleep_ms(300);
                neopixel_clear();
                printf("✅ Alimentação automática HABILITADA\n");
            } else {
                // LEDs vermelhos indicam alimentação automática OFF
                for (int i = 0; i < NEOPIXEL_COUNT; i++) {
                    neopixel_set_color(i, COLOR_RED);
                }
                neopixel_show();
                sleep_ms(300);
                neopixel_clear();
                printf("❌ Alimentação automática DESABILITADA\n");
            }
            break;
            
        default:
            break;
    }
}

void processar_botao_b(void) {
    switch (system_status.menu_atual) {
        case MENU_MAIN:
            // B = Alimentar peixes OU ciclar orientação se segurar
            static int orientacao_teste = 0;
            
            // Testa próxima orientação a cada pressão B no menu principal
            orientacao_teste = (orientacao_teste + 1) % 4;
            
            printf("🔄 Testando orientação %d...\n", orientacao_teste);
            
            // Aplica a orientação usando a função correta (orientação 1-4)
            oled_aplicar_orientacao(orientacao_teste + 1);
            
            // Também executa alimentação
            led_button_feedback('B', 500);
            servo_alimentar_peixes("MANUAL - Botão B");
            break;
            
        case MENU_SENSORES:
            // B = Navegar pelos menus
            system_status.menu_atual = (menu_tipo_t)((system_status.menu_atual + 1) % 7);
            system_status.menu_item_selecionado = 0;
            printf("🔄 Mudando para menu: %d\n", system_status.menu_atual);
            break;
            
        default:
            // B = Voltar ao menu principal
            system_status.menu_atual = MENU_MAIN;
            system_status.menu_item_selecionado = 0;
            printf("🔙 Voltando ao menu principal\n");
            break;
    }
}