/**
 * HydroSense v3.0 - Implementação do Sistema TPA
 */

#include "tpa_system.h"
#include "sensors/vl53l0x.h"
#include "sensors/tcs3200.h"
#include "hydrosense_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

// Controle global TPA
static tpa_controle_t tpa_ctrl = {0};
static uint32_t ultima_tpa_rotatividade = 0;
static uint32_t ultima_tpa_sujeira = 0;
static bool bombas_inicializadas = false;

// Estados das bombas
static bool bomba1_estado = false;
static bool bomba2_estado = false;

void tpa_init(void) {
    printf("🔧 Inicializando sistema TPA...\n");
    
    // Inicializa pinos das bombas
    gpio_init(BOMBA1_PIN);
    gpio_set_dir(BOMBA1_PIN, GPIO_OUT);
    gpio_put(BOMBA1_PIN, 0);
    
    gpio_init(BOMBA2_PIN);
    gpio_set_dir(BOMBA2_PIN, GPIO_OUT);
    gpio_put(BOMBA2_PIN, 0);
    
    bombas_inicializadas = true;
    
    // Inicializa controle
    memset(&tpa_ctrl, 0, sizeof(tpa_controle_t));
    tpa_ctrl.estado = TPA_STATE_IDLE;
    tpa_ctrl.tipo = TPA_NENHUMA;
    
    printf("✅ Sistema TPA inicializado!\n");
    printf("   📋 TPA Sujeira: 25%% -> 100%%\n");
    printf("   📋 TPA Rotatividade: 50%% -> 100%% (a cada 2h)\n");
}

// ============================================================
// Controle das Bombas
// ============================================================

void bomba1_ligar(void) {
    if (!bombas_inicializadas) return;
    gpio_put(BOMBA1_PIN, 1);
    bomba1_estado = true;
    printf("🚰 BOMBA 1 LIGADA (drenagem)\n");
    
    // Atualiza status global
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_status.bomba1_ativa = true;
        xSemaphoreGive(g_status_mutex);
    }
}

void bomba1_desligar(void) {
    if (!bombas_inicializadas) return;
    gpio_put(BOMBA1_PIN, 0);
    bomba1_estado = false;
    printf("⏹️ BOMBA 1 DESLIGADA\n");
    
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_status.bomba1_ativa = false;
        xSemaphoreGive(g_status_mutex);
    }
}

bool bomba1_esta_ligada(void) {
    return bomba1_estado;
}

void bomba2_ligar(void) {
    if (!bombas_inicializadas) return;
    gpio_put(BOMBA2_PIN, 1);
    bomba2_estado = true;
    printf("💧 BOMBA 2 LIGADA (enchimento)\n");
    
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_status.bomba2_ativa = true;
        xSemaphoreGive(g_status_mutex);
    }
}

void bomba2_desligar(void) {
    if (!bombas_inicializadas) return;
    gpio_put(BOMBA2_PIN, 0);
    bomba2_estado = false;
    printf("⏹️ BOMBA 2 DESLIGADA\n");
    
    extern system_status_t g_status;
    extern SemaphoreHandle_t g_status_mutex;
    if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_status.bomba2_ativa = false;
        xSemaphoreGive(g_status_mutex);
    }
}

bool bomba2_esta_ligada(void) {
    return bomba2_estado;
}

// ============================================================
// Controle TPA
// ============================================================

bool tpa_iniciar_sujeira(void) {
    if (tpa_ctrl.estado != TPA_STATE_IDLE) {
        printf("⚠️ TPA já em andamento, ignorando\n");
        return false;
    }
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  🚨 INICIANDO TPA POR SUJEIRA                                ║\n");
    printf("║  📋 Objetivo: Drenar até 25%% (5L) -> Encher até 100%% (20L)  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    tpa_ctrl.tipo = TPA_SUJEIRA;
    tpa_ctrl.estado = TPA_STATE_DRENANDO;
    tpa_ctrl.nivel_alvo_min = TPA_SUJEIRA_NIVEL_MIN;   // 25%
    tpa_ctrl.nivel_alvo_max = TPA_SUJEIRA_NIVEL_MAX;   // 100%
    tpa_ctrl.inicio_timestamp = to_ms_since_boot(get_absolute_time());
    tpa_ctrl.sucesso = false;
    snprintf(tpa_ctrl.motivo, sizeof(tpa_ctrl.motivo), "Sujeira/lodo detectado pelo sensor de cor");
    
    // Inicia drenagem
    bomba1_ligar();
    ultima_tpa_sujeira = tpa_ctrl.inicio_timestamp;
    
    return true;
}

bool tpa_iniciar_rotatividade(void) {
    if (tpa_ctrl.estado != TPA_STATE_IDLE) {
        printf("⚠️ TPA já em andamento, ignorando\n");
        return false;
    }
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  🔄 INICIANDO TPA POR ROTATIVIDADE (Hidroponia)              ║\n");
    printf("║  📋 Objetivo: Drenar até 50%% (10L) -> Encher até 100%% (20L) ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    tpa_ctrl.tipo = TPA_ROTATIVIDADE;
    tpa_ctrl.estado = TPA_STATE_DRENANDO;
    tpa_ctrl.nivel_alvo_min = TPA_ROTATIVIDADE_NIVEL_MIN;   // 50%
    tpa_ctrl.nivel_alvo_max = TPA_ROTATIVIDADE_NIVEL_MAX;   // 100%
    tpa_ctrl.inicio_timestamp = to_ms_since_boot(get_absolute_time());
    tpa_ctrl.sucesso = false;
    snprintf(tpa_ctrl.motivo, sizeof(tpa_ctrl.motivo), "Rotatividade programada (2h) para hidroponia");
    
    // Inicia drenagem
    bomba1_ligar();
    ultima_tpa_rotatividade = tpa_ctrl.inicio_timestamp;
    
    return true;
}

bool tpa_cancelar(void) {
    if (tpa_ctrl.estado == TPA_STATE_IDLE) {
        return false;
    }
    
    printf("⚠️ TPA CANCELADA!\n");
    
    bomba1_desligar();
    bomba2_desligar();
    
    tpa_ctrl.estado = TPA_STATE_IDLE;
    tpa_ctrl.tipo = TPA_NENHUMA;
    tpa_ctrl.sucesso = false;
    
    return true;
}

bool tpa_em_andamento(void) {
    return tpa_ctrl.estado != TPA_STATE_IDLE && tpa_ctrl.estado != TPA_STATE_CONCLUIDO;
}

// ============================================================
// Verificações Automáticas
// ============================================================

bool tpa_verificar_sujeira_necessaria(void) {
    if (tpa_em_andamento()) return false;
    
    // Verifica cooldown
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    if ((agora - ultima_tpa_sujeira) < TPA_COOLDOWN_MS && ultima_tpa_sujeira != 0) {
        return false;
    }
    
    // Verifica sensor de cor
    return tcs3200_detectar_sujeira();
}

bool tpa_verificar_rotatividade_necessaria(void) {
    if (tpa_em_andamento()) return false;
    
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    
    // Verifica se passou 2 horas desde a última rotatividade
    if ((agora - ultima_tpa_rotatividade) >= TPA_ROTATIVIDADE_INTERVALO_MS) {
        return true;
    }
    
    // Primeira execução após 2 horas de uptime
    if (ultima_tpa_rotatividade == 0 && agora >= TPA_ROTATIVIDADE_INTERVALO_MS) {
        return true;
    }
    
    return false;
}

// ============================================================
// Task FreeRTOS
// ============================================================

void tpa_task(void* pvParameters) {
    printf("🔄 Task TPA iniciada\n");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        float nivel_atual = vl53l0x_get_water_level_percent();
        
        // Máquina de estados TPA
        switch (tpa_ctrl.estado) {
            case TPA_STATE_IDLE:
                // Verifica se precisa iniciar TPA
                if (tpa_verificar_sujeira_necessaria()) {
                    tpa_iniciar_sujeira();
                } else if (tpa_verificar_rotatividade_necessaria()) {
                    tpa_iniciar_rotatividade();
                }
                break;
                
            case TPA_STATE_DRENANDO:
                printf("🚰 Drenando... Nível atual: %.1f%% (alvo: %.1f%%)\n", 
                       nivel_atual, tpa_ctrl.nivel_alvo_min);
                
                // Verifica se atingiu nível mínimo
                if (nivel_atual <= tpa_ctrl.nivel_alvo_min) {
                    bomba1_desligar();
                    printf("✅ Drenagem concluída! Nível: %.1f%%\n", nivel_atual);
                    
                    // Pequena pausa antes de encher
                    tpa_ctrl.estado = TPA_STATE_AGUARDANDO;
                }
                break;
                
            case TPA_STATE_AGUARDANDO:
                // Pausa de 2 segundos entre drenar e encher
                vTaskDelay(pdMS_TO_TICKS(2000));
                
                printf("💧 Iniciando enchimento até %.0f%%...\n", tpa_ctrl.nivel_alvo_max);
                bomba2_ligar();
                tpa_ctrl.estado = TPA_STATE_ENCHENDO;
                break;
                
            case TPA_STATE_ENCHENDO:
                printf("💧 Enchendo... Nível atual: %.1f%% (alvo: %.1f%%)\n", 
                       nivel_atual, tpa_ctrl.nivel_alvo_max);
                
                // Verifica se atingiu nível máximo
                if (nivel_atual >= tpa_ctrl.nivel_alvo_max) {
                    bomba2_desligar();
                    printf("✅ Enchimento concluído! Nível: %.1f%%\n", nivel_atual);
                    
                    tpa_ctrl.estado = TPA_STATE_CONCLUIDO;
                }
                break;
                
            case TPA_STATE_CONCLUIDO:
                tpa_ctrl.fim_timestamp = to_ms_since_boot(get_absolute_time());
                tpa_ctrl.duracao_ms = tpa_ctrl.fim_timestamp - tpa_ctrl.inicio_timestamp;
                tpa_ctrl.sucesso = true;
                
                printf("╔══════════════════════════════════════════════════════════════╗\n");
                printf("║  ✅ TPA CONCLUÍDA COM SUCESSO!                                ║\n");
                printf("║  ⏱️ Duração: %lu segundos                                      ║\n", tpa_ctrl.duracao_ms / 1000);
                printf("║  📊 Nível final: %.1f%% (%.1fL)                                ║\n", nivel_atual, nivel_atual * 0.2f);
                printf("╚══════════════════════════════════════════════════════════════╝\n");
                
                // Volta ao estado idle
                tpa_ctrl.estado = TPA_STATE_IDLE;
                tpa_ctrl.tipo = TPA_NENHUMA;
                break;
                
            case TPA_STATE_ERRO:
                printf("❌ ERRO na TPA! Desligando bombas por segurança.\n");
                bomba1_desligar();
                bomba2_desligar();
                tpa_ctrl.estado = TPA_STATE_IDLE;
                break;
        }
        
        // Atualiza status global
        extern system_status_t g_status;
        extern SemaphoreHandle_t g_status_mutex;
        if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_status.tpa_em_andamento = tpa_em_andamento();
            g_status.nivel_percentual = nivel_atual;
            g_status.nivel_litros = nivel_atual * 0.2f;  // 20L = 100%
            xSemaphoreGive(g_status_mutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_TPA_CHECK_INTERVAL_MS));
    }
}

// Getters
tpa_tipo_t tpa_get_tipo_atual(void) {
    return tpa_ctrl.tipo;
}

tpa_estado_t tpa_get_estado(void) {
    return tpa_ctrl.estado;
}

void tpa_get_controle(tpa_controle_t* ctrl) {
    memcpy(ctrl, &tpa_ctrl, sizeof(tpa_controle_t));
}
