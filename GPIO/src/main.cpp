#include <Arduino.h>

#define LED 17 

#define LDR 2

// u8_t lastButtonState_B = 0;
// u8_t ledState_B = 0;

// u32_t debounceDuration = 100;
// u32_t lastTimeChange_B = 0;


void setup() {
    pinMode(LED, OUTPUT); // налаштовуємо пін на ВИХІД (керування)
    pinMode(LDR, INPUT); // налаштовуємо пін на ВХІД (керування)
    Serial.begin(115200);
    Serial.println("setuped\n");
}

void loop() { // нескінченний цикл — серце програми

    u16_t adc_value = analogRead(LDR);

    Serial.printf("adc value %d \n", adc_value);

      // lastButtonState_B = buttonState;
      // lastTimeChange_B = millis();
      // if (buttonState == HIGH)
      // {
        
      //   ledState_B = (ledState_B == HIGH) ? LOW : HIGH;
      //   digitalWrite(LED_B, ledState_B);
      // }
}