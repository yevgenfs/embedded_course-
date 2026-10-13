#pragma once
#include <Arduino.h>

class Button
{
private:
    u32_t lastTimeChangeBtn = 0;
    u32_t btnTimekeeper = 0;
    bool buttonState = false;

public:
    void init();
    void poll();
    bool get_state();
};
