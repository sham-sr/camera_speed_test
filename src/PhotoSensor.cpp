#include "PhotoSensor.h"

namespace hw {

void PhotoSensor::begin() {
  pinMode(cfg::kPinPhotoAdc, INPUT);
}

uint16_t PhotoSensor::readCode() const { return analogRead(cfg::kPinPhotoAdc); }

float PhotoSensor::codeToVolts(uint16_t code) const {
  return static_cast<float>(code) * (cfg::kAdcVrefVolts / cfg::kAdcMaxCode);
}

void PhotoSensor::resetStats(AdcStats *s) {
  if (s == nullptr) {
    return;
  }
  s->minCode = 1023;
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

void PhotoSensor::resetHist(AdcHist *h) {
  if (h == nullptr) {
    return;
  }
  for (uint8_t i = 0; i < cfg::kAdcHistBins; i++) {
    h->counts[i] = 0;
  }
}

void PhotoSensor::accumulateHist(AdcHist *h, uint16_t code) {
  if (h == nullptr) {
    return;
  }
  const uint8_t bin = static_cast<uint8_t>(
      (static_cast<uint32_t>(code) * cfg::kAdcHistBins) / 1024U);
  const uint8_t idx = (bin >= cfg::kAdcHistBins) ? static_cast<uint8_t>(cfg::kAdcHistBins - 1U)
                                                   : bin;
  h->counts[idx]++;
}

namespace {

uint16_t histPercentileCode(const hw::AdcHist &h, const uint32_t total,
                            const uint8_t percentile) {
  if (total == 0U) {
    return 0U;
  }
  uint32_t target = (total * static_cast<uint32_t>(percentile) + 50U) / 100U;
  if (target < 1U) {
    target = 1U;
  }
  uint32_t cum = 0U;
  for (uint8_t i = 0; i < cfg::kAdcHistBins; i++) {
    cum += h.counts[i];
    if (cum >= target) {
      const uint16_t binLow =
          static_cast<uint16_t>((static_cast<uint32_t>(i) * 1024U) / cfg::kAdcHistBins);
      const uint16_t binHigh =
          static_cast<uint16_t>((static_cast<uint32_t>(i + 1U) * 1024U) / cfg::kAdcHistBins);
      return static_cast<uint16_t>((binLow + binHigh) / 2U);
    }
  }
  return 1023U;
}

}  // namespace

RobustStats PhotoSensor::computeRobust(const AdcHist &h) {
  RobustStats out{};
  uint32_t total = 0U;
  for (uint8_t i = 0; i < cfg::kAdcHistBins; i++) {
    total += h.counts[i];
  }
  out.sampleCount = total;
  if (total == 0U) {
    return out;
  }
  out.pLow = histPercentileCode(h, total, cfg::kHistPercentileLow);
  out.pHigh = histPercentileCode(h, total, cfg::kHistPercentileHigh);
  out.level = histPercentileCode(h, total, cfg::kHistPercentileMid);
  out.spread = (out.pHigh >= out.pLow) ? static_cast<uint16_t>(out.pHigh - out.pLow) : 0U;
  return out;
}

}  // namespace hw
