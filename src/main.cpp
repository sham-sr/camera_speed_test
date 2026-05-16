#include <Arduino.h>

const int PIN_A = 4;
const int PIN_B = 5;

void setup() {
    pinMode(PIN_A, OUTPUT);
    pinMode(PIN_B, OUTPUT);
}

void loop() {

    // Первый цвет
    digitalWrite(PIN_A, HIGH);
    digitalWrite(PIN_B, LOW);

    delay(5000);

    // Второй цвет
    digitalWrite(PIN_A, LOW);
    digitalWrite(PIN_B, HIGH);

    delay(5000);
}