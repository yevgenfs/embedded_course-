#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>

/* ---------- pins ---------- */
#define SERVO_PORT GPIOA
#define SERVO_PIN GPIO_PIN_1 /* TIM2_CH2, AF1 */
#define ENC_PORT GPIOB
#define ENC_A_PIN GPIO_PIN_6 /* TIM4_CH1, AF2 */
#define ENC_B_PIN GPIO_PIN_7 /* TIM4_CH2, AF2 */

/* ---------- servo PWM: 50 Hz, pulse in microseconds ---------- */
#define SERVO_FREQ_HZ 50U
#define TIM_TICK_HZ 1000000U /* 16 MHz / PSC 16 = 1 MHz -> 1 tick = 1 us */
#define PWM_PSC 16U
#define PWM_ARR (TIM_TICK_HZ / SERVO_FREQ_HZ - 1U) /* 20000 - 1 = 20 ms */

#define SERVO_MIN_US 500U  /* pulse for 0 deg   */
#define SERVO_MAX_US 2400U /* pulse for 180 deg */
#define SERVO_MAX_ANGLE 180U

/* ---------- encoder ---------- */
#define ENC_COUNTS_PER_DETENT 4 /* EC11: 4 counts per click (use 2 if it skips) */
#define ENC_DEG_PER_DETENT 5    /* servo degrees per click */
#define ENC_MAX_COUNTS ((SERVO_MAX_ANGLE / ENC_DEG_PER_DETENT) * ENC_COUNTS_PER_DETENT)

/* ---------- globals ---------- */
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart2;

/* ---------- helpers ---------- */

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

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

    /* Encoder A/B: timer inputs with pull-ups (encoder common goes to GND) */
    g.Pin = ENC_A_PIN | ENC_B_PIN;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    g.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(ENC_PORT, &g);
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

/* TIM4 in hardware quadrature encoder mode (counts on both edges of A and B) */
static void TIM4_Encoder_Init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 0xFFFF;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    TIM_Encoder_InitTypeDef e = {0};
    e.EncoderMode = TIM_ENCODERMODE_TI12;
    e.IC1Polarity = TIM_ICPOLARITY_RISING;
    e.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    e.IC1Prescaler = TIM_ICPSC_DIV1;
    e.IC1Filter = 15;
    e.IC2Polarity = TIM_ICPOLARITY_RISING;
    e.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    e.IC2Prescaler = TIM_ICPSC_DIV1;
    e.IC2Filter = 15;

    HAL_TIM_Encoder_Init(&htim4, &e);

    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
}

/* Set servo angle 0..180 deg */
static void set_servo_angle(uint32_t angle)
{
    if (angle > SERVO_MAX_ANGLE)
    {
        angle = SERVO_MAX_ANGLE;
    }

    uint32_t pulse_us = SERVO_MIN_US +
                        ((SERVO_MAX_US - SERVO_MIN_US) * angle) / SERVO_MAX_ANGLE;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pulse_us); /* 1 tick = 1 us */
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    TIM2_Init();
    TIM4_Encoder_Init();

    setvbuf(stdout, NULL, _IONBF, 0); /* no buffering: text appears immediately */

    int32_t pos = ENC_MAX_COUNTS / 2;
    uint16_t last_cnt = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
    int last_angle = -1;

    printf("Encoder servo control started\r\n");

    while (1)
    {

        uint16_t cnt = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
        int16_t delta = (uint16_t)(cnt - last_cnt);
        last_cnt = cnt;

        pos += delta;

        if (0 > pos)
        {
            pos = 0;
        }
        if (ENC_MAX_COUNTS < (uint32_t)pos)
        {
            pos = ENC_MAX_COUNTS;
        }

        int angle = (int)((pos * ENC_DEG_PER_DETENT) / ENC_COUNTS_PER_DETENT);

        if (angle != last_angle)
        {
            last_angle = angle;
            set_servo_angle((uint32_t)angle);
            printf("enc=%3ld  angle=%3d deg\r\n", (long)pos, angle);
        }

        HAL_Delay(5);
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