#include "DisplayUi.h"

#include <string.h>

namespace ui {

namespace {
// Левый отступ и шаг строки для шрифта 5x8 (с однопиксельным межсимвольным пробелом).
constexpr int16_t kMarginX = 0;
constexpr int16_t kLineStep = 8;
}  // namespace

DisplayUi::DisplayUi()
    : disp_(static_cast<int16_t>(cfg::kDisplayWidth), static_cast<int16_t>(cfg::kDisplayHeight),
            &Wire, cfg::kOledResetPin) {}

void DisplayUi::flush() { disp_.display(); }

void DisplayUi::initOled() {
  Wire.begin();
  Wire.setClock(cfg::kOledI2cClockHz);
  if (!disp_.begin(SSD1306_SWITCHCAPVCC, cfg::kOledI2cAddress7bit)) {
    // Не хватило RAM под буфер — редко на Nano, но оставляем явный сигнал.
    Serial.begin(115200);
    Serial.println(F("SSD1306 begin failed"));
  }
  disp_.clearDisplay();
  disp_.setTextColor(SSD1306_WHITE);
  disp_.setTextSize(1);
  disp_.setTextWrap(false);
  flush();
}

void DisplayUi::formatFloat(char* buf, const size_t bufSize, const float value,
                            const uint8_t width, const uint8_t prec) {
  if (buf == nullptr || bufSize == 0U) {
    return;
  }
  memset(buf, 0, bufSize);
  dtostrf(static_cast<double>(value), static_cast<int>(width), static_cast<int>(prec), buf);
  buf[bufSize - 1U] = '\0';
}

void DisplayUi::begin() {
  Serial.begin(115200);
  initOled();
}

// ---------------------------------------------------------------------------
// Главное меню: подсказки управления и краткое описание.
// ---------------------------------------------------------------------------
void DisplayUi::showMainMenu() {
  // Времена тянем из Config.h, чтобы текст не расходился с фактическими параметрами.
  const unsigned long calPhaseSec = cfg::kCalibPhaseDurationMs / 1000UL;
  const unsigned long sessionSec  = cfg::kLatencySessionMs / 1000UL;

  // Дубль в Serial-консоль.
  Serial.println();
  Serial.print(F("[MENU]  m=back  c=cal("));
  Serial.print(cfg::kScanPhaseCount);
  Serial.print(F("ph x "));
  Serial.print(calPhaseSec);
  Serial.print(F("s)  r=run("));
  Serial.print(sessionSec);
  Serial.print(F("s, max~"));
  Serial.print(cfg::kLatencyMaxExpectedMs);
  Serial.println(F("ms)  h=help"));

  disp_.clearDisplay();
  disp_.setCursor(kMarginX, 0);
  disp_.println(F("KRAN  glass2glass"));   // 17 знаков
  disp_.setCursor(kMarginX, kLineStep);
  disp_.println(F("tap   = back"));         // 12
  disp_.setCursor(kMarginX, kLineStep * 2);
  disp_.println(F("2tap  = CAL R/B"));      // 15
  disp_.setCursor(kMarginX, kLineStep * 3);
  disp_.println(F("hold  = MEASURE"));      // 15

  disp_.setCursor(kMarginX, kLineStep * 5);
  disp_.print(F("cal:  "));
  disp_.print(cfg::kScanPhaseCount);
  disp_.print(F("ph x "));
  disp_.print(calPhaseSec);
  disp_.println(F("s"));

  disp_.setCursor(kMarginX, kLineStep * 6);
  disp_.print(F("meas: "));
  disp_.print(sessionSec);
  disp_.println(F("s R<->B"));

  disp_.setCursor(kMarginX, kLineStep * 7);
  disp_.print(F("max ~"));
  disp_.print(cfg::kLatencyMaxExpectedMs);
  disp_.println(F("ms"));
  flush();
}

// ---------------------------------------------------------------------------
// Экран длинной калибровки во время прохождения фазы.
// ---------------------------------------------------------------------------
void DisplayUi::showCalibrationScan(const uint8_t phaseIndex, const uint8_t phaseTotal,
                                    const __FlashStringHelper* phaseLabel,
                                    const uint16_t adcMin, const uint16_t adcMax,
                                    const uint16_t adcAvg, const uint16_t samples) {
  const uint16_t sprd =
      static_cast<uint16_t>(adcMax >= adcMin ? adcMax - adcMin : 0);
  Serial.print(F("[CAL] phase="));
  Serial.print(phaseIndex + 1U);
  Serial.print('/');
  Serial.print(phaseTotal);
  Serial.print(' ');
  Serial.print(phaseLabel);
  Serial.print(F("  avg="));
  Serial.print(adcAvg);
  Serial.print(F(" mn="));
  Serial.print(adcMin);
  Serial.print(F(" mx="));
  Serial.print(adcMax);
  Serial.print(F(" sprd="));
  Serial.print(sprd);
  Serial.print(F(" n="));
  Serial.println(samples);

  disp_.clearDisplay();

  disp_.setCursor(kMarginX, 0);
  disp_.print(F("CAL "));
  disp_.print(phaseIndex + 1U);
  disp_.print(F("/"));
  disp_.print(phaseTotal);
  disp_.print(F("  "));
  disp_.println(phaseLabel);

  disp_.setCursor(kMarginX, kLineStep);
  disp_.print(F("adc   "));
  disp_.println(adcAvg);

  disp_.setCursor(kMarginX, kLineStep * 2);
  disp_.print(F("mn "));
  disp_.print(adcMin);
  disp_.print(F("  mx "));
  disp_.println(adcMax);

  disp_.setCursor(kMarginX, kLineStep * 3);
  disp_.print(F("sprd  "));
  disp_.println(static_cast<uint16_t>(adcMax >= adcMin ? adcMax - adcMin : 0));

  disp_.setCursor(kMarginX, kLineStep * 4);
  disp_.print(F("n     "));
  disp_.println(samples);

  disp_.setCursor(kMarginX, kLineStep * 7);
  disp_.println(F("tap = menu"));
  flush();
}

// ---------------------------------------------------------------------------
// Итог калибровки + флаг пригодности.
// ---------------------------------------------------------------------------
void DisplayUi::showCalibrationSummary(const uint16_t cR, const uint16_t cB,
                                       const uint16_t spreadR, const uint16_t spreadB,
                                       const int16_t dRB, const bool feasible,
                                       const __FlashStringHelper* reasonLabel) {
  Serial.println();
  Serial.print(F("[CAL_RESULT] R="));
  Serial.print(cR);
  Serial.print(F(" +/-"));
  Serial.print(spreadR);
  Serial.print(F("  B="));
  Serial.print(cB);
  Serial.print(F(" +/-"));
  Serial.print(spreadB);
  Serial.print(F("  dRB="));
  Serial.print(dRB);
  Serial.print(F("  status="));
  if (feasible) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("FAIL: "));
    Serial.println(reasonLabel);
  }

  disp_.clearDisplay();

  disp_.setCursor(kMarginX, 0);
  disp_.println(F("CAL  RESULT"));

  disp_.setCursor(kMarginX, kLineStep * 2);
  disp_.print(F("R    "));
  disp_.print(cR);
  disp_.print(F(" +/-"));
  disp_.println(spreadR);

  disp_.setCursor(kMarginX, kLineStep * 3);
  disp_.print(F("B    "));
  disp_.print(cB);
  disp_.print(F(" +/-"));
  disp_.println(spreadB);

  disp_.setCursor(kMarginX, kLineStep * 4);
  disp_.print(F("dRB  "));
  disp_.println(dRB);

  disp_.setCursor(kMarginX, kLineStep * 6);
  if (feasible) {
    disp_.println(F("status: OK"));
  } else {
    disp_.print(F("FAIL: "));
    disp_.println(reasonLabel);
  }

  disp_.setCursor(kMarginX, kLineStep * 7);
  disp_.println(F("tap = menu"));
  flush();
}

// ---------------------------------------------------------------------------
// Короткий авто-прогрев перед замером.
// ---------------------------------------------------------------------------
void DisplayUi::showLatencyWarmup(const uint8_t phaseIndex, const uint8_t phaseTotal,
                                  const __FlashStringHelper* phaseLabel) {
  Serial.print(F("[WARMUP] phase="));
  Serial.print(phaseIndex + 1U);
  Serial.print('/');
  Serial.print(phaseTotal);
  Serial.print(F("  LED="));
  Serial.println(phaseLabel);

  disp_.clearDisplay();

  disp_.setCursor(kMarginX, 0);
  disp_.println(F("WARMUP"));

  disp_.setCursor(kMarginX, kLineStep);
  disp_.print(F("phase "));
  disp_.print(phaseIndex + 1U);
  disp_.print(F("/"));
  disp_.println(phaseTotal);

  disp_.setCursor(kMarginX, kLineStep * 2);
  disp_.print(F("LED   "));
  disp_.println(phaseLabel);

  disp_.setCursor(kMarginX, kLineStep * 4);
  disp_.println(F("checking R/B"));
  disp_.setCursor(kMarginX, kLineStep * 5);
  disp_.println(F("levels ..."));

  disp_.setCursor(kMarginX, kLineStep * 7);
  disp_.println(F("tap = menu"));
  flush();
}

// ---------------------------------------------------------------------------
// Невозможно измерять: чёткая причина и диагностика уровней.
// ---------------------------------------------------------------------------
void DisplayUi::showLatencyAborted(const __FlashStringHelper* reasonLabel,
                                   const uint16_t cR, const uint16_t cB, const int16_t dRB) {
  Serial.println();
  Serial.print(F("[ABORT] reason="));
  Serial.print(reasonLabel);
  Serial.print(F("  R="));
  Serial.print(cR);
  Serial.print(F("  B="));
  Serial.print(cB);
  Serial.print(F("  dRB="));
  Serial.println(dRB);

  disp_.clearDisplay();

  disp_.setCursor(kMarginX, 0);
  disp_.println(F("ABORT  no measure"));

  disp_.setCursor(kMarginX, kLineStep);
  disp_.print(F("reason: "));
  disp_.println(reasonLabel);

  disp_.setCursor(kMarginX, kLineStep * 3);
  disp_.print(F("R    "));
  disp_.println(cR);

  disp_.setCursor(kMarginX, kLineStep * 4);
  disp_.print(F("B    "));
  disp_.println(cB);

  disp_.setCursor(kMarginX, kLineStep * 5);
  disp_.print(F("dRB  "));
  disp_.println(dRB);

  disp_.setCursor(kMarginX, kLineStep * 7);
  disp_.println(F("tap = menu"));
  flush();
}

// ---------------------------------------------------------------------------
// Live во время измерения: «mn / av / mx» сразу в мс + два направления.
// ---------------------------------------------------------------------------
void DisplayUi::showLatencyLive(const unsigned long elapsedMs, const unsigned long totalMs,
                                const uint16_t totalCount, const uint16_t lost,
                                const float minMs, const float avgMs, const float maxMs,
                                const float avgRBms, const float avgBRms) {
  Serial.print(F("[LAT] t="));
  Serial.print(static_cast<float>(elapsedMs) / 1000.0F, 1);
  Serial.print('/');
  Serial.print(static_cast<float>(totalMs) / 1000.0F, 1);
  Serial.print(F("  n="));
  Serial.print(totalCount);
  Serial.print(F(" lost="));
  Serial.print(lost);
  Serial.print(F("  min="));
  Serial.print(minMs, 1);
  Serial.print(F(" avg="));
  Serial.print(avgMs, 1);
  Serial.print(F(" max="));
  Serial.print(maxMs, 1);
  Serial.print(F("  RB="));
  Serial.print(avgRBms, 1);
  Serial.print(F(" BR="));
  Serial.println(avgBRms, 1);

  char bMin[8];
  char bAvg[8];
  char bMax[8];
  char bRB[8];
  char bBR[8];
  formatFloat(bMin, sizeof(bMin), minMs, 5, 1);
  formatFloat(bAvg, sizeof(bAvg), avgMs, 5, 1);
  formatFloat(bMax, sizeof(bMax), maxMs, 5, 1);
  formatFloat(bRB, sizeof(bRB), avgRBms, 5, 1);
  formatFloat(bBR, sizeof(bBR), avgBRms, 5, 1);

  disp_.clearDisplay();

  // Шапка: текущее/целое время, кол-во выборок, потери.
  disp_.setCursor(kMarginX, 0);
  disp_.print(F("LAT "));
  disp_.print(static_cast<float>(elapsedMs) / 1000.0F, 1);
  disp_.print(F("/"));
  disp_.println(static_cast<float>(totalMs) / 1000.0F, 1);

  disp_.setCursor(kMarginX, kLineStep);
  disp_.print(F("n "));
  disp_.print(totalCount);
  disp_.print(F("  lost "));
  disp_.println(lost);

  // Главное: min / avg / max в миллисекундах.
  disp_.setCursor(kMarginX, kLineStep * 2);
  disp_.print(F("min "));
  disp_.print(bMin);
  disp_.println(F(" ms"));

  disp_.setCursor(kMarginX, kLineStep * 3);
  disp_.print(F("avg "));
  disp_.print(bAvg);
  disp_.println(F(" ms"));

  disp_.setCursor(kMarginX, kLineStep * 4);
  disp_.print(F("max "));
  disp_.print(bMax);
  disp_.println(F(" ms"));

  // По направлениям — компактно одной строкой.
  disp_.setCursor(kMarginX, kLineStep * 6);
  disp_.print(F("RB "));
  disp_.print(bRB);
  disp_.print(F("  BR "));
  disp_.println(bBR);

  disp_.setCursor(kMarginX, kLineStep * 7);
  disp_.println(F("tap = exit"));
  flush();
}

// ---------------------------------------------------------------------------
// Финальный экран — самое важное крупно и подписано.
// ---------------------------------------------------------------------------
void DisplayUi::showLatencyResult(const float minMs, const float avgMs, const float maxMs,
                                  const float avgRBms, const float avgBRms,
                                  const uint16_t totalCount, const uint16_t lost) {
  // В Serial — крупный блок «GLASS->GLASS» с одним полем на строку,
  // удобно копировать в протокол испытаний.
  Serial.println();
  Serial.println(F("[RESULT] GLASS->GLASS, ms:"));
  Serial.print(F("  min  = "));
  Serial.println(minMs, 2);
  Serial.print(F("  avg  = "));
  Serial.println(avgMs, 2);
  Serial.print(F("  max  = "));
  Serial.println(maxMs, 2);
  Serial.print(F("  RB av= "));
  Serial.println(avgRBms, 2);
  Serial.print(F("  BR av= "));
  Serial.println(avgBRms, 2);
  Serial.print(F("  n    = "));
  Serial.println(totalCount);
  Serial.print(F("  lost = "));
  Serial.println(lost);
  Serial.println();

  char bMin[8];
  char bAvg[8];
  char bMax[8];
  char bRB[8];
  char bBR[8];
  formatFloat(bMin, sizeof(bMin), minMs, 6, 2);
  formatFloat(bAvg, sizeof(bAvg), avgMs, 6, 2);
  formatFloat(bMax, sizeof(bMax), maxMs, 6, 2);
  formatFloat(bRB, sizeof(bRB), avgRBms, 6, 2);
  formatFloat(bBR, sizeof(bBR), avgBRms, 6, 2);

  disp_.clearDisplay();

  disp_.setCursor(kMarginX, 0);
  disp_.println(F("GLASS->GLASS  ms"));   // 16

  disp_.setCursor(kMarginX, kLineStep);
  disp_.print(F("min "));
  disp_.println(bMin);

  disp_.setCursor(kMarginX, kLineStep * 2);
  disp_.print(F("avg "));
  disp_.println(bAvg);

  disp_.setCursor(kMarginX, kLineStep * 3);
  disp_.print(F("max "));
  disp_.println(bMax);

  disp_.setCursor(kMarginX, kLineStep * 4);
  disp_.print(F("RB  "));
  disp_.println(bRB);

  disp_.setCursor(kMarginX, kLineStep * 5);
  disp_.print(F("BR  "));
  disp_.println(bBR);

  disp_.setCursor(kMarginX, kLineStep * 6);
  disp_.print(F("n="));
  disp_.print(totalCount);
  disp_.print(F("  lost="));
  disp_.println(lost);

  disp_.setCursor(kMarginX, kLineStep * 7);
  disp_.println(F("tap = menu"));
  flush();
}

}  // namespace ui
