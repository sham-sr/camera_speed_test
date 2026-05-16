#pragma once

#include <Arduino.h>

// Настройки под STM32F103C8 «Blue Pill»: ST7789 76×284, биколор LED, фотоприёмник на PA0, кнопка PB12.
// USART2 по умолчанию: PA2=TX, PA3=RX — не занимаем их под TFT.

namespace cfg {

// --- Пины (см. pins.md) ----------------------------------------------------

inline constexpr uint8_t kPinButton = PB12;     // Кнопка → GND, INPUT_PULLUP
inline constexpr uint8_t kPinLedA = PB6;        // Биколорный LED (вторая сторона kPinLedB)
inline constexpr uint8_t kPinLedB = PB7;
inline constexpr uint8_t kPinPhotoAdc = PA0;  // Фотоприёмник (схема на 3.3 В, см. pins.md)

inline constexpr uint8_t kPinTftBl = PB1;      // Подсветка TFT (если BL управляется с MCU)
inline constexpr uint8_t kPinTftDc = PA1;      // TFT DC
inline constexpr uint8_t kPinTftRst = PB0;     // TFT RST
inline constexpr uint8_t kPinTftCs = PA4;      // TFT CS (аппаратный SPI1)

// --- Опрос и ввод ------------------------------------------------------------

inline constexpr unsigned long kButtonDebounceMs = 45UL;
inline constexpr unsigned long kDoubleClickMaxGapMs = 420UL;
inline constexpr unsigned long kLongPressMs = 3000UL;
inline constexpr unsigned long kShortPressMaxMs = 800UL;

// --- Калибровка ------------------------------------------------------------

inline constexpr unsigned long kCalibPhaseDurationMs = 5000UL;
inline constexpr uint8_t kCalibPhaseCount = 4;
inline constexpr unsigned long kCalibSamplePeriodMs = 15UL;

// --- Измерение задержки -----------------------------------------------------

inline constexpr unsigned long kLatencySessionMs = 15000UL;
inline constexpr unsigned long kLatencyWarmupDarkMs = 150UL;
inline constexpr unsigned long kLatencyWarmupLitMs = 150UL;
inline constexpr unsigned long kLatencyBlinkHalfPeriodMs = 250UL;
inline constexpr uint16_t kLatencyMinGoodSamples = 5;
inline constexpr unsigned long kLatencySensorTimeoutUs = 50000UL;
inline constexpr int16_t kLatencyThresholdHysteresisAdc = 8;

// --- Дисплей ST7789 ---------------------------------------------------------

inline constexpr uint16_t kDisplayWidth = 76;
inline constexpr uint16_t kDisplayHeight = 284;

inline constexpr bool kDisplayInitNative240x320 = false;

inline constexpr uint8_t kDisplaySpiDataMode = 3;
inline constexpr uint8_t kDisplayRotation = 0;

inline constexpr bool kBacklightHardwiredToGnd = true;
inline constexpr bool kBacklightUsePwm = false;
inline constexpr bool kBacklightActiveHigh = true;
inline constexpr uint8_t kBacklightPwm = 255;

inline constexpr unsigned long kDisplayResetSettleMs = 50UL;
inline constexpr bool kDisplayBootSelfTest = true;
inline constexpr bool kDisplayBootProfileSweep = true;
inline constexpr unsigned long kDisplayProfileStepMs = 1600UL;
inline constexpr unsigned long kDisplaySelfTestStepMs = 1800UL;
inline constexpr unsigned long kDisplaySelfTestBlackHoldMs = 1200UL;
inline constexpr unsigned long kDisplaySelfTestSerialBaud = 115200UL;
inline constexpr bool kDisplaySelfTestBlProbeAtEnd = false;
inline constexpr uint8_t kDisplaySelfTestBlPulses = 2;
inline constexpr unsigned long kDisplaySelfTestBlHalfMs = 350UL;

inline constexpr bool kDisplayInvertColors = true;

// --- АЦП (STM32: 12 бит после analogReadResolution в PhotoSensor::begin) --

inline constexpr float kAdcVrefVolts = 3.3F;
inline constexpr float kAdcMaxCode = 4095.0F;
inline constexpr uint16_t kAdcMaxCodeU = 4095U;

}  // namespace cfg
