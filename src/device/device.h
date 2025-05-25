#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>

union setValue {
    bool b;
    int i;
    float f;
};

enum class setValueType {
    BOOL,
    INT,
    FLOAT,
    STRING
};

enum class setterOutput {
    OK,
    NOT_SUPPORTED,
    READ_ONLY,
    INVALID_VALUE,
    ERROR
};

class Device {
protected:
    String name = "Unnamed Device";
public:
    virtual bool spin() = 0;
    virtual setValueType getType() = 0;
    virtual String serialize() = 0;
    
    virtual void get(setValue&) {};
    virtual String getName() const { return name; }
    virtual setterOutput set(const setValue&) { 
        return setterOutput::NOT_SUPPORTED; 
    };
    virtual setterOutput deserialize(const String& value) {
        return setterOutput::NOT_SUPPORTED;
    }

    virtual ~Device() {}
};

#endif  // DEVICE_H