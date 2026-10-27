#include <Arduino.h>

#define FREACUENCY 1000

hw_timer_t *timer = nullptr;
volatile uint32_t count = 0;

void IRAM_ATTR onCTC()
{ // спрацьовує на кожному «65535 → 0»
  count++;
}

void setup()
{
  Serial.begin(115200);
  timer = timerBegin(0, 80, true); // дільник 80 → 1 МГц
  timerAttachInterrupt(timer, &onCTC, true);
  timerAlarmWrite(timer, 1000000 / FREACUENCY, true); // «як 16-біт»: 0..65535 і знову 0
  timerAlarmEnable(timer);
  Serial.println("Емуляція 16-біт: рахуємо переповнення (кожні ~65.5 мс)");
}

void loop()
{
  static u32_t last = 0;
  if (millis() - last >= FREACUENCY)
  {
    last = millis();
    Serial.printf("CTC #%lu ISR за секунду \n", count);
    count = 0;
  }
}
