#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "BicolorLed.h"
#include "ButtonInput.h"
#include "Config.h"
#include "DisplayUi.h"
#include "PhotoSensor.h"

// Оркестрация режимов приложения: главное меню, калибровка, измерение задержки.
namespace app {

enum class Phase : uint8_t {
  MainMenu = 0,
  Calibration,
  CalibrationDone,
  LatencyWarmDark,
  LatencyWarmLit,
  LatencyBlink,
  LatencyDone,
};

enum class EdgeWait : uint8_t { None = 0, Low, High };

class Application {
public:
  void begin();
  void loop();

private:
  Phase phase_{Phase::MainMenu};

  hw::BicolorLed led_{};
  hw::PhotoSensor photo_{};
  input::ButtonInput button_{};
  ui::DisplayUi ui_{};

  // --- Калибровка ---
  uint8_t calibPhaseIdx_{0};
  unsigned long calibPhaseStartMs_{0};
  unsigned long calibLastSampleMs_{0};
  hw::AdcStats calibAcc_{};

  // --- Задержка ---
  unsigned long latWarmStartMs_{0};
  unsigned long latWarmLastSampleMs_{0};
  uint32_t latWarmSum_{0};
  uint32_t latWarmCnt_{0};
  uint16_t latDarkAvg_{0};
  uint16_t latLitAvg_{0};

  unsigned long latSessionStartMs_{0};
  unsigned long latLastToggleMs_{0};
  unsigned long latLastUiMs_{0};
  bool latLit_{false};

  EdgeWait latWait_{EdgeWait::None};
  unsigned long latEdgeT0Us_{0};

  uint16_t latThrLow_{0};
  uint16_t latThrHigh_{cfg::kAdcMaxCodeU};

  uint16_t latSampleCount_{0};
  uint32_t latSumUs_{0};
  uint32_t latMinUs_{0};
  uint32_t latMaxUs_{0};

  void enterMainMenu();
  void startCalibration();
  void tickCalibration();
  void applyCalibrationLed(uint8_t phaseIndex);
  void finishCalibration();

  void startLatency();
  void tickLatency();
  void finishLatencySuccess();
  void latencyArmAfterToggle();

  void handleGesture(input::ButtonGesture gesture);
  void recordLatencySample(uint32_t deltaUs);
};

}  // namespace app
