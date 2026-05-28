#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "BicolorLed.h"
#include "ButtonInput.h"
#include "Config.h"
#include "DisplayUi.h"
#include "PhotoSensor.h"
#include "SerialInput.h"

// Оркестрация режимов приложения: главное меню, калибровка (R/B),
// прогрев перед замером, измерение задержки переключения R<->B,
// итоговый вывод (min/avg/max) и экран ошибки пригодности.
namespace app {

enum class Phase : uint8_t {
  MainMenu = 0,
  Calibration,       // Длинная явная калибровка (2 фазы x kCalibPhaseDurationMs)
  CalibrationDone,   // Экран итогов калибровки + флаг пригодности
  LatencyWarmup,     // Короткий авто-прогрев (2 фазы x kWarmupPhaseDurationMs)
  LatencyMeasure,    // Основной замер R<->B
  LatencyDone,       // Финальный экран (min/avg/max + по направлениям)
  LatencyAborted,    // Невозможно измерять — экран причины
};

// Какое направление пересечения порога ждём после очередного переключения LED.
enum class EdgeWait : uint8_t {
  None = 0,
  ToRed,    // только что включили Red — ждём, что АЦП перешёл «на сторону R»
  ToBlue,   // только что включили Blue — ждём, что АЦП перешёл «на сторону B»
};

// Причина отказа от измерения (рассчитывается после сканера 2 фаз).
enum class AbortReason : uint8_t {
  None = 0,
  SpreadLow,     // |cR - cB| мал — цвета не различаются на датчике/в кадре
  Clipped,       // АЦП упирается в 0 или 1023 (подкрутить подтяжку/яркость)
  NoData,        // сканер не успел набрать выборки (не должно случаться)
};

// Данные калибровки. Живут только в RAM, обнуляются при перезапуске.
// Заполняются как явной калибровкой, так и авто-прогревом перед замером.
struct CalibData {
  uint16_t cR{0};             // Среднее АЦП при Red
  uint16_t cB{0};             // Среднее АЦП при Blue

  uint16_t spreadR{0};        // p90 - p10 за фазу (робастный шум, после settle)
  uint16_t spreadB{0};

  uint16_t mid{512};          // (cR + cB) / 2
  uint16_t hysteresisAdc{8};  // адаптивный H: tLow/tHigh = mid ∓ H
  uint16_t tLow{500};         // mid - гистерезис, ограниченное [0, 1023]
  uint16_t tHigh{524};        // mid + гистерезис, ограниченное [0, 1023]
  int8_t   dirSign{1};        // sign(cB - cR): +1, если уровень Blue выше Red, иначе -1

  bool        valid{false};   // Прошла ли проверка пригодности
  AbortReason failReason{AbortReason::None};
};

class Application {
public:
  void begin();
  void loop();

private:
  Phase phase_{Phase::MainMenu};

  hw::BicolorLed led_{};
  hw::PhotoSensor photo_{};
  input::ButtonInput button_{};
  input::SerialInput serialIn_{};
  ui::DisplayUi ui_{};

  // --- Калибровка / прогрев (общий движок 2 фаз: Red, Blue) ---
  CalibData calib_{};
  uint8_t       scanPhaseIdx_{0};
  unsigned long scanPhaseStartMs_{0};
  unsigned long scanLastSampleMs_{0};
  unsigned long scanLastUiMs_{0};
  unsigned long scanSamplePeriodMs_{cfg::kCalibSamplePeriodMs};
  hw::AdcHist   scanHist_{};
  bool          scanIsLongCalibration_{false};

  unsigned long scanPhaseSettleMs_() const;

  // --- Замер задержки ---
  unsigned long latSessionStartMs_{0};
  unsigned long latLastToggleMs_{0};
  unsigned long latLastUiMs_{0};
  unsigned long latLastSerialMs_{0};
  hw::LedColor  latCurrentColor_{hw::LedColor::Off};
  EdgeWait      latWait_{EdgeWait::None};
  unsigned long latEdgeT0Us_{0};

  // Раздельная статистика по направлениям + счётчик потерь (таймауты).
  uint16_t latCountRB_{0};
  uint32_t latSumUsRB_{0};
  uint32_t latMinUsRB_{0xFFFFFFFFUL};
  uint32_t latMaxUsRB_{0};

  uint16_t latCountBR_{0};
  uint32_t latSumUsBR_{0};
  uint32_t latMinUsBR_{0xFFFFFFFFUL};
  uint32_t latMaxUsBR_{0};

  uint16_t latLost_{0};

  // --- Переходы режимов и обработка ввода ---
  void enterMainMenu();
  void handleGesture(input::ButtonGesture gesture);

  // --- Сканер 2 фаз ---
  void startCalibration();          // явная калибровка (kCalibPhaseDurationMs/фаза)
  void startLatencyWarmup();        // короткий прогрев перед замером
  void tickScan(unsigned long phaseDurMs);
  void scanApplyLedForPhase(uint8_t phaseIndex);
  void scanCommitPhase(uint8_t phaseIndex);
  void scanFinalize();              // считает mid/thr/dirSign/feasibility
  void scanNextPhaseOrFinish(unsigned long phaseDurMs);

  // --- Измерение задержки ---
  void startLatencyMeasure();
  void tickLatency();
  void latencyPollEdge();
  void latencyRefreshLiveUi(unsigned long nowMs);
  void latencyArmAfterToggle();
  void latencyPollSensor();
  void recordLatencySample(bool wasRedToBlue, uint32_t deltaUs);
  void finishLatencySuccess();
  void abortLatency(AbortReason reason);

  struct LatencyStats {
    uint16_t totalCount{0};
    float minMs{0.0F};
    float avgMs{0.0F};
    float maxMs{0.0F};
    float avgRBms{0.0F};
    float avgBRms{0.0F};
  };
  LatencyStats computeLatencyStats_() const;

  // --- Утилиты ---
  static const __FlashStringHelper* phaseLabel(uint8_t phaseIndex);
  static const __FlashStringHelper* abortReasonLabel(AbortReason r);
};

}  // namespace app
