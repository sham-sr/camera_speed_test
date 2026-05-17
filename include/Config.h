#pragma once

#include <Arduino.h>

// Параметры измерения задержки «стекло—стекло» и OLED SSD1306 128×64 (I2C) на Arduino Nano.
// Схема: ARD <-> LED (R/B) перед камерой; фотоприёмник, прижатый к экрану монитора.
// Измеряем задержку переключения цвета LED → отображение цвета на мониторе.

namespace cfg {

// --- Пины (см. pins.md) ----------------------------------------------------

inline constexpr uint8_t kPinButton = 2;     // Кнопка → GND, INPUT_PULLUP
inline constexpr uint8_t kPinLedA = 4;       // Биколор LED
inline constexpr uint8_t kPinLedB = 5;
inline constexpr uint8_t kPinPhotoAdc = A1;  // Фотоприёмник

// --- OLED SSD1306 I2C (по умолчанию Wire: SDA=A4, SCL=A5 на Nano) ----------

inline constexpr uint8_t kDisplayWidth = 128;
inline constexpr uint8_t kDisplayHeight = 64;

// Адрес модуля: чаще 0x3C, реже 0x3D (перемычка на плате).
inline constexpr uint8_t kOledI2cAddress7bit = 0x3C;

// -1: вывод RST не используется (типичный модуль с общим сбросом).
inline constexpr int8_t kOledResetPin = -1;

// Частота I2C, Гц (400 кГц — быстрее обновление; при длинных проводах попробуйте 100000).
inline constexpr uint32_t kOledI2cClockHz = 400000UL;

// Параметры шрифта по умолчанию (5x8) с межсимвольным пикселем: 6x8.
// На экране 128x64 умещается 21 символ x 8 строк — учитывается при вёрстке UI.
inline constexpr uint8_t kCharsPerLine = 21;
inline constexpr uint8_t kLinesPerScreen = 8;

// --- Опрос и ввод ------------------------------------------------------------

inline constexpr unsigned long kButtonDebounceMs = 45UL;
inline constexpr unsigned long kDoubleClickMaxGapMs = 420UL;
inline constexpr unsigned long kLongPressMs = 3000UL;
inline constexpr unsigned long kShortPressMaxMs = 800UL;

// --- Сканер 2 фаз: общий движок для калибровки и авто-прогрева --------------
// Порядок фаз: 0 — Red, 1 — Blue. Без Off-фаз: фоновый уровень не требуется,
// проверка пригодности обходится только по самим R/B (см. ниже).
inline constexpr uint8_t       kScanPhaseCount = 2;
inline constexpr unsigned long kCalibPhaseDurationMs = 5000UL;  // длинная калибровка
inline constexpr unsigned long kCalibSamplePeriodMs  = 15UL;
inline constexpr unsigned long kWarmupPhaseDurationMs = 500UL;  // короткий авто-прогрев
inline constexpr unsigned long kWarmupSamplePeriodMs  = 10UL;
inline constexpr unsigned long kScanUiPeriodMs = 250UL;

// --- Измерение задержки -----------------------------------------------------
// В измерении только R<->B (без Off), чтобы стабилизировать AE/AGC камеры
// и измерять чистое время реакции цепи камера → транспорт → монитор.
//
// ГЛАВНЫЙ ПАРАМЕТР — ожидаемая верхняя граница задержки тракта (мс).
// Из него автоматически выводятся полупериод стимула, таймаут датчика и
// длительность сессии. Меняйте, если ваш тракт заведомо медленнее.
//
// Типовые значения:
//   150  — HDMI / низкозадержный аналог
//   300  — OpenIPC RTSP UDP локально
//   500  — IP / RTSP / Wi-Fi                       ← по умолчанию
//   1000 — Wi-Fi с большой буферизацией
//   2000 — экстремум (плохой WAN, много буферов)
//
// Правила, на которых построены формулы ниже:
//   - Полупериод > задержки, иначе следующее переключение случится ДО прихода
//     фронта от текущего → алиасинг и ложные выборки.
//   - Таймаут датчика < полупериода (чтобы освободить ожидание перед сменой LED).
//   - Сессия — не меньше 15 с И не меньше ~30 переключений (по 15 в каждую сторону).
inline constexpr unsigned long kLatencyMaxExpectedMs = 500UL;

// Полупериод: 1.5x от ожидаемого максимума (запас 50%).
inline constexpr unsigned long kLatencyBlinkHalfPeriodMs =
    (kLatencyMaxExpectedMs * 3UL) / 2UL;

// Таймаут датчика: 1.2x от ожидаемого максимума, гарантированно < полупериода.
inline constexpr unsigned long kLatencySensorTimeoutUs =
    (kLatencyMaxExpectedMs * 12UL / 10UL) * 1000UL;

// Длительность сессии: max(15 с, 30 полных переключений).
inline constexpr unsigned long kLatencySessionMs =
    (kLatencyBlinkHalfPeriodMs * 30UL > 15000UL)
        ? (kLatencyBlinkHalfPeriodMs * 30UL)
        : 15000UL;

inline constexpr unsigned long kLatencyUiPeriodMs            = 250UL;
inline constexpr int16_t       kLatencyThresholdHysteresisAdc = 8;

// --- Пригодность измерения --------------------------------------------------
// Решение «возможно ли измерение» по результатам сканера 2 фаз (только R/B).
inline constexpr int16_t  kFeasibleMinSpreadAdc = 32;    // |cR - cB|
inline constexpr uint16_t kFeasibleClipLow  = 4;         // cR/cB не должны быть < clipLow
inline constexpr uint16_t kFeasibleClipHigh = 1019;      // и не > clipHigh

// --- АЦП --------------------------------------------------------------------

inline constexpr float kAdcVrefVolts = 5.0F;
inline constexpr float kAdcMaxCode = 1023.0F;

}  // namespace cfg
