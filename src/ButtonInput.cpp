#include "ButtonInput.h"

namespace input {

void ButtonInput::begin() {
  pinMode(cfg::kPinButton, INPUT_PULLUP);
  stablePressed_ = false;
  rawPrev_ = digitalRead(cfg::kPinButton) == LOW;
  debounceDeadlineMs_ = 0;
  downTracked_ = false;
  pendingSingle_ = false;
  pendingSingleDeadlineMs_ = 0;
  latchedGesture_ = ButtonGesture::None;
}

ButtonGesture ButtonInput::tick() {
  const unsigned long nowMs = millis();
  const bool rawPressed = digitalRead(cfg::kPinButton) == LOW;

  if (rawPressed != rawPrev_) {
    debounceDeadlineMs_ = nowMs + cfg::kButtonDebounceMs;
    rawPrev_ = rawPressed;
  }

  if (debounceDeadlineMs_ != 0UL && nowMs < debounceDeadlineMs_) {
    // Ждём окончания окна антидребезга.
  } else {
    debounceDeadlineMs_ = 0UL;
    if (rawPressed != stablePressed_) {
      stablePressed_ = rawPressed;
      onStableTransition(stablePressed_, nowMs);
    }
  }

  if (latchedGesture_ != ButtonGesture::None) {
    const ButtonGesture out = latchedGesture_;
    latchedGesture_ = ButtonGesture::None;
    return out;
  }

  if (pendingSingle_ && !stablePressed_ && nowMs >= pendingSingleDeadlineMs_) {
    pendingSingle_ = false;
    return ButtonGesture::Single;
  }

  return ButtonGesture::None;
}

void ButtonInput::onStableTransition(bool nowPressed, unsigned long nowMs) {
  if (nowPressed) {
    if (!downTracked_) {
      downTracked_ = true;
      downStartMs_ = nowMs;
    }
    return;
  }

  if (!downTracked_) {
    return;
  }
  downTracked_ = false;

  const unsigned long pressDurMs = nowMs - downStartMs_;

  if (pressDurMs >= cfg::kLongPressMs) {
    pendingSingle_ = false;
    latchedGesture_ = ButtonGesture::LongHold;
    return;
  }

  if (pressDurMs > cfg::kShortPressMaxMs) {
    // Неклассифицируемый интервал — вероятный дребезг или нестандартный жест.
    return;
  }

  if (pendingSingle_) {
    pendingSingle_ = false;
    latchedGesture_ = ButtonGesture::DoubleShort;
    return;
  }

  pendingSingle_ = true;
  pendingSingleDeadlineMs_ = nowMs + cfg::kDoubleClickMaxGapMs;
}

}  // namespace input
