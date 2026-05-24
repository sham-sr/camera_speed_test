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

// Гистограмма кодов АЦП за фазу (после settle) — перцентили без хранения всех выборок.
struct AdcHist {
  uint16_t counts[cfg::kAdcHistBins]{};
};

struct RobustStats {
  uint16_t level{};   // медиана (p50)
  uint16_t pLow{};    // p10
  uint16_t pHigh{};   // p90
  uint16_t spread{};  // p90 - p10
  uint32_t sampleCount{};
};

class PhotoSensor {
public:
  void begin();
  uint16_t readCode() const;
  float codeToVolts(uint16_t code) const;
  static void resetStats(AdcStats *s);
  static void accumulate(AdcStats *s, uint16_t code);

  static void resetHist(AdcHist *h);
  static void accumulateHist(AdcHist *h, uint16_t code);
  static RobustStats computeRobust(const AdcHist &h);

  // Завершить фазу: рассчитать среднее по накопленным выборкам.
  static float averageVolts(const AdcStats &s);
};

}  // namespace hw
