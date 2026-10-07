#include <Arduino.h>


#define LED_B 16 // blue
#define LED_R 17 // red

#define BUTTON_B 20 // blue
#define BUTTON_R 21 // red

u8_t lastButtonState_B = 0;
u8_t lastButtonState_R = 0;

u8_t ledState_B = 0;
u8_t ledState_R = 0;

u32_t debounceDuration = 100;
u32_t lastTimeChange_B = 0;
u32_t lastTimeChange_R = 0;


void setup() {
    pinMode(LED_B, OUTPUT); // налаштовуємо пін на ВИХІД (керування)
    pinMode(LED_R,  OUTPUT  ); // налаштовуємо пін на ВИХІД (керування)
    pinMode(BUTTON_B, INPUT_PULLDOWN); // налаштовуємо пін на ВХІД (керування)
    pinMode(BUTTON_R, INPUT); // налаштовуємо пін на ВХІД (керування)
    Serial.begin(115200);
}

void loop() { // нескінченний цикл — серце програми
  if ((millis() - lastTimeChange_B) > debounceDuration)
  {
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
  }

  if ((millis() - lastTimeChange_R) > debounceDuration)
  {
    u8_t buttonState = digitalRead(BUTTON_R);
    if (buttonState != lastButtonState_R)
    {
      lastButtonState_R = buttonState;
      lastTimeChange_R = millis();
      if (buttonState == HIGH)
      {
        ledState_R = (ledState_R == HIGH) ? LOW : HIGH;
        digitalWrite(LED_R, ledState_R);
      }
    }
  }
}