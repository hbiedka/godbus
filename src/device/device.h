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

#define MAX_NAME_SIZE 16

class Device {
protected:
    char name[MAX_NAME_SIZE] = "Unnamed";
public:
    virtual bool spin() = 0;
    virtual setValueType getType() = 0;
    virtual unsigned int serialize(char *s, size_t len) = 0;
    
    virtual void get(setValue&) {};
    virtual const char* getName() { return name; }
    virtual setterOutput set(const setValue&) { 
        return setterOutput::NOT_SUPPORTED; 
    };
    virtual setterOutput deserialize(const String& value) {
        return setterOutput::NOT_SUPPORTED;
    }

    virtual ~Device() {}
};

#endif  // DEVICE_H