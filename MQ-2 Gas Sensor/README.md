# MQ-2 Gas Sensor Driver

Blocking MQ-2 driver that reads averaged ADC samples, computes sensor resistance, and reports Rs/R0 when calibrated.

## Files
- `gas_sensor_driver.h` / `gas_sensor_driver.c`: Driver API and implementation.
- `Example.c`: Minimal init/read routines that can be dropped into your project.

## Quick start
1. Copy the driver sources and `Example.c` into your STM32 project.
2. Configure an ADC channel in CubeMX and expose its handle (for example, `hadc1`).
3. Call `Gas_Sensor_Example_Init()` after the MQ-2 warm-up period to bind the ADC and capture the clean-air baseline.
4. Periodically call `Gas_Sensor_Example_Read()` (or `Gas_Sensor_Read` directly) to obtain averaged counts, voltage, Rs, and Rs/R0.

## Notes
- Update `vref_volts` and `load_resistance_ohms` in `Gas_Sensor_Example_Init` to match your PCB.
- `ratio_vs_r0` is only valid after calibration and can be mapped to PPM using the MQ-2 datasheet curve.
- The driver is polling-based to remain predictable inside a simple main loop.
