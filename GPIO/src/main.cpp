#include <Arduino.h>
#include <stdint.h>

#define BASE_PIN 17
#define PWM_CH 0      // канал LEDC (0..15)
#define PWM_FREQ 5000 // 5 кГц
#define PWM_RES 8     // 8 біт → duty 0..255

enum Speed
{
  off = 0,
  low = 60,
  mid = 150,
  high = 255, // 100%
};

void io_init()
{                                       // Init()-патерн: явно й детерміновано
  ledcSetup(PWM_CH, PWM_FREQ, PWM_RES); // налаштувати канал
  ledcAttachPin(BASE_PIN, PWM_CH);      // прив'язати пін до каналу
}

void setup()
{
  Serial.begin(115200);
  io_init();
  Serial.println("press btn to begin\n");
  Serial.println("PWM-керування мотором");
  Serial.println("ведіть швидкість команда (off / low / mid / high)");
}

void loop()
{ // нескінченний цикл — серце програми
  // знімаємо подію кнопки один раз за прохід (атомарно для нашого ядра)
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "off")
    {
      ledcWrite(PWM_CH, off);
    }
    else if (cmd == "low")
    {
      ledcWrite(PWM_CH, low);
    }
    else if (cmd == "mid")
    {
      ledcWrite(PWM_CH, mid);
    }
    else if (cmd == "high")
    {
      ledcWrite(PWM_CH, high);
    }
    else 
    {
      Serial.println("не правильна команда (off / low / mid / high)");
    }
  }
}