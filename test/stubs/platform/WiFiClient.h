#pragma once
#include "Stream.h"

class NetworkClient : public Stream {
public:
    virtual ~NetworkClient() = default;
    int available() override { return 1024; }
    virtual size_t readBytes(uint8_t *buffer, size_t length) { return length; }
    virtual void setTimeout(int timeout) {}

    virtual size_t readBytes(char *buffer, size_t length) { return length; }
};


typedef NetworkClient WiFiClient;
