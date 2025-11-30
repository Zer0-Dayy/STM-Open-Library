#include "ultrasonic_sensor_driver.h"

extern TIM_HandleTypeDef htim1;   // TRIG TIMER EXAMPLE
extern TIM_HandleTypeDef htim2;   // ECHO TIMER EXAMPLE

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();

    right_sensor.htim_trig    = &htim1;
    right_sensor.trig_channel = TIM_CHANNEL_1;
    right_sensor.htim_echo    = &htim2;
    right_sensor.echo_channel = TIM_CHANNEL_1;
    Ultrasonic_Sensor_Init(&right_sensor);

    left_sensor.htim_trig    = &htim1;
    left_sensor.trig_channel = TIM_CHANNEL_2;
    left_sensor.htim_echo    = &htim2;
    left_sensor.echo_channel = TIM_CHANNEL_2;
    Ultrasonic_Sensor_Init(&left_sensor);

    while (1)
    {
        if (Ultrasonic_Trigger(&right_sensor) == ULTRASONIC_OK)
        {
            HAL_Delay(60);
            if (Ultrasonic_IsReady(&right_sensor))
            {
                float distance_right = Ultrasonic_GetDistance(&right_sensor);
            }
        }
        if (Ultrasonic_Trigger(&left_sensor) == ULTRASONIC_OK)
        {
            HAL_Delay(60);
            if (Ultrasonic_IsReady(&left_sensor))
            {
                float distance_left = Ultrasonic_GetDistance(&left_sensor);
            }
        }
    }
}
