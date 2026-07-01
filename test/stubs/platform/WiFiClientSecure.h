#pragma once
#include "WiFiClient.h"

class WiFiClientSecure : public WiFiClient {
public:
    WiFiClientSecure() = default;
    virtual ~WiFiClientSecure() = default;
    void setCACert(const char* rootCA) {}
    void setInsecure() {}
};
