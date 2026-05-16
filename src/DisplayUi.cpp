#include "DisplayUi.h"

#include <string.h>

namespace ui {

namespace {
enum LineLayout : int16_t { kMarginX = 2, kLineStep = 9 };

uint8_t resolveSt7789SpiMode() {
  // В Adafruit_ST7789::init() в SPCR должен попасть **SPI_MODE** из SPI.h. На ATmega328 **SPI_MODE3 = 0x0C**, не число 3.
  if (cfg::kDisplaySpiDataMode == 3) {
    return SPI_MODE3;
  }
  return SPI_MODE0;
}

void tuneSpiAfterBegin() {
#if defined(__AVR__)
  if (!cfg::kDisplaySpiSlowAvr) {
    return;
  }
  switch (cfg::kDisplayAvrSpiDivider) {
    case 16:
      SPI.setClockDivider(SPI_CLOCK_DIV16);
      break;
    case 32:
      SPI.setClockDivider(SPI_CLOCK_DIV32);
      break;
    case 64:
      SPI.setClockDivider(SPI_CLOCK_DIV64);
      break;
    case 128:
      SPI.setClockDivider(SPI_CLOCK_DIV128);
      break;
    default:
      SPI.setClockDivider(SPI_CLOCK_DIV32);
      break;
  }
#endif
}
}  // namespace

DisplayUi::DisplayUi() : tft_(cfg::kPinTftCs, cfg::kPinTftDc, cfg::kPinTftRst) {}

void DisplayUi::backlightOn() {
  backlightSet(true);
}

void DisplayUi::backlightSet(const bool on) {
  if (cfg::kBacklightHardwiredToGnd) {
    (void)on;
    return;
  }
  pinMode(cfg::kPinTftBl, OUTPUT);
  if (cfg::kBacklightUsePwm) {
    if (on) {
      const uint8_t v =
          cfg::kBacklightActiveHigh ? cfg::kBacklightPwm : static_cast<uint8_t>(255 - cfg::kBacklightPwm);
      analogWrite(cfg::kPinTftBl, v);
    } else {
      digitalWrite(cfg::kPinTftBl, cfg::kBacklightActiveHigh ? LOW : HIGH);
    }
  } else {
    const bool level = on ? cfg::kBacklightActiveHigh : !cfg::kBacklightActiveHigh;
    digitalWrite(cfg::kPinTftBl, level ? HIGH : LOW);
  }
}

void DisplayUi::initDisplayHardware() {
  SPI.begin();
  tuneSpiAfterBegin();
  const uint16_t iw = cfg::kDisplayInitNative240x320 ? 240U : cfg::kDisplayWidth;
  const uint16_t ih = cfg::kDisplayInitNative240x320 ? 320U : cfg::kDisplayHeight;
  tft_.init(iw, ih, resolveSt7789SpiMode());
  tft_.setRotation(cfg::kDisplayRotation & 3U);
  if (cfg::kDisplayInvertColors) {
    tft_.invertDisplay(true);
  }
}

void DisplayUi::probeBacklightPin() {
  Serial.println(F(""));
  if (cfg::kBacklightHardwiredToGnd) {
    Serial.println(F("=== BL на GND с модуля — импульсы с MCU пропущены ==="));
    return;
  }
  Serial.println(F("=== BL only (matrix unchanged) — смотрите смену яркости подсветки ==="));
  for (uint8_t i = 0; i < cfg::kDisplaySelfTestBlPulses; i++) {
    Serial.print(F("BL off "));
    Serial.println(i + 1U);
    backlightSet(false);
    delay(static_cast<unsigned int>(cfg::kDisplaySelfTestBlHalfMs));
    backlightSet(true);
    delay(static_cast<unsigned int>(cfg::kDisplaySelfTestBlHalfMs));
  }
  Serial.println(F("BL steady ON"));
  backlightOn();
}

void DisplayUi::runBootSelfTest() {
  Serial.begin(cfg::kDisplaySelfTestSerialBaud);
  delay(80);

  Serial.println(F(""));
  Serial.println(F("======== ST7789 boot self-test ========"));
  Serial.print(F("UI logical W x H "));
  Serial.print(cfg::kDisplayWidth);
  Serial.print(F(" x "));
  Serial.println(cfg::kDisplayHeight);
  if (cfg::kDisplayInitNative240x320) {
    Serial.println(F("init TFT as native 240 x 320 (set false if your glass is only a centered window)."));
  }
  Serial.print(F("SPI request cfg kDisplaySpiDataMode="));
  Serial.print(cfg::kDisplaySpiDataMode);
  Serial.print(F(" -> Adafruit mode 0x"));
  Serial.println(resolveSt7789SpiMode(), HEX);
  Serial.println(F("Инициализация матрицы; в конце самотеста по умолчанию без мигания BL."));

  backlightOn();
  delay(static_cast<unsigned int>(cfg::kDisplayResetSettleMs));

  Serial.println(F("Init SPI/display..."));
  initDisplayHardware();
  if (cfg::kDisplayInvertColors) {
    Serial.println(F("invertDisplay(true)"));
  } else {
    Serial.println(F("invertDisplay(false)"));
  }

  tft_.fillScreen(ST77XX_BLACK);
  Serial.println(F("Hold BLACK — должно быть темное поле при включенной подсветке (~не белое)."));
  delay(static_cast<unsigned int>(cfg::kDisplaySelfTestBlackHoldMs));

  tft_.fillScreen(ST77XX_RED);
  Serial.println(F("Hold RED ~2s"));
  delay(static_cast<unsigned int>(cfg::kDisplaySelfTestStepMs));

  tft_.fillScreen(ST77XX_GREEN);
  Serial.println(F("Hold GREEN ~2s"));
  delay(static_cast<unsigned int>(cfg::kDisplaySelfTestStepMs));

  tft_.fillScreen(ST77XX_BLUE);
  Serial.println(F("Hold BLUE ~2s"));
  delay(static_cast<unsigned int>(cfg::kDisplaySelfTestStepMs));

  tft_.fillScreen(ST77XX_WHITE);
  Serial.println(F("Hold WHITE ~2s"));
  delay(static_cast<unsigned int>(cfg::kDisplaySelfTestStepMs));

  tft_.fillScreen(ST77XX_BLACK);
  Serial.println(F("Hold BLACK again"));
  delay(static_cast<unsigned int>(cfg::kDisplaySelfTestStepMs));

  // Рамка по фактическому width()/height() после init (для 240×320 — весь экран).
  tft_.fillScreen(ST77XX_BLACK);
  {
    const int16_t gw = static_cast<int16_t>(tft_.width());
    const int16_t gh = static_cast<int16_t>(tft_.height());
    tft_.drawFastHLine(0, 0, gw, ST77XX_WHITE);
    tft_.drawFastHLine(0, gh - 1, gw, ST77XX_WHITE);
    tft_.drawFastVLine(0, 0, gh, ST77XX_WHITE);
    tft_.drawFastVLine(gw - 1, 0, gh, ST77XX_WHITE);
  }
  Serial.println(F("White 1px frame on black"));
  delay(static_cast<unsigned int>(cfg::kDisplaySelfTestStepMs));

  if (cfg::kDisplaySelfTestBlProbeAtEnd) {
    probeBacklightPin();
  }

  Serial.println(F("Interpret:"));
  Serial.println(F("- Видны смены BLACK/RED/GREEN/BLUE: матрица и SPI в целом работают."));
  Serial.println(F("- Нет цветов, только белый экран: чаще всего нет SPI (MOSI D11 SCK D13 CS D10 DC D8 RST D9 GND)."));
  Serial.println(F("- Поменяйте kDisplaySpiDataMode 0 или 3 (=SPI_MODE3) и kDisplayAvrSpiDivider 64 (медленнее)."));
  Serial.println(F("- Всё «белое» после fill BLACK: попробуйте kDisplayInvertColors=true (инверсия стекла)."));
  Serial.println(F("- kDisplayRotation 1..3 если картинка смещена/повёрнута."));
  Serial.println(F("- Картинка есть: при узком стекле можно kDisplayInitNative240x320=false."));
  Serial.println(F("========================================"));
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
  if (cfg::kDisplayBootSelfTest) {
    runBootSelfTest();
  } else {
    backlightOn();
    delay(static_cast<unsigned int>(cfg::kDisplayResetSettleMs));
    initDisplayHardware();
    tft_.fillScreen(ST77XX_BLACK);
  }

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
