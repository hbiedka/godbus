#ifndef BINARY_INPUT_H
#define BINARY_INPUT_H

#include <Arduino.h>
#include "device.h"

enum class InputState {
    OFF,
    DEBOUNCE_ON,
    ON,
    DEBOUNCE_OFF
};

class BinaryInput : public Device {
    private:
        int pin;
        unsigned long ts;
        unsigned long debounceInterval = 50;
        InputState state = InputState::OFF;

        bool getState();
    public:
        BinaryInput(int pin);
        bool spin() override;
        void get(setValue &value) override;
        String serialize() override;
        setValueType getType() override { return setValueType::BOOL; }
};

#endif