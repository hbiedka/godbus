#include "modbus.h"

bool Modbus::spin() {
    bool busy = false;

    switch (state) {
        case ModbusState::NOT_STARTED:
            server.begin();
            state = ModbusState::LISTEN;
            busy = true;
            break;

        case ModbusState::LISTEN:
            client = server.available();
            if (client) {
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
                state = ModbusState::LISTEN; // Go back to listening for new clients
                busy = true;
            }
            break;
    }

    return busy;
}

void Modbus::processRequest() {
    
    //validate the MBAP header
    int pid = (mbap[2] << 8) | mbap[3]; // Protocol ID
    if (pid != 0) {
        return; // Only Protocol ID 0 is valid for Modbus
    }

    // get data from the MBAP header and PDU
    char unitId = mbap[6]; // Get the Unit ID from the MBAP header
    ModbusFunctionCode functionCode = static_cast<ModbusFunctionCode>(pdu[0]); // Get the function code from the PDU
    
    unsigned char *rqPayload = &pdu[1]; // Pointer to the payload data in the PDU
    int rqPayloadLength = pduLength - 1; // Length of the payload (excluding function code)

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

    // Length of the MBAP header (PDU len + 1)
    sendbuf[4] = 0;
    sendbuf[5] = respPayloadLength + 1;

    // Set the length of the response buffer
    sendbufLength = mbapLength + respPayloadLength; // 7 bytes for MBAP + 2 bytes for PDU (function code + exception code)
    
}
int Modbus::modbusQuery(const ModbusFunctionCode &functionCode, 
                unsigned char *rqPayload, 
                const int &rqPayloadLength, 
                unsigned char *outputBuf, 
                const unsigned int &maxOutputBufLength) {

    if (rqPayload == nullptr || outputBuf == nullptr || maxOutputBufLength <= 0) {
        return 0; // Invalid parameters
    }

    ModbusExceptionCode exceptionCode = ModbusExceptionCode::SUCCESS;

    unsigned int respPayloadLength = 0; // Length of the response payload
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

            // Prepare the response
            outputBuf[1] = quantity * 2; // Number of bytes to follow
            respPayloadLength =  2 + quantity * 2; // Return the length of the response payload

            if (respPayloadLength > maxOutputBufLength) {
                //the buffer is too small to hold the response
                exceptionCode = ModbusExceptionCode::SLAVE_DEVICE_FAILURE;
                break;
            }

            exceptionCode = getRegisters(startAddress, quantity, &outputBuf[2], maxOutputBufLength - 2);
            
            break;
        }
        case ModbusFunctionCode::READ_COILS:
        case ModbusFunctionCode::READ_DISCRETE_INPUTS: {
            if (rqPayloadLength < 2) {
                exceptionCode = ModbusExceptionCode::ILLEGAL_DATA_ADDRESS; // Not enough data
                break;
            }
            unsigned int startAddress = (rqPayload[0] << 8) | rqPayload[1]; // Get the starting address
            unsigned int quantity = (rqPayload[2] << 8) | rqPayload[3]; // Get the quantity of registers

            // Prepare the response
            unsigned int numBytes = quantity >> 3; // div by 8 without residue, but faster
            if (quantity % 8 > 0) numBytes++;

            outputBuf[1] = numBytes; // Number of bytes to follow
            respPayloadLength = 2 + numBytes; // Return the length of the response payload
            
            if (respPayloadLength > maxOutputBufLength) {
                // The buffer is too small to hold the response
                exceptionCode = ModbusExceptionCode::SLAVE_DEVICE_FAILURE;
                break;
            }

            // Call a function to get the coils or discrete inputs
            exceptionCode = getDiscreteInputs(startAddress, quantity, &outputBuf[2], maxOutputBufLength - 2);
            break;
        }
        case ModbusFunctionCode::WRITE_SINGLE_COIL: {
            if (rqPayloadLength < 4) {
                exceptionCode = ModbusExceptionCode::ILLEGAL_DATA_ADDRESS; // Not enough data
                break;
            }
            unsigned int address = (rqPayload[0] << 8) | rqPayload[1]; // Get the address

            bool value = false; // Initialize the value to be written
            if (rqPayload[2] == 0xFF && rqPayload[3] == 0x00) {
                value = true;
            } else if (rqPayload[2] == 0x00 && rqPayload[3] == 0x00) {
                value = false;
            } else {
                // If the value is not 0xFF00 or 0x0000, it's an invalid value
                exceptionCode = ModbusExceptionCode::ILLEGAL_DATA_VALUE; // Invalid value
                break;
            }

            respPayloadLength = 5; // Length of the response payload
            outputBuf[1] = rqPayload[0]; // Echo back the address
            outputBuf[2] = rqPayload[1];
            outputBuf[3] = rqPayload[2];
            outputBuf[4] = rqPayload[3];

            exceptionCode = writeSingleCoil(address,value);

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

ModbusExceptionCode Modbus::writeSingleCoil(unsigned int address, bool value) {
    
    setterOutput setterOut = setterOutput::OK; // Initialize the setter output

    // Find the corresponding ModbusNode for the address
    for (ModbusNode *node = registerTable; node->dev != nullptr; ++node) {
        if (node->startAddress == address && node->type == setValueType::BOOL) {
            setValue val;
            val.b = value;
            setterOut = node->dev->set(val); // Set the value in the device
            
            // handle the output of the setter
            if (setterOut == setterOutput::NOT_SUPPORTED) {
                return ModbusExceptionCode::ILLEGAL_FUNCTION; // Function not supported
            } else if (setterOut == setterOutput::READ_ONLY) {
                return ModbusExceptionCode::ILLEGAL_DATA_VALUE; // Value is read-only
            } else if (setterOut == setterOutput::INVALID_VALUE) {
                return ModbusExceptionCode::ILLEGAL_DATA_VALUE; // Invalid value
            } else if (setterOut != setterOutput::OK) {
                return ModbusExceptionCode::SLAVE_DEVICE_FAILURE; // Device failed to set the value
            }

            //OK
            return ModbusExceptionCode::SUCCESS;
        }
    }
    
    //if match address not found
    return ModbusExceptionCode::ILLEGAL_DATA_ADDRESS; // Address not found
                                
}

ModbusExceptionCode Modbus::getDiscreteInputs(unsigned int startAddress, 
                                          unsigned int quantity, 
                                          unsigned char *outputBuf, 
                                          const unsigned int &maxOutputBufLength) {
    if (registerTable == nullptr || outputBuf == nullptr || maxOutputBufLength <= 0) {
        return ModbusExceptionCode::SLAVE_DEVICE_FAILURE; // Invalid parameters
    }

    // Prepare the response
    unsigned int addr = startAddress;
    unsigned int endAddress = startAddress + quantity;
    unsigned int byteIndex = 0; // Index for the output buffer byte
    unsigned char bitIndex = 0; // Index for the bit within the byte
    bool bitValue = false; // Value of the bit to be written

    //reset output buffer
    unsigned int numBytes = quantity >> 3; // div by 8 without residue, but faster
    if (quantity % 8 > 0) numBytes++;
    for (unsigned int i = 0; i < numBytes; i++)
        outputBuf[i] = 0;

    bool found = false; // Flag to check if the address was found in the register table
    while (addr < endAddress) {
        found = false; // Reset the found flag for each address

        // Find the corresponding ModbusNode for the address
        for (ModbusNode *node = registerTable; node->dev != nullptr; ++node) {
            if (node->startAddress == addr && node->type == setValueType::BOOL) {

                // Check if the output buffer has enough space
                if ((addr - startAddress) / 8 + 1 >= maxOutputBufLength) {
                    return ModbusExceptionCode::SLAVE_DEVICE_FAILURE; // Buffer too small
                }

                setValue value; // Initialize the value to be written
                node->dev->get(value); // Get the current value from the device

                // Write the value to the output buffer
                switch (node->type) {
                    case setValueType::BOOL:
                        bitValue = value.b; // Get the boolean value
                        break;
                    default:
                        return ModbusExceptionCode::ILLEGAL_DATA_VALUE; // Unsupported type
                }

                // Set the bit in the output buffer
                // first register is at LSB of the first byte
                outputBuf[byteIndex] |= ((bitValue ? 1 : 0) << bitIndex);
                bitIndex++; // Move to the next bit
                if (bitIndex >= 8) { // If we have filled the byte, move to the next byte
                    byteIndex++;
                    bitIndex = 0; // Reset the bit index
                    if (byteIndex >= maxOutputBufLength) {
                        return ModbusExceptionCode::SLAVE_DEVICE_FAILURE; // Buffer too small
                    }
                }

                addr++; // Move to the next address
                found = true; // Address found in the register table
                break; // Move to the next address
            }
        }

        // If the address was not found in the register table, continue to the next address
        if (!found) {
            return ModbusExceptionCode::ILLEGAL_DATA_ADDRESS; // Address not found
        }
    }
    return ModbusExceptionCode::SUCCESS; // Success
}

ModbusExceptionCode Modbus::getRegisters(unsigned int startAddress, 
                                          unsigned int quantity, 
                                          unsigned char *outputBuf, 
                                          const unsigned int &maxOutputBufLength) {
    if (registerTable == nullptr || outputBuf == nullptr || maxOutputBufLength <= 0) {
        return ModbusExceptionCode::SLAVE_DEVICE_FAILURE; // Invalid parameters
    }

    // Prepare the response
    unsigned int addr = startAddress;
    unsigned int endAddress = startAddress + quantity;

    int regValue = 0; // Initialize the register value
    setValue value; // Initialize the value to be written

    bool found = false; // Flag to check if the address was found in the register table
    while (addr < endAddress) {
        found = false; // Reset the found flag for each address

        // Find the corresponding ModbusNode for the address
        for (ModbusNode *node = registerTable; node->dev != nullptr; ++node) {
            if (node->startAddress == addr &&
                (node->type == setValueType::INT || node->type == setValueType::FLOAT)
            ) {

                // Check if the output buffer has enough space
                if ((addr - startAddress) * 2 + 1 >= maxOutputBufLength) {
                    return ModbusExceptionCode::SLAVE_DEVICE_FAILURE; // Buffer too small
                }

                node->dev->get(value); // Get the current value from the device

                // Write the value to the output buffer
                switch (node->type) {
                    case setValueType::INT:
                        regValue = value.i * node->multiplier; // Get the integer value
                        break;
                    case setValueType::FLOAT: {
                        regValue = static_cast<unsigned int>(value.f * node->multiplier);
                        break;
                    }
                    default:
                        return ModbusExceptionCode::ILLEGAL_DATA_VALUE; // Unsupported type
                }

                // Write the register value to the output buffer
                unsigned int offset = (addr - startAddress) * 2; // Calculate the offset in the output buffer
                if (offset + 1 >= maxOutputBufLength) {
                    return ModbusExceptionCode::SLAVE_DEVICE_FAILURE; // Buffer too small
                }
                outputBuf[offset] = (regValue >> 8) & 0xFF; // High byte
                outputBuf[offset + 1] = regValue & 0xFF; // Low byte
                
                //TODO support muliple registers
                addr++;

                found = true; // Address found in the register table
                break; // Move to the next address
            }
        }

        // If the address was not found in the register table, continue to the next address
        if (!found) {
            return ModbusExceptionCode::ILLEGAL_DATA_ADDRESS; // Address not found
        }
    }
    return ModbusExceptionCode::SUCCESS; // Success
}