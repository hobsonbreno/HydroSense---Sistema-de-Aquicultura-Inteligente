/*
 * Exemplo de integração das funções OLED detalhadas no HydroSense
 * Este arquivo mostra como usar as novas funções de display para cada etapa
 */

#include "hydrosense_system.h"

// Variáveis globais para controle de estado
static hydrosense_status_t sistema_status;
static uint8_t menu_item_atual = 0;
static bool alimentacao_em_andamento = false;
static bool tpa_em_andamento = false;

// Exemplo de uso da tela principal em tempo real
void exemplo_tela_principal() {
    // Atualizar dados dos sensores
    sistema_status.temperatura = sensor_ler_temperatura();
    sistema_status.ph = sensor_ler_ph();
    sistema_status.nivel_agua = sensor_ler_nivel_agua();
    sistema_status.uptime = get_timestamp_ms() / 1000;
    
    // Exibir tela principal com informações em tempo real
    oled_tela_principal_tempo_real(&sistema_status);
    
    // Verificar alertas
    if (sistema_status.temperatura < TEMP_MIN || sistema_status.temperatura > TEMP_MAX) {
        oled_alerta_temperatura(sistema_status.temperatura);
        sleep_ms(2000);
    }
    
    if (sistema_status.ph < PH_MIN || sistema_status.ph > PH_MAX) {
        oled_alerta_ph(sistema_status.ph);
        sleep_ms(2000);
    }
    
    if (sistema_status.nivel_agua < NIVEL_CRITICO) {
        oled_alerta_nivel_critico(sistema_status.nivel_agua);
        sleep_ms(2000);
    }
}

// Exemplo de alimentação manual (Botão B)
void exemplo_alimentacao_manual() {
    printf("🍽️ Alimentação manual iniciada pelo usuário\n");
    
    // Etapa 1: Mostrar que a alimentação manual foi iniciada
    oled_alimentacao_manual_iniciada();
    sleep_ms(2000);
    
    // Etapa 2: Acionar servo de 0° para 180°
    printf("🔄 Servo girando 0° -> 180°\n");
    // Aqui você chamaria a função real do servo
    // servo_set_angle(180);
    sleep_ms(3000); // Simular tempo do movimento
    
    // Etapa 3: Mostrar servo retornando
    oled_alimentacao_servo_retornando();
    sleep_ms(2000);
    
    // Etapa 4: Retornar servo para 0°
    printf("🔄 Servo retornando 180° -> 0°\n");
    // servo_set_angle(0);
    sleep_ms(2000);
    
    // Etapa 5: Mostrar conclusão
    oled_alimentacao_manual_concluida();
    
    printf("✅ Alimentação manual concluída\n");
}

// Exemplo de alimentação programada (08:00 e 16:00)
void exemplo_alimentacao_programada() {
    uint32_t hora_atual = (sistema_status.uptime / 3600) % 24;
    
    // Verificar se é horário de alimentação (8:00 ou 16:00)
    if (hora_atual == 8 || hora_atual == 16) {
        uint8_t quantidade_racoes = 3; // 3 porções por vez
        
        printf("⏰ Horário programado de alimentação: %02lu:00\n", hora_atual);
        
        // Etapa 1: Alerta do horário programado
        oled_alimentacao_programada_alerta(hora_atual, quantidade_racoes);
        
        // Etapa 2: Executar alimentação com progresso
        for (uint8_t porcao = 1; porcao <= quantidade_racoes; porcao++) {
            printf("🍽️ Dispensando porção %d/%d\n", porcao, quantidade_racoes);
            
            oled_alimentacao_programada_executando(hora_atual, porcao, quantidade_racoes);
            
            // Movimento do servo para cada porção
            // servo_set_angle(180);
            sleep_ms(1500);
            // servo_set_angle(0);
            sleep_ms(1500);
        }
        
        printf("✅ Alimentação programada concluída\n");
        
        // Voltar à tela principal
        sleep_ms(2000);
        oled_tela_principal_tempo_real(&sistema_status);
    }
}

// Exemplo completo do sistema TPA
void exemplo_sistema_tpa() {
    printf("🌊 Iniciando Sistema TPA (Troca Parcial de Água)\n");
    
    // FASE 1: BOMBA 1 - Esvaziamento (25% do volume)
    printf("💧 FASE 1: Esvaziando tanque até 25%\n");
    oled_tpa_bomba1_iniciando();
    sleep_ms(3000);
    
    // Simular esvaziamento com progresso
    float nivel_atual = sistema_status.nivel_agua; // Começar do nível atual
    float meta_esvaziamento = 25.0; // Meta: 25% do volume
    
    // Ligar bomba 1 (água suja)
    // gpio_put(BOMBA_SUJA_PIN, true);
    
    while (nivel_atual > meta_esvaziamento) {
        // Simular diminuição do nível
        nivel_atual -= 1.0; // Decremento simulado
        
        // Atualizar display com progresso
        oled_tpa_bomba1_progresso(nivel_atual, meta_esvaziamento);
        
        sleep_ms(500); // Atualizar a cada 500ms
        
        // Em um sistema real, você leria o sensor:
        // nivel_atual = sensor_ler_nivel_agua();
    }
    
    // Desligar bomba 1
    // gpio_put(BOMBA_SUJA_PIN, false);
    
    // Meta atingida
    oled_tpa_bomba1_meta_atingida();
    printf("✅ Meta de esvaziamento atingida: 25%\n");
    
    // FASE 2: BOMBA 2 - Reabastecimento (até 100%)
    printf("💧 FASE 2: Reabastecendo tanque até 100%\n");
    oled_tpa_bomba2_iniciando();
    sleep_ms(3000);
    
    // Simular reabastecimento com progresso
    // Ligar bomba 2 (água limpa)
    // gpio_put(BOMBA_LIMPA_PIN, true);
    
    while (nivel_atual < 100.0) {
        // Simular aumento do nível
        nivel_atual += 2.0; // Incremento simulado (bomba mais rápida)
        
        if (nivel_atual > 100.0) nivel_atual = 100.0;
        
        // Atualizar display com progresso
        oled_tpa_bomba2_progresso(nivel_atual);
        
        sleep_ms(300); // Atualizar mais frequentemente
        
        // Em um sistema real:
        // nivel_atual = sensor_ler_nivel_agua();
    }
    
    // Desligar bomba 2
    // gpio_put(BOMBA_LIMPA_PIN, false);
    
    // TPA concluído
    oled_tpa_bomba2_concluida();
    sistema_status.nivel_agua = 100.0; // Atualizar status
    
    printf("🎉 Sistema TPA concluído com sucesso!\n");
    
    // Retornar à tela principal após alguns segundos
    sleep_ms(3000);
    oled_tela_principal_tempo_real(&sistema_status);
}

// Exemplo de navegação no menu
void exemplo_menu_navegacao() {
    static uint8_t item_selecionado = 0;
    
    // Exibir menu principal
    oled_menu_principal(item_selecionado);
    
    // Simular navegação (em um sistema real, seria pelos botões)
    if (botao_a_pressionado()) {
        // Botão A: navegar para baixo
        item_selecionado = (item_selecionado + 1) % 5;
        oled_menu_principal(item_selecionado);
        buzzer_feedback_botao('A');
    }
    
    if (botao_b_pressionado()) {
        // Botão B: selecionar item
        buzzer_feedback_botao('B');
        
        switch (item_selecionado) {
            case 0: // Sensores
                oled_menu_sensores(&sistema_status);
                sleep_ms(5000);
                break;
                
            case 1: // Alimentação
                exemplo_alimentacao_manual();
                break;
                
            case 2: // Sistema TPA
                exemplo_sistema_tpa();
                break;
                
            case 3: // Configurações
                oled_log_mensagem("Configuracoes");
                sleep_ms(2000);
                break;
                
            case 4: // Diagnósticos
                oled_diagnostico_completo();
                break;
        }
    }
}

// Loop principal integrado
void loop_principal_integrado() {
    // Inicializar sistema
    hydrosense_init();
    
    printf("🚀 HydroSense iniciado - Display melhorado!\n");
    
    while (true) {
        // Atualizar sensores
        sistema_status.temperatura = sensor_ler_temperatura();
        sistema_status.ph = sensor_ler_ph();
        sistema_status.nivel_agua = sensor_ler_nivel_agua();
        sistema_status.uptime = get_timestamp_ms() / 1000;
        
        // Verificar alimentação programada
        if (alimentacao_is_horario_programado() && !alimentacao_em_andamento) {
            exemplo_alimentacao_programada();
        }
        
        // Verificar se TPA é necessário
        if (tpa_verificar_necessario() && !tpa_em_andamento) {
            exemplo_sistema_tpa();
        }
        
        // Processar botões
        if (botao_b_pressionado() && !alimentacao_em_andamento) {
            // Botão B a qualquer momento = alimentação manual
            exemplo_alimentacao_manual();
        }
        
        if (botao_a_pressionado()) {
            // Botão A = menu
            exemplo_menu_navegacao();
        }
        
        // Exibir tela principal em tempo real
        exemplo_tela_principal();
        
        sleep_ms(1000); // Atualizar a cada segundo
    }
}