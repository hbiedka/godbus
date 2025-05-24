#include "http.h"

void Http::setResponse(const String& resp) {
    response = resp;
}

bool Http::spin() {
    bool busy = false;

    switch(state) {
        case HttpState::NOT_STARTED:
            // Start the server
            if (Ethernet.hardwareStatus() != EthernetNoHardware) {
                Serial.println("Ethernet shield found.");
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
                Serial.println("Client disconnected");
                state = HttpState::LISTEN;
            } else {
                while (client.available()) {
                    busy = true;
                    char c = client.read();
                    if (c == '\r') {
                        // End of request
                        Serial.println("End of request");
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
            Serial.println("Flushed client");
            state = HttpState::SENDING_RESPONSE;
            break;
        case HttpState::SENDING_RESPONSE:
            busy = true;
            // Prepare the response
            prepareResponse();
            
            // Send a simple HTTP response
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            client.println(response);
            Serial.println("Response sent");
            // Close the connection
            client.stop();
            state = HttpState::LISTEN;
            break;
            
    }
    return busy;
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
    Serial.println("Prepared response: " + response);
}