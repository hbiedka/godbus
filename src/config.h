#ifndef CONFIG_H
#define CONFIG_H

// Note: there is no recommended to enable all features at once due to memory constraints.

#define USE_MODBUS // Uncomment to enable Modbus support
#define USE_HTTP // Uncomment to enable HTTP server support
// #define USE_SERIAL // Uncomment to enable debug serial

#define MODBUS_SOCKETS 2    // Number of Modbus sockets to use


#endif // CONFIG_H