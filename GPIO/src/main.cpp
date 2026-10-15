#include <Arduino.h>
#include "config.h"
#include "Button.h"
#include "Ldr.h"
#include "Led.h"
#include <esp_task_wdt.h>

Button button;
Ldr ldr;
Led led;
// void task (task_handler{})
void setup()
{
  delay(2000);

  button.init();
  ldr.init();
  led.init();
  Serial.begin(115200);
  Serial.printf("wdg inited  %d \n", esp_task_wdt_init(5, true));
  Serial.printf("wdg inited  %d \n", esp_task_wdt_add(NULL));

  Serial.printf("setuped  %d \n", esp_reset_reason());
}

void loop() // while true
{
  esp_task_wdt_reset();
  // Serial.println("WDT reseted \n");
  ldr.poll();
  button.poll();
  led.handler(button.get_state() && ldr.get_state());
}
// }