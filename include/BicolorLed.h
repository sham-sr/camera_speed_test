#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "Config.h"

// Управление двухвыводным биколорным светодиодом (два тока по встречной схеме в корпусе).
namespace hw {

enum class LedColor : uint8_t { Off, Red, Blue };

class BicolorLed {
public:
  void begin();
  void set(LedColor c);

private:
  static void drivePins(bool aHigh, bool bHigh);
};

}  // namespace hw
