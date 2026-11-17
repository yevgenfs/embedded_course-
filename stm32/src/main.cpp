#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>

#define LED_PORT GPIOA
#define LED_PIN GPIO_PIN_1 /* TIM2_CH2, AF1 */

#define PWM_FREQ_HZ 1000U
#define TIM_TICK_HZ 1000000U  /* 16 MHz / PSC 16 */
#define TIM_FREQ_HZ 16000000U /* 16 MHz */
#define PWM_PSC 16U
#define PWM_ARR (TIM_TICK_HZ / PWM_FREQ_HZ - 1U)

/* ---------- globals (shared with ISR context) ---------- */
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

/* ---------- helpers ---------- */

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    g.Pin = LED_PIN;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(LED_PORT, &g);

    g.Pin = GPIO_PIN_2; /* USART2 TX */
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);
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

static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = PWM_PSC - 1; /* 16 MHz / 16 = 1 MHz */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = PWM_ARR; /* 1 MHz / 1000 = 1 kHz */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2);

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

static void set_tone_freq(int freq_hz)
{
    if (0 == freq_hz)
    {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
        return;
    }

    uint32_t f_clk = TIM_FREQ_HZ;
    uint32_t psc = PWM_PSC - 1;
    uint32_t arr = (f_clk / (psc + 1)) / freq_hz - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, arr / 2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

static void play_note(int frequency_hz, int duration_ms)
{
    set_tone_freq(frequency_hz);
    HAL_Delay(duration_ms);
    set_tone_freq(0);
    HAL_Delay(50);
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    TIM2_Init();

    setvbuf(stdout, NULL, _IONBF, 0); /* no buffering: text appears immediately */

    while (1)
    {
        play_note(262, 400);
        play_note(262, 400); // До, До
        play_note(392, 400);
        play_note(392, 400); // Соль, Соль
        play_note(440, 400);
        play_note(440, 400); // Ля, Ля
        play_note(392, 800); // Соль (довга)

        play_note(349, 400);
        play_note(349, 400); // Фа, Фа
        play_note(330, 400);
        play_note(330, 400); // Мі, Мі
        play_note(294, 400);
        play_note(294, 400); // Ре, Ре
        play_note(262, 800); // До (довга)
        HAL_Delay(2000);
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