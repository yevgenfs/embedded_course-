#include <Arduino.h>
#include "config.h"
#include "Button.h"
#include "Ldr.h"
#include "Led.h"

Button button;
Ldr ldr;
Led led;

void setup()
{
  button.init();
  ldr.init();
  led.init();
  Serial.begin(115200);
  Serial.println("setuped\n");
}

void loop()
{
  ldr.poll();
  button.poll();
  led.handler(button.get_state() && ldr.get_state());
}