#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdio.h>

#define GPIO_PORT GPIOA
#define LED_PIN GPIO_PIN_1
#define LDR_PIN GPIO_PIN_0
#define LED_ACTIVE_HIGH 1

#define TH_DARK 2100
#define TH_LIGHT 2700

#define LDR_PERIOD_MS 20U   /* 50 Hz */
#define VREF_PERIOD_MS 100U /* 10 Hz */

/* ---------- VREFINT calibration ----------
 *   VDDA [mV] = 3300 * VREFINT_CAL / VREFINT_DATA
 *   Vin  [mV] = raw * VDDA / 4095
 * VREFINT_CAL: factory value measured at VDDA = 3.3 V, 30 C
 * (STM32F411: address 0x1FFF7A2A, check the datasheet table).
 */
#define VDDA_NOMINAL_MV 3300U
#define VREFINT_AVG_N 8 /* average of the last N VREFINT samples, 1 = no averaging */

/* ---------- globals (shared with ISR context) ---------- */
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;
UART_HandleTypeDef huart2;

volatile uint16_t g_raw = 0;
volatile bool g_new = false;
volatile bool g_dark = false;
volatile uint16_t g_vdda_mv = VDDA_NOMINAL_MV; /* updated by the injected-group interrupt */

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

    g.Pin = GPIO_PIN_2; /* USART2 TX */
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIO_PORT, &g);

    g.Pin = LDR_PIN; /* LDR input */
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
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

/* Generic helper: timer that only produces a TRGO pulse on every update event. */
static void TimerTrgo_Init(TIM_HandleTypeDef *h, TIM_TypeDef *inst, uint32_t period_ms)
{
    h->Instance = inst;
    h->Init.Prescaler = 16000 - 1; /* 16 MHz / 16000 = 1 kHz */
    h->Init.CounterMode = TIM_COUNTERMODE_UP;
    h->Init.Period = period_ms - 1; /* 1 kHz / period_ms */
    h->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(h);

    __HAL_TIM_CLEAR_FLAG(h, TIM_FLAG_UPDATE);

    TIM_MasterConfigTypeDef m = {0};
    m.MasterOutputTrigger = TIM_TRGO_UPDATE;
    m.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(h, &m);
    __HAL_TIM_CLEAR_FLAG(h, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(h);
}

void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART2_Init();

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();
    TimerTrgo_Init(&htim2, TIM2, LDR_PERIOD_MS);
    TimerTrgo_Init(&htim5, TIM5, VREF_PERIOD_MS);
    ADC_Init();

    setvbuf(stdout, NULL, _IONBF, 0); /* no buffering: text appears immediately */

    HAL_ADC_Start_IT(&hadc1);           /* regular group: EOC interrupt on every TIM2 TRGO */
    HAL_ADCEx_InjectedStart_IT(&hadc1); /* injected group: JEOC interrupt on every TIM5 TRGO */

    while (1)
    {
        if (g_new)
        {
            uint16_t raw = g_raw;
            uint32_t vdda = g_vdda_mv;
            g_new = false;

            uint32_t before = raw * VDDA_NOMINAL_MV / 4095u;
            uint32_t after = raw * vdda / 4095u;

                printf("raw=%u  vdda=%u mV  before=%u mV  after=%u mV  diff=%d mV\r\n",
                       (unsigned)raw, (unsigned)vdda, (unsigned)before, (unsigned)after,
                       (int)after - (int)before);
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
        if (ADC1 == hadc->Instance)
        {
            g_raw = (uint16_t)HAL_ADC_GetValue(hadc);
            g_new = true;

            if (!g_dark && g_raw < TH_DARK)
            {
                g_dark = true;
                led_set(1);
            }
            else if (g_dark && g_raw > TH_LIGHT)
            {
                g_dark = false;
                led_set(0);
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
        {
            return;
        }
        uint16_t data = (uint16_t)HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);
        sum -= buf[idx];
        buf[idx] = data;
        sum += data;
        idx = (idx + 1) % VREFINT_AVG_N;

        if (cnt < VREFINT_AVG_N)
        {
            cnt++;
        }

        uint32_t avg = sum / cnt;

        if (0 != avg)
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