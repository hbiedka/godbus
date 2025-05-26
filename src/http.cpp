#include "http.h"

//static
String decodeStatusCode(int code) {
    switch(code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}

void Http::setResponse(const String& resp) {
    response = resp;
}

bool Http::spin() {
    bool busy = false;

    switch(state) {
        case HttpState::NOT_STARTED:
            // Start the server
            if (Ethernet.hardwareStatus() != EthernetNoHardware) {
                server.begin();
                Serial.println("Server started");
                state = HttpState::LISTEN;
                busy = true;
            }
            break;

        case HttpState::LISTEN:
            // Check for incoming clients
            client = server.available();
            if (client) {
                Serial.println("New client");
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
                        Serial.println(request);
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
            client.println("HTTP/1.1 " + String(statuscode) + " " + decodeStatusCode(statuscode));
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
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
    response = "{}";
    
    //get URL from request
    int start = request.indexOf("GET ") + 4; // 5 is the length of "GET /"
    //read url until space or end of line
    int end = request.indexOf(' ', start);
    if (end == -1) {
        end = request.indexOf('\n', start);
    }

    if (start == -1 || end == -1) {
        // response = "{\"status\": \"error\", \"message\": \"Invalid request format\"}";
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
            // response = "{\"status\": \"error\", \"message\": \"Invalid URL format\"}";
            statuscode = 400; // Bad Request
            return;
        }
        String deviceId = url.substring(1, deviceIdEnd); // Extract the device ID
        String state = url.substring(deviceIdEnd + 1); // Extract the state

        // Find the device by ID
        for (Device** dev = devices; *dev != nullptr; ++dev) {
            if ((*dev)->getName() == deviceId) {
                // Device found, set the state
                setterOutput result = (*dev)->deserialize(state);

                if (result == setterOutput::OK) {
                    response = "{\"status\": \"OK\"}";
                    statuscode = 200; // OK
                } else {
                    // response = "{\"status\": \"error\", \"message\": \"Failed to set state\", \"code\": " + String(static_cast<int>(result)) + "}";
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
    response = "{";
    for (Device** dev = devices; *dev != nullptr; ++dev) {
        if (response.length() > 1) {
            response += ", ";
        }
        response += "\"" + String((*dev)->getName()) + "\": \"" + String((*dev)->serialize()) + "\"";
    }
    response += "}";
}