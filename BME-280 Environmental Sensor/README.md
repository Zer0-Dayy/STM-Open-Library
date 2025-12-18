# BME280 Environmental Sensor Driver

Forced-mode BME280 driver that probes the device, loads calibration coefficients, and returns compensated temperature, humidity, and pressure.

## Files
- `bme280_sensor_driver.h` / `bme280_sensor_driver.c`: Driver API and implementation.
- `Example.c`: Minimal init/read routines that can be dropped into your project.

## Quick start
1. Copy the driver sources and `Example.c` into your STM32 project.
2. Configure an I²C peripheral in CubeMX and expose its handle (for example, `hi2c1`).
3. Call `BME280_Example_Init()` during startup to bind the I²C handle, probe the sensor, and load calibration data.
4. Periodically call `BME280_Example_Read()` (or `BME280_Sensor_Read` directly) to trigger one forced conversion and read compensated values.

## Notes
- Choose `BME280_I2C_ADDR_LOW` or `BME280_I2C_ADDR_HIGH` depending on the SDO pin.
- The driver keeps compensation math close to the Bosch datasheet for readability.
- Forced mode keeps measurements on-demand, which pairs well with low-power designs.
