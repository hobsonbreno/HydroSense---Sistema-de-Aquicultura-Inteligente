#include "feeding_task.h"
#include "system_config.h"
#include "actuators/servo_feeder.h"
#include "hardware/rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

extern SystemStatus_t g_system_status;
extern SemaphoreHandle_t system_data_mutex;

void feeding_task(void *pvParameters) {
    printf("🐠 Task de alimentação iniciada\n");
    
    servo_feeder_init();
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        if (is_feeding_time()) {
            printf("🐠 [Alimentação] Horário programado detectado\n");
            servo_feed_fish();
            
            // Atualiza último horário de alimentação
            if (xSemaphoreTake(system_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_system_status.last_feeding_time = to_ms_since_boot(get_absolute_time()) / 1000;
                xSemaphoreGive(system_data_mutex);
            }
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(FEEDING_CHECK_INTERVAL));
    }
}

bool is_feeding_time(void) {
    uint16_t current_time = get_current_time_hhmm();
    
    for (int i = 0; i < feeding_times_count; i++) {
        if (current_time == feeding_times[i]) {
            return true;
        }
    }
    
    return false;
}

uint16_t get_current_time_hhmm(void) {
    datetime_t t;
    rtc_get_datetime(&t);
    return (t.hour * 100) + t.min;
}