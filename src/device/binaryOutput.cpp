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

setterOutput BinaryOutput::deserialize(char *s, size_t len) {
    bool newState = false;
    
    char lowercase[len];
    for (size_t i = 0; i < len; i++) {
        if(s[i] >= 'A' && s[i] <= 'Z') {
            lowercase[i] = s[i] + 32;
        } else {
            lowercase[i] = s[i];
        }
        if (s[i] == '\0') break;
    }

    if (strcmp(lowercase,"true") == 0 ||
                strcmp(lowercase,"on") == 0 || 
                strcmp(lowercase,"1") == 0 ) 
    {
        newState = true;
    } else  if (strcmp(lowercase,"false") == 0 ||
        strcmp(lowercase,"off") == 0 || 
        strcmp(lowercase,"0") == 0 )
    {
        newState = false;
    } else {
        return setterOutput::INVALID_VALUE; // Invalid value
    }

    // Call set with the explicitly created object
    return set(setValue{.b = newState});
}
