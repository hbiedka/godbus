#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include "config.h"
#include <Arduino.h>

#ifdef USE_SERIAL
    #define DEBUG(x) Serial.print(x)
    #define DEBUGLN(x) Serial.println(x)
    #define DEBUGHEX(x) Serial.print(x, HEX)
    #define DEBUGHEXLN(x) Serial.println(x, HEX)
#else
    #define DEBUG(x)
    #define DEBUGLN(x)
    #define DEBUGHEX(x)
    #define DEBUGHEXLN(x)
#endif // USE_SERIAL


#endif // DEBUG_SERIAL_H