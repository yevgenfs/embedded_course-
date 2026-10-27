#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define LED_PORT GPIOC
#define LED_PIN GPIO_PIN_13

TIM_HandleTypeDef htim2;

static void GPIO_Init(void);
static void TIM2_Init(void);
void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    TIM2_Init();

    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    while (1)
    {
    }
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16000 - 1;            // 16 Mhz -> 1kHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP; // 16 Mhz
    htim2.Init.Period = 1000 - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/* ---------------- Interrupt handlers ---------------- */

/* Required by HAL: drives HAL_GetTick()/HAL_Delay() */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (TIM2 == htim->Instance)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}