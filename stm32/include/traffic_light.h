#pragma once
#include "event.h"
 
#ifdef __cplusplus
extern "C" {
#endif
 
/* Sets up everything the traffic light owns: LED pins, button (EXTI),
   TIM2 (1 ms tick) and their interrupts. Starts in the green state. */
void traffic_light_init(void);
 
/* Run one event through the state machine (main loop only) */
void traffic_light_dispatch(const event_t *e);
 
#ifdef __cplusplus
}
#endif
 