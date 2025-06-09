#define USE_MODBUS // Uncomment to enable Modbus support
#define USE_HTTP // Uncomment to enable HTTP server support
//#define USE_SERIAL // Uncomment to enable debug serial

#include <Arduino.h>

#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#ifdef USE_HTTP
#include "net/http.h"
#endif // USE_HTTP

#ifdef USE_MODBUS
#include "net/modbus.h"
#endif // USE_MODBUS

#include "device/ds18b20.h"
#include "device/diagLed.h"
#include "device/binaryInput.h"
#include "device/binaryOutput.h"

// MAC address must be unique on your network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Use a static IP or comment it to use DHCP
IPAddress ip(192, 168, 1, 177);


// DS18B20 Pins (separate buses)
#define SENSOR1_PIN 8
#define SENSOR2_PIN 9

#define RELAY1_PIN 15
#define RELAY2_PIN 16
#define RELAY3_PIN 7 

#define IN1_PIN 14
#define IN2_PIN 17
#define IN3_PIN 18
#define IN4_PIN 19

#define DIAG_LED 3

DS18B20 sensor1("sensor_1",SENSOR1_PIN);
DS18B20 sensor2("sensor_2",SENSOR2_PIN);

BinaryInput in1("input_1",IN1_PIN);
BinaryInput in2("input_2",IN2_PIN);
BinaryInput in3("input_3",IN3_PIN);
BinaryInput in4("input_4",IN4_PIN);

// Initialize the relay outputs
BinaryOutput relay1("relay_1",RELAY1_PIN);
BinaryOutput relay2("relay_2",RELAY2_PIN);
BinaryOutput relay3("relay_3",RELAY3_PIN);

// Create a list of devices
Device* devices[] = {
    &sensor1,
    &sensor2,
    &relay1,
    &relay2,
    &relay3,
    &in1,
    &in2,
    &in3,
    &in4,
    nullptr // Null-terminated list
};

#ifdef USE_HTTP
// Initialize the Ethernet server
Http httpServer(devices);
#endif // USE_HTTP

#ifdef USE_MODBUS
ModbusNode modbusNodes[] = {
    ModbusNode{&sensor1, setValueType::FLOAT, 0, 1,10}, // Sensor 1
    ModbusNode{&sensor2, setValueType::FLOAT, 1, 1,10}, // Sensor 2
    ModbusNode{&relay1, setValueType::BOOL, 0}, // Relay 1
    ModbusNode{&relay2, setValueType::BOOL, 1}, // Relay 2
    ModbusNode{&relay3, setValueType::BOOL, 2}, // Relay 3
    ModbusNode{&in1, setValueType::BOOL, 3}, // Input 1
    ModbusNode{&in2, setValueType::BOOL, 4}, // Input 2
    ModbusNode{&in3, setValueType::BOOL, 5}, // Input 3
    ModbusNode{&in4, setValueType::BOOL, 6}, // Input 4
    {} // sentinel node
};
// Initialize the Modbus server
ModbusServer modbusServer(modbusNodes);
#endif // USE_MODBUS

// Initialize the diagnostic LED
DiagLed diagLed(DIAG_LED);
