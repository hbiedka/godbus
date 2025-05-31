#include "modbus.h"

bool Modbus::spin() {
    bool busy = false;

    switch (state) {
        case ModbusState::NOT_STARTED:
            server.begin();
            // Serial.println("Modbus server started");
            state = ModbusState::LISTEN;
            busy = true;
            break;

        case ModbusState::LISTEN:
            client = server.available();
            if (client) {
                Serial.println("New Modbus client connected");
                mbapReceived = 0; // Reset the MBAP received counter
                state = ModbusState::RECV_MBAP;
                busy = true;
            }
            break;

        case ModbusState::RECV_MBAP:
            if (!client.connected()) {
                state = ModbusState::LISTEN; // If client disconnected, go back to listening
                busy = true;
                break;
            }

            if (client.available()) {
                mbap[mbapReceived++] = client.read(); // Read one byte at a time
            }

            if ( mbapReceived >= mbapLength) {
                pduLength = (mbap[4] << 8 | mbap[5])-1;
                pduReceived = 0; // Reset the PDU received counter
                state = ModbusState::RECV_PDU;
                busy = true;
            }
            break;

        case ModbusState::RECV_PDU:
            if (!client.connected()) {
                state = ModbusState::LISTEN; // If client disconnected, go back to listening
                busy = true;
                break;
            }
            // Read the PDU data
            if (pduReceived < pduLength && client.available()) {
                pdu[pduReceived++] = client.read(); // Read one byte at a time
                busy = true; // Mark as busy since we are receiving data
            }
            // If we have received enough bytes for the PDU, process the request

            if (pduReceived >= pduLength) {
                state = ModbusState::PROCESS_REQUEST;
                busy = true;
            }
            break;

        case ModbusState::PROCESS_REQUEST:

            processRequest(); // Call a function to process the request and prepare the response
            
            state = ModbusState::SENDING_RESPONSE;
            busy = true;
            break;

        case ModbusState::SENDING_RESPONSE:
            if (client.connected()) {
                client.write(sendbuf, sendbufLength);
                // client.stop(); // Close the connection
                Serial.println("Resp sent");
                state = ModbusState::LISTEN; // Go back to listening for new clients
                busy = true;
            }
            break;
    }

    return busy;
}

void Modbus::processRequest() {
    
    //validate the MBAP header
    int tid = (mbap[0] << 8) | mbap[1]; // Transaction ID
    int pid = (mbap[2] << 8) | mbap[3]; // Protocol ID

    if (pid != 0) {
        Serial.println("Inv PID");
        return; // Only Protocol ID 0 is valid for Modbus
    }

    // get data from the MBAP header and PDU
    char unitId = mbap[6]; // Get the Unit ID from the MBAP header
    ModbusFunctionCode functionCode = static_cast<ModbusFunctionCode>(pdu[0]); // Get the function code from the PDU
    
    unsigned char *rqPayload = &pdu[1]; // Pointer to the payload data in the PDU
    int rqPayloadLength = pduLength - 1; // Length of the payload (excluding function code)

    Serial.print("TID=");
    Serial.print(tid, HEX);
    Serial.print(", PID=");
    Serial.print(pid, HEX);
    Serial.print(", UID=");
    Serial.print(unitId, HEX);
    Serial.print(", Code=");
    Serial.print((unsigned char)functionCode, HEX);
    Serial.print(", Len=");
    Serial.println(rqPayloadLength);
    Serial.print("Payload: ");
    for (int i = 0; i < rqPayloadLength; i++) {
        Serial.print(rqPayload[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    sendbuf[0] = mbap[0]; // Transaction ID
    sendbuf[1] = mbap[1];
    sendbuf[2] = 0; // Protocol ID (always 0 for Modbus)
    sendbuf[3] = 0;
    //we dont know the length yet
    sendbuf[6] = unitId;

    const int outputBufLength = sizeof(sendbuf)-mbapLength; // Length of the response payload
    int respPayloadLength = modbusQuery(functionCode, 
        rqPayload, 
        rqPayloadLength, 
        &sendbuf[mbapLength], 
        outputBufLength
    );

    //generate response ILLEGAL FUNCTION

    // Length of the MBAP header (PDU len + 1)
    sendbuf[4] = 0;
    sendbuf[5] = respPayloadLength + 1;

    // Set the length of the response buffer
    sendbufLength = mbapLength + respPayloadLength; // 7 bytes for MBAP + 2 bytes for PDU (function code + exception code)
    
    Serial.print("SendResp TID=");
    Serial.print(sendbuf[0], HEX);
    Serial.print(", PID=");
    Serial.print(sendbuf[2], HEX);
    Serial.print(", UID=");
    Serial.print(sendbuf[6], HEX);
    Serial.print(", FCode=");
    Serial.print(sendbuf[7], HEX);
    Serial.print(", ExcCode=");
    Serial.println(sendbuf[8], HEX);
    

}
int Modbus::modbusQuery(const ModbusFunctionCode &functionCode, 
                unsigned char *rqPayload, 
                const int &rqPayloadLength, 
                unsigned char *outputBuf, 
                const int &maxOutputBufLength) {

    if (rqPayload == nullptr || outputBuf == nullptr || maxOutputBufLength <= 0) {
        return 0; // Invalid parameters
    }

    ModbusExceptionCode exceptionCode = ModbusExceptionCode::SUCCESS;

    int respPayloadLength = 0; // Length of the response payload
    // switch()
    switch(functionCode) {
        case ModbusFunctionCode::READ_INPUT_REGISTERS:
        case ModbusFunctionCode::READ_HOLDING_REGISTERS: {
            if (rqPayloadLength < 2) {
                exceptionCode = ModbusExceptionCode::ILLEGAL_DATA_ADDRESS; // Not enough data
                break;
            }
            unsigned int startAddress = (rqPayload[0] << 8) | rqPayload[1]; // Get the starting address
            unsigned int quantity = (rqPayload[2] << 8) | rqPayload[3]; // Get the quantity of registers

            if (startAddress < 0 || startAddress >= sizeof(exampleRegisterTable) / sizeof(exampleRegisterTable[0]) ||
                quantity <= 0 || startAddress + quantity > sizeof(exampleRegisterTable) / sizeof(exampleRegisterTable[0])) {
                exceptionCode = ModbusExceptionCode::ILLEGAL_DATA_ADDRESS; // Invalid address or quantity
                break;
            }
            // Prepare the response
            outputBuf[1] = quantity * 2; // Number of bytes to follow
            respPayloadLength =  2 + quantity * 2; // Return the length of the response payload

            if (respPayloadLength > maxOutputBufLength) {
                //the buffer is too small to hold the response
                Serial.println("Buffer too small");
                exceptionCode = ModbusExceptionCode::SLAVE_DEVICE_FAILURE;
                break;
            }

            for (unsigned int i = 0; i < quantity; i++) {
                outputBuf[2 + i * 2] = (exampleRegisterTable[startAddress + i] >> 8) & 0xFF; // High byte
                outputBuf[3 + i * 2] = exampleRegisterTable[startAddress + i] & 0xFF; // Low byte
            }
            
            break;
        }
        default: {
            // Handle other function codes or set an exception
            exceptionCode = ModbusExceptionCode::ILLEGAL_FUNCTION; // Unsupported function code
            break;
        }
    }

    outputBuf[0] = static_cast<unsigned char>(functionCode); // Set the function code in the response

    if (exceptionCode != ModbusExceptionCode::SUCCESS) {
        // If there was an exception, set the exception code in the response
        outputBuf[0] |= 0x80; // Set the exception flag
        outputBuf[1] = static_cast<unsigned char>(exceptionCode); // Set the exception code
        respPayloadLength = 2; // Length of the response payload
    }

    return respPayloadLength; // Return the length of the response payload
}