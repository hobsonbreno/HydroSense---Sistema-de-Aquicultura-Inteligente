/**
 * HydroSense v3.0 - Servidor HTTP com Dashboard
 * 
 * Servidor web para exibir dados em tempo real
 * Acessível via qualquer dispositivo (TV, celular, PC)
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdbool.h>

// Configuração do servidor
#define WEBSERVER_PORT          80
#define WEBSERVER_MAX_CLIENTS   4
#define HTTP_BUFFER_SIZE        4096

// Configuração Wi-Fi (Access Point)
#define WIFI_AP_SSID            "HydroSense"
#define WIFI_AP_PASSWORD        "hydro2024"

// Funções principais
bool webserver_init(void);
void webserver_task(void* pvParameters);
void webserver_stop(void);

// Geração de páginas
void webserver_send_dashboard(int client_fd);
void webserver_send_json_status(int client_fd);
void webserver_send_json_logs(int client_fd);

// Helpers
bool wifi_init_ap(void);
const char* wifi_get_ip(void);

#endif // WEBSERVER_H
