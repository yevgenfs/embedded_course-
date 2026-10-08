#include <Arduino.h>


#define LED_B 16 // blue

#define BUTTON_B 20 // blue

u8_t lastButtonState_B = 0;
u8_t ledState_B = 0;

u32_t debounceDuration = 100;
u32_t lastTimeChange_B = 0;


void setup() {
    pinMode(LED_B, OUTPUT); // налаштовуємо пін на ВИХІД (керування)
    pinMode(BUTTON_B, INPUT_PULLDOWN); // налаштовуємо пін на ВХІД (керування)
    Serial.begin(115200);
}

void loop() { // нескінченний цикл — серце програми
  // if ((millis() - lastTimeChange_B) > debounceDuration)
  // {
    u8_t buttonState = digitalRead(BUTTON_B);
    if (buttonState != lastButtonState_B)
    {
      lastButtonState_B = buttonState;
      lastTimeChange_B = millis();
      if (buttonState == HIGH)
      {
        ledState_B = (ledState_B == HIGH) ? LOW : HIGH;
        digitalWrite(LED_B, ledState_B);
      }
    }
  // }
}