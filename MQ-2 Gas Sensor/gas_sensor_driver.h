#ifndef GAS_SENSOR_DRIVER_H
#define GAS_SENSOR_DRIVER_H

#include "main.h"
#include <math.h>
#include <stdint.h>


typedef enum
{
    GAS_SENSOR_OK = 0,
    GAS_SENSOR_ERROR
} gas_sensor_status_t;

// Raw ADC plus derived electrical values.
typedef struct
{
    uint16_t raw_counts;        // Averaged ADC code.
    float voltage_volts;        // ADC code mapped to vref.
    float resistance_ohms;      // Sensor resistance Rs.
    float ratio_vs_r0;          // Rs/R0 (NAN if not calibrated).
} gas_sensor_reading_t;

// Driver context and calibration state.
typedef struct
{
    ADC_HandleTypeDef *hadc;    // HAL ADC handle.
    float vref_volts;           // ADC reference voltage (should mirror CubeMX ADC config).
    float load_resistance_ohms; // Actual RL on the PCB; used to back-calc sensor resistance.
    float r0_ohms;              // Saved clean-air resistance reference (computed once).
    uint8_t baseline_ready;     // Flag so callers know if ratio_vs_r0 is trustworthy.
} gas_sensor_t;

// Bind the ADC handle and electrical defaults.
void Gas_Sensor_Init(gas_sensor_t *sensor, ADC_HandleTypeDef *hadc);
// Sample clean air to compute R0; call once after warm-up.
gas_sensor_status_t Gas_Sensor_Calibrate(gas_sensor_t *sensor, uint32_t sample_count, uint32_t settle_ms);
// Read averaged ADC and fill voltage, Rs, and Rs/R0 (when calibrated).
gas_sensor_status_t Gas_Sensor_Read(gas_sensor_t *sensor, uint32_t sample_count, gas_sensor_reading_t *reading);


#endif /* GAS_SENSOR_DRIVER_H */
