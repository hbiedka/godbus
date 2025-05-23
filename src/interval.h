#ifndef INTERVAL_H
#define INTERVAL_H

#include <Arduino.h>

class IntervalOperation {
    private:
        unsigned long interval = 0;
        unsigned long last_ts = 0;
        
    public:
        IntervalOperation(unsigned long _interval) : interval(_interval) {}
        
        bool trig() {
            unsigned long ts = millis();
            if (ts - last_ts >= interval) {
                last_ts = ts;
                return true;
            }
            return false;
        }
};

#endif  // INTERVAL_H