#include "SerialInput.h"

namespace input {

void SerialInput::begin() {
  // Serial должен быть открыт в Application::begin() ДО этого вызова —
  // здесь только выводим помощь.
  printHelp();
}

void SerialInput::printHelp() {
  Serial.println();
  Serial.println(F("--- KRAN glass2glass: serial console ---"));
  Serial.println(F("commands (one char, case-insensitive):"));
  Serial.println(F("  m  - menu / back / exit any screen"));
  Serial.println(F("  c  - CALIBRATION (R/B, 2 phases x 5s, robust p10-p90)"));
  Serial.println(F("  r  - RUN (warmup 2x2.5s + measure R<->B)"));
  Serial.println(F("  h  - this help"));
  Serial.println(F("---"));
}

ButtonGesture SerialInput::tick() {
  while (Serial.available() > 0) {
    const int raw = Serial.read();
    if (raw < 0) {
      break;
    }
    const char ch = static_cast<char>(raw);
    // Игнорируем переводы строк, пробелы и табуляцию.
    if (ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t') {
      continue;
    }
    const char lower =
        (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
    switch (lower) {
      case 'm':
        Serial.println(F(">> [cmd] MENU"));
        return ButtonGesture::Single;
      case 'c':
        Serial.println(F(">> [cmd] CALIBRATION"));
        return ButtonGesture::DoubleShort;
      case 'r':
        Serial.println(F(">> [cmd] MEASURE"));
        return ButtonGesture::LongHold;
      case 'h':
      case '?':
        printHelp();
        break;
      default:
        Serial.print(F("?? unknown cmd: '"));
        Serial.print(ch);
        Serial.println(F("' (type 'h' for help)"));
        break;
    }
  }
  return ButtonGesture::None;
}

}  // namespace input
