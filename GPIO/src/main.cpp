#include <Arduino.h>
#include <stdint.h>

#define LED_PIN 18
#define BTN_PIN 17

u8_t lastButtonState_B = 0;
u8_t ledState_B = 0;

u32_t debounceDuration = 100;
u32_t lastTimeChange_B = 0;


void io_init() {                     // Init()-патерн: явно й детерміновано
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLDOWN);
}

void setup() {
    Serial.begin(115200);
    io_init();
    randomSeed(micros());
    Serial.println("setuped\n");
}

void loop() { // нескінченний цикл — серце програми
  
}