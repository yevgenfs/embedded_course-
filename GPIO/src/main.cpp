#include <Arduino.h>
#include <stdint.h>

#define LED_PIN 18
constexpr uint32_t LED_MASK = 1u << LED_PIN;

// друк 8 молодших бітів зліва направо (від старшого до молодшого):
void printBits(uint8_t v) {
  // i = 7..0 — номер біта; (v >> i) підсуває потрібний біт на позицію 0,
  // а & 1 «зрізає» все інше, лишаючи сам біт (0 або 1)
  for (int i = 7; i >= 0; i--) Serial.print((v >> i) & 1);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  uint8_t flags = 0b00000000;  // «регістр» з 8 прапорів, усі скинуті в 0
  printBits(flags);            // 00000000

  // SET (|=): АБО з 1 ставить біт у 1; АБО з 0 не чіпає інші біти
  flags |= (1u << 2);  // ставимо біт 2 -> 00000100
  printBits(flags);
  flags |= (1u << 5);  // ставимо біт 5 -> 00100100
  printBits(flags);

  // CLEAR (&= ~): ~маска = нулі лише в потрібних позиціях; І з 0 гасить,
  // І з 1 зберігає -> обнуляємо тільки біт 2, а біт 5 лишається!
  flags &= ~(1u << 2); // скидаємо біт 2 -> 00100000 (біт 5 цілий!)
  printBits(flags);

  // TOGGLE (^=): XOR з 1 перевертає біт, XOR з 0 зберігає решту
  flags ^= (1u << 5);  // перемикаємо біт 5-> 00000000
  printBits(flags);

  // READ (&): зсуваємо біт 5 у позицію 0 і & 1 -> дістаємо його значення
  bool bit5 = (flags >> 5) & 1; // READ біт 5 (0 або 1)
  Serial.print("bit5 = "); Serial.println(bit5);
}

void loop() {
  // той самий XOR-прийом, але вже на СПРАВЖНЬОМУ регістрі GPIO:
  // ^= перевертає лише біт нашого LED (бит 4), інші піни не чіпає
  GPIO.out ^= LED_MASK; // перемкнути LED однією побітовою дією
  delay(300);
}