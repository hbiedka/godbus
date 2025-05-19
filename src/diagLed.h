#ifndef DIAG_LED_H
#define DIAG_LED_H

#include <Arduino.h>

class DiagLed {
    private:
        int pin;
        unsigned long ts;
        unsigned long blinkInterval = 100;
        bool state = false;

    public:
        DiagLed(int pin);
        bool spin();
        void blink();
};

#endif