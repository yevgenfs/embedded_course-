#pragma once

#include <Arduino.h>

class Led
{
private:
    /* data */
public:
    void init();
    void handler(bool light_on);
};

