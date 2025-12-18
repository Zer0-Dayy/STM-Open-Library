#include "ultrasonic_sensor_driver.h"

static ultrasonic_Handle_t *sensors[ULTRASONIC_MAX_SENSORS];
static uint8_t sensor_count = 0;

// Bind timers and arm input capture for the sensor.
void Ultrasonic_Sensor_Init(ultrasonic_Handle_t *ultrasonic_sensor)
{
    if (sensor_count < ULTRASONIC_MAX_SENSORS)
    {
        sensors[sensor_count++] = ultrasonic_sensor;
    }

    ultrasonic_sensor->echo_state = WAITING_FOR_RISING; // Begin by watching for the rising edge on ECHO.
    ultrasonic_sensor->ultrasonic_ready = 0;
    HAL_TIM_IC_Start_IT(ultrasonic_sensor->htim_echo, ultrasonic_sensor->echo_channel);
}

// Send a single pulse to the ultrasonic sensor's TRIG pin.
ultrasonic_status_t Ultrasonic_Trigger(ultrasonic_Handle_t *ultrasonic_sensor)
{
    if (ultrasonic_sensor->ultrasonic_ready || ultrasonic_sensor->echo_state == WAITING_FOR_FALLING)
    {
        return ULTRASONIC_BUSY;
    }

    ultrasonic_sensor->ultrasonic_ready = 0; // Clear timestamps to avoid mixing old and new measurements.
    ultrasonic_sensor->t_fall = 0;
    ultrasonic_sensor->t_rise = 0;

    __HAL_TIM_DISABLE(ultrasonic_sensor->htim_trig);        // Stop the trigger timer so we can reset it.
    __HAL_TIM_SET_COUNTER(ultrasonic_sensor->htim_trig, 0); // Reset the counter before emitting the pulse.
    HAL_TIM_PWM_Start(ultrasonic_sensor->htim_trig, ultrasonic_sensor->trig_channel); // Arm the output channel.
    __HAL_TIM_ENABLE(ultrasonic_sensor->htim_trig);         // Re-enable counting so the pulse is generated.
    // HAL_TIM_PWM_Start arms the channel; the counter must be enabled separately to emit the pulse.

    return ULTRASONIC_OK;
}

// Non-blocking input capture callback that records rising and falling edges.
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    for (uint8_t i = 0; i < sensor_count; i++)
    {
        ultrasonic_Handle_t *sensor = sensors[i];
        if (htim == sensor->htim_echo)
        {
            uint32_t active_channel = htim->Channel;
            if ((active_channel == HAL_TIM_ACTIVE_CHANNEL_1 && sensor->echo_channel == TIM_CHANNEL_1) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_2 && sensor->echo_channel == TIM_CHANNEL_2) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_3 && sensor->echo_channel == TIM_CHANNEL_3) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_4 && sensor->echo_channel == TIM_CHANNEL_4))
            {
                if (sensor->echo_state == WAITING_FOR_RISING)
                {
                    sensor->t_rise = __HAL_TIM_GET_COMPARE(sensor->htim_echo, sensor->echo_channel);

                    TIM_IC_InitTypeDef sConfig = {0};
                    sConfig.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING; // Switch to falling edge to detect the echo return.
                    sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;
                    sConfig.ICFilter = 0;
                    sConfig.ICPrescaler = TIM_ICPSC_DIV1;

                    HAL_TIM_IC_ConfigChannel(sensor->htim_echo, &sConfig, sensor->echo_channel); // Load the new configuration into the timer channel.
                    HAL_TIM_IC_Start_IT(sensor->htim_echo, sensor->echo_channel);
                    sensor->echo_state = WAITING_FOR_FALLING;
                }
                else if (sensor->echo_state == WAITING_FOR_FALLING)
                {
                    sensor->t_fall = __HAL_TIM_GET_COMPARE(sensor->htim_echo, sensor->echo_channel);

                    TIM_IC_InitTypeDef sConfig = {0};
                    sConfig.ICFilter = 0;
                    sConfig.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
                    sConfig.ICPrescaler = TIM_ICPSC_DIV1;
                    sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;

                    HAL_TIM_IC_ConfigChannel(sensor->htim_echo, &sConfig, sensor->echo_channel);
                    HAL_TIM_IC_Start_IT(sensor->htim_echo, sensor->echo_channel);

                    sensor->ultrasonic_ready = 1; // Mark the measurement as ready until the caller consumes it.
                    sensor->echo_state = WAITING_FOR_RISING;
                }
            }
        }
    }
}

uint8_t Ultrasonic_IsReady(ultrasonic_Handle_t *ultrasonic_sensor)
{
    return ultrasonic_sensor->ultrasonic_ready;
}

float Ultrasonic_GetDistance(ultrasonic_Handle_t *ultrasonic_sensor)
{
    if (Ultrasonic_IsReady(ultrasonic_sensor) == 0) // If data cannot be fetched yet, return NAN.
    {
        return NAN;
    }

    // Distance = speed of sound * (t_fall - t_rise) / 2.
    uint16_t time_difference = ultrasonic_sensor->t_fall - ultrasonic_sensor->t_rise;
    ultrasonic_sensor->ultrasonic_ready = 0;
    float distance = (time_difference / 2.0f) * 0.0343f; // 0.0343 m/s is speed of sound at ~20 - 25C.
    return distance;
}
