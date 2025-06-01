#include "main.h"

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Start Ethernet connection
  Ethernet.begin(mac, ip);

  delay(1000);
  Serial.print("IP: ");
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

#ifdef USE_HTTP
  busy |= httpServer.spin();
#endif // USE_HTTP

#ifdef USE_MODBUS
  busy |= modbusServer.spin(); // Spin the Modbus server
#endif

  // Spin through all devices
  for (Device** dev = devices; *dev != nullptr; ++dev) {
    busy |= (*dev)->spin();
  }

  diagLed.spin();

  if (busy) {
    diagLed.blink();
  }

}