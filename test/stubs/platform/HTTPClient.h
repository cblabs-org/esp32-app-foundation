#pragma once
#include "Arduino.h"
#include <sstream>

class WiFiClient {};

class WiFiClientSecure : public WiFiClient {
public:
    void setInsecure() {}
    void setCACert(const char* rootCA) {}
};

class HTTPClient {
public:
    bool begin(WiFiClient& client, const char* url) { return true; }
    int GET() { return 200; }
    std::istream& getStream() { return _stream; }
    void end() {}
private:
    std::stringstream _stream;
};
