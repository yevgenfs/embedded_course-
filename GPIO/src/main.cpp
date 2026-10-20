#include <Arduino.h>
#include <stdint.h>

#define LED_PIN 18
#define BTN_PIN 17

// --- constexpr-конфіг: усе відоме наперед лягає у Flash ---
constexpr u32_t LED_MASK = 1u << LED_PIN; // маска для MMIO-регістра
constexpr u32_t WAIT_MIN = 1500;          // мін. пауза перед сигналом, мс
constexpr u32_t WAIT_MAX = 4000;          // макс. пауза, мс
constexpr u32_t DEBOUNCE_MS = 50;

volatile bool BtnEvent = false;
volatile u32_t BtnTime = 0;
volatile u32_t LastPress = 0;

u32_t waitTime = 0;
u32_t signalTime = 0;
u32_t result = 0;
u32_t delayTime = 0;

enum State
{
  IDLE,
  WAITING,
  SIGNAL,
  RESULT
};

State state = IDLE;

void ledOn()
{
  GPIO.out_w1ts = LED_MASK; // керуєи ледом за допомогою регістрів out_w1ts для такого щоб виставити прапор
}

void ledOff()
{
  GPIO.out_w1tc = LED_MASK; // керуєи ледом за допомогою регістрів out_w1tc для такого щоб очистити прапор
}

void IRAM_ATTR onButton()
{
  u32_t now = millis();
  if (now - LastPress > DEBOUNCE_MS)
  {
    LastPress = now;
    BtnTime = now;
    BtnEvent = true;
  }
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
  // знімаємо подію кнопки один раз за прохід (атомарно для нашого ядра)
  bool pressed = BtnEvent;

  if (pressed)
  {
    BtnEvent = false;
  }

  switch (state)
  {
  case IDLE:
    ledOff();
    if (pressed)
    {
      waitTime = millis();
      delayTime = WAIT_MIN + random(WAIT_MAX - WAIT_MIN); // випадкова пауза
      state = WAITING;
      BtnTime = 0;
      Serial.println("stayready for signal");
    }
    break;

  case WAITING:
    if (pressed)
    {
      state = IDLE;
      Serial.println("false start press again to start");
    }
    else if (millis() - waitTime > delayTime)
    {
      ledOn();
      signalTime = millis();
      state = SIGNAL;
      Serial.println("start\n");
    }
    break;

  case SIGNAL:
    if (pressed)
    {
      ledOff();
      result = BtnTime - signalTime;
      state = RESULT;
    }
    break;

  case RESULT:
    Serial.printf("result %d ms \n", result);
    state = IDLE;
    Serial.println("press Start");
    break;

  default:
    break;
  }
}