#include <Arduino.h>

#include "Application.h"

// Точка входа прошивки: делегирование в прикладной слой (чистая архитектура — минимум логики в main).
namespace {
app::Application gApp;
}

void setup() { gApp.begin(); }

void loop() { gApp.loop(); }
