#include "Button.h"
#include "config.h"
#include <Arduino.h>

void Button::init()
{
    pinMode(BTN, INPUT_PULLDOWN); // налаштовуємо пін на ВХІД (btn)
}

void Button::poll()
{
    if (millis() - lastTimeChangeBtn > BTN_PERIOD_MS) // періодична пекревірка стану кнопки
    {
        lastTimeChangeBtn = millis();
        u16_t btn_value = digitalRead(BTN);

        if (btn_value == HIGH)
        {
            if (btnTimekeeper == 0)
            {
                btnTimekeeper = millis(); //  якщо кнопка нажата і каунтер на 0 тоді івент тіки трапився і ми присвоюєм цій змінні час івента
            }
            else if (millis() - btnTimekeeper > BTN_LONG_PRESS_MS)
            {
                buttonState = true;
                // while (true)
                // {
                    Serial.printf("btn %d \n", millis());
                // }
                delay(6000);
            }
        }
        else
        {
            buttonState = false;
            btnTimekeeper = 0;
        }
    }
}

bool Button::get_state()
{
    return buttonState;
}
