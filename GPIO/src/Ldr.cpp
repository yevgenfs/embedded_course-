#include "Ldr.h"
#include "config.h"
#include <Arduino.h>

bool Ldr::get_state()
{
    return ldrState;
}

void Ldr::init()
{
    pinMode(LDR, INPUT);         // налаштовуємо пін на ВХІД (btn)
    filterInit(analogRead(LDR)); // filter  init
}

void Ldr::poll()
{
    if (millis() - lastTimeChangeLdr > LDR_PERIOD_MS && err_count < 5)
    {
        lastTimeChangeLdr = millis();
        u16_t adc_value = analogRead(LDR);
        u16_t filtered_adc_value = movingAvg(adc_value);
        if ( 0 == filtered_adc_value)
        {
            err_count++;
            ldrState = true;
            return;

        }

        if (filtered_adc_value < LDR_THRESHOLD)
        {
            ldrState = true;
            Serial.printf("adc raw value %d adc filtered  %d  %d \n", adc_value, filtered_adc_value , err_count );
        }
        else
        {
            ldrState = false;
        }

        // Serial.printf("adc raw value %d adc filtered  %d \n", adc_value, filtered_adc_value);
    }
}

void Ldr::filterInit(u16_t initData)
{
    for (u8_t i = 0; i < N; i++)
    {
        readings[i] = initData;
        sum += initData;
    }
}

u16_t Ldr::movingAvg(u16_t rawData)
{
    sum -= readings[idx];
    sum += rawData;
    readings[idx] = rawData;
    idx++;

    if (idx >= N)
    {
        idx = 0;
    }

    return sum / N;
}