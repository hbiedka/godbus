#ifndef HTTP_H
#define HTTP_H

#include <Arduino.h>
#include <Ethernet.h>

#include "device/device.h"

enum class HttpState {
    NOT_STARTED,
    LISTEN,
    RECV_REQUEST,
    FLUSHING,
    SENDING_RESPONSE,
};

class Http {
private:
    EthernetServer server;
    Device** devices = nullptr; // Array to hold device pointers, adjust size as needed
    EthernetClient client;
    HttpState state = HttpState::NOT_STARTED;
    int statuscode = 200; // Default status code
    String request;
    String response;
    void processRequest();
    void prepareResponse();
public:
    Http(Device** _devices) :
        server(80),  // Initialize the Ethernet server on port 80
        devices(_devices)
    {}

    void setResponse(const String& resp);
    bool spin();
};

#endif  // HTTP_H