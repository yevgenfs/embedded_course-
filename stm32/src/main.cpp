#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ---------- pins ---------- */
#define SERVO_PORT GPIOA
#define SERVO_PIN GPIO_PIN_1 /* TIM2_CH2, AF1 */
#define POT_PORT GPIOA
#define POT_PIN GPIO_PIN_0 /* ADC1_IN0 */

/* ---------- servo PWM: 50 Hz, pulse in microseconds ---------- */
#define SERVO_FREQ_HZ 50U
#define TIM_TICK_HZ 1000000U /* 16 MHz / PSC 16 = 1 MHz -> 1 tick = 1 us */
#define PWM_PSC 16U
#define PWM_ARR (TIM_TICK_HZ / SERVO_FREQ_HZ - 1U) /* 20000 - 1 = 20 ms */

#define SERVO_MIN_US 1000U /* pulse for 0 deg   (tune for your servo) */
#define SERVO_MAX_US 2000U /* pulse for 180 deg (tune for your servo) */
#define SERVO_MAX_ANGLE 180U

#define ADC_MAX 4095U
#define ADC_SAMPLES 8U
#define ANGLE_HYST 1 /* ignore changes smaller than this (deg) -> no jitter */

/* ---------- globals ---------- */
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;

/* ---------- helpers ---------- */

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    /* Servo PWM output */
    g.Pin = SERVO_PIN;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(SERVO_PORT, &g);

    /* USART2 TX */
    g.Pin = GPIO_PIN_2;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);

    /* Potentiometer: analog input */
    g.Pin = POT_PIN;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(POT_PORT, &g);
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
    htim2.Init.Prescaler = PWM_PSC - 1; /* 1 MHz tick */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = PWM_ARR; /* 20 ms period = 50 Hz */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = (SERVO_MIN_US + SERVO_MAX_US) / 2U; /* start at center */
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

static void ADC1_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel = ADC_CHANNEL_0; /* PA0 */
    ch.Rank = 1;
    ch.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);
}


static uint32_t adc_read_avg(void)
{
    uint32_t sum = 0;

    for (uint32_t i = 0; i < ADC_SAMPLES; i++)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 10);
        sum += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }
    return sum / ADC_SAMPLES;
}

/* Set servo angle 0..180 deg */
static void set_servo_angle(uint32_t angle)
{

}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    TIM2_Init();
    ADC1_Init();

    setvbuf(stdout, NULL, _IONBF, 0); /* no buffering: text appears immediately */

    printf("Servo control started\r\n");

    int last_angle = -100; /* force first update */

    while (1)
    {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500);
        HAL_Delay(2000); // мінімум
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 2400);
        HAL_Delay(2000); // максимум
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1450);
        HAL_Delay(2000); // центр

        // uint32_t raw = adc_read_avg();

        // int angle = 

        // if (abs(angle - last_angle) >= ANGLE_HYST)
        // {
        //     last_angle = angle;
        //     set_servo_angle((uint32_t)angle);
        //     printf("ADC=%4lu  angle=%3d deg\r\n", (unsigned long)raw, angle);
        // }

        // HAL_Delay(20); /* one servo period */
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