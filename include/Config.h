#pragma once

#include <Arduino.h>

// Все настраиваемые параметры системы измерения задержки «стекло—стекло».
// Значения подобраны под: Nano, ST7789, биколор D4/D5, фотоприёмник на A1, кнопка D2.

namespace cfg {

// --- Пины (см. pins.md) ----------------------------------------------------

inline constexpr uint8_t kPinButton = 2;       // Кнопка → GND, внутренняя подтяжка INPUT_PULLUP
inline constexpr uint8_t kPinLedA = 4;           // Вывод биколорного LED (вторая сторона kPinLedB)
inline constexpr uint8_t kPinLedB = 5;           // Вывод биколорного LED
inline constexpr uint8_t kPinPhotoAdc = A1;    // Сигнал фотоприёмника (в документации — фототранзистор)
inline constexpr uint8_t kPinTftBl = 6;        // Подсветка TFT (ШИМ/цифра)
inline constexpr uint8_t kPinTftRst = 8;       // TFT RST
inline constexpr uint8_t kPinTftDc = 9;        // TFT DC
inline constexpr uint8_t kPinTftCs = 10;       // TFT CS

// --- Опрос и ввод ------------------------------------------------------------

// Антидребезг кнопки: игнорировать дребезг контактов в течение этого времени после смены уровня.
inline constexpr unsigned long kButtonDebounceMs = 45UL;

// Окно второго щелчка для распознавания «двойного короткого» нажатия (отпускание → следующее нажатие).
inline constexpr unsigned long kDoubleClickMaxGapMs = 420UL;

// Порог длительного нажатия: при удержании не меньше этого времени — режим измерения задержки.
inline constexpr unsigned long kLongPressMs = 3000UL;

// Максимальная длительность одного «короткого» нажатия; иначе жест может быть отбракован как нечёткий.
inline constexpr unsigned long kShortPressMaxMs = 800UL;

// --- Калибровка (уровни на фотоприёмнике) -----------------------------------

// Длительность одной фазы «только красный» или «только синий» при калибровке (стабилизация + сбор выборок).
inline constexpr unsigned long kCalibPhaseDurationMs = 5000UL;

// Сколько полных фаз (красный и синий по очереди, всего kCalibPhaseDurationMs каждая) выполнить.
// Итого 4 фазы = 2 цикла красный→синий при kCalibPhaseCount = 4.
inline constexpr uint8_t kCalibPhaseCount = 4;

// Период опроса АЦП в режиме калибровки (не чаще, чтобы не занимать CPU зря).
inline constexpr unsigned long kCalibSamplePeriodMs = 15UL;

// --- Измерение задержки -----------------------------------------------------

// Общая длительность сессии измерения задержки (мигание + оценка фронтов).
inline constexpr unsigned long kLatencySessionMs = 15000UL;

// Прогрев: короткий сбор среднего АЦП при погашенном светодиоде (темновой уровень).
inline constexpr unsigned long kLatencyWarmupDarkMs = 150UL;

// Прогрев: короткий сбор среднего АЦП при включённом красном канале (яркий уровень).
inline constexpr unsigned long kLatencyWarmupLitMs = 150UL;

// Период мигания: половина этого времени — один цвет, вторая половина — другой (частая смена для многих отсчётов).
inline constexpr unsigned long kLatencyBlinkHalfPeriodMs = 250UL;

// Минимум выборок задержки, чтобы считать статистику устойчивой (иначе показываем предупреждение в логике UI).
inline constexpr uint16_t kLatencyMinGoodSamples = 5;

// Таймаут ожидания реакции датчика после переключения LED; дольше — пропуск цикла (помеха, засветка).
inline constexpr unsigned long kLatencySensorTimeoutUs = 50000UL;

// Гистерезис порога в единицах АЦП (0..1023): снижает дребезг около порога.
inline constexpr int16_t kLatencyThresholdHysteresisAdc = 8;

// --- Дисплей ST7789, аппаратный SPI (фактическое окно вывода по вашей сборке) --------

// Ширина и высота активной области в пикселях (порядок для Adafruit: ширина, затем высота).
inline constexpr uint16_t kDisplayWidth = 76;
inline constexpr uint16_t kDisplayHeight = 284;

// Примечание: при сдвиге изображения на модуле смотрите init/offsets в Adafruit_ST7789 под ваш PCB.

// Яркость подсветки 0…255 при управлении через analogWrite на kPinTftBl.
inline constexpr uint8_t kBacklightPwm = 220;

// Вспомогательная задержка после аппаратного сброса дисплея перед инициализацией драйвера.
inline constexpr unsigned long kDisplayResetSettleMs = 50UL;

// --- АЦП --------------------------------------------------------------------

// Опорное напряжение для перевода кодов АЦП в вольты (Nano по умолчанию 5 В).
inline constexpr float kAdcVrefVolts = 5.0F;

inline constexpr float kAdcMaxCode = 1023.0F;

}  // namespace cfg
