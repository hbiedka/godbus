#ifndef MODBUS_SERVER_H
#define MODBUS_SERVER_H

#include "modbus.h"

class ModbusServer {
    private:
        EthernetServer server;
        ModbusNode *registerTable = nullptr; // Pointer to the example register table
        ModbusClient socket[MODBUS_SOCKETS];

        bool started = false;

    public:
        ModbusServer(ModbusNode *regs, uint16_t port) : 
        server(port) 
        {
            registerTable = regs; // Initialize the register table with the provided nodes
        };
        ModbusServer(ModbusNode *regs) : 
        server(502) // Default Modbus TCP port
        {
            registerTable = regs; // Initialize the register table with the provided nodes
        };
        bool spin();

};

#endif // MODBUS_SERVER_H