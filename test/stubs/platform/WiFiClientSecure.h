#pragma once
#include "WiFiClient.h"
#include "HTTPClient.h"  // for sslclient_context
#include <memory>

class WiFiClientSecure : public WiFiClient {
public:
    WiFiClientSecure() = default;
    virtual ~WiFiClientSecure() = default;
    void setCACert(const char* rootCA) {}
    void setInsecure() {}
    // Members accessed by WeatherModule's SecureClient subclass
    std::shared_ptr<sslclient_context> sslclient;
    bool _use_ca_bundle = false;
};
