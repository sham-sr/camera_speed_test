#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include "Config.h"

// Слой представления: ST7789 через Adafruit GFX (лёгкий стек для AVR + SPI).
namespace ui {

class DisplayUi {
public:
  DisplayUi();
  void begin();
  void showMainMenu();
  void showCalibrationScreen(uint8_t phaseIndex, uint8_t phaseTotal, float minV, float maxV, float avgV);
  void showCalibrationFinished();
  void showLatencyPreparing();
  void showLatencyLive(unsigned long elapsedMs, unsigned long totalMs, uint16_t samples);
  void showLatencyResult(float minMs, float maxMs, float avgMs, uint16_t samples);

private:
  Adafruit_ST7789 tft_;

  void backlightOn();
  void backlightSet(bool on);
  void probeBacklightPin();
  void initDisplayHardware();
  void runBootSelfTest();
  static void formatFloat(char *buf, size_t bufSize, float value, uint8_t width, uint8_t prec);
};

}  // namespace ui
