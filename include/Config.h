#pragma once

#include <Arduino.h>

// Параметры измерения задержки «стекло—стекло» и OLED SSD1306 128×64 (I2C) на Arduino Nano.
// Схема: ARD <-> LED (R/B) перед камерой; фотоприёмник, прижатый к экрану монитора.
// Измеряем задержку переключения цвета LED → отображение цвета на мониторе.

namespace cfg {

// --- Пины (см. README.md, раздел «Распиновка») -----------------------------

static constexpr uint8_t kPinButton = 2;     // Кнопка → GND, INPUT_PULLUP
static constexpr uint8_t kPinLedA = 4;       // Биколор LED
static constexpr uint8_t kPinLedB = 5;
static constexpr uint8_t kPinPhotoAdc = A1;  // Фотоприёмник

// --- OLED SSD1306 I2C (по умолчанию Wire: SDA=A4, SCL=A5 на Nano) ----------

// false — OLED не подключён: Wire/I2C не трогаем (нет зависаний на «висящей» шине).
// true  — модуль на A4/A5; при отсутствии ответа остаётся только Serial.
static constexpr bool kOledEnabled = true;

static constexpr uint8_t kDisplayWidth = 128;
static constexpr uint8_t kDisplayHeight = 64;

// Адрес модуля: чаще 0x3C, реже 0x3D (перемычка на плате).
static constexpr uint8_t kOledI2cAddress7bit = 0x3C;

// -1: вывод RST не используется (типичный модуль с общим сбросом).
static constexpr int8_t kOledResetPin = -1;

// Частота I2C, Гц (400 кГц — быстрее обновление; при длинных проводах попробуйте 100000).
static constexpr uint32_t kOledI2cClockHz = 400000UL;

// Таймаут операций Wire (мкс), если ядро Arduino поддерживает WIRE_HAS_TIMEOUT.
static constexpr uint32_t kOledWireTimeoutUs = 2500UL;

// Параметры шрифта по умолчанию (5x8) с межсимвольным пикселем: 6x8.
// На экране 128x64 умещается 21 символ x 8 строк — учитывается при вёрстке UI.
static constexpr uint8_t kCharsPerLine = 21;
static constexpr uint8_t kLinesPerScreen = 8;

// --- Опрос и ввод ------------------------------------------------------------

static constexpr unsigned long kButtonDebounceMs = 45UL;
static constexpr unsigned long kDoubleClickMaxGapMs = 420UL;
static constexpr unsigned long kLongPressMs = 3000UL;
static constexpr unsigned long kShortPressMaxMs = 800UL;

// --- Сканер 2 фаз: общий движок для калибровки и авто-прогрева --------------
// Порядок фаз: 0 — Red, 1 — Blue. Без Off-фаз: фоновый уровень не требуется,
// проверка пригодности обходится только по самим R/B (см. ниже).
static constexpr uint8_t       kScanPhaseCount = 2;
static constexpr unsigned long kCalibPhaseDurationMs = 5000UL;  // длинная калибровка
static constexpr unsigned long kCalibPhaseSettleMs   = 1200UL;  // не учитывать выборки после смены LED
static constexpr unsigned long kCalibSamplePeriodMs  = 15UL;
static constexpr unsigned long kWarmupPhaseDurationMs = 2500UL; // авто-прогрев перед замером
static constexpr unsigned long kWarmupPhaseSettleMs   = 700UL;
static constexpr unsigned long kWarmupSamplePeriodMs  = 10UL;
static constexpr unsigned long kScanUiPeriodMs = 250UL;

// Робастная оценка уровня/шума по гистограмме АЦП (перцентили, без min/max по выбросам).
static constexpr uint8_t kAdcHistBins = 64;
static constexpr uint8_t kHistPercentileLow  = 10;  // «mn» и нижняя граница шума
static constexpr uint8_t kHistPercentileMid  = 50;  // уровень cR/cB
static constexpr uint8_t kHistPercentileHigh = 90;  // «mx» и верхняя граница шума

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
static constexpr unsigned long kLatencyMaxExpectedMs = 500UL;

// Полупериод: 1.5x от ожидаемого максимума (запас 50%).
static constexpr unsigned long kLatencyBlinkHalfPeriodMs =
    (kLatencyMaxExpectedMs * 3UL) / 2UL;

// Таймаут датчика: 1.2x от ожидаемого максимума, гарантированно < полупериода.
static constexpr unsigned long kLatencySensorTimeoutUs =
    (kLatencyMaxExpectedMs * 12UL / 10UL) * 1000UL;

// Длительность сессии: max(15 с, 30 полных переключений).
static constexpr unsigned long kLatencySessionMs =
    (kLatencyBlinkHalfPeriodMs * 30UL > 15000UL)
        ? (kLatencyBlinkHalfPeriodMs * 30UL)
        : 15000UL;

static constexpr unsigned long kLatencyUiPeriodMs = 250UL;

// Сколько подряд читать АЦП за один проход loop() пока ждём фронт (без UI/Serial).
static constexpr uint8_t kLatencyEdgePollBurst = 8U;

// --- Адаптивный гистерезис порога (после калибровки) -------------------------
// H = clamp(|cR−cB| / kAdaptiveHysteresisSpreadDiv, min, max).
// При слабом сигнале (dRB≈12…30) H≈2…5; при сильном (dRB≈600) H не выше max.
static constexpr int16_t kAdaptiveHysteresisMin = 2;
static constexpr int16_t kAdaptiveHysteresisMax = 24;
static constexpr int16_t kAdaptiveHysteresisSpreadDiv = 6;

// --- Пригодность измерения --------------------------------------------------
// Решение «возможно ли измерение» по результатам сканера 2 фаз (только R/B).
static constexpr int16_t  kFeasibleMinSpreadAdc = 10;   // |cR - cB| (слабый луч на A1)
static constexpr int16_t  kFeasibleSpreadAboveNoise = 4;  // dRB > max(sprdR, sprdB) + это
static constexpr uint16_t kFeasibleClipLow  = 4;         // cR/cB не должны быть < clipLow
static constexpr uint16_t kFeasibleClipHigh = 1019;      // и не > clipHigh

// --- АЦП --------------------------------------------------------------------

static constexpr float kAdcVrefVolts = 5.0F;
static constexpr float kAdcMaxCode = 1023.0F;

}  // namespace cfg
