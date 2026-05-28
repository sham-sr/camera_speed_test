#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "Config.h"

// Вывод на SSD1306 128×64 по I2C (Adafruit GFX).
// Шрифт по умолчанию 5x8 + 1 px пробел → 6x8 на знак, помещается 21 символ x 8 строк.
namespace ui {

class DisplayUi {
public:
  DisplayUi();
  void begin();

  // Главное меню.
  void showMainMenu();

  // Сканер (явная калибровка) — текущая фаза с накопленной статистикой.
  void showCalibrationScan(uint8_t phaseIndex, uint8_t phaseTotal,
                           const __FlashStringHelper* phaseLabel,
                           uint16_t adcMin, uint16_t adcMax, uint16_t adcAvg,
                           uint16_t samples);

  // Итог калибровки: уровни R/B, разница, флаг пригодности.
  void showCalibrationSummary(uint16_t cR, uint16_t cB,
                              uint16_t spreadR, uint16_t spreadB,
                              int16_t dRB, bool feasible,
                              const __FlashStringHelper* reasonLabel);

  // Короткий авто-прогрев перед замером: минимально информативно.
  void showLatencyWarmup(uint8_t phaseIndex, uint8_t phaseTotal,
                         const __FlashStringHelper* phaseLabel);

  // Невозможно измерять — экран причины с диагностикой уровней.
  void showLatencyAborted(const __FlashStringHelper* reasonLabel,
                          uint16_t cR, uint16_t cB, int16_t dRB);

  // Промежуточный экран измерения с агрегированными значениями.
  void showLatencyLive(unsigned long elapsedMs, unsigned long totalMs,
                       uint16_t totalCount, uint16_t lost,
                       float minMs, float avgMs, float maxMs,
                       float avgRBms, float avgBRms,
                       bool logToSerial = false);

  // Финальный экран: min / avg / max + по направлениям + n / lost.
  void showLatencyResult(float minMs, float avgMs, float maxMs,
                         float avgRBms, float avgBRms,
                         uint16_t totalCount, uint16_t lost);

private:
  Adafruit_SSD1306 disp_;
  bool oledOk_{false};

  void flush();
  void initOled();
  static void formatFloat(char* buf, size_t bufSize, float value, uint8_t width, uint8_t prec);
};

}  // namespace ui
