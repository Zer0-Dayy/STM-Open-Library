# MQ-2 Gas Sensor Driver

Blocking MQ-2 driver that averages ADC samples, computes sensor resistance, and reports Rs/R0 after calibration.

## How the driver works
- **ADC sampling**: `Gas_Sensor_Read()` triggers an ADC conversion loop, averages the raw counts, and converts to voltage using `vref_volts`.
- **Resistance math**: The driver calculates `rs_ohms` from the voltage divider (sensor + load resistor) and derives `ratio_vs_r0 = rs/r0` once calibrated.
- **Calibration helper**: `Gas_Sensor_Calibrate()` captures a clean-air baseline by averaging multiple readings and storing `r0_ohms`.

## Pinout and setup
- Power the MQ-2 heater per the datasheet and allow a warm-up period.
- Connect the sensing element through a load resistor to your ADC channel (voltage divider).
- Configure the ADC in CubeMX (single-ended input, appropriate sampling time) and expose the handle (for example, `hadc1`).

## Minimal usage example
```c
#include "gas_sensor_driver.h"

extern ADC_HandleTypeDef hadc1;   // ADC connected to the MQ-2 voltage divider

gas_sensor_t gas_sensor;
gas_sensor_reading_t gas_reading;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_ADC1_Init();

    Gas_Sensor_Init(&gas_sensor, &hadc1);

    // Match these values to your PCB before calibration.
    gas_sensor.vref_volts             = 3.3f;
    gas_sensor.load_resistance_ohms   = 5000.0f;

    // Capture the clean-air baseline after the heater has warmed up.
    if (Gas_Sensor_Calibrate(&gas_sensor, 50U, 200U) != GAS_SENSOR_OK)
    {
        Error_Handler();
    }

    while (1)
    {
        if (Gas_Sensor_Read(&gas_sensor, 16U, &gas_reading) == GAS_SENSOR_OK)
        {
            float ratio_vs_r0 = gas_reading.ratio_vs_r0;
            float adc_voltage = gas_reading.voltage;
            (void)ratio_vs_r0;
            (void)adc_voltage;
        }

        HAL_Delay(500);
    }
}
```

## Tips for beginners
- Update `vref_volts` and `load_resistance_ohms` to match your board before running calibration.
- Leave enough time for the heater to stabilize; recalibrate if conditions change significantly.
- Map `ratio_vs_r0` to PPM using the MQ-2 datasheet curve that matches your target gas.
