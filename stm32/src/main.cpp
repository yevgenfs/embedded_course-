#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>

#define LDR_PORT GPIOA
#define LDR_PIN GPIO_PIN_0 /* ADC1_IN0 */
#define LED_PORT GPIOA
#define LED_PIN GPIO_PIN_1 /* TIM2_CH2, AF1 */
#define LED_ACTIVE_HIGH 1  /* 1 = LED lights with HIGH, 0 = lights with LOW (PWM output polarity) */

#define PWM_FREQ_HZ 1000U
#define TIM_TICK_HZ 1000000U /* 16 MHz / PSC 16 */
#define PWM_PSC 16U
#define PWM_ARR (TIM_TICK_HZ / PWM_FREQ_HZ - 1U)
#define PWM_STEPS (PWM_ARR + 1U)

#define PWM_RAW_DARK 1500U
#define PWM_RAW_LIGHT 3000U

#define PRINT_EVERY 100U /* ADC runs at 1 kHz, print 10 lines per second */

/* ---------- TIM5: trigger for the injected group (VREFINT) ---------- */
#define VREF_PERIOD_MS 100U /* 10 Hz */

#define VREFINT_CAL_ADDR ((const uint16_t *)0x1FFF7A2AU)
#define VDDA_NOMINAL_MV 3300U
#define VREFINT_AVG_N 8 /* average of the last N VREFINT samples, 1 = no averaging */

/* ---------- globals (shared with ISR context) ---------- */
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;
UART_HandleTypeDef huart2;

volatile uint16_t g_raw = 0;
volatile uint32_t g_duty = 0; /* current CCR value */
volatile bool g_new = false;
volatile uint32_t g_vdda_mv = VDDA_NOMINAL_MV;

/* ---------- helpers ---------- */
static uint32_t duty_from_raw(uint16_t raw)
{
    if (PWM_RAW_DARK >= raw)
    {
        return PWM_STEPS;
    }
    if (PWM_RAW_LIGHT <= raw)
    {
        return 0;
    }
    return (uint32_t)(PWM_RAW_LIGHT - raw) * PWM_STEPS / (PWM_RAW_LIGHT - PWM_RAW_DARK);
}

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

    g.Pin = LDR_PIN; /* LDR input */
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LDR_PORT, &g);
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
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};

    ch.Channel = ADC_CHANNEL_0;
    ch.Rank = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    ADC_InjectionConfTypeDef inj = {0};

    inj.InjectedChannel = ADC_CHANNEL_VREFINT;
    inj.InjectedRank = ADC_INJECTED_RANK_1;
    inj.InjectedNbrOfConversion = 1;
    inj.InjectedSamplingTime = ADC_SAMPLETIME_480CYCLES;
    inj.InjectedOffset = 0;
    inj.InjectedDiscontinuousConvMode = DISABLE;
    inj.AutoInjectedConv = DISABLE;
    inj.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_RISING;
    inj.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJECCONV_T5_TRGO;
    HAL_ADCEx_InjectedConfigChannel(&hadc1, &inj);

    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
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

    TIM_MasterConfigTypeDef m = {0};
    m.MasterOutputTrigger = TIM_TRGO_UPDATE; /* update event -> TRGO -> ADC trigger (kept) */
    m.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &m);

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

static void TIM5_Init(uint32_t period_ms)
{
    __HAL_RCC_TIM5_CLK_ENABLE();

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 16000 - 1; /* 16 MHz / 16000 = 1 kHz */
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = period_ms - 1;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim5);

    __HAL_TIM_CLEAR_FLAG(&htim5, TIM_FLAG_UPDATE);

    TIM_MasterConfigTypeDef m = {0};
    m.MasterOutputTrigger = TIM_TRGO_UPDATE;
    m.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim5, &m);
    __HAL_TIM_CLEAR_FLAG(&htim5, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&htim5);
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();
    TIM2_Init();
    TIM5_Init(VREF_PERIOD_MS);
    ADC_Init();

    setvbuf(stdout, NULL, _IONBF, 0); /* no buffering: text appears immediately */

    HAL_ADC_Start_IT(&hadc1);           /* regular group: EOC interrupt on every TIM2 TRGO */
    HAL_ADCEx_InjectedStart_IT(&hadc1); /* injected group: JEOC interrupt on every TIM5 TRGO */

    while (1)
    {
        if (g_new)
        {
            uint16_t raw = g_raw;
            uint32_t duty = g_duty;
            uint32_t vdda = g_vdda_mv;
            g_new = false;

            uint32_t before = (uint32_t)raw * VDDA_NOMINAL_MV / 4095u; /* assumes VDDA = 3.300 V */
            uint32_t after = (uint32_t)raw * vdda / 4095u;             /* uses measured VDDA     */

            printf("raw=%u  vdda=%u mV  before=%u mV  after=%u mV  led=%u%%\r\n",
                   (unsigned)raw, (unsigned)vdda, (unsigned)before, (unsigned)after,
                   (unsigned)(duty * 100u / PWM_STEPS));
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

    void ADC_IRQHandler(void)
    {
        HAL_ADC_IRQHandler(&hadc1);
    }

    /* ---------------- Callbacks ---------------- */

    void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
    {
        static uint16_t cnt = 0;

        if (ADC1 == hadc->Instance)
        {
            uint16_t raw = (uint16_t)HAL_ADC_GetValue(hadc);
            uint32_t duty = duty_from_raw(raw);
            
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, duty);

            g_raw = raw;
            g_duty = duty;
            if (++cnt >= PRINT_EVERY)
            {
                cnt = 0;
                g_new = true;
            }
        }
    }

    void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
    {
        static uint16_t buf[VREFINT_AVG_N];
        static uint32_t sum = 0;
        static uint8_t idx = 0;
        static uint8_t cnt = 0;

        if (ADC1 != hadc->Instance)
            return;

        uint16_t data = (uint16_t)HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);
        sum -= buf[idx];
        buf[idx] = data;
        sum += data;
        idx = (uint8_t)((idx + 1) % VREFINT_AVG_N);
        if (cnt < VREFINT_AVG_N)
            cnt++;

        uint32_t avg = sum / cnt;
        if (avg != 0)
        {
            g_vdda_mv = VDDA_NOMINAL_MV * (*VREFINT_CAL_ADDR) / avg;
        }
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