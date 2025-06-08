#ifndef BINARY_OUTPUT_H
#define BINARY_OUTPUT_H

#include <Arduino.h>
#include "device.h"

class BinaryOutput : public Device {
    private:
        int pin;
        unsigned long ts;
        bool state = false;

    public:
        BinaryOutput(String _name, int _pin);
        bool spin() override;
        setterOutput set(const setValue &value) override;
        void get(setValue &value) override;
        unsigned int serialize(char *s, size_t len) override;
        setterOutput deserialize(char *s, size_t len) override;
        setValueType getType() override { return setValueType::BOOL; }
};

#endif