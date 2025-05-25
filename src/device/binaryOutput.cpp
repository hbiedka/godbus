#include "binaryOutput.h"

BinaryOutput::BinaryOutput(String _name, int _pin) {
    name = _name;
    pin = _pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

bool BinaryOutput::spin() {
    return false; // No periodic action needed
}

setterOutput BinaryOutput::set(const setValue& value) {
    state = value.b;
    digitalWrite(pin, state);
    ts = millis();
    return setterOutput::OK;
}

String BinaryOutput::serialize() {
    return String(state);
}

setterOutput BinaryOutput::deserialize(const String& value) {
    bool newState = false;
    
    // lowercase the value for easier comparison
    String lowerValue = value;
    lowerValue.toLowerCase();

    if (lowerValue == "true" || lowerValue == "1" || lowerValue == "on") {
        newState = true;
    } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "off") {
        newState = false;
    } else {
        return setterOutput::INVALID_VALUE; // Invalid value
    }

    // Call set with the explicitly created object
    return set(setValue{.b = newState});
}
