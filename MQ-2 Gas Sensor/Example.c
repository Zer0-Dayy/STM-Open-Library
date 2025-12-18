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
