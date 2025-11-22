\# Ultrasonic Sensor Driver (HC-SR04 style) for STM32



This library measures distance using an HC-SR04-compatible ultrasonic sensor on STM32 microcontrollers.  

Unlike delay-based or GPIO-toggling approaches, this driver uses \*\*two hardware timers\*\*: one for generating the TRIG pulse and one for capturing the ECHO signal. All timing is handled by the timers, keeping the CPU free and improving measurement stability.



\## How the driver works



\- \*\*TRIG via PWM timer\*\*  

&nbsp; `Ultrasonic\_Trigger()` resets the TRIG timer counter, arms the PWM output, and enables the timer so it produces a clean 10 µs HIGH pulse. The pulse is entirely hardware-generated; no GPIO writes or software delays are used.



\- \*\*ECHO via input capture + interrupts\*\*  

&nbsp; `Ultrasonic\_Sensor\_Init()` stores the handle and enables input-capture interrupts on the ECHO timer.  

&nbsp; - On the \*\*rising edge\*\*, the callback reads the CCR latch and switches polarity to FALLING.  

&nbsp; - On the \*\*falling edge\*\*, the callback captures the second timestamp, restores RISING polarity, and marks the measurement as ready.



\- \*\*Timestamps and flags\*\*  

&nbsp; - `t\_rise` and `t\_fall` store hardware-latched CNT values from the CCR register.  

&nbsp; - `echo\_state` tracks whether the callback should expect a rising or falling edge.  

&nbsp; - `ultrasonic\_ready` becomes `1` when a full echo pulse has been measured.



\- \*\*Distance computation\*\*  

&nbsp; `Ultrasonic\_GetDistance()` calculates "distance = (t\_fall - t\_rise) / 2 \* 0.0343" using ~343 m/s as the speed of sound (typical Tunis temperature range). It then clears the ready flag for the next cycle.



\## Pinout and setup



\- Connect \*\*TRIG\*\* to a timer channel configured for PWM output.  

\- Connect \*\*ECHO\*\* to a timer channel configured for input capture (interrupt mode).  

\- Configure the timers in CubeMX so the timer tick resolution is in microseconds.  



\## Minimal usage example

```c

\#include "ultrasonic\_sensor\_driver.h"



int main(void)

{

&nbsp;   HAL\_Init();

&nbsp;   SystemClock\_Config();



&nbsp;   MX\_TIM1\_Init(); // Example TRIG timer

&nbsp;   MX\_TIM2\_Init(); // Example ECHO timer



&nbsp;   ultrasonic\_Handle\_t sensor;

&nbsp;   sensor.htim\_trig = \&htim1;

&nbsp;   sensor.trig\_channel = TIM\_CHANNEL\_1;

&nbsp;   sensor.htim\_echo = \&htim2;

&nbsp;   sensor.echo\_channel = TIM\_CHANNEL\_1;



&nbsp;   Ultrasonic\_Sensor\_Init(\&sensor);



&nbsp;   while (1)

&nbsp;   {

&nbsp;       if (Ultrasonic\_Trigger(\&sensor) == ULTRASONIC\_OK)

&nbsp;       {

&nbsp;           HAL\_Delay(60); // Wait between pings if needed

&nbsp;       }



&nbsp;       if (Ultrasonic\_IsReady(\&sensor))

&nbsp;       {

&nbsp;           float distance = Ultrasonic\_GetDistance(\&sensor);

&nbsp;           printf("Distance: %.2f cm\\n", distance);

&nbsp;       }

&nbsp;   }

}



```

\## Tips for beginners

\- Keep TRIG and ECHO timers running on the same time base (1 µs tick recommended).

\- If readings seem inconsistent, ensure ARR and prescaler values are not overflowing during long echo pulses.

\- Avoid triggering the sensor again until Ultrasonic\_IsReady() returns 1.

\- A logic analyzer helps confirm that the PWM TRIG pulse and input-capture edges match your timer settings.

