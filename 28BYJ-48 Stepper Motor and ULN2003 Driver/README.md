# 28BYJ-48 Stepper Motor Driver (Dual Motor, STM32 HAL)

Non-blocking 28BYJ-48 (ULN2003-style) stepper driver for STM32 HAL that can drive **two motors simultaneously** using a shared timer interrupt and an 8-step half-step coil sequence.

## How the driver works
- **Motor registration**: `Stepper28BYJ_Init()` stores the 4 GPIO coil pins for each motor (A and B) and enforces that both motors share the same timer instance.
- **Non-blocking moves**: `Stepper28BYJ_Move()` loads `stepsRemaining` and direction. The motor is stepped from the timer interrupt; the call returns immediately.
- **Timer-driven stepping**: Call `Stepper28BYJ_HandleTimerInterrupt()` from `HAL_TIM_PeriodElapsedCallback()`. Each timer tick advances the half-step sequence and decrements `stepsRemaining` for any active motor.
- **Auto-stop**: When both motors finish, the driver stops the timer interrupt and de-energizes the coils.

## Pinout and setup
- Each 28BYJ-48 uses **4 GPIO outputs** connected to the driver board inputs (IN1–IN4). Configure these GPIOs as output push-pull.
- Use one **basic timer** (TIMx) configured to generate **period elapsed interrupts**. The timer rate controls step speed.
- Both motors must share the same timer instance (the driver uses one shared timer internally).

## Minimal usage example
```c
#include "Stepper28BYJ.h"

extern TIM_HandleTypeDef htim5;   // Timer used for step interrupts

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM5_Init();

    // Register Motor A (example pins)
    if (Stepper28BYJ_Init(STEPPER28BYJ_MOTOR_A, &htim5,
                          GPIOB, GPIO_PIN_12,
                          GPIOB, GPIO_PIN_1,
                          GPIOB, GPIO_PIN_2,
                          GPIOB, GPIO_PIN_10) != HAL_OK)
    {
        Error_Handler();
    }

    // Register Motor B (example pins)
    if (Stepper28BYJ_Init(STEPPER28BYJ_MOTOR_B, &htim5,
                          GPIOB, GPIO_PIN_3,
                          GPIOB, GPIO_PIN_4,
                          GPIOB, GPIO_PIN_5,
                          GPIOB, GPIO_PIN_11) != HAL_OK)
    {
        Error_Handler();
    }

    // Set shared speed (only allowed when both motors are idle)
    if (Stepper28BYJ_SetSpeedPreset(STEPPER28BYJ_SPEED_MEDIUM) != HAL_OK)
    {
        Error_Handler();
    }

    // Start both motors (non-blocking)
    (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_A, 2048, STEPPER28BYJ_DIR_FORWARD);
    (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_B, 2048, STEPPER28BYJ_DIR_FORWARD);

    while (1)
    {
        // Application loop can do other work.
        // Use Stepper28BYJ_IsBusy() to know when a motor finished its move.
        if (!Stepper28BYJ_IsBusy(STEPPER28BYJ_MOTOR_A) &&
            !Stepper28BYJ_IsBusy(STEPPER28BYJ_MOTOR_B))
        {
            // Example: queue another move
            (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_A, 512, STEPPER28BYJ_DIR_FORWARD);
            (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_B, 512, STEPPER28BYJ_DIR_FORWARD);
        }

        HAL_Delay(1);
    }
}


// Timer ISR hook (required)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    Stepper28BYJ_HandleTimerInterrupt(htim);
}

```

## Tips for beginners
The driver is timer-interrupt based: if the timer interrupt isn’t firing, the motor won’t move.
Stepper28BYJ_Move() is non-blocking. It schedules steps; the ISR performs the stepping.
Stepper28BYJ_SetSpeedPreset() changes the shared timer speed, so it affects both motors and must be called only when both motors are idle.

If your motor vibrates or moves incorrectly, double-check the coil order (IN1–IN4) wiring to match the driver’s sequence.

