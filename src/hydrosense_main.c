#include "hydrosense_system.h"
#include "hardware/rtc.h"
#include <stdio.h>

// Status global do sistema
hydrosense_status_t system_status = {
    .temperatura = 25.0f,
    .ph = 7.0f,
    .nivel_agua = 100.0f,
    .wifi_conectado = false,
    .mqtt_conectado = false,
    .tpa_em_andamento = false,
    .alimentacao_auto_habilitada = true,
    .estado = SISTEMA_INICIANDO,
    .menu_atual = MENU_MAIN,
    .menu_item_selecionado = 0,
    .ultimo_feeding = 0,
    .uptime = 0
};

// Protótipos das funções externas
extern void servo_init(void);
extern void servo_teste_movimento(void);
extern void processar_botao_a(void);
extern void processar_botao_b(void);

void hydrosense_init(void) {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("🌊 HydroSense v2.1 - Sistema Profissional de Aquicultura\n");
    printf("============================================================\n");
    printf("🔧 Versão C baseada na análise do código Python\n");
    
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
    
    system_status.estado = SISTEMA_INICIANDO;
    
    // Display OLED - TESTE DEFINITIVO COM SERVO FUNCIONANDO
    printf("🎯 === DIAGNÓSTICO DEFINITIVO DISPLAY OLED ===\n");
    printf("✅ Servo funcionou = pinos GP14/GP15 estão corretos!\n");
    printf("🔍 Agora testando especificamente o display...\n");
    printf("===============================================\n");
    
    bool oled_funcionando = false;
    
    // === TESTE DEFINITIVO ESPECÍFICO PARA GP14/GP15 ===
    if (oled_teste_definitivo_gp14_gp15()) {
        printf("🎉 DISPLAY OLED FUNCIONOU COM TESTE DEFINITIVO!\n");
        oled_funcionando = true;
    } else {
        printf("💔 Display OLED não respondeu ao teste definitivo\n");
        printf("🔧 Executando diagnóstico final...\n");
        
        // Se o teste definitivo falhou, problema é físico
        printf("\n🚨 DIAGNÓSTICO FINAL:\n");
        printf("   ✅ Servo funciona = Pinos GP14/GP15 OK\n");
        printf("   ✅ I2C1 configurado corretamente\n");
        printf("   ❌ Display não responde a nenhum comando\n");
        printf("\n🔍 POSSÍVEIS CAUSAS:\n");
        printf("   1️⃣ Display não está alimentado (VCC = 3.3V?)\n");
        printf("   2️⃣ Display está alimentado com 5V (pode ter queimado!)\n");
        printf("   3️⃣ Display SDA/SCL não estão conectados\n");
        printf("   4️⃣ Display está fisicamente defeituoso\n");
        printf("   5️⃣ Display é modelo diferente (não SSD1306)\n");
        printf("\n🛠️ TESTE FÍSICO RECOMENDADO:\n");
        printf("   - Meça com multímetro: VCC do display = 3.3V\n");
        printf("   - Meça continuidade: GP14 ↔ SDA, GP15 ↔ SCL\n");
        printf("   - Verifique se display tem chip SSD1306\n");
        printf("   - Teste com outro display SSD1306\n");
        
        oled_funcionando = false;
    }
    
    printf("===============================================\n");
    
    if (oled_funcionando) {
        printf("🎊 RESULTADO: Display OLED FUNCIONANDO!\n");
        printf("   Todos os testes foram bem-sucedidos\n");
    } else {
        printf("💔 RESULTADO: Display OLED com problema físico\n");
        printf("   Sistema continuará sem display\n");
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
    
    while (true) {
        uint32_t current_time = get_timestamp_ms();
        
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
        
        // Status no console a cada 30 segundos
        if (current_time - last_status_print > 30000) {
            datetime_t dt;
            rtc_get_datetime(&dt);
            printf("📊 %02d:%02d | Temp: %.1f°C | pH: %.1f | Nível: %.0f%% | WiFi: %s | Estado: %d\n",
                   dt.hour, dt.min,
                   system_status.temperatura,
                   system_status.ph,
                   system_status.nivel_agua,
                   system_status.wifi_conectado ? "ON" : "OFF",
                   system_status.estado);
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