#include "binaryOutput.h"

BinaryOutput::BinaryOutput(int _pin) {
    pin = _pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

bool BinaryOutput::spin() {
    return false; // No periodic action needed
}

void BinaryOutput::set(const setValue& value) {
    state = value.b;
    digitalWrite(pin, state);
    ts = millis();
}

String BinaryOutput::serialize() {
    return String(state);
}
