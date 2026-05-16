#include "BicolorLed.h"

namespace hw {

void BicolorLed::begin() {
  pinMode(cfg::kPinLedA, OUTPUT);
  pinMode(cfg::kPinLedB, OUTPUT);
  set(LedColor::Off);
}

void BicolorLed::set(LedColor c) {
  // По таблице из pins.md: противоположные уровни создают ток в одном или другом направлении.
  switch (c) {
    case LedColor::Red:
      drivePins(true, false);
      break;
    case LedColor::Blue:
      drivePins(false, true);
      break;
    case LedColor::Off:
    default:
      drivePins(false, false);
      break;
  }
}

void BicolorLed::drivePins(bool aHigh, bool bHigh) {
  digitalWrite(cfg::kPinLedA, aHigh ? HIGH : LOW);
  digitalWrite(cfg::kPinLedB, bHigh ? HIGH : LOW);
}

}  // namespace hw
