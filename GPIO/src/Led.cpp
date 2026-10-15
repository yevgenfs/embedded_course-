#include "Led.h"
#include "config.h"
#include <Arduino.h>

void Led::init()
{
    pinMode(LED, OUTPUT); // налаштовуємо пін на ВИХІД (led)
}

void Led::handler(bool light_on)
{
    
    digitalWrite(LED, (light_on) ? HIGH : LOW);
}