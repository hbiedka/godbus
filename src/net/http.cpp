#include "http.h"

bool Http::spin() {
    bool busy = false;

    switch(state) {
        case HttpState::NOT_STARTED:
            // Start the server
            if (Ethernet.hardwareStatus() != EthernetNoHardware) {
                server.begin();
                state = HttpState::LISTEN;
                busy = true;
            }
            break;

        case HttpState::LISTEN:
            // Check for incoming clients
            client = server.available();
            if (client) {
                rqLen = 0;
                statuscode = 200;
                state = HttpState::RECV_REQUEST;
            }
            break;

        case HttpState::RECV_REQUEST:
            if (!client.connected()) {
                state = HttpState::LISTEN;
            } else {
                while (client.available()) {
                    busy = true;
                    request[rqLen] = client.read();
                    if (request[rqLen] == '\r') {
                        // End of request
                        // Send response
                        request[rqLen] = '\0';
                        state = HttpState::FLUSHING;
                        break;
                    }
                    rqLen++;
                    if (rqLen >= MAX_REQUEST_SIZE) {
                        statuscode = 500; //TODO "rq too long statuscode"
                        state = HttpState::FLUSHING;
                    }
                }
            }
            break;
        case HttpState::FLUSHING:
            // Flush the client, this operation may block
            // for a while if the client is slow
            busy = true;

            client.flush();
            state = HttpState::SENDING_RESPONSE;
            break;
        case HttpState::SENDING_RESPONSE:
            busy = true;
            // Prepare the response
            processRequest();
            
            // Send a simple HTTP response
            client.print(F("HTTP/1.1 "));
            client.print(statuscode);
            client.print(F(" "));
            
            if (statuscode == 200) {
                client.print(F("OK"));
            } else if (statuscode == 400) {
                client.print(F("Bad Request"));
            } else if (statuscode == 404) {
                client.print(F("Not Found"));
            } else if (statuscode == 500) {
                client.print(F("Internal Server Error"));
            } else {
                client.print(F("Unknown Status"));
            }
            client.println();

            client.print(F("Content-Type: application/json\r\nConnection: close\r\n\r\n"));
            client.println(response);

            // Close the connection
            client.stop();
            state = HttpState::LISTEN;
            break;
            
    }
    return busy;
}

void Http::processRequest() {
    // Process the request here
    snprintf(response, MAX_RESPONSE_SIZE, "{}");

    if (statuscode != 200) {
        //there was an error during receiving request eg, request too long
        return;
    }

    if (strncmp(request,"GET ",4) != 0) {
        statuscode = 400;   // Bad request
        return;
    }

    char *url = &request[4];

    for (int i = 0; i < (MAX_REQUEST_SIZE-4); i++ ) {
        if (url[i] == ' ' || url[i] == '\r' || url[i] == '\n' || url[i] == '\0') {
            url[i] = '\0';
        }
    }
    
    // Check if the URL is valid and process it
    if (strncmp(url,"/",1) == 0 && strlen(url) == 1) {
        // Prepare the response with all devices
        prepareResponse();
        return;
    } else if (strncmp(url,"/",1) == 0) {
        char *deviceId = &url[1];
        char *state = nullptr;

        for (int i = 1; url[i] != '\0'; i++) {
            if (url[i] == '/') {
                url[i] = '\0';
                state = &url[i+1];
                break;
            } else if (url[i] == ' ' || url[i] == ' ' || url[i] == '\r' || url[i] == '\n') {
                break;
            }
        }

        if (state == nullptr) {
            // Serial.println("state not found");
            statuscode = 400;  //Bad request
            return;
        }

        // Find the device by ID
        for (Device** dev = devices; *dev != nullptr; ++dev) {
            if (strncmp((*dev)->getName(), deviceId, MAX_NAME_SIZE) == 0) {
                // Device found, set the state

                //TODO convert deserialize to const char
                setterOutput result = (*dev)->deserialize(state,MAX_DEV_DATA_LEN);

                if (result == setterOutput::OK) {
                    snprintf(response, MAX_RESPONSE_SIZE, "{\"status\": \"OK\"}");
                    statuscode = 200; // OK
                } else {
                    statuscode = 500; // Internal Server Error
                }
                return;
            }
        }
        // Device not found
        statuscode = 404; // Not Found
    }
}

void Http::prepareResponse() {
    size_t responseLength = 0;
    bool first = true;

    // Prepare the response with all devices
    responseLength += snprintf(response, MAX_RESPONSE_SIZE, "{");
    
    for (Device** dev = devices; *dev != nullptr; ++dev) {
        if (!first) {
            responseLength += snprintf(&response[responseLength], 
                MAX_RESPONSE_SIZE - responseLength, 
                ", "
            );
        }
        first = false;

        // Serialize the device and add it to the response
        const char* deviceName = (*dev)->getName();
        // String deviceData = (*dev)->serialize();
        char deviceData[MAX_DEV_DATA_LEN];
        (*dev)->serialize(deviceData,MAX_DEV_DATA_LEN);        
        
        responseLength += snprintf(&response[responseLength], 
            MAX_RESPONSE_SIZE - responseLength,
            "\"%s\": \"%s\"", 
            deviceName, 
            deviceData
        );
    }
    responseLength += snprintf(&response[responseLength], 
        MAX_RESPONSE_SIZE - responseLength, 
        "}"
    );
}