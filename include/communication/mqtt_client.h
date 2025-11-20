#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "FreeRTOS.h"
#include "task.h"
#include "system_config.h"

typedef struct {
    bool connected;
    char broker_url[64];
    int broker_port;
} mqtt_client_t;

void mqtt_task(void *pvParameters);
void mqtt_init(void);
void mqtt_publish_sensor_data(SystemStatus_t *status);
void mqtt_publish_alert(const char* alert_message);

#endif // MQTT_CLIENT_H