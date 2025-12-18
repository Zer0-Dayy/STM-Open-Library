#include "bme280_sensor_driver.h"

// Example usage for the BME280 driver in forced-conversion mode.
// This snippet assumes hi2c1 is configured in STM32CubeMX.
extern I2C_HandleTypeDef hi2c1;

static bme280_sensor_t g_bme280;

void BME280_Example_Init(void)
{
    // Most breakout boards pull SDO low, which selects address 0x76.
    BME280_Sensor_Init(&g_bme280, &hi2c1, BME280_I2C_ADDR_LOW);

    // Begin probes the sensor, performs a soft reset, reads calibration, and configures oversampling.
    if (BME280_Sensor_Begin(&g_bme280) != BME280_OK)
    {
        Error_Handler();
    }
}

void BME280_Example_Read(void)
{
    bme280_reading_t reading;
    if (BME280_Sensor_Read(&g_bme280, &reading) != BME280_OK)
    {
        return;
    }

    // reading.temperature_c, reading.humidity_rh, and reading.pressure_pa now hold compensated data.
}
