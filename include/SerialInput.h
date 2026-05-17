#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "ButtonInput.h"

// Декодер команд из Serial: выдаёт те же ButtonGesture, что и физическая кнопка.
// Позволяет полностью управлять прибором даже без OLED и без кнопки.
//
// Соглашение по командам (нечувствительно к регистру, по одному символу):
//   'm' — Single       (выход / возврат в главное меню)
//   'c' — DoubleShort  (явная калибровка R/B)
//   'r' — LongHold     (авто-прогрев + замер 15 с)
//   'h' / '?'          — печать помощи (не жест)
// Все остальные символы (включая \r, \n, пробел, табуляцию) игнорируются.
namespace input {

class SerialInput {
public:
  void begin();
  ButtonGesture tick();

  static void printHelp();
};

}  // namespace input
