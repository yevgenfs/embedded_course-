#include <Arduino.h>
#include <stdint.h>

#define LED_PIN 18
#define BTN_PIN 17

volatile bool BtnEvent = false;
volatile u32_t BtnTime = 0;

u32_t waitTime = 0;
u32_t result = 0;
u32_t Delay = 0;

enum State
{
  IDLE,
  WAITING,
  SIGNAL,
  RESULT
};

State state = IDLE;

void IRAM_ATTR onButton()
{
  BtnEvent = true;
  BtnTime = millis();
  Serial.println("interrupt");
}

void io_init()
{ // Init()-патерн: явно й детерміновано
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButton, RISING);
}

void setup()
{
  Serial.begin(115200);
  io_init();
  randomSeed(micros());
  Serial.println("press btn to begin\n");
}

void loop()
{ // нескінченний цикл — серце програми

  bool pressed = BtnEvent;

  if (BtnEvent)
  {
    BtnEvent = false;
  }

  switch (state)
  {
  case IDLE:
    digitalWrite(LED_PIN, LOW);
    if (pressed)
    {
      waitTime = millis();
      Delay =  2000;
      state = WAITING;
      Serial.printf("press Start \n");
      BtnTime = 0;
      Serial.printf("waitTime %d  Delay %d\n", waitTime, Delay);
    }
    break;

  case WAITING:
    if (pressed)
    {
      waitTime = 0;
      Delay = 0;
      state = IDLE;
      Serial.println("false start\n");
      BtnTime = 0;
    }
    else if (millis() - waitTime > Delay )
    {
      state = SIGNAL;
      Serial.println("start\n");
    }
    break;
  case SIGNAL:
    digitalWrite(LED_PIN, HIGH);
    if (pressed)
    {
      result = BtnTime - Delay;
      state = RESULT;
      BtnTime = 0;
    }
    break;

  case RESULT:
    Serial.printf("result %d\n", result);
    state = IDLE;
    waitTime = 0;
    Delay = 0;
    Serial.printf("press Start \n");
    break;

  default:
    break;
  }
}