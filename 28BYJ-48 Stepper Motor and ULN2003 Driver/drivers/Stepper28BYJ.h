#ifndef STEPPER28BYJ_H
#define STEPPER28BYJ_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define STEPPER28BYJ_STEPS_PER_REV (2048U)
#define STEPPER28BYJ_DIR_FORWARD   (+1)
#define STEPPER28BYJ_DIR_REVERSE   (-1)

typedef enum
{
    STEPPER28BYJ_MOTOR_A = 0,
    STEPPER28BYJ_MOTOR_B = 1,
    STEPPER28BYJ_MOTOR_MAX
} Stepper28BYJ_Motor;

typedef enum
{
    STEPPER28BYJ_SPEED_LOW,
    STEPPER28BYJ_SPEED_MEDIUM,
    STEPPER28BYJ_SPEED_HIGH
} Stepper28BYJ_Speed;

HAL_StatusTypeDef Stepper28BYJ_Init(Stepper28BYJ_Motor motor,
                                    TIM_HandleTypeDef *timer,
                                    GPIO_TypeDef *portCoil1, uint16_t pinCoil1,
                                    GPIO_TypeDef *portCoil2, uint16_t pinCoil2,
                                    GPIO_TypeDef *portCoil3, uint16_t pinCoil3,
                                    GPIO_TypeDef *portCoil4, uint16_t pinCoil4);

HAL_StatusTypeDef Stepper28BYJ_Move(Stepper28BYJ_Motor motor, uint16_t steps, int8_t direction);
HAL_StatusTypeDef Stepper28BYJ_Stop(Stepper28BYJ_Motor motor);
uint8_t          Stepper28BYJ_IsBusy(Stepper28BYJ_Motor motor);
HAL_StatusTypeDef Stepper28BYJ_SetSpeedPreset(Stepper28BYJ_Speed preset);
void Stepper28BYJ_HandleTimerInterrupt(TIM_HandleTypeDef *htim);

#endif /* STEPPER28BYJ_H */
