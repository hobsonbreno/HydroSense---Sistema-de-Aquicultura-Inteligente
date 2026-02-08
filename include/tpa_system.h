/**
 * HydroSense v3.0 - Sistema TPA (Troca Parcial de Água)
 * 
 * Gerencia dois tipos de TPA:
 * 1. TPA por Sujeira: Quando sensor de cor detecta lodo (25% -> 100%)
 * 2. TPA por Rotatividade: A cada 2 horas para hidroponia (50% -> 100%)
 */

#ifndef TPA_SYSTEM_H
#define TPA_SYSTEM_H

#include "pico/stdlib.h"
#include "hydrosense_config.h"
#include <stdbool.h>

// Tipos de TPA
typedef enum {
    TPA_NENHUMA,
    TPA_SUJEIRA,            // Detectou sujeira/lodo
    TPA_ROTATIVIDADE        // Rotatividade para hidroponia (2h)
} tpa_tipo_t;

// Estados da máquina de estados TPA
typedef enum {
    TPA_STATE_IDLE,
    TPA_STATE_DRENANDO,
    TPA_STATE_AGUARDANDO,
    TPA_STATE_ENCHENDO,
    TPA_STATE_CONCLUIDO,
    TPA_STATE_ERRO
} tpa_estado_t;

// Estrutura de controle TPA
typedef struct {
    tpa_tipo_t tipo;
    tpa_estado_t estado;
    float nivel_alvo_min;       // Nível para parar drenagem (%)
    float nivel_alvo_max;       // Nível para parar enchimento (%)
    uint32_t inicio_timestamp;
    uint32_t fim_timestamp;
    uint32_t duracao_ms;
    bool sucesso;
    char motivo[64];
} tpa_controle_t;

// Funções principais
void tpa_init(void);
void tpa_task(void* pvParameters);  // Task FreeRTOS

// Controle das TPAs
bool tpa_iniciar_sujeira(void);
bool tpa_iniciar_rotatividade(void);
bool tpa_cancelar(void);
bool tpa_em_andamento(void);

// Getters de estado
tpa_tipo_t tpa_get_tipo_atual(void);
tpa_estado_t tpa_get_estado(void);
void tpa_get_controle(tpa_controle_t* ctrl);

// Controle das bombas
void bomba1_ligar(void);    // Drenar
void bomba1_desligar(void);
bool bomba1_esta_ligada(void);

void bomba2_ligar(void);    // Encher
void bomba2_desligar(void);
bool bomba2_esta_ligada(void);

// Verificações automáticas
bool tpa_verificar_sujeira_necessaria(void);
bool tpa_verificar_rotatividade_necessaria(void);

#endif // TPA_SYSTEM_H
