/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test stub for ESPmDNS.h in native unit tests.
 */

#pragma once

#include "Arduino.h"
#include <cstdint>
#include <string>

class MDNSResponder {
public:
    bool began = false;
    std::string hostname;
    std::string service;
    std::string proto;
    uint16_t port = 0;

    bool begin(const char* hostName) {
        began = true;
        hostname = hostName ? hostName : "";
        return true;
    }
    bool begin(const String& hostName) {
        return begin(hostName.c_str());
    }
    void end() {
        began = false;
        hostname.clear();
        service.clear();
        proto.clear();
        port = 0;
    }
    bool addService(const char* s, const char* p, uint16_t pt) {
        service = s ? s : "";
        proto = p ? p : "";
        port = pt;
        return true;
    }
    bool addService(const String& s, const String& p, uint16_t pt) {
        return addService(s.c_str(), p.c_str(), pt);
    }
};

inline MDNSResponder MDNS;
