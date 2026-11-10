#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>

#define GPIO_PORT GPIOA
#define LED_PIN GPIO_PIN_1
#define LDR_PIN GPIO_PIN_0
#define LED_ACTIVE_HIGH 1

#define TH_DARK  2100  /* filtered value below this  -> dark, LED ON  */
#define TH_LIGHT 2700 /* filtered value above this  -> light, LED OFF */

/* ---------- globals (shared with ISR context) ---------- */
ADC_HandleTypeDef  hadc1;
TIM_HandleTypeDef  htim2;
UART_HandleTypeDef huart2;

volatile uint16_t g_raw = 0;
volatile bool     g_new = false;

/* ---------- helpers ---------- */
static void led_set(int on)
{
    GPIO_PinState s = (on == LED_ACTIVE_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(GPIO_PORT, LED_PIN, s);
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    g.Pin = LED_PIN; /* LED output */
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PORT, &g);
    led_set(0);

    g.Pin = GPIO_PIN_2;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIO_PORT, &g);

}

static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static void ADC_Init(void)
{
}

static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16000 - 1; /* 16 MHz / 16000 = 1 kHz */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 20 - 1; /* 1 kHz / 20 = 50 Hz */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim2);
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    ADC_Init();
    TIM2_Init();

    setvbuf(stdout, NULL, _IONBF, 0); /* no buffering: text appears immediately */

    while (1)
    {
        if (g_new)
        {
            printf("raw value %d \n", g_raw);
            g_new = false;
        }
    }
}

#ifdef __cplusplus
extern "C"
{
#endif
    /* ---------------- Interrupt handlers ---------------- */

    void SysTick_Handler(void)
    {
        HAL_IncTick();
    }

    int _write(int fd, char *ptr, int len)
    {
        (void)fd;
        HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
        return len;
    }

    void TIM2_IRQHandler(void)
    {
        HAL_TIM_IRQHandler(&htim2);
    }

    void ADC_IRQHandler(void)
    {
        HAL_ADC_IRQHandler(&hadc1); /* calls HAL_ADC_ConvCpltCallback */
    }

    /* ---------------- Callbacks ---------------- */

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
    {
        if (TIM2 == htim->Instance)
        {
        }
    }

    void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
    {
    }

#ifdef __cplusplus
}
#endif

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}