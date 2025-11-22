#include "hydrosense_system.h"
#include <stdio.h>

// Arquivo de teste para as funções de correção de orientação do display OLED

// Declaração externa da variável (ela está definida em hydrosense_oled.c)
extern bool ssd1306_init_done;

void test_orientacao_display(void) {
    printf("🔄 === TESTE DE CORREÇÃO DE ORIENTAÇÃO DO DISPLAY ===\n");
    printf("🎯 Objetivo: Corrigir o texto que aparece deitado/rotacionado\n\n");
    
    // Passo 1: Inicializar o display com a função corrigida
    printf("📡 Passo 1: Inicializando display com I2C1 (GP14/GP15)...\n");
    if (!oled_init_final_corrigido()) {
        printf("❌ Falha na inicialização do display!\n");
        return;
    }
    printf("✅ Display inicializado com sucesso!\n\n");
    
    // Passo 2: Aplicar correção automática
    printf("🤖 Passo 2: Aplicando correção automática de orientação...\n");
    if (oled_auto_corrigir_orientacao()) {
        printf("✅ Correção automática aplicada!\n");
        printf("📺 Observe o display: o texto deve estar na posição horizontal normal\n");
        sleep_ms(5000); // 5 segundos para observar
    } else {
        printf("⚠️ Correção automática não funcionou, testando manualmente...\n");
        
        // Passo 3: Testar todas as orientações
        printf("\n🧪 Passo 3: Testando todas as orientações possíveis...\n");
        printf("📝 Observe cada orientação e anote qual mostra o texto LEGÍVEL:\n\n");
        
        oled_testar_orientacoes();
        
        printf("\n❓ Qual orientação mostrou o texto LEGÍVEL (1, 2, 3 ou 4)?\n");
        printf("   1 = A0+C0 (original)\n");
        printf("   2 = A1+C0 (espelho horizontal)\n");
        printf("   3 = A0+C8 (espelho vertical)\n");
        printf("   4 = A1+C8 (rotação 180°)\n");
        printf("\n🎯 Para aplicar uma orientação específica, use: oled_aplicar_orientacao(numero)\n");
        
        // Por padrão, aplicar a orientação mais comum (4)
        printf("\n🔧 Aplicando orientação padrão (4 = A1+C8)...\n");
        oled_aplicar_orientacao(4);
    }
    
    // Passo 4: Teste final com texto normal do sistema
    printf("\n🎨 Passo 4: Teste final com interface do HydroSense...\n");
    
    // Simula dados do sistema
    extern hydrosense_status_t system_status;
    system_status.temperatura = 26.5f;
    system_status.ph = 7.2f;
    system_status.nivel_agua = 85.0f;
    
    // Mostra tela principal do sistema
    oled_mostrar_tela_principal();
    
    printf("📺 RESULTADO FINAL:\n");
    printf("   ✅ Se você consegue ler 'HydroSense', dados de temperatura, pH, etc.\n");
    printf("   ✅ E tudo está na posição HORIZONTAL normal (não deitado)\n");
    printf("   ✅ Então a correção de orientação foi BEM-SUCEDIDA! 🎉\n");
    printf("\n   ❌ Se o texto ainda estiver deitado/rotacionado,\n");
    printf("   ❌ Teste as outras orientações com oled_aplicar_orientacao(1-4)\n");
    
    printf("\n🎉 TESTE DE ORIENTAÇÃO CONCLUÍDO!\n");
    printf("=================================================\n");
}

// Função para testar correção específica de texto deitado
void test_corrigir_texto_deitado(void) {
    printf("🔧 === CORREÇÃO ESPECÍFICA PARA TEXTO DEITADO ===\n");
    
    // Verifica se o display foi inicializado usando uma das funções init
    printf("🔍 Verificando se o display está inicializado...\n");
    
    // Tenta inicializar se não estiver
    if (!ssd1306_init_done) {
        printf("⚠️ Display não inicializado. Tentando inicializar...\n");
        if (!oled_init_final_corrigido()) {
            printf("❌ Falha na inicialização do display!\n");
            printf("   Execute primeiro: oled_init_final_corrigido()\n");
            return;
        }
    }
    
    printf("🎯 Aplicando correção específica para texto deitado...\n");
    
    if (oled_corrigir_texto_deitado()) {
        printf("✅ Correção aplicada! Verifique se o texto está normal agora.\n");
    } else {
        printf("❌ Falha na correção. Tente as outras funções de orientação.\n");
    }
}

// Função para aplicar rapidamente uma orientação específica
void aplicar_orientacao_rapida(int orientacao) {
    printf("⚡ Aplicando orientação %d rapidamente...\n", orientacao);
    
    // Verifica se o display foi inicializado
    if (!ssd1306_init_done) {
        printf("⚠️ Display não inicializado. Tentando inicializar...\n");
        if (!oled_init_final_corrigido()) {
            printf("❌ Falha na inicialização! Execute oled_init_final_corrigido() primeiro.\n");
            return;
        }
    }
    
    oled_aplicar_orientacao(orientacao);
    
    printf("✅ Orientação %d aplicada!\n", orientacao);
    printf("📺 Observe o display para verificar se o texto está legível.\n");
}