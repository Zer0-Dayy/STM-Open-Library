#ifndef STEPPER28BYJ_H
#define STEPPER28BYJ_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define STEPPER28BYJ_STEPS_PER_REV (2048U) // 28BYJ-48 half-step count for one revolution.
#define STEPPER28BYJ_DIR_FORWARD   (+1)    // Clockwise movement.
#define STEPPER28BYJ_DIR_REVERSE   (-1)    // Counter-clockwise movement.

// Identifies which motor is being controlled (driver supports two motors sharing one timer).
typedef enum
{
    STEPPER28BYJ_MOTOR_A = 0,
    STEPPER28BYJ_MOTOR_B = 1,
    STEPPER28BYJ_MOTOR_MAX
} Stepper28BYJ_Motor;

// Predefined timer prescaler presets; use Stepper28BYJ_SetSpeedPreset to apply.
typedef enum
{
    STEPPER28BYJ_SPEED_LOW,
    STEPPER28BYJ_SPEED_MEDIUM,
    STEPPER28BYJ_SPEED_HIGH
} Stepper28BYJ_Speed;

// Registers a motor instance, storing the shared timer and its four GPIO coil pins.
HAL_StatusTypeDef Stepper28BYJ_Init(Stepper28BYJ_Motor motor,
                                    TIM_HandleTypeDef *timer,
                                    GPIO_TypeDef *portCoil1, uint16_t pinCoil1,
                                    GPIO_TypeDef *portCoil2, uint16_t pinCoil2,
                                    GPIO_TypeDef *portCoil3, uint16_t pinCoil3,
                                    GPIO_TypeDef *portCoil4, uint16_t pinCoil4);

// Enqueues a movement for the selected motor; direction uses STEPPER28BYJ_DIR_* constants.
HAL_StatusTypeDef Stepper28BYJ_Move(Stepper28BYJ_Motor motor, uint16_t steps, int8_t direction);
// Immediately stops a motor, de-energising the coils.
HAL_StatusTypeDef Stepper28BYJ_Stop(Stepper28BYJ_Motor motor);
// Returns 1 if the selected motor still has pending steps, otherwise 0.
uint8_t          Stepper28BYJ_IsBusy(Stepper28BYJ_Motor motor);
// Updates the shared timer prescaler to LOW/MEDIUM/HIGH when both motors are idle.
HAL_StatusTypeDef Stepper28BYJ_SetSpeedPreset(Stepper28BYJ_Speed preset);
// Interrupt handler; call from HAL_TIM_PeriodElapsedCallback.
void Stepper28BYJ_HandleTimerInterrupt(TIM_HandleTypeDef *htim);

#endif /* STEPPER28BYJ_H */
