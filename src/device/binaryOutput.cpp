#include "binaryOutput.h"

BinaryOutput::BinaryOutput(String _name, int _pin) {
    strncpy(name, _name.c_str(), MAX_NAME_SIZE - 1);
    pin = _pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

bool BinaryOutput::spin() {
    return false; // No periodic action needed
}

void BinaryOutput::get(setValue& value) {
    value.b = state; // Get the current state
}

setterOutput BinaryOutput::set(const setValue& value) {
    state = value.b;
    digitalWrite(pin, state);
    ts = millis();
    return setterOutput::OK;
}

unsigned int BinaryOutput::serialize(char *s, size_t len) {
    
    //if no place to write
    if (len < 1) return 0;

    s[0] = state ? '1' : '0';
    s[1] = '\0';
    return 1; 
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
