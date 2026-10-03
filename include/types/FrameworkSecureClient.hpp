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
/** @brief Attach the default root CA certificate bundle for TLS validation. */
void attach_ssl_certificate_bundle(sslclient_context *ssl_client, bool att);

extern const uint8_t rootca_crt_bundle_start[];

struct FrameworkSecureClient : public WiFiClientSecure {
    /** @brief Attach the default root CA certificate bundle. */
    void useDefaultCertBundle() {
        attach_ssl_certificate_bundle(sslclient.get(), true);
        _use_ca_bundle = true;
    }

    /** @brief Apply deadline token timeouts to connection and handshake. */
    void applyDeadline(const DeadlineToken& d) {
        uint32_t connectMs = std::min(d.remainingMs(), FetchContext::CONNECT_TIMEOUT_MS);
        if (connectMs == 0) connectMs = 1;  // never pass 0 — means "no timeout"
        setConnectionTimeout(connectMs);
        // setHandshakeTimeout takes seconds — round up so we don't truncate below 1s
        uint32_t handshakeSec = std::max((uint32_t)1, (connectMs + 999) / 1000);
        setHandshakeTimeout(handshakeSec);
    }
};

#else

#include <WiFiClientSecure.h>
#include "types/FetchContext.hpp"

/**
 * @brief Native build mock for FrameworkSecureClient.
 */
struct FrameworkSecureClient : public WiFiClientSecure {
    /** @brief Mock for default cert bundle attachment on native. */
    void useDefaultCertBundle() {}

    /** @brief Mock for deadline application on native. */
    void applyDeadline(const DeadlineToken& d) {}

    /** @brief Mock for last error retrieval on native. */
    int lastError(char* buf, size_t size) { return 0; }
};

#endif
