#ifndef HYDROSENSE_UTILS_H
#define HYDROSENSE_UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Common utility functions and definitions for HydroSense project

// Data structure for sensor readings
typedef struct {
    float temperature;
    float ph_level;
    float turbidity;
    float dissolved_oxygen;
    uint32_t timestamp;
} sensor_data_t;

// Function declarations
void init_sensors(void);
bool read_sensor_data(sensor_data_t* data);
void log_sensor_data(const sensor_data_t* data);
float calculate_water_quality_index(const sensor_data_t* data);
bool is_data_valid(const sensor_data_t* data);

// Constants
#define MAX_SENSOR_READINGS 100
#define SENSOR_READ_INTERVAL_MS 1000
#define QUALITY_THRESHOLD 7.0

#endif // HYDROSENSE_UTILS_H