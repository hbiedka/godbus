#include "diagLed.h"

DiagLed::DiagLed(int _pin) {
    pin = _pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

bool DiagLed::spin() {
    bool busy = false;

    unsigned long now = millis();
    if (now - ts > blinkInterval) {
        state = false;
        digitalWrite(pin, LOW);
        ts = now;
        busy = true;
    }
    return busy;
}

void DiagLed::blink() {
    digitalWrite(pin, HIGH);
    state = true;
    ts = millis();
}