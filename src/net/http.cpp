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
                request = "";
                state = HttpState::RECV_REQUEST;
            }
            break;

        case HttpState::RECV_REQUEST:
            if (!client.connected()) {
                state = HttpState::LISTEN;
            } else {
                while (client.available()) {
                    busy = true;
                    char c = client.read();
                    if (c == '\r') {
                        // End of request
                        // Send response
                        state = HttpState::FLUSHING;
                        break;
                    }
                    request += c;
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
    
    //get URL from request
    int start = request.indexOf("GET ") + 4; // 5 is the length of "GET /"
    //read url until space or end of line
    int end = request.indexOf(' ', start);
    if (end == -1) {
        end = request.indexOf('\n', start);
    }

    if (start == -1 || end == -1) {
        statuscode = 400; // Bad Request
        return;
    }
    
    String url = request.substring(start, end);
    
    // Check if the URL is valid and process it
    if (url == "/") {
        // Prepare the response with all devices
        prepareResponse();
        return;
    } else {
        //process url /device_id/state
        int deviceIdEnd = url.indexOf('/', 1); // Find the first '/' after the initial '/'
        if (deviceIdEnd == -1) {
            statuscode = 400; // Bad Request
            return;
        }
        String deviceId = url.substring(1, deviceIdEnd); // Extract the device ID
        String state = url.substring(deviceIdEnd + 1); // Extract the state

        // Find the device by ID
        for (Device** dev = devices; *dev != nullptr; ++dev) {
            if (strncmp((*dev)->getName(), deviceId.c_str(), MAX_NAME_SIZE) == 0) {
                // Device found, set the state
                setterOutput result = (*dev)->deserialize(state);

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
        String deviceData = (*dev)->serialize();
        
        responseLength += snprintf(&response[responseLength], 
            MAX_RESPONSE_SIZE - responseLength,
            "\"%s\": \"%s\"", 
            deviceName, 
            deviceData.c_str()
        );
    }
    responseLength += snprintf(&response[responseLength], 
        MAX_RESPONSE_SIZE - responseLength, 
        "}"
    );
}