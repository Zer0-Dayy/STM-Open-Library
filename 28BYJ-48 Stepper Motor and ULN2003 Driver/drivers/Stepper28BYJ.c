#include "Stepper28BYJ.h"

typedef struct
{
    TIM_HandleTypeDef *timer;
    GPIO_TypeDef *port[4];
    uint16_t pin[4];
    uint8_t stepIndex;
    int8_t direction;
    uint16_t stepsRemaining;
} Stepper28BYJ_Context;

#define STEPPER28BYJ_SEQUENCE_LENGTH (8U)

static Stepper28BYJ_Context stepperCtx[STEPPER28BYJ_MOTOR_MAX] = {0};
static TIM_HandleTypeDef *sharedTimer = NULL;
static const uint8_t halfStepSequence[STEPPER28BYJ_SEQUENCE_LENGTH] = {
    0b0001U, 0b0011U, 0b0010U, 0b0110U,
    0b0100U, 0b1100U, 0b1000U, 0b1001U
};

// Helper that applies the 4-bit pattern to the four GPIO pins belonging to a motor.
static void Stepper28BYJ_WriteOutputs(const Stepper28BYJ_Context *ctx, uint8_t pattern)
{
    HAL_GPIO_WritePin(ctx->port[0], ctx->pin[0], (pattern & 0b0001U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ctx->port[1], ctx->pin[1], (pattern & 0b0010U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ctx->port[2], ctx->pin[2], (pattern & 0b0100U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ctx->port[3], ctx->pin[3], (pattern & 0b1000U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint8_t Stepper28BYJ_AnyBusy(void)
{
    for (uint8_t i = 0; i < STEPPER28BYJ_MOTOR_MAX; i++)
    {
        if (stepperCtx[i].stepsRemaining > 0U)
        {
            return 1U;
        }
    }
    return 0U;
}

// Registers one motor (stores its GPIO pins, resets its state, ensures both motors share the same timer).
HAL_StatusTypeDef Stepper28BYJ_Init(Stepper28BYJ_Motor motor,
                                    TIM_HandleTypeDef *timer,
                                    GPIO_TypeDef *portCoil1, uint16_t pinCoil1,
                                    GPIO_TypeDef *portCoil2, uint16_t pinCoil2,
                                    GPIO_TypeDef *portCoil3, uint16_t pinCoil3,
                                    GPIO_TypeDef *portCoil4, uint16_t pinCoil4)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX || timer == NULL)
    {
        return HAL_ERROR;
    }

    Stepper28BYJ_Context *ctx = &stepperCtx[motor];
    ctx->timer = timer;
    ctx->port[0] = portCoil1;
    ctx->pin[0] = pinCoil1;
    ctx->port[1] = portCoil2;
    ctx->pin[1] = pinCoil2;
    ctx->port[2] = portCoil3;
    ctx->pin[2] = pinCoil3;
    ctx->port[3] = portCoil4;
    ctx->pin[3] = pinCoil4;
    ctx->stepIndex = 0U;
    ctx->direction = STEPPER28BYJ_DIR_FORWARD;
    ctx->stepsRemaining = 0U;
    Stepper28BYJ_WriteOutputs(ctx, 0U);

    if (sharedTimer == NULL)
    {
        sharedTimer = timer;
    }
    else if (sharedTimer->Instance != timer->Instance)
    {
        return HAL_ERROR; /* both motors must share the same timer */
    }

    return HAL_OK;
}

// Schedules a move for that motor by loading steps/direction and starting timer interrupts if needed.
HAL_StatusTypeDef Stepper28BYJ_Move(Stepper28BYJ_Motor motor, uint16_t steps, int8_t direction)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX || sharedTimer == NULL || steps == 0U)
    {
        return HAL_ERROR;
    }

    Stepper28BYJ_Context *ctx = &stepperCtx[motor];
    if (ctx->timer == NULL)
    {
        return HAL_ERROR;
    }

    ctx->direction = (direction >= 0) ? STEPPER28BYJ_DIR_FORWARD : STEPPER28BYJ_DIR_REVERSE;
    ctx->stepsRemaining = steps;

    // If the shared timer is already running (other motor started it), HAL may return HAL_BUSY.
    // That is not a failure for this driver, because the move is already scheduled above.
    HAL_StatusTypeDef st = HAL_TIM_Base_Start_IT(sharedTimer);
    return (st == HAL_BUSY) ? HAL_OK : st;
}

// Halts a specific motor, de‑energises its coils, and stops timer once no motors remain active.
HAL_StatusTypeDef Stepper28BYJ_Stop(Stepper28BYJ_Motor motor)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX)
    {
        return HAL_ERROR;
    }

    Stepper28BYJ_Context *ctx = &stepperCtx[motor];
    if (ctx->timer == NULL)
    {
        return HAL_ERROR;
    }

    ctx->stepsRemaining = 0U;
    Stepper28BYJ_WriteOutputs(ctx, 0U);

    if (sharedTimer != NULL && Stepper28BYJ_AnyBusy() == 0U)
    {
        HAL_TIM_Base_Stop_IT(sharedTimer);
    }

    return HAL_OK;
}

// Returns 1 if the selected motor still has steps to execute, otherwise 0.
uint8_t Stepper28BYJ_IsBusy(Stepper28BYJ_Motor motor)
{
    if (motor >= STEPPER28BYJ_MOTOR_MAX)
    {
        return 0U;
    }
    return (stepperCtx[motor].stepsRemaining > 0U) ? 1U : 0U;
}

// Reprograms shared timer prescaler/period to LOW/MEDIUM/HIGH, only when both motors are idle.
HAL_StatusTypeDef Stepper28BYJ_SetSpeedPreset(Stepper28BYJ_Speed preset)
{
    if (sharedTimer == NULL)
    {
        return HAL_ERROR;
    }

    if (Stepper28BYJ_AnyBusy() == 1U)
    {
        return HAL_BUSY;
    }

    uint32_t prescaler;
    switch (preset)
    {
        case STEPPER28BYJ_SPEED_LOW:
            prescaler = 16799U;
            break;
        case STEPPER28BYJ_SPEED_HIGH:
            prescaler = 4199U;
            break;
        case STEPPER28BYJ_SPEED_MEDIUM:
        default:
            prescaler = 8399U;
            break;
    }

    sharedTimer->Init.Prescaler = prescaler;
    sharedTimer->Init.Period = 9U;
    return HAL_TIM_Base_Init(sharedTimer);
}

// ISR entry: on each timer overflow it advances active motors, updates stepsRemaining, and disables the timer when all moves finish.
void Stepper28BYJ_HandleTimerInterrupt(TIM_HandleTypeDef *htim)
{
    if ((sharedTimer == NULL) || (htim->Instance != sharedTimer->Instance))
    {
        return;
    }

    uint8_t anyActive = 0U;

    for (uint8_t motor = 0; motor < STEPPER28BYJ_MOTOR_MAX; motor++)
    {
        Stepper28BYJ_Context *ctx = &stepperCtx[motor];
        if ((ctx->timer == NULL) || (ctx->stepsRemaining == 0U))
        {
            continue;
        }

        anyActive = 1U;

        int8_t newIndex = (int8_t)ctx->stepIndex + ctx->direction;
        if (newIndex < 0)
        {
            newIndex = (int8_t)(STEPPER28BYJ_SEQUENCE_LENGTH - 1U);
        }
        else if (newIndex >= (int8_t)STEPPER28BYJ_SEQUENCE_LENGTH)
        {
            newIndex = 0;
        }
        ctx->stepIndex = (uint8_t)newIndex;

        Stepper28BYJ_WriteOutputs(ctx, halfStepSequence[ctx->stepIndex]);
        ctx->stepsRemaining--;
    }

    if (anyActive == 0U)
    {
        HAL_TIM_Base_Stop_IT(sharedTimer);
        for (uint8_t motor = 0; motor < STEPPER28BYJ_MOTOR_MAX; motor++)
        {
            if (stepperCtx[motor].timer != NULL)
            {
                Stepper28BYJ_WriteOutputs(&stepperCtx[motor], 0U);
            }
        }
    }
}
