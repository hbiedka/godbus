#ifndef HTTP_H
#define HTTP_H

#include <Arduino.h>
#include <Ethernet.h>

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
    EthernetClient client;
    HttpState state = HttpState::NOT_STARTED;
    String request;
    String response;
public:
    Http() :
        server(80)  // Initialize the Ethernet server on port 80
    {}

    void setResponse(const String& resp);
    bool spin();
};

#endif  // HTTP_H