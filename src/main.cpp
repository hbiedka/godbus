#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// MAC address must be unique on your network
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Use a static IP or comment it to use DHCP
IPAddress ip(192, 168, 2, 177);


// DS18B20 Pins (separate buses)
#define SENSOR1_PIN 8
#define SENSOR2_PIN 9

#define RELAY1_PIN 15
#define RELAY2_PIN 16

// OneWire instances
OneWire oneWire1(SENSOR1_PIN);
OneWire oneWire2(SENSOR2_PIN);

// DallasTemperature instances
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);

// Initialize the Ethernet server
EthernetServer server(80);


void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  sensor1.begin();
  sensor2.begin();
  sensor1.setWaitForConversion(false);
  sensor2.setWaitForConversion(false);

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
}

float temp1 = 0;
float temp2 = 0;
bool pending = false;
unsigned long lastMillis = 0;
unsigned long pendingPeriod = 750;
unsigned long interval = 2000; // Interval to read temperature

void loop() {
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
      digitalWrite(RELAY1_PIN, HIGH);
      Serial.println("Relay 1 ON");
    }
    else if (req.indexOf("GET /relay1/off") != -1) {
      digitalWrite(RELAY1_PIN, LOW);
      Serial.println("Relay 1 OFF");
    }
    else if (req.indexOf("GET /relay2/on") != -1) {
      digitalWrite(RELAY2_PIN, HIGH);
      Serial.println("Relay 2 ON");
    }
    else if (req.indexOf("GET /relay2/off") != -1) {
      digitalWrite(RELAY2_PIN, LOW);
      Serial.println("Relay 2 OFF");
    } else {
      client.print("Sensor 1: ");
      client.print(temp1);
      client.print(" °C\n");
      client.print("Sensor 2: ");
      client.print(temp2);
      client.print(" °C\n");
      client.print("Client IP: ");
      client.print(client.remoteIP());
      client.print("\n");    
    }

    delay(1);
    client.stop();
    Serial.println("Client disconnected");
  }
  else {
    if (pending) {
      if (millis() - lastMillis > pendingPeriod) {
        lastMillis = millis();
        pending = false;

        temp1 = sensor1.getTempCByIndex(0);
        temp2 = sensor2.getTempCByIndex(0);
        
        Serial.print("Sensor 1: ");
        Serial.print(temp1);
        Serial.print(" °C, Sensor 2: ");
        Serial.print(temp2);
        Serial.println(" °C");

      }
    } else {
      if (millis() - lastMillis > interval) {
        pending = true;
        lastMillis = millis();
        Serial.print("Requesting temperatures...");

        sensor1.requestTemperatures();
        sensor2.requestTemperatures();
      }
    }
  }
}