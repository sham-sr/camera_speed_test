#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "Config.h"

// Опрос фотоприёмника на АЦП: перевод в вольты и накопление статистики за фазу измерения.
namespace hw {

struct AdcStats {
  uint16_t minCode{};
  uint16_t maxCode{};
  uint32_t sumCount{};
  uint32_t sumCodes{};
};

class PhotoSensor {
public:
  void begin();
  uint16_t readCode() const;
  float codeToVolts(uint16_t code) const;
  static void resetStats(AdcStats *s);
  static void accumulate(AdcStats *s, uint16_t code);

  // Завершить фазу: рассчитать среднее по накопленным выборкам.
  static float averageVolts(const AdcStats &s);
};

}  // namespace hw
