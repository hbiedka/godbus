#include <Arduino.h>

#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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

#define IN1_PIN 14
#define IN2_PIN 17

#define DIAG_LED 3

// Initialize the Ethernet server
EthernetServer server(80);

DS18B20 sensor1(SENSOR1_PIN);
DS18B20 sensor2(SENSOR2_PIN);

BinaryInput in1(IN1_PIN);
BinaryInput in2(IN2_PIN);

// Initialize the relay outputs
BinaryOutput relay1(RELAY1_PIN);
BinaryOutput relay2(RELAY2_PIN);

// Initialize the diagnostic LED
DiagLed diagLed(DIAG_LED);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

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

  // Start listening for clients
  server.begin();

  diagLed.blink();
}

void loop() {

  bool busy = false;
  busy |= sensor1.spin();
  busy |= sensor2.spin();
  busy |= in1.spin();
  busy |= in2.spin();
  diagLed.spin();

  if (busy) {
    diagLed.blink();
  }

  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client");

    // Wait until client sends data
    while (client.connected() && !client.available()) {
      delay(1);
    }

    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();

    // Simple HTTP response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();

    if (req.indexOf("GET /relay1/on") != -1) {
      relay1.set(true);
      Serial.println("Relay 1 ON");
    }
    else if (req.indexOf("GET /relay1/off") != -1) {
      relay1.set(false);
      Serial.println("Relay 1 OFF");
    }
    else if (req.indexOf("GET /relay2/on") != -1) {
      relay2.set(true);
      Serial.println("Relay 2 ON");
    }
    else if (req.indexOf("GET /relay2/off") != -1) {
      relay2.set(false);
      Serial.println("Relay 2 OFF");
    } else {
      client.print("Sensor 1: ");
      client.print(sensor1.serialize());
      client.print(" °C\n");
      client.print("Sensor 2: ");
      client.print(sensor2.serialize());
      client.print(" °C\n");
      client.print("Input 1: ");
      client.print(in1.serialize());
      client.print("\n");
      client.print("Input 2: ");
      client.print(in2.serialize());
      client.print("\n");
      client.print("Relay 1: ");
      client.print(relay1.serialize());
      client.print("\n");
      client.print("Relay 2: ");
      client.print(relay2.serialize());
      client.print("\n");
      client.print("Client IP: ");
      client.print(client.remoteIP());
      client.print("\n");    
    }

    delay(1);
    client.stop();
    Serial.println("Client disconnected");
  }
}