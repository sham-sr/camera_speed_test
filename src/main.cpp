#include <Arduino.h>

#include "Application.h"

namespace {
app::Application gApp;
}

// Точка входа прошивки: делегирование в прикладной слой (чистая архитектура — минимум логики в main).
void setup() { gApp.begin(); }

void loop() { gApp.loop(); }
