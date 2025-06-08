#include "binaryInput.h"

BinaryInput::BinaryInput(String _name, int _pin) {
    strncpy(name, _name.c_str(), MAX_NAME_SIZE - 1);
    pin = _pin;
    pinMode(pin, INPUT_PULLUP);
    state = InputState::OFF;
}

bool BinaryInput::spin() {
    bool busy = false;
    unsigned long now = millis();
    int value = digitalRead(pin);

    switch (state) {
        case InputState::OFF:
            if (value == LOW) {
                state = InputState::DEBOUNCE_ON;
                ts = now;
                busy = true;
            }
            break;

        case InputState::DEBOUNCE_ON:
            if (now - ts > debounceInterval) {
                state = InputState::ON;
                busy = true;
            }
            break;

        case InputState::ON:
            if (value == HIGH) {
                state = InputState::DEBOUNCE_OFF;
                ts = now;
                busy = true;
            }
            break;

        case InputState::DEBOUNCE_OFF:
            if (now - ts > debounceInterval) {
                state = InputState::OFF;
                busy = true;
            }
            break;
    }
    return busy;
}

void BinaryInput::get(setValue &value) {
    value.b = getState();
}

bool BinaryInput::getState() {
    return state == InputState::ON || state == InputState::DEBOUNCE_OFF;
}

unsigned int BinaryInput::serialize(char *s, size_t len) {

    //if no place to write
    if (len < 1) return 0;

    s[0] = getState() ? '1' : '0';
    s[1] = '\0';
    return 1; 
}

