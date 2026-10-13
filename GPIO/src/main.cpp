#include <Arduino.h>
#include "config.h"
#include "Button.h"
#include "Ldr.h"

bool ldrState = false;

u16_t movingAvg(u16_t rawData)
{

}

Button button;
Ldr ldr;
void setup()
{
  button.init();
  ldr.init();
  pinMode(LED, OUTPUT); // налаштовуємо пін на ВИХІД (led)
  Serial.begin(115200);
  Serial.println("setuped\n");
}

void loop()
{
  ldr.poll();
  button.poll();

  digitalWrite(LED, (button.get_state() && ldr.get_state() ) ? HIGH : LOW);
}