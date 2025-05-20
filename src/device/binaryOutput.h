#ifndef BINARY_OUTPUT_H
#define BINARY_OUTPUT_H

#include <Arduino.h>

class BinaryOutput {
    private:
        int pin;
        unsigned long ts;
        bool state = false;

    public:
        BinaryOutput(int pin);
        bool spin();
        void set(bool value);
        String serialize();
};

#endif