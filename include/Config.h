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
// Если подсветку замкнули на GND по разводке модуля (см. pins.md, issue TFT_eSPI) — пин не трогаем.
inline constexpr uint8_t kPinTftBl = 6;        // Подсветка TFT (только если BL управляется с MCU)
inline constexpr uint8_t kPinTftDc = 8;        // TFT DC (Data/Command)
inline constexpr uint8_t kPinTftRst = 9;       // TFT RST
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

// Логический размер области под ваш UI (текст, вёрстка). Не обязан совпадать с init() драйвера.
inline constexpr uint16_t kDisplayWidth = 76;
inline constexpr uint16_t kDisplayHeight = 284;

// ST7789 2.25" 76×284 (ST7789P3), см. обсуждения:
// https://github.com/Bodmer/TFT_eSPI/pull/3769 — для этого стекла нужны смещения в RAM **col=82, row=18**
// относительно кристалла 240×320; иначе шум / пустой экран.
// https://github.com/Bodmer/TFT_eSPI/issues/3804 — ориентация 284×76, инверсия цветов, CGRAM_OFFSET в TFT_eSPI.
// У Adafruit ST7789 при **init(76, 284)** те же отступы задаются автоматически (центрирование окна).
// Не включайте kDisplayInitNative240x320 для этого модуля: init(240,320) даёт (0,0) и картинка может быть
// **вне видимой области** узкого стекла.
inline constexpr bool kDisplayInitNative240x320 = false;

// Режим SPI для ST7789: только **0** или **3** (смысл см. DisplayUi — на AVR нельзя передать «сырое» 3 вместо SPI_MODE3).
// 0 → SPI_MODE0; 3 → SPI_MODE3 (0x0C). При «весь экран белый» сначала проверьте проводку MOSI/SCK/CS/DC/RST, затем 3 и kDisplayInvertColors.
inline constexpr uint8_t kDisplaySpiDataMode = 0;

// Ориентация Adafruit 0…3.
inline constexpr uint8_t kDisplayRotation = 0;

// Делитель SCK на AVR при kDisplaySpiSlowAvr: допустимо 16, 32, 64, 128 (как у SPI_CLOCK_DIV*).
inline constexpr uint8_t kDisplayAvrSpiDivider = 32;

// --- Подсветка BL (см. pins.md, пин kPinTftBl) -------------------------------

// true: вывод BL на плате дисплея **постоянно на GND** (как в обсуждениях ST7789 76×284); с MCU **не связывать**.
// Тогда backlightSet/backlightOn не конфигурируют kPinTftBl (иначе риск короткого, если провод всё ещё к BL).
inline constexpr bool kBacklightHardwiredToGnd = true;

// Многие модули BL стабильнее работают от постоянного HIGH/LOW, чем от ШИМ ~490 Гц на Nano:
// при «белом экране» или мигании сначала установите kBacklightUsePwm = false.
inline constexpr bool kBacklightUsePwm = false;

// Уровень «включено»: true = HIGH включает подсветку (типично); false = активный низкий транзистор на плате.
inline constexpr bool kBacklightActiveHigh = true;

// Яркость 0…255 только если kBacklightUsePwm == true (иначе игнорируется).
inline constexpr uint8_t kBacklightPwm = 255;

// Вспомогательная задержка после аппаратного сброса дисплея перед инициализацией драйвера.
inline constexpr unsigned long kDisplayResetSettleMs = 50UL;

// Самотест при включении: вспышки подсветки + полноэкранные цвета + подсказки в Serial Monitor.
// Поставьте false после отладки, чтобы не ждать паузы при каждом сбросе.
inline constexpr bool kDisplayBootSelfTest = true;

// Пауза на каждом цвете самотеста, мс (долго, чтобы успеть отличить R/G/B от «белой вспышки»).
inline constexpr unsigned long kDisplaySelfTestStepMs = 1800UL;

// Первая заливка после init: чистый чёрный, мс (подсветка горит, пиксели должны быть тёмными).
inline constexpr unsigned long kDisplaySelfTestBlackHoldMs = 1200UL;

// Скорость UART для расшифровки (Arduino Nano: USB‑UART, например CH340).
inline constexpr unsigned long kDisplaySelfTestSerialBaud = 115200UL;

// Мигание подсветкой в конце самотеста (часто выглядит как «белые вспышки» — отключите, если мешает).
inline constexpr bool kDisplaySelfTestBlProbeAtEnd = false;

// Сколько полных циклов «BL выкл / вкл» в хвосте самотеста.
inline constexpr uint8_t kDisplaySelfTestBlPulses = 2;

// Половина периода импульса BL, мс.
inline constexpr unsigned long kDisplaySelfTestBlHalfMs = 350UL;

// На AVR с длинными проводами — более медленный SPI (устойчивее, чем максимальная частота).
inline constexpr bool kDisplaySpiSlowAvr = true;

// Некоторые платы ST7789: инверсия цвета; при «всё белое» или «не тем то» попробуйте true.
inline constexpr bool kDisplayInvertColors = false;

// --- АЦП --------------------------------------------------------------------

// Опорное напряжение для перевода кодов АЦП в вольты (Nano по умолчанию 5 В).
inline constexpr float kAdcVrefVolts = 5.0F;

inline constexpr float kAdcMaxCode = 1023.0F;

}  // namespace cfg
