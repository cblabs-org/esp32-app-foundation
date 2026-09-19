/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/ble/SampleHrClient.cpp
 * Description: Implementation of Sample HR Client provider.
 */

#include "SampleHrClient.hpp"
#include "SampleHrRepeater.hpp"
#include "esp_log.h"
#include "Arduino.h"

static const char* TAG = "SampleHrClient";

size_t SampleHrClient::getTargetServiceUUIDs(uint16_t* out, size_t max) const {
    if (max >= 1) {
        out[0] = 0x180D;
        return 1;
    }
    return 0;
}

size_t SampleHrClient::getSubscriptions(BLESubscriptionSpec* out, size_t max) const {
    if (max >= 1) {
        out[0].serviceUuid16 = 0x180D;
        out[0].charUuid16 = 0x2A37;
        out[0].indicate = false;
        out[0].required = true;
        return 1;
    }
    return 0;
}

void SampleHrClient::onConnected(const BLEStoredDevice& /*device*/) {
    _connected = true;
    _lastHrMs = millis();
    ESP_LOGI(TAG, "HR Client connected to peer.");
}

void SampleHrClient::onNotification(const BLENotification& notification) {
    if (notification.serviceUuid16 == 0x180D && notification.charUuid16 == 0x2A37 && notification.length > 0) {
        uint8_t flags = notification.data[0];
        uint16_t bpm = 0;
        if (flags & 0x01) {
            if (notification.length >= 3) {
                bpm = notification.data[1] | (notification.data[2] << 8);
            }
        } else {
            if (notification.length >= 2) {
                bpm = notification.data[1];
            }
        }

        uint32_t now = millis();
        uint32_t latency = now - notification.receivedAtMs;
        ESP_LOGD(TAG, "HR=%u BPM, relay_latency=%lu ms", bpm, latency);

        if (_repeater && bpm > 0 && bpm <= 255) {
            _repeater->publishHr(static_cast<uint8_t>(bpm));
        }

        _lastHrMs = now;
    }
}

void SampleHrClient::onDisconnected(BLEDisconnectReason reason) {
    _connected = false;
    ESP_LOGI(TAG, "HR Client disconnected, reason=%u", static_cast<uint8_t>(reason));
    if (_repeater) {
        _repeater->publishHr(0);
    }
}

void SampleHrClient::tick(uint32_t currentMillis) {
    if (_connected) {
        if (currentMillis - _lastHrMs > 10000) { // 10s stale detection
            ESP_LOGW(TAG, "HR data stale (no update for 10s)");
            if (_repeater) {
                _repeater->publishHr(0);
            }
            _lastHrMs = currentMillis; // prevent log flood
        }
    }
}
