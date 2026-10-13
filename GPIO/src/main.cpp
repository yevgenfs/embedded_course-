#include <Arduino.h>

#define LED 18
#define LDR 2
#define BTN 17

#define LDR_PERIOD_MS 10
#define BTN_PERIOD_MS 50

#define BTN_LONG_PRESS_MS 2000
#define LDR_THRESHOLD 2000

#define N 32

u16_t readings[N];
u8_t idx = 0;
u32_t sum = 0;

bool buttonState = false;
bool ldrState = false;
// u8_t ledState_B = 0;

u32_t lastTimeChangeLdr = 0;
u32_t lastTimeChangeBtn = 0;
u32_t btnTimekeeper = 0;

u16_t movingAvg(u16_t rawData)
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

void filterInit(u16_t initData)
{
  for (u8_t i = 0; i < N; i++)
  {
    readings[i] = initData;
    sum += initData;
  }
}

void setup()
{
  pinMode(LED, OUTPUT);         // налаштовуємо пін на ВИХІД (led)
  pinMode(LDR, INPUT);          // налаштовуємо пін на ВХІД (ldr)
  pinMode(BTN, INPUT_PULLDOWN); // налаштовуємо пін на ВХІД (btn)
  Serial.begin(115200);
  Serial.println("setuped\n");
  filterInit(analogRead(LDR));
}

void loop()
{
  if (millis() - lastTimeChangeLdr > LDR_PERIOD_MS)
  {
    lastTimeChangeLdr = millis();
    u16_t adc_value = analogRead(LDR);
    u16_t filtered_adc_value = movingAvg(adc_value);

    if (filtered_adc_value < LDR_THRESHOLD)
    {
      ldrState = true;
    }
    else
    {
      ldrState = false;
    }

    Serial.printf("adc raw value %d adc filtered  %d \n", adc_value, filtered_adc_value);
  }

  if (millis() - lastTimeChangeBtn > BTN_PERIOD_MS)
  {
    lastTimeChangeBtn = millis();
    u16_t btn_value = digitalRead(BTN);
    Serial.printf("btn raw value %d \n", btn_value);

    if (btn_value == HIGH)
    {
      if (btnTimekeeper == 0)
      {
        btnTimekeeper = millis();
      }
      else if (millis() - btnTimekeeper > BTN_LONG_PRESS_MS)
      {
        buttonState = true;
      }
    }
    else
    {
      buttonState = false;
      btnTimekeeper = 0;
    }
  }


  digitalWrite(LED, (buttonState && ldrState) ? HIGH : LOW);
}