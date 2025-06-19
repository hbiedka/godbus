#ifndef __MODBUS_H__
#define __MODBUS_H__


#include <Arduino.h>
#include <Ethernet.h>

#include "config.h"
#include "debugSerial.h"
#include "device/device.h"

struct ModbusNode {
    Device *dev; // Pointer to the device
    setValueType type; // Type of the device value
    unsigned int startAddress; // Starting address for the device
    unsigned int quantity = 1; // Number of registers for the device
    float multiplier = 1.0; // Multiplier for the value

    ModbusNode(Device *device, setValueType valueType, unsigned int address, 
               unsigned int qty = 1, float mult = 1.0) 
        : dev(device), type(valueType), startAddress(address), 
          quantity(qty), multiplier(mult) {}
          
    //sentinel constructor for ModbusNode
    ModbusNode() : dev(nullptr), type(setValueType::INT), startAddress(0), 
                quantity(1), multiplier(1.0) {} // Default constructor
};

enum class ModbusState {
    NOT_STARTED,
    LISTEN,
    START_RECV,
    RECV_MBAP,
    RECV_PDU,
    PROCESS_REQUEST,
    SENDING_RESPONSE,
    CEASING_CONNECTION,
};

enum class ModbusFunctionCode {
    READ_COILS = 0x01,
    READ_DISCRETE_INPUTS = 0x02,
    READ_HOLDING_REGISTERS = 0x03,
    READ_INPUT_REGISTERS = 0x04,
    WRITE_SINGLE_COIL = 0x05,
    WRITE_SINGLE_REGISTER = 0x06,
    WRITE_MULTIPLE_COILS = 0x0F,
    WRITE_MULTIPLE_REGISTERS = 0x10,
    // REPORT_SLAVE_ID = 0x11,
    // MASK_WRITE_REGISTER = 0x16,
    // READ_FIFO_QUEUE = 0x18
};

enum class ModbusExceptionCode {
    SUCCESS = 0x00,
    ILLEGAL_FUNCTION = 0x01,
    ILLEGAL_DATA_ADDRESS = 0x02,
    ILLEGAL_DATA_VALUE = 0x03,
    SLAVE_DEVICE_FAILURE = 0x04,
    ACKNOWLEDGE = 0x05,
    SLAVE_DEVICE_BUSY = 0x06,
    MEMORY_PARITY_ERROR = 0x08,
    GATEWAY_PATH_UNAVAILABLE = 0x0A,
    GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND = 0x0B
};

class ModbusClient {
private:
    EthernetServer *server;
    EthernetClient client;

    unsigned char mbap[8]; // Buffer for incoming requests
    unsigned char pdu[16];
    unsigned char sendbuf[64];
    
    int mbapReceived = 0; // Number of bytes received in the MBAP header
    int pduReceived = 0; // Number of bytes received in the PDU

    int mbapLength = 7; // Length of the MBAP header
    int pduLength = 0; // Length of the PDU
    int sendbufLength = 0; // Length of the send buffer

    ModbusNode *registerTable = nullptr; // Pointer to the example register table

    ModbusState state = ModbusState::NOT_STARTED;

    public:
    ModbusClient() :
        server(nullptr),
        registerTable(nullptr)
    {}
    ModbusClient(EthernetServer *srv, ModbusNode *regs) : 
        server(srv)
    {
        registerTable = regs; // Initialize the register table with the provided nodes
    };
    bool spin();
    bool isAssignedToMe(EthernetClient &c);
    bool tryAssignNewConnection(const EthernetClient &c);
    void processRequest();
    int modbusQuery(const ModbusFunctionCode &functionCode, 
                    unsigned char *rqPayload, 
                    const int &rqPayloadLength, 
                    unsigned char *outputBuf, 
                    const unsigned int &maxOutputBufLength);
    ModbusExceptionCode getRegisters(unsigned int startAddress, 
                     unsigned int quantity, 
                     unsigned char *outputBuf, 
                     const unsigned int &maxOutputBufLength);
    ModbusExceptionCode getDiscreteInputs(unsigned int startAddress,
                        unsigned int quantity, 
                        unsigned char *outputBuf, 
                        const unsigned int &maxOutputBufLength);
    ModbusExceptionCode writeSingleCoil(unsigned int address, bool value);
};

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

        bool spin() {
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


};

#endif // __MODBUS_H__