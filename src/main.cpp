#include "main.h"

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Start Ethernet connection
  Ethernet.begin(mac, ip);

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

  // Spin through all devices
  for (Device** dev = devices; *dev != nullptr; ++dev) {
    busy |= (*dev)->spin();
  }

  diagLed.spin();

  if (busy) {
    diagLed.blink();
  }

}