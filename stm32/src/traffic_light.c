#include "traffic_light.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* ---- Pins (change to match your wiring) ---- */
#define LIGHT_PORT GPIOA
#define RED_PIN GPIO_PIN_1 /* active high */
#define YELLOW_PIN GPIO_PIN_2
#define GREEN_PIN GPIO_PIN_3

#define BTN_PORT GPIOB
#define BTN_PIN GPIO_PIN_0 /* to 3V3 when pressed, internal pull-down */
#define BTN_IRQn EXTI0_IRQn

/* ---- Timing (ms) ---- */
#define DEBOUNCE_MS 50u
#define T_GREEN_HOLD_MS 5000u
#define T_BLINK_HALF_S 500u
#define BLINK_HALF_PERIODS 6u /* 3 blinks = 6 half periods               */
#define T_YELLOW_MS 3000u
#define T_RED_MS 5000u
#define T_RED_YELLOW_MS 3000u

typedef enum
{
    ST_GREEN_WAIT,  /* green, waiting for the button          */
    ST_GREEN_HOLD,  /* green, 5 s countdown                   */
    ST_GREEN_BLINK, /* green blinking 3 times                 */
    ST_YELLOW,      /* yellow, 3 s                            */
    ST_RED,         /* red, 5 s                               */
    ST_RED_YELLOW,  /* red + yellow, 3 s, then back to green  */
} state_t;

static state_t state;
static uint8_t blink_left;
static bool blink_green_on;

static TIM_HandleTypeDef htim2;

/* Software countdowns, both served by the single 1 ms TIM2 interrupt */
static volatile uint32_t debounce_ms = 0; /* button debounce window */
static volatile uint32_t fsm_ms = 0;      /* FSM one-shot timer     */

void Error_Handler(void); /* defined in main.cpp */

/* ======================= Hardware init ======================= */

static void lights_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = RED_PIN | YELLOW_PIN | GREEN_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LIGHT_PORT, &gpio);
    HAL_GPIO_WritePin(LIGHT_PORT, RED_PIN | YELLOW_PIN | GREEN_PIN, GPIO_PIN_RESET);
}

static void button_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Rising edge, debounced in software by TIM2 */
    gpio.Pin = BTN_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BTN_PORT, &gpio);

    HAL_NVIC_SetPriority(BTN_IRQn, 2, 1);
    HAL_NVIC_EnableIRQ(BTN_IRQn);
}

static void timer_init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 1600 - 1; // 16 MHz / 1600 = 10 kHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 10 - 1; // 10 ticks at 10 kHz = 1 ms interrupt
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE); /* drop the UG flag set by Init */
    HAL_TIM_Base_Start_IT(&htim2);                 /* runs forever, 1 ms tick */
}

/* ======================= Hardware helpers ======================= */

static void lights_set(bool red, bool yellow, bool green)
{
    HAL_GPIO_WritePin(LIGHT_PORT, RED_PIN, red ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGHT_PORT, YELLOW_PIN, yellow ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGHT_PORT, GREEN_PIN, green ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void timer_start(uint32_t ms)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    fsm_ms = ms;
    __set_PRIMASK(primask);
}

static void timer_stop(void)
{
    timer_start(0);
}

/* ======================= ISRs: only count and post events ======================= */

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BTN_PIN);
}

void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

/* First edge: mute the line and open the debounce window */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (BTN_PIN == pin)
    {
        HAL_NVIC_DisableIRQ(BTN_IRQn);
        debounce_ms = DEBOUNCE_MS;
    }
}

/* 1 ms tick: serves the debounce window and the FSM timer */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (TIM2 == htim->Instance)
    {
        if (debounce_ms != 0 && --debounce_ms == 0u)
        {
            if (GPIO_PIN_SET == HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN))
            {
                event_post(EVT_BTN_PRESS);
            }

            __HAL_GPIO_EXTI_CLEAR_IT(BTN_PIN);
            HAL_NVIC_ClearPendingIRQ(BTN_IRQn);
            HAL_NVIC_EnableIRQ(BTN_IRQn);
        }

        if (fsm_ms != 0 && --fsm_ms == 0u)
        {
            event_post(EVT_TIMEOUT);
        }
    }
}

/* ======================= State entry actions ======================= */

static void enter_green_wait(void)
{
    state = ST_GREEN_WAIT;
    lights_set(false, false, true);
    timer_stop();
}

static void enter_green_hold(void)
{
    state = ST_GREEN_HOLD;
    timer_start(T_GREEN_HOLD_MS);
}

static void enter_green_blink(void)
{
    state = ST_GREEN_BLINK;
    blink_left = BLINK_HALF_PERIODS;
    blink_green_on = true;
    timer_start(T_BLINK_HALF_S);
}

static void enter_yellow(void)
{
    state = ST_YELLOW;
    lights_set(false, true, false);
    timer_start(T_YELLOW_MS);
}

static void enter_red(void)
{
    state = ST_RED;
    lights_set(true, false, false);
    timer_start(T_RED_MS);
}

static void enter_red_yellow(void)
{
    state = ST_RED_YELLOW;
    lights_set(true, true, false);
    timer_start(T_RED_YELLOW_MS);
}

/* ======================= Public API ======================= */

void traffic_light_init(void)
{
    lights_gpio_init();
    enter_green_wait();
    button_init();
    timer_init();
}

void traffic_light_dispatch(const event_t *e)
{
    switch (state)
    {
    case ST_GREEN_WAIT:
        if (EVT_BTN_PRESS == e->type)
        {
            enter_green_hold();
        }
        break;

    case ST_GREEN_HOLD:
        if (EVT_TIMEOUT == e->type)
        {
            enter_green_blink();
        }
        break;

    case ST_GREEN_BLINK:
        if (EVT_TIMEOUT == e->type)
        {
            if (0u == blink_left)
            {
                enter_yellow();
            }
            else
            {
                blink_left--;
                blink_green_on = !blink_green_on;
                lights_set(false, false, blink_green_on);
                timer_start(T_BLINK_HALF_S);
            }
        }
        break;

    case ST_YELLOW:
        if (EVT_TIMEOUT == e->type)
        {
            enter_red();
        }
        break;

    case ST_RED:
        if (EVT_TIMEOUT == e->type)
        {
            enter_red_yellow();
        }
        break;

    case ST_RED_YELLOW:
        if (EVT_TIMEOUT == e->type)
        {
            enter_green_wait();
        }
        break;
    }
}