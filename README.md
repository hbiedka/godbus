# Godbus

IoT multitool with Ethernet connectivity

### What is it?

My goal was to utilize my old stuff, such as one of a dozen of AtMega328p and W5500 Ethernet module to aggregate signals in my home simple IoT system and provide simple but powerful device to integrate with my home automation based on Node-Red

### What does it can?

For now, the Godbus provides:
- 3 relay outputs
- 4 binary inputs (2 of them are optoinsulated)
- 2 DS18B20 temperature sensors in 2 separated 1-Wire buses.

### Communication protocols

- HTTP (JSON) - port 80
- Modbus TCP - port 502

### Basic configuration

In `src/config.h` you can enable/disable protocols, enable debug serial info (mostly for Modbus TCP package debug) and set the number of modbus TCP sockets.
