#ifndef BINARY_INPUT_H
#define BINARY_INPUT_H

#include <Arduino.h>

enum class InputState {
    OFF,
    DEBOUNCE_ON,
    ON,
    DEBOUNCE_OFF
};

class BinaryInput {
    private:
        int pin;
        unsigned long ts;
        unsigned long debounceInterval = 50;
        InputState state = InputState::OFF;

    public:
        BinaryInput(int pin);
        bool spin();
        bool get();
        String serialize();
};

#endif