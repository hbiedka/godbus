#include <Arduino.h>

#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "http.h"
#include "interval.h"
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

// Initialize the Ethernet server
Http httpServer(devices);


// Initialize the diagnostic LED
DiagLed diagLed(DIAG_LED);

IntervalOperation updateResponse(1000); // 1 second interval for updating the response

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Start Ethernet connection
  Ethernet.begin(mac, ip);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5100) {
    Serial.println("W5100 Ethernet controller detected.");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5200) {
    Serial.println("W5200 Ethernet controller detected.");
  }
  else if (Ethernet.hardwareStatus() == EthernetW5500) {
    Serial.println("W5500 Ethernet controller detected.");
  }

  delay(1000);
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());

  diagLed.blink();

  // list all devices
  for (Device** dev = devices; *dev != nullptr; ++dev) {
    Serial.print("Device: ");
    Serial.println((*dev)->getName());
  }
}

void loop() {

  bool busy = false;
  busy |= httpServer.spin();
  busy |= sensor1.spin();
  busy |= sensor2.spin();
  busy |= in1.spin();
  busy |= in2.spin();
  diagLed.spin();

  if (busy) {
    diagLed.blink();
  }

  // Update the response every second
  if (updateResponse.trig()) {
    String response = "{\"sensor_1\": " + sensor1.serialize() + ", \"sensor_2\": " + sensor2.serialize() + "}";
    httpServer.setResponse(response);
  }

}