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

void get(int *get_value)// c = &a; ff -> a
{
  // *get_value = (int*)malloc(sizeof(int)); -> 0xbb
  Serial.printf("%d %d\n", get_value, &get_value);
}

void setup()
{
   int a[3] = {0,1,2};
  Serial.begin(115200);
  io_init();
  Serial.println("press btn to begin\n");
  Serial.println("PWM-керування мотором");
  Serial.println("ведіть швидкість команда (off / low / mid / high)");
  // int a = 0;
  // int *b = &a;
  Serial.printf("%d %d\n", a[1], *(a+1));
  // get(b); //== &a
  Serial.printf("%d %d\n", a , &a[0]);
}

// комірка памяті &a = 0хFF -> 0  &b = 0xAA *b = *a // &a = 5  b == 0xff

void loop()
{ // нескінченний цикл — серце програми
  // знімаємо подію кнопки один раз за прохід (атомарно для нашого ядра)
}