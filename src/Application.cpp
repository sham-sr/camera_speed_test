#include "Application.h"

namespace app {

void Application::begin() {
  led_.begin();
  photo_.begin();
  button_.begin();
  ui_.begin();
  enterMainMenu();
}

void Application::loop() {
  const input::ButtonGesture gesture = button_.tick();
  handleGesture(gesture);

  switch (phase_) {
    case Phase::Calibration:
      tickCalibration();
      break;
    case Phase::LatencyWarmDark:
    case Phase::LatencyWarmLit:
    case Phase::LatencyBlink:
      tickLatency();
      break;
    default:
      break;
  }
}

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

  if (gesture == input::ButtonGesture::Single) {
    // Одиночное короткое — всегда возврат в главное меню (или обновление, если уже там).
    enterMainMenu();
    return;
  }

  if (phase_ != Phase::MainMenu) {
    // Остальные жесты интерпретируем только на главном экране.
    return;
  }

  if (gesture == input::ButtonGesture::DoubleShort) {
    startCalibration();
    return;
  }
  if (gesture == input::ButtonGesture::LongHold) {
    startLatency();
  }
}

void Application::startCalibration() {
  phase_ = Phase::Calibration;
  calibPhaseIdx_ = 0;
  calibPhaseStartMs_ = millis();
  calibLastSampleMs_ = calibPhaseStartMs_;
  hw::PhotoSensor::resetStats(&calibAcc_);
  applyCalibrationLed(calibPhaseIdx_);
  ui_.showCalibrationScreen(0, cfg::kCalibPhaseCount, 0.0F, 0.0F, 0.0F);
}

void Application::applyCalibrationLed(const uint8_t phaseIndex) {
  const bool redPhase = (phaseIndex % 2U) == 0U;
  led_.set(redPhase ? hw::LedColor::Red : hw::LedColor::Blue);
}

void Application::tickCalibration() {
  const unsigned long nowMs = millis();

  if (nowMs - calibLastSampleMs_ >= cfg::kCalibSamplePeriodMs) {
    calibLastSampleMs_ = nowMs;
    const uint16_t code = photo_.readCode();
    hw::PhotoSensor::accumulate(&calibAcc_, code);
  }

  if (nowMs - calibPhaseStartMs_ < cfg::kCalibPhaseDurationMs) {
    return;
  }

  const float minV = photo_.codeToVolts(calibAcc_.minCode);
  const float maxV = photo_.codeToVolts(calibAcc_.maxCode);
  const float avgV = hw::PhotoSensor::averageVolts(calibAcc_);
  ui_.showCalibrationScreen(calibPhaseIdx_, cfg::kCalibPhaseCount, minV, maxV, avgV);

  calibPhaseIdx_++;
  if (calibPhaseIdx_ >= cfg::kCalibPhaseCount) {
    finishCalibration();
    return;
  }

  hw::PhotoSensor::resetStats(&calibAcc_);
  calibPhaseStartMs_ = nowMs;
  calibLastSampleMs_ = nowMs;
  applyCalibrationLed(calibPhaseIdx_);
}

void Application::finishCalibration() {
  led_.set(hw::LedColor::Off);
  phase_ = Phase::CalibrationDone;
  ui_.showCalibrationFinished();
}

void Application::startLatency() {
  phase_ = Phase::LatencyWarmDark;
  latWarmStartMs_ = millis();
  latWarmLastSampleMs_ = latWarmStartMs_;
  latWarmSum_ = 0;
  latWarmCnt_ = 0;
  led_.set(hw::LedColor::Off);

  latSampleCount_ = 0;
  latSumUs_ = 0;
  latMinUs_ = 0xFFFFFFFFUL;
  latMaxUs_ = 0;
  latWait_ = EdgeWait::None;
  latLit_ = false;

  ui_.showLatencyPreparing();
}

void Application::tickLatency() {
  const unsigned long nowMs = millis();

  if (phase_ == Phase::LatencyWarmDark) {
    if (nowMs - latWarmLastSampleMs_ >= 10UL) {
      latWarmLastSampleMs_ = nowMs;
      latWarmSum_ += photo_.readCode();
      latWarmCnt_++;
    }
    if (nowMs - latWarmStartMs_ < cfg::kLatencyWarmupDarkMs) {
      return;
    }
    if (latWarmCnt_ == 0U) {
      latDarkAvg_ = 0;
    } else {
      latDarkAvg_ = static_cast<uint16_t>(latWarmSum_ / latWarmCnt_);
    }
    latWarmSum_ = 0;
    latWarmCnt_ = 0;
    latWarmStartMs_ = nowMs;
    latWarmLastSampleMs_ = nowMs;
    led_.set(hw::LedColor::Red);
    phase_ = Phase::LatencyWarmLit;
    return;
  }

  if (phase_ == Phase::LatencyWarmLit) {
    if (nowMs - latWarmLastSampleMs_ >= 10UL) {
      latWarmLastSampleMs_ = nowMs;
      latWarmSum_ += photo_.readCode();
      latWarmCnt_++;
    }
    if (nowMs - latWarmStartMs_ < cfg::kLatencyWarmupLitMs) {
      return;
    }
    if (latWarmCnt_ == 0U) {
      latLitAvg_ = 0;
    } else {
      latLitAvg_ = static_cast<uint16_t>(latWarmSum_ / latWarmCnt_);
    }

    int32_t mid = (static_cast<int32_t>(latDarkAvg_) + static_cast<int32_t>(latLitAvg_)) / 2;
    const int32_t tLow =
        mid - static_cast<int32_t>(cfg::kLatencyThresholdHysteresisAdc);
    const int32_t tHigh =
        mid + static_cast<int32_t>(cfg::kLatencyThresholdHysteresisAdc);
    latThrLow_ = static_cast<uint16_t>(
        constrain(tLow, 0L, static_cast<int32_t>(cfg::kAdcMaxCodeU)));
    latThrHigh_ = static_cast<uint16_t>(
        constrain(tHigh, 0L, static_cast<int32_t>(cfg::kAdcMaxCodeU)));
    if (latThrLow_ >= latThrHigh_) {
      // Запасной путь при инверсии или шуме — минимальный интервал порогов.
      if (latThrLow_ > 0) {
        latThrLow_ = static_cast<uint16_t>(latThrLow_ - 1U);
      } else {
        latThrHigh_ = static_cast<uint16_t>(latThrHigh_ + 1U);
      }
    }

    phase_ = Phase::LatencyBlink;
    latSessionStartMs_ = nowMs;
    latLastToggleMs_ = nowMs;
    latLastUiMs_ = nowMs;

    // Старт с «яркой» фазы: красный включён; первый осмысленный замер — на выключении в темноту.
    latLit_ = true;
    led_.set(hw::LedColor::Red);
    latWait_ = EdgeWait::None;
    return;
  }

  if (phase_ == Phase::LatencyBlink) {
    if (nowMs - latSessionStartMs_ >= cfg::kLatencySessionMs) {
      finishLatencySuccess();
      return;
    }

    if (nowMs - latLastUiMs_ >= 250UL) {
      latLastUiMs_ = nowMs;
      ui_.showLatencyLive(nowMs - latSessionStartMs_, cfg::kLatencySessionMs, latSampleCount_);
    }

    // Ожидание фронта на фотоприёмнике после последней смены состояния светодиода.
    if (latWait_ != EdgeWait::None) {
      const uint16_t adc = photo_.readCode();
      const unsigned long nowUs = micros();
      bool captured = false;
      uint32_t deltaUs = 0;

      if (latWait_ == EdgeWait::Low && adc < latThrLow_) {
        deltaUs = nowUs - latEdgeT0Us_;
        captured = true;
      } else if (latWait_ == EdgeWait::High && adc > latThrHigh_) {
        deltaUs = nowUs - latEdgeT0Us_;
        captured = true;
      } else if ((nowUs - latEdgeT0Us_) > cfg::kLatencySensorTimeoutUs) {
        latWait_ = EdgeWait::None;
      }

      if (captured) {
        recordLatencySample(deltaUs);
        latWait_ = EdgeWait::None;
      }
    }

    if (nowMs - latLastToggleMs_ >= cfg::kLatencyBlinkHalfPeriodMs) {
      latLastToggleMs_ = nowMs;
      latLit_ = !latLit_;
      if (latLit_) {
        led_.set(hw::LedColor::Red);
      } else {
        led_.set(hw::LedColor::Off);
      }
      latencyArmAfterToggle();
    }
  }
}

void Application::latencyArmAfterToggle() {
  latEdgeT0Us_ = micros();
  if (latLit_) {
    // Переход к освещению — ожидаем падение уровня АЦП (свет ярче на датчике, ниже код при типовой схеме).
    latWait_ = EdgeWait::Low;
  } else {
    latWait_ = EdgeWait::High;
  }
}

void Application::recordLatencySample(const uint32_t deltaUs) {
  latSampleCount_++;
  latSumUs_ += deltaUs;
  if (deltaUs < latMinUs_) {
    latMinUs_ = deltaUs;
  }
  if (deltaUs > latMaxUs_) {
    latMaxUs_ = deltaUs;
  }
}

void Application::finishLatencySuccess() {
  led_.set(hw::LedColor::Off);
  phase_ = Phase::LatencyDone;

  float minMs = 0.0F;
  float maxMs = 0.0F;
  float avgMs = 0.0F;

  if (latSampleCount_ > 0U) {
    minMs = static_cast<float>(latMinUs_) / 1000.0F;
    maxMs = static_cast<float>(latMaxUs_) / 1000.0F;
    avgMs = static_cast<float>(latSumUs_) / static_cast<float>(latSampleCount_) / 1000.0F;
  }

  ui_.showLatencyResult(minMs, maxMs, avgMs, latSampleCount_);
}

}  // namespace app
