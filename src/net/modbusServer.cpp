#include "modbusServer.h"

bool ModbusServer::spin() {
    bool busy = false;

    if (!started) {
        server.begin();
        for (size_t i = 0; i < MODBUS_SOCKETS; i++) {
            socket[i] = ModbusClient(&server,registerTable);
        }
        started = true;
        return true;
    }

    //if new connection
    EthernetClient newClient = server.available();
    if (newClient) {

        //look for duplicates
        bool duplicate = false;
        for(size_t i = 0; i < MODBUS_SOCKETS; i++) {
            if(socket[i].isAssignedToMe(newClient)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            //find available client
            for (size_t i = 0; i < MODBUS_SOCKETS; i++) {
                if (socket[i].tryAssignNewConnection(newClient)) {
                    DEBUG("New Modbus connection assigned to socket");
                    DEBUGLN(newClient.getSocketNumber());
                    break;
                }
            }
        }
    }
    for (size_t i = 0; i < MODBUS_SOCKETS; i++) {
        busy |= socket[i].spin();
    }

    return busy;
}
