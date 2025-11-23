# Ultrasonic Sensors Driver (HC-SR04 and HY-SRF05) for STM32

This library measures distance using any HC-SR04/HY-SRF05-compatible ultrasonic sensor on STM32 microcontrollers.
It relies on **two hardware timers**—one for generating the TRIG pulse and one for capturing the ECHO signal—so timing is
handled in hardware, keeping the CPU free and improving measurement stability.

## How the driver works
- **TRIG via PWM timer**: `Ultrasonic_Trigger()` resets the TRIG timer counter, arms the PWM output, and enables the timer
  so it produces a clean 10 µs HIGH pulse. The pulse is entirely hardware-generated; no GPIO writes or software delays are
  used.
- **ECHO via input capture + interrupts**: `Ultrasonic_Sensor_Init()` stores the handle and enables input-capture
  interrupts on the ECHO timer.
  - On the **rising edge**, the callback reads the CCR latch and switches polarity to FALLING.
  - On the **falling edge**, the callback captures the second timestamp, restores RISING polarity, and marks the measurement
    as ready.
- **Timestamps and flags**:
  - `t_rise` and `t_fall` store hardware-latched CNT values from the CCR register.
  - `echo_state` tracks whether the callback should expect a rising or falling edge.
  - `ultrasonic_ready` becomes `1` when a full echo pulse has been measured.
- **Distance computation**: `Ultrasonic_GetDistance()` calculates `distance = (t_fall - t_rise) / 2 * 0.0343` using
  ~343 m/s as the speed of sound. It then clears the ready flag for the next cycle.

## Pinout and setup
- Connect **TRIG** to a timer channel configured for PWM output.
- Connect **ECHO** to a timer channel configured for input capture (interrupt mode).
- Configure the timers in CubeMX so the timer tick resolution is in microseconds.

## Minimal usage example
```c
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
```

## Tips for beginners
- Keep TRIG and ECHO timers running on the same time base (1 µs tick recommended).
- If readings seem inconsistent, ensure ARR and prescaler values are not overflowing during long echo pulses.
- Avoid triggering the sensor again until `Ultrasonic_IsReady()` returns `1`.
- A logic analyzer helps confirm that the PWM TRIG pulse and input-capture edges match your timer settings.
