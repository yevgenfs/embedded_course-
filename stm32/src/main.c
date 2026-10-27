#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define LED_PORT  GPIOC
#define LED_PIN   GPIO_PIN_13
 
TIM_HandleTypeDef htim2;
 
static void GPIO_Init(void);
static void TIM2_Init(void);
void Error_Handler(void);
 
int main(void)
{
    HAL_Init();
    GPIO_Init();
    TIM2_Init();
 
    while (1) {

    }
}
 
static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
 
    __HAL_RCC_GPIOC_CLK_ENABLE();
 
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);   /* LED off */
 
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
}
 
static void TIM2_Init(void)
{

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

}
 
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}