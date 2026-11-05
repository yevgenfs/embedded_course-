#include "stm32f4xx_hal.h"
#include "event.h"
#include "traffic_light.h"
 
extern "C" void Error_Handler(void);
 
/* ======================= Main loop = dispatcher ======================= */
 
int main(void)
{
    HAL_Init();
    traffic_light_init();
 
    event_t e;
 
    while (1)
    {
        while (event_get(&e) == 0)
        {
            traffic_light_dispatch(&e);
        }
 
        __disable_irq();
        if (event_is_empty())
        {
            __WFI();
        }
        __enable_irq();
    }
}
 
extern "C"
{
    void SysTick_Handler(void)
    {
        HAL_IncTick();
    }
 
    void Error_Handler(void)
    {
        __disable_irq();
        while (1)
        {
        }
    }
}