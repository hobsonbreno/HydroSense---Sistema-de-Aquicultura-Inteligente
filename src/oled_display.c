#include "hydrosense_system.h"
#include <stdio.h>
#include <string.h>

// Funções compatíveis com a API OLED do HydroSense
void oled_mostrar_sensores(hydrosense_status_t *status) {
    char temp_str[32];
    char ph_str[32];
    char nivel_str[32];
    
    oled_clear();
    oled_write_string(0, 0, "=== SENSORES ===");
    
    // Temperatura
    snprintf(temp_str, sizeof(temp_str), "Temp: %.1fC", status->temperatura);
    oled_write_string(0, 16, temp_str);
    
    // pH
    snprintf(ph_str, sizeof(ph_str), "pH: %.2f", status->ph);
    oled_write_string(0, 24, ph_str);
    
    // Nível da água
    snprintf(nivel_str, sizeof(nivel_str), "Nivel: %.1f%%", status->nivel_agua);
    oled_write_string(0, 32, nivel_str);
    
    // Status TPA
    if (status->tpa_em_andamento) {
        oled_write_string(0, 48, "TPA: ATIVO");
    } else {
        oled_write_string(0, 48, "TPA: OK");
    }
    
    oled_display_buffer();
}

void oled_mostrar_wifi(hydrosense_status_t *status) {
    oled_clear();
    oled_write_string(0, 0, "=== CONECTIVIDADE ===");
    
    // Status WiFi
    if (status->wifi_conectado) {
        oled_write_string(0, 16, "WiFi: CONECTADO");
    } else {
        oled_write_string(0, 16, "WiFi: DESCONECTADO");
    }
    
    // Status MQTT
    if (status->mqtt_conectado) {
        oled_write_string(0, 24, "MQTT: CONECTADO");
    } else {
        oled_write_string(0, 24, "MQTT: DESCONECTADO");
    }
    
    // Uptime
    uint32_t horas = status->uptime / 3600;
    uint32_t minutos = (status->uptime % 3600) / 60;
    char uptime_str[32];
    snprintf(uptime_str, sizeof(uptime_str), "Uptime: %luh%lum", horas, minutos);
    oled_write_string(0, 40, uptime_str);
    
    oled_display_buffer();
}

void oled_mostrar_alimentacao(hydrosense_status_t *status) {
    oled_clear();
    oled_write_string(0, 0, "=== ALIMENTACAO ===");
    
    // Status da alimentação automática
    if (status->alimentacao_auto_habilitada) {
        oled_write_string(0, 16, "Auto: HABILITADA");
    } else {
        oled_write_string(0, 16, "Auto: DESABILITADA");
    }
    
    // Último feeding
    if (status->ultimo_feeding > 0) {
        uint32_t tempo_desde = status->uptime - status->ultimo_feeding;
        uint32_t horas = tempo_desde / 3600;
        uint32_t minutos = (tempo_desde % 3600) / 60;
        char ultimo_str[32];
        snprintf(ultimo_str, sizeof(ultimo_str), "Ultimo: %luh%lum", horas, minutos);
        oled_write_string(0, 24, ultimo_str);
    } else {
        oled_write_string(0, 24, "Ultimo: Nunca");
    }
    
    // Horários programados
    oled_write_string(0, 40, "Horarios: 8h, 18h");
    
    oled_write_string(0, 56, "A: Alimentar agora");
    
    oled_display_buffer();
}