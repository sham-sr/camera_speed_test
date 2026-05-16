#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "Config.h"

// Декодирование жестов одной кнопки: короткое одинарное, двойное короткое, длительное удержание.
namespace input {

enum class ButtonGesture : uint8_t {
  None = 0,
  Single,       // короткое одинарное: выход в главное меню / подтверждение «назад»
  DoubleShort,  // два коротких нажатия подряд: калибровка
  LongHold      // удержание ≥ порога: измерение задержки
};

class ButtonInput {
public:
  void begin();
  // Вызов из loop(); возвращает не более одного события за тик.
  ButtonGesture tick();

private:
  bool stablePressed_ = false;
  bool rawPrev_ = false;
  unsigned long debounceDeadlineMs_ = 0;

  bool downTracked_ = false;
  unsigned long downStartMs_ = 0;

  bool pendingSingle_ = false;
  unsigned long pendingSingleDeadlineMs_ = 0;

  ButtonGesture latchedGesture_ = ButtonGesture::None;

  void onStableTransition(bool nowPressed, unsigned long nowMs);
};

}  // namespace input
