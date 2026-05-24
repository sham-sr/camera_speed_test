#include "Application.h"

namespace app {

// ---------------------------------------------------------------------------
// Жизненный цикл.
// ---------------------------------------------------------------------------

void Application::begin() {
  // Serial поднимаем первым, чтобы Serial-консоль работала даже если OLED не отвечает.
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("=== KRAN glass2glass: boot ==="));

  led_.begin();
  photo_.begin();
  button_.begin();
  serialIn_.begin();
  ui_.begin();
  enterMainMenu();
}

void Application::loop() {
  // Жесты могут прийти и с кнопки, и из Serial — берём первый ненулевой.
  const input::ButtonGesture gButton = button_.tick();
  const input::ButtonGesture gSerial = serialIn_.tick();
  const input::ButtonGesture gesture =
      (gButton != input::ButtonGesture::None) ? gButton : gSerial;
  handleGesture(gesture);

  switch (phase_) {
    case Phase::Calibration:
      tickScan(cfg::kCalibPhaseDurationMs);
      break;
    case Phase::LatencyWarmup:
      tickScan(cfg::kWarmupPhaseDurationMs);
      break;
    case Phase::LatencyMeasure:
      tickLatency();
      break;
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Главное меню и обработка ввода.
// ---------------------------------------------------------------------------

void Application::enterMainMenu() {
  led_.set(hw::LedColor::Off);
  phase_ = Phase::MainMenu;
  latWait_ = EdgeWait::None;
  ui_.showMainMenu();
}

void Application::handleGesture(const input::ButtonGesture gesture) {
  if (gesture == input::ButtonGesture::None) {
    return;
  }

  // Короткое одиночное — всегда выход в главное меню (прерывает идущий режим).
  if (gesture == input::ButtonGesture::Single) {
    enterMainMenu();
    return;
  }

  // Калибровка / замер — из меню и с экранов итога; повторный жест перезапускает режим.
  if (gesture == input::ButtonGesture::DoubleShort) {
    startCalibration();
    return;
  }
  if (gesture == input::ButtonGesture::LongHold) {
    startLatencyWarmup();
  }
}

// ---------------------------------------------------------------------------
// Сканер 2 фаз: Red, Blue (без Off).
// Используется и для длинной калибровки, и для авто-прогрева перед замером.
// ---------------------------------------------------------------------------

unsigned long Application::scanPhaseSettleMs_() const {
  return scanIsLongCalibration_ ? cfg::kCalibPhaseSettleMs : cfg::kWarmupPhaseSettleMs;
}

void Application::startCalibration() {
  phase_ = Phase::Calibration;
  scanIsLongCalibration_ = true;
  scanSamplePeriodMs_ = cfg::kCalibSamplePeriodMs;
  scanPhaseIdx_ = 0;
  scanApplyLedForPhase(0);
  hw::PhotoSensor::resetHist(&scanHist_);
  const unsigned long now = millis();
  scanPhaseStartMs_ = now;
  scanLastSampleMs_ = now;
  scanLastUiMs_ = now;
  ui_.showCalibrationScan(scanPhaseIdx_, cfg::kScanPhaseCount,
                          phaseLabel(scanPhaseIdx_),
                          /*adcMin*/ 0, /*adcMax*/ 0, /*adcAvg*/ 0, /*samples*/ 0);
}

void Application::startLatencyWarmup() {
  phase_ = Phase::LatencyWarmup;
  scanIsLongCalibration_ = false;
  scanSamplePeriodMs_ = cfg::kWarmupSamplePeriodMs;
  scanPhaseIdx_ = 0;
  scanApplyLedForPhase(0);
  hw::PhotoSensor::resetHist(&scanHist_);
  const unsigned long now = millis();
  scanPhaseStartMs_ = now;
  scanLastSampleMs_ = now;
  scanLastUiMs_ = now;
  ui_.showLatencyWarmup(scanPhaseIdx_, cfg::kScanPhaseCount,
                        phaseLabel(scanPhaseIdx_));
}

void Application::scanApplyLedForPhase(const uint8_t phaseIndex) {
  led_.set((phaseIndex == 0) ? hw::LedColor::Red : hw::LedColor::Blue);
}

void Application::tickScan(const unsigned long phaseDurMs) {
  const unsigned long nowMs = millis();

  const unsigned long phaseElapsedMs = nowMs - scanPhaseStartMs_;
  const bool pastSettle = phaseElapsedMs >= scanPhaseSettleMs_();

  // Опрос АЦП с заданным периодом (после settle — в гистограмму для робастных порогов).
  if (nowMs - scanLastSampleMs_ >= scanSamplePeriodMs_) {
    scanLastSampleMs_ = nowMs;
    const uint16_t code = photo_.readCode();
    if (pastSettle) {
      hw::PhotoSensor::accumulateHist(&scanHist_, code);
    }
  }

  // Периодическое обновление UI (только для длинной калибровки — там есть данные).
  if (scanIsLongCalibration_ && (nowMs - scanLastUiMs_ >= cfg::kScanUiPeriodMs)) {
    scanLastUiMs_ = nowMs;
    const hw::RobustStats rs = hw::PhotoSensor::computeRobust(scanHist_);
    ui_.showCalibrationScan(scanPhaseIdx_, cfg::kScanPhaseCount,
                            phaseLabel(scanPhaseIdx_),
                            rs.pLow, rs.pHigh, rs.level,
                            static_cast<uint16_t>(rs.sampleCount));
  }

  if (nowMs - scanPhaseStartMs_ < phaseDurMs) {
    return;
  }

  // Фаза завершена — сохраняем и переходим дальше.
  scanCommitPhase(scanPhaseIdx_);
  scanNextPhaseOrFinish(phaseDurMs);
}

void Application::scanCommitPhase(const uint8_t phaseIndex) {
  const hw::RobustStats rs = hw::PhotoSensor::computeRobust(scanHist_);

  switch (phaseIndex) {
    case 0:
      calib_.cR = rs.level;
      calib_.spreadR = rs.spread;
      break;
    case 1:
      calib_.cB = rs.level;
      calib_.spreadB = rs.spread;
      break;
    default:
      break;
  }

  if (rs.sampleCount == 0U) {
    calib_.failReason = AbortReason::NoData;
  }
}

namespace {

// Гистерезис Шмитта от разницы уровней R/B: при малом dRB порог ближе к mid.
int16_t adaptiveHysteresisAdc(const int32_t spreadRB) {
  if (spreadRB <= 0) {
    return cfg::kAdaptiveHysteresisMin;
  }
  int32_t h = spreadRB / cfg::kAdaptiveHysteresisSpreadDiv;
  if (h < cfg::kAdaptiveHysteresisMin) {
    h = cfg::kAdaptiveHysteresisMin;
  }
  if (h > cfg::kAdaptiveHysteresisMax) {
    h = cfg::kAdaptiveHysteresisMax;
  }
  return static_cast<int16_t>(h);
}

}  // namespace

void Application::scanFinalize() {
  // Расчёт порогов Шмитта вокруг середины между уровнями R и B.
  const int32_t cR = calib_.cR;
  const int32_t cB = calib_.cB;
  const int32_t mid = (cR + cB) / 2;
  const int32_t spreadRB = (cB >= cR) ? (cB - cR) : (cR - cB);
  const int16_t hysteresis = adaptiveHysteresisAdc(spreadRB);

  int32_t tLow = mid - hysteresis;
  int32_t tHigh = mid + hysteresis;
  if (tLow >= tHigh) {
    // Защита от вырождения при очень малом спреде.
    if (tLow > 0) {
      tLow -= 1;
    } else {
      tHigh += 1;
    }
  }
  calib_.mid = static_cast<uint16_t>(constrain(mid, 0L, 1023L));
  calib_.hysteresisAdc = static_cast<uint16_t>(hysteresis);
  calib_.tLow = static_cast<uint16_t>(constrain(tLow, 0L, 1023L));
  calib_.tHigh = static_cast<uint16_t>(constrain(tHigh, 0L, 1023L));
  calib_.dirSign = (cB >= cR) ? static_cast<int8_t>(+1) : static_cast<int8_t>(-1);

  // Проверка пригодности.
  const int32_t phaseNoise =
      static_cast<int32_t>(max(calib_.spreadR, calib_.spreadB));

  if (calib_.failReason == AbortReason::NoData) {
    calib_.valid = false;
    Serial.print(F("[CAL_THRESH] mid="));
    Serial.print(calib_.mid);
    Serial.print(F(" H="));
    Serial.print(calib_.hysteresisAdc);
    Serial.print(F(" tLo="));
    Serial.print(calib_.tLow);
    Serial.print(F(" tHi="));
    Serial.println(calib_.tHigh);
    return;
  }

  calib_.failReason = AbortReason::None;
  if (calib_.cR <= cfg::kFeasibleClipLow || calib_.cR >= cfg::kFeasibleClipHigh ||
             calib_.cB <= cfg::kFeasibleClipLow || calib_.cB >= cfg::kFeasibleClipHigh) {
    calib_.failReason = AbortReason::Clipped;
  } else if (spreadRB < cfg::kFeasibleMinSpreadAdc) {
    calib_.failReason = AbortReason::SpreadLow;
  } else if (spreadRB < phaseNoise + cfg::kFeasibleSpreadAboveNoise) {
    // Разница цветов не больше шума фазы — уровни «плавают», фронт ненадёжен.
    calib_.failReason = AbortReason::SpreadLow;
  }
  calib_.valid = (calib_.failReason == AbortReason::None);

  Serial.print(F("[CAL_THRESH] mid="));
  Serial.print(calib_.mid);
  Serial.print(F(" H="));
  Serial.print(calib_.hysteresisAdc);
  Serial.print(F(" tLo="));
  Serial.print(calib_.tLow);
  Serial.print(F(" tHi="));
  Serial.println(calib_.tHigh);
}

void Application::scanNextPhaseOrFinish(const unsigned long phaseDurMs) {
  scanPhaseIdx_++;

  if (scanPhaseIdx_ >= cfg::kScanPhaseCount) {
    scanFinalize();
    led_.set(hw::LedColor::Off);

    if (scanIsLongCalibration_) {
      phase_ = Phase::CalibrationDone;
      ui_.showCalibrationSummary(calib_.cR, calib_.cB,
                                 calib_.spreadR, calib_.spreadB,
                                 static_cast<int16_t>(static_cast<int32_t>(calib_.cB) -
                                                      static_cast<int32_t>(calib_.cR)),
                                 calib_.valid,
                                 abortReasonLabel(calib_.failReason));
    } else {
      if (calib_.valid) {
        startLatencyMeasure();
      } else {
        abortLatency(calib_.failReason);
      }
    }
    return;
  }

  // Подготовка следующей фазы сканера.
  scanApplyLedForPhase(scanPhaseIdx_);
  hw::PhotoSensor::resetHist(&scanHist_);
  const unsigned long now = millis();
  scanPhaseStartMs_ = now;
  scanLastSampleMs_ = now;
  scanLastUiMs_ = now;
  (void)phaseDurMs;  // длительность определяется фазой в loop()

  if (scanIsLongCalibration_) {
    ui_.showCalibrationScan(scanPhaseIdx_, cfg::kScanPhaseCount,
                            phaseLabel(scanPhaseIdx_), 0U, 0U, 0U, 0U);
  } else {
    ui_.showLatencyWarmup(scanPhaseIdx_, cfg::kScanPhaseCount,
                          phaseLabel(scanPhaseIdx_));
  }
}

// ---------------------------------------------------------------------------
// Измерение задержки: только R<->B, двусторонний детектор кроссинга через mid.
// ---------------------------------------------------------------------------

void Application::startLatencyMeasure() {
  phase_ = Phase::LatencyMeasure;

  const unsigned long now = millis();
  latSessionStartMs_ = now;
  latLastToggleMs_ = now;
  latLastUiMs_ = now;

  latCountRB_ = 0;
  latSumUsRB_ = 0;
  latMinUsRB_ = 0xFFFFFFFFUL;
  latMaxUsRB_ = 0;

  latCountBR_ = 0;
  latSumUsBR_ = 0;
  latMinUsBR_ = 0xFFFFFFFFUL;
  latMaxUsBR_ = 0;

  latLost_ = 0;

  // Стартовый цвет — Red. Первое осмысленное переключение произойдёт через kLatencyBlinkHalfPeriodMs.
  latCurrentColor_ = hw::LedColor::Red;
  led_.set(latCurrentColor_);
  latWait_ = EdgeWait::None;

  ui_.showLatencyLive(/*elapsedMs*/ 0UL, cfg::kLatencySessionMs,
                      /*totalCount*/ 0U, /*lost*/ 0U,
                      /*minMs*/ 0.0F, /*avgMs*/ 0.0F, /*maxMs*/ 0.0F,
                      /*avgRBms*/ 0.0F, /*avgBRms*/ 0.0F);
}

void Application::latencyPollEdge() {
  for (uint8_t poll = 0; poll < cfg::kLatencyEdgePollBurst; ++poll) {
    const uint16_t adc = photo_.readCode();
    const unsigned long nowUs = micros();
    bool captured = false;
    bool wasRedToBlue = false;
    uint32_t deltaUs = 0;

    // dirSign > 0: Blue выше Red по АЦП.
    //   ToBlue → adc > tHigh; ToRed → adc < tLow. dirSign < 0 — наоборот.
    if (latWait_ == EdgeWait::ToBlue) {
      wasRedToBlue = true;
      const bool crossed =
          (calib_.dirSign > 0) ? (adc > calib_.tHigh) : (adc < calib_.tLow);
      if (crossed) {
        deltaUs = static_cast<uint32_t>(nowUs - latEdgeT0Us_);
        captured = true;
      }
    } else {  // EdgeWait::ToRed
      wasRedToBlue = false;
      const bool crossed =
          (calib_.dirSign > 0) ? (adc < calib_.tLow) : (adc > calib_.tHigh);
      if (crossed) {
        deltaUs = static_cast<uint32_t>(nowUs - latEdgeT0Us_);
        captured = true;
      }
    }

    if (captured) {
      recordLatencySample(wasRedToBlue, deltaUs);
      latWait_ = EdgeWait::None;
      return;
    }
    if (static_cast<uint32_t>(nowUs - latEdgeT0Us_) > cfg::kLatencySensorTimeoutUs) {
      latWait_ = EdgeWait::None;
      latLost_++;
      return;
    }
  }
}

Application::LatencyStats Application::computeLatencyStats_() const {
  LatencyStats s;
  s.totalCount = latCountRB_ + latCountBR_;
  if (s.totalCount == 0U) {
    return s;
  }

  const uint32_t totalSumUs = latSumUsRB_ + latSumUsBR_;
  s.avgMs = static_cast<float>(totalSumUs) / static_cast<float>(s.totalCount) / 1000.0F;

  uint32_t minUsAll = 0xFFFFFFFFUL;
  uint32_t maxUsAll = 0;
  if (latCountRB_ > 0U) {
    if (latMinUsRB_ < minUsAll) minUsAll = latMinUsRB_;
    if (latMaxUsRB_ > maxUsAll) maxUsAll = latMaxUsRB_;
    s.avgRBms = static_cast<float>(latSumUsRB_) / static_cast<float>(latCountRB_) / 1000.0F;
  }
  if (latCountBR_ > 0U) {
    if (latMinUsBR_ < minUsAll) minUsAll = latMinUsBR_;
    if (latMaxUsBR_ > maxUsAll) maxUsAll = latMaxUsBR_;
    s.avgBRms = static_cast<float>(latSumUsBR_) / static_cast<float>(latCountBR_) / 1000.0F;
  }
  s.minMs = static_cast<float>(minUsAll) / 1000.0F;
  s.maxMs = static_cast<float>(maxUsAll) / 1000.0F;
  return s;
}

void Application::latencyRefreshLiveUi(const unsigned long nowMs) {
  latLastUiMs_ = nowMs;
  const LatencyStats s = computeLatencyStats_();
  ui_.showLatencyLive(nowMs - latSessionStartMs_, cfg::kLatencySessionMs,
                      s.totalCount, latLost_, s.minMs, s.avgMs, s.maxMs, s.avgRBms, s.avgBRms);
}

void Application::tickLatency() {
  const unsigned long nowMs = millis();

  if (nowMs - latSessionStartMs_ >= cfg::kLatencySessionMs) {
    finishLatencySuccess();
    return;
  }

  // 1) Расписание LED — до опроса датчика, чтобы фаза не съезжала из-за UI.
  if (nowMs - latLastToggleMs_ >= cfg::kLatencyBlinkHalfPeriodMs) {
    latLastToggleMs_ = nowMs;
    latCurrentColor_ =
        (latCurrentColor_ == hw::LedColor::Red) ? hw::LedColor::Blue : hw::LedColor::Red;
    led_.set(latCurrentColor_);
    latencyArmAfterToggle();
  }

  // 2) Критический путь: только АЦП + micros(), без OLED и Serial.
  if (latWait_ != EdgeWait::None) {
    latencyPollEdge();
    return;
  }

  // 3) OLED между окнами ожидания фронта (как tickScan: сначала датчик, потом UI).
  if (nowMs - latLastUiMs_ >= cfg::kLatencyUiPeriodMs) {
    latencyRefreshLiveUi(nowMs);
  }
}

void Application::latencyArmAfterToggle() {
  latEdgeT0Us_ = micros();
  latWait_ = (latCurrentColor_ == hw::LedColor::Blue) ? EdgeWait::ToBlue : EdgeWait::ToRed;
}

void Application::recordLatencySample(const bool wasRedToBlue, const uint32_t deltaUs) {
  if (wasRedToBlue) {
    latCountRB_++;
    latSumUsRB_ += deltaUs;
    if (deltaUs < latMinUsRB_) latMinUsRB_ = deltaUs;
    if (deltaUs > latMaxUsRB_) latMaxUsRB_ = deltaUs;
  } else {
    latCountBR_++;
    latSumUsBR_ += deltaUs;
    if (deltaUs < latMinUsBR_) latMinUsBR_ = deltaUs;
    if (deltaUs > latMaxUsBR_) latMaxUsBR_ = deltaUs;
  }
}

void Application::finishLatencySuccess() {
  led_.set(hw::LedColor::Off);
  phase_ = Phase::LatencyDone;

  const LatencyStats s = computeLatencyStats_();
  ui_.showLatencyResult(s.minMs, s.avgMs, s.maxMs, s.avgRBms, s.avgBRms, s.totalCount, latLost_);
}

void Application::abortLatency(const AbortReason reason) {
  led_.set(hw::LedColor::Off);
  phase_ = Phase::LatencyAborted;
  ui_.showLatencyAborted(abortReasonLabel(reason),
                         calib_.cR, calib_.cB,
                         static_cast<int16_t>(static_cast<int32_t>(calib_.cB) -
                                              static_cast<int32_t>(calib_.cR)));
}

// ---------------------------------------------------------------------------
// Утилиты: подписи фаз и причин отказа (во flash, чтобы не есть RAM).
// ---------------------------------------------------------------------------

const __FlashStringHelper* Application::phaseLabel(const uint8_t phaseIndex) {
  switch (phaseIndex) {
    case 0:
      return F("RED");
    case 1:
      return F("BLUE");
    default:
      return F("?");
  }
}

const __FlashStringHelper* Application::abortReasonLabel(const AbortReason r) {
  switch (r) {
    case AbortReason::SpreadLow:
      return F("dRB low");
    case AbortReason::Clipped:
      return F("ADC clip");
    case AbortReason::NoData:
      return F("no data");
    case AbortReason::None:
    default:
      return F("ok");
  }
}

}  // namespace app
