/**
 * HydroSense v3.0 - Sistema de Logs/Histórico
 * 
 * Registra todos os eventos do sistema para análise
 */

#ifndef LOG_SYSTEM_H
#define LOG_SYSTEM_H

#include "hydrosense_config.h"
#include "pico/stdlib.h"
#include <stdbool.h>

// Funções principais
void log_init(void);
void log_task(void* pvParameters);

// Registro de eventos
void log_evento(log_tipo_t tipo, const char* mensagem, float valor1, float valor2);
void log_sistema_inicio(void);
void log_alimentacao(bool programada, uint8_t horario);
void log_tpa_inicio(const char* tipo);
void log_tpa_fim(const char* tipo, bool sucesso, float duracao_seg);
void log_bomba(uint8_t bomba, bool ligada);
void log_alerta(const char* tipo, float valor);
void log_sensor_erro(const char* sensor);

// Recuperação de logs
int log_get_count(void);
bool log_get_entry(int index, log_entry_t* entry);
void log_get_all_json(char* buffer, size_t max_len);
void log_clear(void);

// Persistência (future: salvar em flash)
void log_save_to_flash(void);
void log_load_from_flash(void);

#endif // LOG_SYSTEM_H
