#ifndef HTTP_H
#define HTTP_H

#include <Arduino.h>
#include <Ethernet.h>

#include "device/device.h"

#define MAX_DEV_DATA_LEN 16
#define MAX_REQUEST_SIZE 64
#define MAX_RESPONSE_SIZE 256

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
    char request[MAX_REQUEST_SIZE];
    unsigned char rqLen = 0;
    char response[MAX_RESPONSE_SIZE]; // Buffer for the response;
    void processRequest();
    void prepareResponse();
public:
    Http(Device** _devices) :
        server(80),  // Initialize the Ethernet server on port 80
        devices(_devices)
    {}

    bool spin();
};

#endif  // HTTP_H