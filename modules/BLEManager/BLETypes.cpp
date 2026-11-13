/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLETypes.cpp
 * Description: Implementation of BLE value types, address formatting, parsing,
 *              and parameter safety clamping. Authoritative: RFC #225 rev 2 §4.
 *
 * Exported Functions/Methods:
 * - BLEPeerAddress::isValid(): Validates non-zero address with known type
 * - BLEPeerAddress::operator==(): Compares address bytes and type
 * - BLEPeerAddress::toString(): Formats full MAC address string
 * - BLEPeerAddress::toRedactedString(): Formats privacy-safe redacted MAC string
 * - BLEPeerAddress::parse(): Parses colon/hyphen MAC string
 * - clampScanParams(): Clamps scan window to <= 50% of scan interval
 */

#include "BLETypes.hpp"
#include <cstdio>
#include <cstring>
#include "esp_log.h"

static const char* TAG = "BLETypes";

bool BLEPeerAddress::isValid() const {
    if (type == Type::Unknown) {
        return false;
    }
    for (size_t i = 0; i < 6; ++i) {
        if (bytes[i] != 0) {
            return true;
        }
    }
    return false;
}

bool BLEPeerAddress::operator==(const BLEPeerAddress& other) const {
    return type == other.type && (std::memcmp(bytes, other.bytes, 6) == 0);
}

void BLEPeerAddress::toString(char* buf, size_t len) const {
    if (!buf || len < 18) {
        if (buf && len > 0) buf[0] = '\0';
        return;
    }
    // Formatted MSB first (bytes[5] down to bytes[0])
    std::snprintf(buf, len, "%02X:%02X:%02X:%02X:%02X:%02X",
                  bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]);
}

void BLEPeerAddress::toRedactedString(char* buf, size_t len) const {
    if (!buf || len < 18) {
        if (buf && len > 0) buf[0] = '\0';
        return;
    }
    // Redacted format masks first 4 octets, showing last two octets (bytes[1] and bytes[0])
    std::snprintf(buf, len, "**:**:**:**:%02X:%02X", bytes[1], bytes[0]);
}

bool BLEPeerAddress::parse(const char* str, Type addrType, BLEPeerAddress& out) {
    if (!str) {
        return false;
    }
    unsigned int b[6] = {};
    int count = std::sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
                            &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]);
    if (count != 6) {
        count = std::sscanf(str, "%02x-%02x-%02x-%02x-%02x-%02x",
                            &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]);
    }
    if (count != 6) {
        return false;
    }

    for (size_t i = 0; i < 6; ++i) {
        out.bytes[i] = static_cast<uint8_t>(b[i]);
    }
    out.type = addrType;
    return true;
}

void clampScanParams(uint16_t& intervalMs, uint16_t& windowMs, bool hasPeripheralConnections) {
    if (intervalMs < 10) {
        intervalMs = 10;
    }
    uint16_t maxWindow = hasPeripheralConnections ? (intervalMs / 4) : (intervalMs / 2);
    if (windowMs > maxWindow) {
        ESP_LOGW(TAG, "Scan window (%u ms) > %u%% of interval (%u ms); clamping to %u ms",
                 windowMs, hasPeripheralConnections ? 25 : 50, intervalMs, maxWindow);
        windowMs = maxWindow;
    }
}

void clampScanParams(BLEScanParams& params, bool hasPeripheralConnections) {
    clampScanParams(params.intervalMs, params.windowMs, hasPeripheralConnections);
}

