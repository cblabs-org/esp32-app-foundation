/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/types/FrameworkSecureClient.hpp
 * Description: FrameworkSecureClient implementation.
 *
 */

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
        uint32_t connectMs = std::min(d.remainingMs(), FetchContext::CONNECT_TIMEOUT_MS);
        if (connectMs == 0) connectMs = 1;  // never pass 0 — means "no timeout"
        setConnectionTimeout(connectMs);
        // setHandshakeTimeout takes seconds — round up so we don't truncate below 1s
        uint32_t handshakeSec = std::max((uint32_t)1, (connectMs + 999) / 1000);
        setHandshakeTimeout(handshakeSec);
    }
};

#endif
