#include "ultrasonic_sensor_driver.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_TIM1_Init(); // Example TRIG timer
    MX_TIM2_Init(); // Example ECHO timer

    ultrasonic_Handle_t sensor;
    sensor.htim_trig = &htim1;
    sensor.trig_channel = TIM_CHANNEL_1;
    sensor.htim_echo = &htim2;
    sensor.echo_channel = TIM_CHANNEL_1;

    Ultrasonic_Sensor_Init(&sensor);

    while (1)
    {
        if (Ultrasonic_Trigger(&sensor) == ULTRASONIC_OK)
        {
            HAL_Delay(60); // Wait between pings if needed
        }

        if (Ultrasonic_IsReady(&sensor))
        {
            float distance = Ultrasonic_GetDistance(&sensor);
            printf("Distance: %.2f cm\n", distance);
        }
    }
}