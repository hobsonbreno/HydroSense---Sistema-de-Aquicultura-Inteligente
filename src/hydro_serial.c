/**
 * HydroSense - Versão Serial (sem OLED)
 * Mostra dados dos sensores via USB serial
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>

#define LED_PIN         25
#define SERVO_PIN       16
#define SENSOR_SDA      2
#define SENSOR_SCL      3
#define VL53L0X_ADDR    0x29
#define AHT10_ADDR      0x38

static float temperatura = 0;
static float umidade = 0;
static uint16_t distancia_mm = 0;
static uint8_t vl53_stop_var = 0;

// AHT10
bool aht10_init(void) {
    uint8_t cmd[] = {0xE1, 0x08, 0x00};
    return i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) >= 0;
}

bool aht10_read(void) {
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    if (i2c_write_blocking(i2c1, AHT10_ADDR, cmd, 3, false) < 0) return false;
    sleep_ms(80);
    
    uint8_t data[6];
    if (i2c_read_blocking(i2c1, AHT10_ADDR, data, 6, false) < 0) return false;
    
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    umidade = (float)hum_raw / 1048576.0f * 100.0f;
    temperatura = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;
    return true;
}

// VL53L0X simplificado
bool vl53_init(void) {
    uint8_t reg = 0xC0, id;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &id, 1, false);
    printf("   VL53 ID: 0x%02X\n", id);
    if (id == 0xFF || id == 0x00) return false;
    
    uint8_t buf[2] = {0xBF, 0x01};
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    sleep_ms(50);
    
    buf[0] = 0x80; buf[1] = 0x01; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x01; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x00; buf[1] = 0x00; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    reg = 0x91;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, &vl53_stop_var, 1, false);
    
    buf[0] = 0x00; buf[1] = 0x01; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x00; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x80; buf[1] = 0x00; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    return true;
}

uint16_t vl53_read_mm(void) {
    uint8_t buf[2], reg, val;
    
    buf[0] = 0x80; buf[1] = 0x01; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x01; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x00; buf[1] = 0x00; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x91; buf[1] = vl53_stop_var; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x00; buf[1] = 0x01; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0xFF; buf[1] = 0x00; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x80; buf[1] = 0x00; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    buf[0] = 0x00; buf[1] = 0x01; i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    reg = 0x00;
    for (int t = 50; t > 0; t--) {
        i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
        i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
        if ((val & 0x01) == 0) break;
        sleep_ms(2);
    }
    
    reg = 0x13;
    for (int t = 100; t > 0; t--) {
        i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
        i2c_read_blocking(i2c1, VL53L0X_ADDR, &val, 1, false);
        if (val & 0x07) break;
        sleep_ms(5);
    }
    
    reg = 0x1E;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    uint16_t dist = ((uint16_t)buf[0] << 8) | buf[1];
    
    buf[0] = 0x0B; buf[1] = 0x01;
    i2c_write_blocking(i2c1, VL53L0X_ADDR, buf, 2, false);
    
    return (dist > 0 && dist < 2000) ? dist : 0xFFFF;
}

// Servo
void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_set_clkdiv(slice, 64.0f);
    pwm_set_wrap(slice, 39062);
    pwm_set_enabled(slice, true);
}

void servo_angle(int angle) {
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    uint chan = pwm_gpio_to_channel(SERVO_PIN);
    int pulse = 1953 + (angle * 1953) / 180;
    pwm_set_chan_level(slice, chan, pulse);
}

void servo_stop(void) {
    pwm_set_gpio_level(SERVO_PIN, 0);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n");
    printf("========================================\n");
    printf("  HydroSense v4 - Serial Only\n");
    printf("========================================\n\n");
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // I2C
    i2c_init(i2c1, 100000);
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);
    sleep_ms(100);
    
    printf("Sensores:\n");
    bool aht_ok = aht10_init();
    printf("  AHT10: %s\n", aht_ok ? "OK" : "ERRO");
    
    bool vl53_ok = vl53_init();
    printf("  VL53: %s\n", vl53_ok ? "OK" : "ERRO");
    
    // Servo teste
    printf("\nServo teste...\n");
    servo_init();
    servo_angle(90);
    sleep_ms(500);
    servo_angle(0);
    sleep_ms(500);
    servo_stop();
    printf("  Servo OK (parado)\n");
    
    printf("\n=== LEITURA CONTINUA ===\n");
    
    int ciclo = 0;
    while (1) {
        ciclo++;
        gpio_put(LED_PIN, ciclo % 2);
        
        printf("\n[%d] ", ciclo);
        
        if (aht_ok && aht10_read()) {
            printf("T=%.1fC H=%.1f%% ", temperatura, umidade);
        }
        
        if (vl53_ok) {
            distancia_mm = vl53_read_mm();
            if (distancia_mm != 0xFFFF) {
                printf("D=%dmm", distancia_mm);
            } else {
                printf("D=---");
            }
        }
        
        sleep_ms(2000);
    }
}
