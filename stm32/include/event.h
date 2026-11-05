#pragma once
#include <stdint.h>
 
#ifdef __cplusplus
extern "C" {
#endif
 
typedef enum
{
    EVT_NONE = 0,
    EVT_BTN_PRESS,
    EVT_TIMEOUT,
} event_type_t;
 
typedef struct
{
    event_type_t type;
    uint32_t ts; /* HAL_GetTick() at the moment of posting */
} event_t;
 
/* Safe to call from any ISR. 0 = ok, -1 = queue full (event dropped). */
int event_post(event_type_t type);
 
/* Main loop only. 0 = event copied to *out, -1 = queue empty. */
int event_get(event_t *out);
 
int event_is_empty(void);
uint32_t event_dropped(void);
 
#ifdef __cplusplus
}
#endif