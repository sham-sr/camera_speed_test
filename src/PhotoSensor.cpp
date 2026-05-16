#include "PhotoSensor.h"

namespace hw {

void PhotoSensor::begin() {
#if defined(INPUT_ANALOG)
  pinMode(cfg::kPinPhotoAdc, INPUT_ANALOG);
#else
  pinMode(cfg::kPinPhotoAdc, INPUT);
#endif
  analogReadResolution(12);
}

uint16_t PhotoSensor::readCode() const { return analogRead(cfg::kPinPhotoAdc); }

float PhotoSensor::codeToVolts(uint16_t code) const {
  return static_cast<float>(code) * (cfg::kAdcVrefVolts / cfg::kAdcMaxCode);
}

void PhotoSensor::resetStats(AdcStats *s) {
  if (s == nullptr) {
    return;
  }
  s->minCode = cfg::kAdcMaxCodeU;
  s->maxCode = 0;
  s->sumCount = 0;
  s->sumCodes = 0;
}

void PhotoSensor::accumulate(AdcStats *s, uint16_t code) {
  if (s == nullptr) {
    return;
  }
  if (code < s->minCode) {
    s->minCode = code;
  }
  if (code > s->maxCode) {
    s->maxCode = code;
  }
  s->sumCodes += code;
  s->sumCount += 1;
}

float PhotoSensor::averageVolts(const AdcStats &s) {
  if (s.sumCount == 0) {
    return 0.0F;
  }
  const float avgCode = static_cast<float>(s.sumCodes) / static_cast<float>(s.sumCount);
  return avgCode * (cfg::kAdcVrefVolts / cfg::kAdcMaxCode);
}

}  // namespace hw
