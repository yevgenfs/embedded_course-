#pragma once

#include <Arduino.h>
#include "config.h"

class Ldr
{
private:
    u16_t readings[N];
    u8_t  idx = 0;
    u32_t sum = 0;
    bool ldrState = false;
    u32_t lastTimeChangeLdr = 0;
    u8_t  err_count = 0;

    u16_t movingAvg(u16_t rawData);
    void filterInit(u16_t initData);

public:
    void init();
    void poll();
    bool get_state();
};
