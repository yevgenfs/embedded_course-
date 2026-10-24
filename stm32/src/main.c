#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Red LED (on-board, active-low) */
#define LED_R_PORT   GPIOC
#define LED_R_PIN    GPIO_PIN_13

/* Green LED (external, active-high: PB0 -> resistor -> LED -> GND) */
#define LED_G_PORT   GPIOC
#define LED_G_PIN    GPIO_PIN_14

/* Buttons: one side to the pin, other side to GND */
#define BTN_R_PORT   GPIOA
#define BTN_R_PIN    GPIO_PIN_5
#define BTN_G_PORT   GPIOB
#define BTN_G_PIN    GPIO_PIN_6

#define DEBOUNCE_MS  100

static volatile uint32_t last_press_r = 0;
static volatile uint32_t last_press_g = 0;
static volatile bool press_r = false;
static volatile bool press_g = false;

#define DEBOUNCE_MS 50

static volatile uint32_t last_press = 0;

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    HAL_Init();
    GPIO_Init();

    while (1) {
    }
}