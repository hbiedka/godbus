#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>

union setValue {
    bool b;
    int i;
    float f;
    String s;
};

enum class setValueType {
    BOOL,
    INT,
    FLOAT,
    STRING
};

class Device {
public:
    virtual bool spin() = 0;
    virtual String serialize() = 0;
    virtual setValueType getType() = 0;
    virtual void set(const setValue&) {};
    virtual void get(setValue&) {};

    virtual ~Device() {}
};

#endif  // DEVICE_H