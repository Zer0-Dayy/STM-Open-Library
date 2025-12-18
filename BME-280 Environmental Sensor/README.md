# BME280 Environmental Sensor Driver

Forced-mode BME280 driver that probes the device, loads calibration coefficients, and returns compensated temperature, humidity, and pressure.

## How the driver works
- **Probe + reset**: `BME280_Sensor_Begin()` checks the chip ID, performs a soft reset, reads calibration registers, and sets oversampling.
- **Forced conversions**: Each call to `BME280_Sensor_Read()` triggers one measurement cycle and fetches compensated temperature, humidity, and pressure.
- **Calibration data**: Raw values are converted using the Bosch-specified compensation equations stored in the driver.

## Pinout and setup
- Wire SDA/SCL to an STM32 I²C peripheral (pull-ups required) and expose the handle (for example, `hi2c1`).
- Most breakout boards pull SDO low (I²C address `0x76`). Tie SDO high to use `0x77`.
- Configure the I²C timing in CubeMX to meet the desired bus speed.

## Minimal usage example
```c
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
```

## Tips for beginners
- Match the I²C address (`BME280_I2C_ADDR_LOW` or `_HIGH`) to the SDO pin on your board.
- Forced mode reads on demand; increase the delay between reads if you need lower power.
- If you see unexpected values, confirm pull-ups and bus speed before debugging the driver code.
