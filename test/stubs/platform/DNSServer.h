#pragma once

class IPAddress;

class DNSServer {
public:
    void start(uint16_t port, const char* domainName, const IPAddress& resolvedIP) {}
    void stop() {}
    void processNextRequest() {}
};
