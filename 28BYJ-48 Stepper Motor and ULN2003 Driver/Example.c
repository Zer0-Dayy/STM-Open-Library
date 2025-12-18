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

    while (1)
    {
        // Move both motors one full revolution forward
        (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_A, STEPPER28BYJ_STEPS_PER_REV, STEPPER28BYJ_DIR_FORWARD);
        (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_B, STEPPER28BYJ_STEPS_PER_REV, STEPPER28BYJ_DIR_FORWARD);

        // Wait until both finish
        while (Stepper28BYJ_IsBusy(STEPPER28BYJ_MOTOR_A) || Stepper28BYJ_IsBusy(STEPPER28BYJ_MOTOR_B))
        {
            HAL_Delay(1);
        }

        HAL_Delay(500);

        // Move both motors one full revolution reverse
        (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_A, STEPPER28BYJ_STEPS_PER_REV, STEPPER28BYJ_DIR_REVERSE);
        (void)Stepper28BYJ_Move(STEPPER28BYJ_MOTOR_B, STEPPER28BYJ_STEPS_PER_REV, STEPPER28BYJ_DIR_REVERSE);

        // Wait until both finish
        while (Stepper28BYJ_IsBusy(STEPPER28BYJ_MOTOR_A) || Stepper28BYJ_IsBusy(STEPPER28BYJ_MOTOR_B))
        {
            HAL_Delay(1);
        }

        HAL_Delay(500);
    }
}

// Timer ISR hook (required)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    Stepper28BYJ_HandleTimerInterrupt(htim);
}
