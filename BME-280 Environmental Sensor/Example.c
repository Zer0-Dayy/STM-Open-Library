#include "bme280_sensor_driver.h"

extern I2C_HandleTypeDef hi2c1;   // I2C connected to the BME280 breakout

bme280_sensor_t bme_sensor;
bme280_reading_t bme_reading;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();

    // Most BME280 boards pull SDO low (address 0x76). Switch to BME280_I2C_ADDR_HIGH for 0x77.
    BME280_Sensor_Init(&bme_sensor, &hi2c1, BME280_I2C_ADDR_LOW);

    if (BME280_Sensor_Begin(&bme_sensor) != BME280_OK)
    {
        Error_Handler();
    }

    while (1)
    {
        if (BME280_Sensor_Read(&bme_sensor, &bme_reading) == BME280_OK)
        {
            float temperature_c = bme_reading.temperature_c;
            float humidity_rh   = bme_reading.humidity_rh;
            float pressure_pa   = bme_reading.pressure_pa;
            (void)temperature_c;
            (void)humidity_rh;
            (void)pressure_pa;
        }

        HAL_Delay(1000);
    }
}
