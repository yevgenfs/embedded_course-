#include <Arduino.h>

hw_timer_t *timer = nullptr;
volatile uint32_t overflows = 0;
volatile bool flag = false;

void IRAM_ATTR onOverflow() { // спрацьовує на кожному «65535 → 0»
  overflows++;
  flag = true;
}

void setup() {
  Serial.begin(115200);
  timer = timerBegin(0, 80, true); // дільник 80 → 1 МГц
  timerAttachInterrupt(timer, &onOverflow, true);
  timerAlarmWrite(timer, 65536, true); // «як 16-біт»: 0..65535 і знову 0
  timerAlarmEnable(timer);
  Serial.println("Емуляція 16-біт: рахуємо переповнення (кожні ~65.5 мс)");
}

void loop() {
  if (flag) {
    flag = false;
    Serial.printf("OVERFLOW #%lu (лічильник крутнувся через нуль)\n", overflows);
  }
}
