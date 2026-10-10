#include <Arduino.h>

#define LED 17 

#define LDR 2

#define N 32

const u16_t LDR_THRESHOLD = 2000;

u16_t readings[N];
u8_t idx = 0;
u32_t sum = 0;

// u8_t lastButtonState_B = 0;
// u8_t ledState_B = 0;

u32_t adcPeriod = 10;
u32_t lastTiimeChange = 0;

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

void setup() {
    pinMode(LED, OUTPUT); // налаштовуємо пін на ВИХІД (керування)
    pinMode(LDR, INPUT); // налаштовуємо пін на ВХІД (керування)
    Serial.begin(115200);
    Serial.println("setuped\n");
    filterInit(analogRead(LDR));
}

void loop() { // нескінченний цикл — серце програми
  if (millis() - lastTiimeChange > adcPeriod)
  {
    lastTiimeChange = millis();
    u16_t adc_value = analogRead(LDR);
    u16_t filtered_adc_value = movingAvg(adc_value);
    Serial.printf("adc raw value %d adc filtered  %d \n", adc_value, filtered_adc_value);

    digitalWrite(LED, filtered_adc_value <  LDR_THRESHOLD ? HIGH : LOW);
  }
}