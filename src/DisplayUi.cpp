#include "DisplayUi.h"

#include <string.h>

namespace ui {

namespace {
enum LineLayout : int16_t { kMarginX = 2, kLineStep = 9 };
}

DisplayUi::DisplayUi() : tft_(cfg::kPinTftCs, cfg::kPinTftDc, cfg::kPinTftRst) {}

void DisplayUi::backlightOn() {
  pinMode(cfg::kPinTftBl, OUTPUT);
  analogWrite(cfg::kPinTftBl, cfg::kBacklightPwm);
}

void DisplayUi::formatFloat(char *buf, const size_t bufSize, const float value, const uint8_t width,
                            const uint8_t prec) {
  if (buf == nullptr || bufSize == 0U) {
    return;
  }
  memset(buf, 0, bufSize);
  dtostrf(static_cast<double>(value), static_cast<int>(width), static_cast<int>(prec), buf);
  buf[bufSize - 1U] = '\0';
}

void DisplayUi::begin() {
  backlightOn();
  tft_.init(cfg::kDisplayWidth, cfg::kDisplayHeight);
  tft_.setRotation(0);
  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.setTextSize(1);
  tft_.setTextWrap(true);
}

void DisplayUi::showMainMenu() {
  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.setTextSize(1);
  int16_t y = 4;
  tft_.setCursor(kMarginX, y);
  tft_.println("Latency");
  y += kLineStep;
  tft_.drawFastHLine(0, y - 2, cfg::kDisplayWidth, ST77XX_WHITE);
  tft_.setCursor(kMarginX, y);
  tft_.println("2glass");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("tap=menu");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("2tap=cal");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("hold3s=T");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("cal RB 5s");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("x4 Vstat");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("lat 15s");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("LED ms");
}

void DisplayUi::showCalibrationScreen(const uint8_t phaseIndex, const uint8_t phaseTotal, const float minV,
                                      const float maxV, const float avgV) {
  char bufMin[12];
  char bufMax[12];
  char bufAvg[12];
  formatFloat(bufMin, sizeof(bufMin), minV, 4, 2);
  formatFloat(bufMax, sizeof(bufMax), maxV, 4, 2);
  formatFloat(bufAvg, sizeof(bufAvg), avgV, 4, 2);

  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.setTextSize(1);
  int16_t y = 4;
  tft_.setCursor(kMarginX, y);
  tft_.println("CAL");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print(phaseIndex + 1U);
  tft_.print("/");
  tft_.println(phaseTotal);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("Mn");
  tft_.println(bufMin);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("Mx");
  tft_.println(bufMax);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("Av");
  tft_.println(bufAvg);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("tap=esc");
}

void DisplayUi::showCalibrationFinished() {
  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.setTextSize(1);
  int16_t y = 4;
  tft_.setCursor(kMarginX, y);
  tft_.println("CAL OK");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("LED off");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("tap=menu");
}

void DisplayUi::showLatencyPreparing() {
  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.setTextSize(1);
  int16_t y = 4;
  tft_.setCursor(kMarginX, y);
  tft_.println("Wait");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("ADC ref");
}

void DisplayUi::showLatencyLive(const unsigned long elapsedMs, const unsigned long totalMs,
                                const uint16_t samples) {
  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.setTextSize(1);
  int16_t y = 4;
  tft_.setCursor(kMarginX, y);
  tft_.println("LAT run");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("t ");
  tft_.println(static_cast<float>(elapsedMs) / 1000.0F, 1);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("T ");
  tft_.println(static_cast<float>(totalMs) / 1000.0F, 1);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("n ");
  tft_.println(samples);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("tap=esc");
}

void DisplayUi::showLatencyResult(const float minMs, const float maxMs, const float avgMs,
                                  const uint16_t samples) {
  char b1[12];
  char b2[12];
  char b3[12];
  formatFloat(b1, sizeof(b1), minMs, 5, 2);
  formatFloat(b2, sizeof(b2), maxMs, 5, 2);
  formatFloat(b3, sizeof(b3), avgMs, 5, 2);

  tft_.fillScreen(ST77XX_BLACK);
  tft_.setTextColor(ST77XX_WHITE);
  tft_.setTextSize(1);
  int16_t y = 4;
  tft_.setCursor(kMarginX, y);
  tft_.println("ms");
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("mn");
  tft_.println(b1);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("mx");
  tft_.println(b2);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("av");
  tft_.println(b3);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.print("n ");
  tft_.println(samples);
  y += kLineStep;
  tft_.setCursor(kMarginX, y);
  tft_.println("tap=menu");
}

}  // namespace ui
