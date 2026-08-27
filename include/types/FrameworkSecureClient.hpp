#pragma once

#ifdef ESP_PLATFORM

#include <WiFiClientSecure.h>
#include <esp_crt_bundle.h>
#include "types/FetchContext.hpp"
#include <algorithm>

// Forward-declare the Arduino ESP32 ssl_client function we need.
struct sslclient_context;
void attach_ssl_certificate_bundle(sslclient_context *ssl_client, bool att);

extern const uint8_t rootca_crt_bundle_start[];

struct FrameworkSecureClient : public WiFiClientSecure {
    void useDefaultCertBundle() {
        attach_ssl_certificate_bundle(sslclient.get(), true);
        _use_ca_bundle = true;
    }

    void applyDeadline(const DeadlineToken& d) {
        uint32_t remainSec = std::max((uint32_t)1, (uint32_t)(d.remainingMs() / 1000));
        uint32_t connectSec = std::min(remainSec, FetchContext::CONNECT_TIMEOUT_MS / 1000);
        setTimeout(connectSec);
        setHandshakeTimeout(connectSec);
    }
};

#endif
