#include "main.h"

void setup() {

#ifdef USE_SERIAL
  Serial.begin(115200);
  while (!Serial);
#endif

  // Start Ethernet connection
  Ethernet.begin(mac, ip);

  delay(1000);
  diagLed.blink();

#ifdef USE_SERIAL
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  // list all devices
  for (Device** dev = devices; *dev != nullptr; ++dev) {
    Serial.print("Device: ");
    Serial.println((*dev)->getName());
  }
#endif
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