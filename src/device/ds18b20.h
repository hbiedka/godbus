#ifndef DS18B20_H
#define DS18B20_H


#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>

#include "device.h"

enum class DS18B20State {
    STARTING,   // started but not data available yet
    IDLE,
    PENDING,    // waiting for the sensor to respond
    ERROR
};

class DS18B20 : public Device {
    private:
        OneWire oneWire;
        DallasTemperature sensor;
        int pin;
        float temperature;

        unsigned long ts;
        unsigned long readInterval = 5000;
        unsigned long pendingInterval = 750;
        unsigned long reconnectInterval = 30000;
        DS18B20State state = DS18B20State::STARTING;
        
    public:
        DS18B20(String _name, int _pin);
        bool spin() override;
        void get(setValue &value) override;
        unsigned int serialize(char *s, size_t len) override;
        setValueType getType() override { return setValueType::FLOAT; }
};

#endif