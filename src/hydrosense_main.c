#include "hydrosense_system.h"
#include "hardware/rtc.h"
#include "hardware/watchdog.h"
#include <stdio.h>

// Versão do sistema
#define HYDROSENSE_VERSION "2.2.0"
#define HYDROSENSE_BUILD_DATE __DATE__

// Status global do sistema
hydrosense_status_t system_status = {
    .temperatura = 25.0f,
    .ph = 7.0f,
    .turbidez = 3.0f,           // NOVO: valor inicial de turbidez (água limpa)
    .nivel_agua = 100.0f,
    .wifi_conectado = false,
    .mqtt_conectado = false,
    .tpa_em_andamento = false,
    .alimentacao_auto_habilitada = true,
    .estado = SISTEMA_INICIANDO,
    .menu_atual = MENU_MAIN,
    .menu_item_selecionado = 0,
    .ultimo_feeding = 0,
    .uptime = 0,
    .alimentacoes_hoje = 0      // NOVO: contador de alimentações
};

// Protótipos das funções externas
extern void servo_init(void);
extern void servo_teste_movimento(void);
extern void processar_botao_a(void);
extern void processar_botao_b(void);

void hydrosense_init(void) {
    stdio_init_all();
    sleep_ms(2000);
    
    // Verifica se foi reset por watchdog
    if (watchdog_caused_reboot()) {
        printf("⚠️ Sistema reiniciado pelo WATCHDOG!\n");
        printf("   Possível travamento detectado e recuperado automaticamente.\n\n");
    }
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  🌊 HydroSense v%s - Sistema de Aquicultura Inteligente   ║\n", HYDROSENSE_VERSION);
    printf("║  📅 Build: %s                                        ║\n", HYDROSENSE_BUILD_DATE);
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("🔧 Iniciando sistema baseado em FreeRTOS + Pico SDK\n\n");
    
    // Inicializa RTC (como no Python)
    rtc_init();
    datetime_t t = {
        .year = 2025,
        .month = 11,
        .day = 20,
        .dotw = 3,
        .hour = 12,
        .min = 0,
        .sec = 0
    };
    rtc_set_datetime(&t);
    printf("🕐 RTC inicializado: %04d-%02d-%02d %02d:%02d\n", 
           t.year, t.month, t.day, t.hour, t.min);
    rtc_set_datetime(&t);
    
    system_status.estado = SISTEMA_INICIANDO;
    
    // Display OLED - FUNÇÃO FINAL CORRIGIDA (I2C1 EXCLUSIVO)
    printf("🎯 === INICIALIZAÇÃO FINAL DISPLAY OLED ===\n");
    printf("🔧 Usando função CORRIGIDA para I2C1 + GP14/GP15\n");
    printf("===============================================\n");
    
    bool oled_funcionando = false;
    
    // === TESTE FINAL COM I2C1 EXCLUSIVO ===
    if (oled_init_final_corrigido()) {
        printf("🎉 DISPLAY OLED FUNCIONANDO (MÉTODO FINAL)!\n");
        
        // Executa teste de alto contraste visual para confirmar
        printf("🎨 Executando teste de alto contraste visual...\n");
        if (oled_teste_alto_contraste_visual()) {
            printf("✅ Teste visual confirmado - display totalmente funcional!\n");
            oled_funcionando = true;
            
            // Mostra splash de inicialização
            sleep_ms(2000);
            oled_mostrar_splash();
            sleep_ms(3000);
        } else {
            printf("❌ Teste visual falhou - problemas de exibição\n");
        }
        
    } else {
        printf("💔 Display OLED falhou na inicialização final\n");
        printf("🔧 Executando diagnóstico definitivo...\n");
        
        // Executa o teste definitivo para diagnóstico detalhado
        if (oled_teste_definitivo_gp14_gp15()) {
            printf("🎉 Teste definitivo conseguiu inicializar!\n");
            oled_funcionando = true;
        } else {
            printf("\n🚨 DIAGNÓSTICO FINAL COMPLETO:\n");
            printf("   ❌ Função final corrigida: FALHOU\n");
            printf("   ❌ Teste definitivo GP14/GP15: FALHOU\n");
            printf("   ❌ Comunicação I2C1: SEM RESPOSTA\n");
            
            printf("\n🔍 CAUSAS MAIS PROVÁVEIS:\n");
            printf("   1️⃣ Display não está alimentado (VCC ≠ 3.3V)\n");
            printf("   2️⃣ Display foi alimentado com 5V (QUEIMADO)\n");
            printf("   3️⃣ Conexões SDA/SCL não estão em GP14/GP15\n");
            printf("   4️⃣ Display é modelo diferente (não SSD1306)\n");
            printf("   5️⃣ Display está fisicamente defeituoso\n");
            
            printf("\n🛠️ AÇÕES RECOMENDADAS:\n");
            printf("   📏 Meça com multímetro: VCC = 3.3V\n");
            printf("   📏 Verifique continuidade: GP14↔SDA, GP15↔SCL\n");
            printf("   🔍 Confirme modelo: deve ser SSD1306 I2C\n");
            printf("   🔄 Teste com outro display SSD1306\n");
            printf("   ⚡ Teste display em Arduino primeiro\n");
            
            oled_funcionando = false;
        }
    }
    
    printf("===============================================\n");
    
    if (oled_funcionando) {
        printf("🎊 RESULTADO: Display OLED TOTALMENTE FUNCIONAL!\n");
        printf("   ✅ Comunicação I2C1 estabelecida\n");
        printf("   ✅ Comandos SSD1306 aceitos\n");
        printf("   ✅ Exibição visual confirmada\n");
        printf("   ✅ Controle ON/OFF funcionando\n");
        printf("   ✅ Testes de alto contraste OK\n");
    } else {
        printf("💔 RESULTADO: Display OLED COM PROBLEMA FÍSICO\n");
        printf("   ⚠️  Sistema continuará sem display\n");
        printf("   ⚠️  Todas as funções restantes ativas\n");
        printf("   📺 Logs disponíveis via Serial Monitor\n");
    }
    
    printf("===============================================\n");
    
    // LEDs NeoPixel
    neopixel_init();
    neopixel_animacao_loading();
    
    // Buzzer (COMPLETAMENTE DESABILITADO)
    // buzzer_init(); // REMOVIDO
    
    // Botões
    botoes_init();
    
    // Servo motor
    servo_init();
    servo_teste_movimento();
    
    // Sensores
    sensores_ler_todos();
    
    printf("🎮 Controles Interativos:\n");
    printf("   - Botão A = Menu/Confirmar (Seta CIANO)\n");
    printf("   - Botão B = Voltar/Alimentar (Seta AMARELA)\n");
    printf("   - A+B = Menu Principal (Padrão MAGENTA)\n");
    
    printf("🔧 Hardware Mapeado:\n");
    printf("   - Display: I2C SDA=%d, SCL=%d\n", I2C_OLED_SDA, I2C_OLED_SCL);
    printf("   - Servo SG90: GPIO%d (PWM)\n", SERVO_PIN);
    printf("   - Botões: A=GPIO%d, B=GPIO%d\n", BUTTON_A_PIN, BUTTON_B_PIN);
    printf("   - NeoPixel: GPIO%d (%d LEDs)\n", NEOPIXEL_PIN, NEOPIXEL_COUNT);
    printf("   - Sensores: pH=ADC%d, Temp=ADC%d, Nível=ADC%d\n", 
           ADC_PH_PIN, ADC_TEMP_PIN, ADC_NIVEL_PIN);
    
    system_status.estado = SISTEMA_OPERACIONAL;
    printf("✅ Sistema HydroSense iniciado com sucesso!\n");
}

void hydrosense_main_loop(void) {
    uint32_t contador = 0;
    uint32_t last_sensor_read = 0;
    uint32_t last_status_print = 0;
    uint32_t last_display_update = 0;
    uint32_t last_led_update = 0;
    
    printf("🔄 Entrando no loop principal do sistema...\n");
    
    // Habilita watchdog com timeout de 8 segundos
    // Se o loop travar, o sistema reinicia automaticamente
    watchdog_enable(8000, true);
    printf("🐕 Watchdog habilitado (timeout: 8s)\n\n");
    
    while (true) {
        uint32_t current_time = get_timestamp_ms();
        
        // Alimenta o watchdog a cada iteração
        watchdog_update();
        
        // Processa comandos seriais (SET_TIME, FEED, etc)
        processar_comando_serial();
        
        // Leitura dos sensores a cada 5 segundos
        if (current_time - last_sensor_read > 5000) {
            sensores_ler_todos();
            last_sensor_read = current_time;
        }
        
        // Atualização do display a cada 1 segundo
        if (current_time - last_display_update > 1000) {
            switch (system_status.menu_atual) {
                case MENU_MAIN:
                    oled_mostrar_tela_principal();
                    break;
                default:
                    oled_mostrar_menu();
                    break;
            }
            last_display_update = current_time;
        }
        
        // Processamento dos botões
        botoes_processar();
        
        // Atualização dos LEDs de status a cada 2 segundos (só no menu principal)
        if (current_time - last_led_update > 2000 && system_status.menu_atual == MENU_MAIN) {
            neopixel_show_status_leds();
            last_led_update = current_time;
        }
        
        // Status no console a cada 30 segundos com mais informações
        if (current_time - last_status_print > 30000) {
            datetime_t dt;
            rtc_get_datetime(&dt);
            
            // Calcula uptime formatado
            uint32_t uptime_sec = current_time / 1000;
            uint32_t horas = uptime_sec / 3600;
            uint32_t minutos = (uptime_sec % 3600) / 60;
            
            // Indicadores de status (usando ASCII para compatibilidade)
            const char* status_temp = (system_status.temperatura >= TEMP_MIN && 
                               system_status.temperatura <= TEMP_MAX) ? "[OK]" : "[!!]";
            const char* status_ph = (system_status.ph >= PH_MIN && 
                             system_status.ph <= PH_MAX) ? "[OK]" : "[!!]";
            const char* status_nivel = (system_status.nivel_agua > NIVEL_CRITICO) ? "[OK]" : "[!!]";
            
            printf("═══════════════════════════════════════════════════════════════\n");
            printf("📊 STATUS [%02d:%02d] | Uptime: %luh%02lum | Alimentações: %d\n",
                   dt.hour, dt.min, horas, minutos, system_status.alimentacoes_hoje);
            printf("🌡️ Temp: %.1f°C %s | 💧 pH: %.2f %s | 🌊 Nível: %.0f%% %s\n",
                   system_status.temperatura, status_temp,
                   system_status.ph, status_ph,
                   system_status.nivel_agua, status_nivel);
            printf("🔬 Turbidez: %.1f NTU | WiFi: %s | TPA: %s\n",
                   system_status.turbidez,
                   system_status.wifi_conectado ? "🟢 ON" : "🔴 OFF",
                   system_status.tpa_em_andamento ? "🔄 ATIVO" : "⏹️ INATIVO");
            printf("═══════════════════════════════════════════════════════════════\n");
            
            last_status_print = current_time;
        }
        
        // Verificação de alimentação automática
        if (system_status.alimentacao_auto_habilitada) {
            alimentacao_verificar_horarios();
        }
        
        // Verificação de TPA necessário
        if (tpa_verificar_necessario()) {
            // TPA foi iniciado automaticamente
        }
        
        // Atualiza uptime
        system_status.uptime = current_time / 1000;
        contador++;
        
        sleep_ms(100); // Loop principal a cada 100ms
    }
}

uint32_t get_timestamp_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

void delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

int main() {
    printf("🚀 Iniciando HydroSense v2.1...\n");
    
    hydrosense_init();
    hydrosense_main_loop();
    
    // Nunca deve chegar aqui, mas se chegar, limpa tudo
    printf("💥 Erro crítico no sistema!\n");
    
    // Limpeza de emergência
    neopixel_clear();
    buzzer_beep(300, 2000);
    
    // Para todas as bombas por segurança
    gpio_init(BOMBA_SUJA_PIN);
    gpio_set_dir(BOMBA_SUJA_PIN, GPIO_OUT);
    gpio_put(BOMBA_SUJA_PIN, 0);
    
    gpio_init(BOMBA_LIMPA_PIN);
    gpio_set_dir(BOMBA_LIMPA_PIN, GPIO_OUT);
    gpio_put(BOMBA_LIMPA_PIN, 0);
    
    return -1;
}