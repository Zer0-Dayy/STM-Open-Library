#include "gas_sensor_driver.h"

// Example usage for the MQ-2 driver in blocking/polling mode.
// This snippet assumes hadc1 is configured in STM32CubeMX.
extern ADC_HandleTypeDef hadc1;

static gas_sensor_t g_gas_sensor;

void Gas_Sensor_Example_Init(void)
{
    Gas_Sensor_Init(&g_gas_sensor, &hadc1);

    // Update electrical characteristics to match your PCB before calibrating.
    g_gas_sensor.vref_volts = 3.3f;
    g_gas_sensor.load_resistance_ohms = 5000.0f;

    // Collect a baseline in clean air. Warm up the sensor before running this step.
    if (Gas_Sensor_Calibrate(&g_gas_sensor, 50U, 200U) != GAS_SENSOR_OK)
    {
        Error_Handler();
    }
}

void Gas_Sensor_Example_Read(void)
{
    gas_sensor_reading_t reading;
    if (Gas_Sensor_Read(&g_gas_sensor, 16U, &reading) != GAS_SENSOR_OK)
    {
        return;
    }

    // reading.ratio_vs_r0 reports Rs/R0 when calibration has completed.
    // Map ratio_vs_r0 to gas concentration using the MQ-2 datasheet curve as needed.
}
