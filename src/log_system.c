/**
 * HydroSense v3.0 - Implementação do Sistema de Logs
 */

#include "log_system.h"
#include "hydrosense_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

// Buffer circular de logs
static log_entry_t log_buffer[LOG_MAX_ENTRIES];
static int log_head = 0;
static int log_count = 0;
static SemaphoreHandle_t log_mutex = NULL;

// Nomes dos tipos de log
static const char* log_tipo_nomes[] = {
    "SISTEMA_INICIO",
    "SISTEMA_ERRO",
    "ALIMENTACAO_PROG",
    "ALIMENTACAO_MAN",
    "TPA_SUJEIRA_INI",
    "TPA_SUJEIRA_FIM",
    "TPA_ROTAT_INI",
    "TPA_ROTAT_FIM",
    "BOMBA1_ON",
    "BOMBA1_OFF",
    "BOMBA2_ON",
    "BOMBA2_OFF",
    "ALERTA_TEMP",
    "ALERTA_NIVEL",
    "ALERTA_TURBIDEZ",
    "SENSOR_ERRO"
};

void log_init(void) {
    printf("📋 Inicializando sistema de logs...\n");
    
    log_mutex = xSemaphoreCreateMutex();
    if (log_mutex == NULL) {
        printf("❌ Falha ao criar mutex de logs\n");
        return;
    }
    
    memset(log_buffer, 0, sizeof(log_buffer));
    log_head = 0;
    log_count = 0;
    
    printf("✅ Sistema de logs inicializado (max: %d entradas)\n", LOG_MAX_ENTRIES);
}

void log_evento(log_tipo_t tipo, const char* mensagem, float valor1, float valor2) {
    if (log_mutex == NULL) return;
    
    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }
    
    // Adiciona nova entrada
    log_entry_t* entry = &log_buffer[log_head];
    entry->timestamp = to_ms_since_boot(get_absolute_time()) / 1000;  // em segundos
    entry->tipo = tipo;
    entry->valor1 = valor1;
    entry->valor2 = valor2;
    
    if (mensagem != NULL) {
        strncpy(entry->mensagem, mensagem, sizeof(entry->mensagem) - 1);
        entry->mensagem[sizeof(entry->mensagem) - 1] = '\0';
    } else {
        entry->mensagem[0] = '\0';
    }
    
    // Avança ponteiro circular
    log_head = (log_head + 1) % LOG_MAX_ENTRIES;
    if (log_count < LOG_MAX_ENTRIES) {
        log_count++;
    }
    
    xSemaphoreGive(log_mutex);
    
    // Print no console
    printf("📝 [LOG] %s: %s (%.2f, %.2f)\n", 
           log_tipo_nomes[tipo], mensagem ? mensagem : "", valor1, valor2);
}

void log_sistema_inicio(void) {
    log_evento(LOG_SISTEMA_INICIO, "HydroSense v3.0 iniciado", 0, 0);
}

void log_alimentacao(bool programada, uint8_t horario) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Alimentação %s às %02d:00", 
             programada ? "programada" : "manual", horario);
    log_evento(programada ? LOG_ALIMENTACAO_PROGRAMADA : LOG_ALIMENTACAO_MANUAL, 
               msg, (float)horario, 0);
}

void log_tpa_inicio(const char* tipo) {
    char msg[64];
    snprintf(msg, sizeof(msg), "TPA %s iniciada", tipo);
    
    if (strcmp(tipo, "sujeira") == 0) {
        log_evento(LOG_TPA_SUJEIRA_INICIO, msg, 0, 0);
    } else {
        log_evento(LOG_TPA_ROTATIVIDADE_INICIO, msg, 0, 0);
    }
}

void log_tpa_fim(const char* tipo, bool sucesso, float duracao_seg) {
    char msg[64];
    snprintf(msg, sizeof(msg), "TPA %s %s (%.1fs)", 
             tipo, sucesso ? "concluída" : "falhou", duracao_seg);
    
    if (strcmp(tipo, "sujeira") == 0) {
        log_evento(LOG_TPA_SUJEIRA_FIM, msg, duracao_seg, sucesso ? 1 : 0);
    } else {
        log_evento(LOG_TPA_ROTATIVIDADE_FIM, msg, duracao_seg, sucesso ? 1 : 0);
    }
}

void log_bomba(uint8_t bomba, bool ligada) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Bomba %d %s", bomba, ligada ? "ligada" : "desligada");
    
    if (bomba == 1) {
        log_evento(ligada ? LOG_BOMBA1_LIGADA : LOG_BOMBA1_DESLIGADA, msg, 0, 0);
    } else {
        log_evento(ligada ? LOG_BOMBA2_LIGADA : LOG_BOMBA2_DESLIGADA, msg, 0, 0);
    }
}

void log_alerta(const char* tipo, float valor) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Alerta %s: %.2f", tipo, valor);
    
    if (strcmp(tipo, "temperatura") == 0) {
        log_evento(LOG_ALERTA_TEMPERATURA, msg, valor, 0);
    } else if (strcmp(tipo, "nivel") == 0) {
        log_evento(LOG_ALERTA_NIVEL, msg, valor, 0);
    } else {
        log_evento(LOG_ALERTA_TURBIDEZ, msg, valor, 0);
    }
}

void log_sensor_erro(const char* sensor) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Erro no sensor: %s", sensor);
    log_evento(LOG_SENSOR_ERRO, msg, 0, 0);
}

int log_get_count(void) {
    return log_count;
}

bool log_get_entry(int index, log_entry_t* entry) {
    if (index < 0 || index >= log_count || log_mutex == NULL) {
        return false;
    }
    
    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }
    
    // Calcula índice real no buffer circular
    int real_index;
    if (log_count < LOG_MAX_ENTRIES) {
        real_index = index;
    } else {
        real_index = (log_head + index) % LOG_MAX_ENTRIES;
    }
    
    memcpy(entry, &log_buffer[real_index], sizeof(log_entry_t));
    
    xSemaphoreGive(log_mutex);
    return true;
}

void log_get_all_json(char* buffer, size_t max_len) {
    if (log_mutex == NULL || buffer == NULL) return;
    
    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        snprintf(buffer, max_len, "[]");
        return;
    }
    
    int pos = 0;
    pos += snprintf(buffer + pos, max_len - pos, "[");
    
    for (int i = 0; i < log_count && pos < (int)max_len - 100; i++) {
        int real_index;
        if (log_count < LOG_MAX_ENTRIES) {
            real_index = log_count - 1 - i;  // Mais recente primeiro
        } else {
            real_index = (log_head - 1 - i + LOG_MAX_ENTRIES) % LOG_MAX_ENTRIES;
        }
        
        log_entry_t* e = &log_buffer[real_index];
        
        if (i > 0) {
            pos += snprintf(buffer + pos, max_len - pos, ",");
        }
        
        pos += snprintf(buffer + pos, max_len - pos,
            "{\"ts\":%lu,\"tipo\":\"%s\",\"msg\":\"%s\",\"v1\":%.2f,\"v2\":%.2f}",
            e->timestamp,
            log_tipo_nomes[e->tipo],
            e->mensagem,
            e->valor1,
            e->valor2
        );
    }
    
    snprintf(buffer + pos, max_len - pos, "]");
    
    xSemaphoreGive(log_mutex);
}

void log_clear(void) {
    if (log_mutex == NULL) return;
    
    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memset(log_buffer, 0, sizeof(log_buffer));
        log_head = 0;
        log_count = 0;
        xSemaphoreGive(log_mutex);
        printf("🗑️ Logs limpos\n");
    }
}

void log_task(void* pvParameters) {
    printf("📋 Task de logs iniciada\n");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Log periódico de status do sistema
        extern system_status_t g_status;
        extern SemaphoreHandle_t g_status_mutex;
        
        system_status_t status;
        if (xSemaphoreTake(g_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            status = g_status;
            xSemaphoreGive(g_status_mutex);
        }
        
        // Verifica alertas e loga
        if (status.alerta_temperatura) {
            log_alerta("temperatura", status.temperatura);
        }
        if (status.alerta_nivel) {
            log_alerta("nivel", status.nivel_percentual);
        }
        if (status.alerta_turbidez) {
            log_alerta("turbidez", status.turbidez);
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_LOG_INTERVAL_MS));
    }
}

// Stubs para persistência (implementar com flash no futuro)
void log_save_to_flash(void) {
    printf("💾 Salvando logs em flash (não implementado)\n");
}

void log_load_from_flash(void) {
    printf("📂 Carregando logs da flash (não implementado)\n");
}
