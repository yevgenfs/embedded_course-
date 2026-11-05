#include "event.h"
#include "stm32f4xx_hal.h"
 
#define QUEUE_SIZE 16u /* must be a power of two */
#define QUEUE_MASK (QUEUE_SIZE - 1u)
 
static event_t queue[QUEUE_SIZE];
static volatile uint8_t head = 0; /* written by producers (ISRs)  */
static volatile uint8_t tail = 0; /* written by consumer (main)   */
static volatile uint32_t dropped = 0;
 
int event_post(event_type_t type)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
 
    uint8_t next = (uint8_t)((head + 1u) & QUEUE_MASK);
    if (next == tail)
    {
        dropped++;
        __set_PRIMASK(primask);
        return -1;
    }
 
    queue[head].type = type;
    queue[head].ts = HAL_GetTick();
    head = next;
 
    __set_PRIMASK(primask);
    return 0;
}
 
int event_get(event_t *out)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
 
    if (head == tail)
    {
        __set_PRIMASK(primask);
        return -1;
    }
 
    *out = queue[tail];
    tail = (uint8_t)((tail + 1u) & QUEUE_MASK);
 
    __set_PRIMASK(primask);
    return 0;
}
 
int event_is_empty(void)
{
    return head == tail;
}
 
uint32_t event_dropped(void)
{
    return dropped;
}