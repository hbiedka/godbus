#include "ds18b20.h"

DS18B20::DS18B20(String _name, int _pin) {
    name = _name;
    pin = _pin;
    oneWire = OneWire(pin);
    sensor = DallasTemperature(&oneWire);

    sensor.setWaitForConversion(false);
    sensor.begin();

}

bool DS18B20::spin() {
    bool busy = false;
    unsigned long now = millis();

    switch (state) {
        case DS18B20State::STARTING:
        case DS18B20State::IDLE:
            if (now - ts > readInterval) {
                state = DS18B20State::PENDING;
                sensor.requestTemperatures();
                ts = now;
                busy = true;
            }
            break;

        case DS18B20State::PENDING:
            if (now - ts > pendingInterval) {
                float t = sensor.getTempCByIndex(0);
                if (t == DEVICE_DISCONNECTED_C) {
                    state = DS18B20State::ERROR;
                } else {
                    temperature = t;
                    state = DS18B20State::IDLE;
                }
                ts = now;
                busy = true;
            }
            break;

        case DS18B20State::ERROR:
            //reconnect
            if (now - ts > reconnectInterval) {
                state = DS18B20State::IDLE;
                sensor.begin();
                ts = now;
                busy = true;
            }
            break;
    }
    return busy;
}

void DS18B20::get(setValue &value) {
    value.f = temperature;
}

String DS18B20::serialize() {
    if (state == DS18B20State::ERROR || state == DS18B20State::STARTING) {
        return "null";
    }
    String result = String(temperature);
    return result;
}