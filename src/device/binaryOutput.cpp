#include "binaryOutput.h"

BinaryOutput::BinaryOutput(int _pin) {
    pin = _pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

bool BinaryOutput::spin() {
}

void BinaryOutput::set(bool value) {
    digitalWrite(pin, value);
    state = value;
    ts = millis();
}

String BinaryOutput::serialize() {
    return String(state);
}
