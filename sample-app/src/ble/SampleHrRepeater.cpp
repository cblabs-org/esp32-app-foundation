/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/ble/SampleHrRepeater.cpp
 * Description: Sample Heart Rate Service repeater implementation.
 *              Authoritative: RFC #225 rev 2 §7.4, WORKORDER WO-2.
 */

#include "SampleHrRepeater.hpp"

#if defined(ESP_PLATFORM) && defined(WITH_BLE)
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLECharacteristic.h>
#include "esp_log.h"
#endif

static const char* TAG = "SampleHr";

bool SampleHrRepeater::registerServices(NimBLEServer& server) {
#if defined(ESP_PLATFORM) && defined(WITH_BLE)
    ESP_LOGI(TAG, "Registering Heart Rate Service (0x180D)...");

    // Heart Rate Service (0x180D)
    NimBLEService* pHrs = server.createService(NimBLEUUID((uint16_t)0x180D));
    if (!pHrs) {
        ESP_LOGE(TAG, "Failed to create Heart Rate service");
        return false;
    }

    // Heart Rate Measurement characteristic: 0x2A37 (Notify)
    _pHrChar = pHrs->createCharacteristic(
        NimBLEUUID((uint16_t)0x2A37),
        NIMBLE_PROPERTY::NOTIFY
    );

    return true;
#else
    (void)server;
    return true;
#endif
}

void SampleHrRepeater::contributeAdvertising(BLEAdvertisingBuilder& adv) {
    adv.addServiceUuid16(0x180D, BLEAdvertisingBuilder::Priority::Normal);
}

void SampleHrRepeater::onPeerConnected(const BLEPeerInfo& /*peer*/) {
}

void SampleHrRepeater::onPeerDisconnected(BLEConnHandle /*handle*/, BLEDisconnectReason /*reason*/) {
}

void SampleHrRepeater::onSubscriptionChanged(BLEConnHandle /*handle*/, uint16_t /*attrHandle*/,
                                             uint16_t uuid16, bool notify, bool /*indicate*/) {
    if (uuid16 == 0x2A37 || uuid16 == 0) {
        _subscribed = notify;
#if defined(ESP_PLATFORM) && defined(WITH_BLE)
        ESP_LOGI(TAG, "Heart Rate 0x2A37 subscription state changed: %s", notify ? "SUBSCRIBED" : "UNSUBSCRIBED");
#endif
    }
}

void SampleHrRepeater::onStackPaused() {
    _paused = true;
}

void SampleHrRepeater::onStackResumed() {
    _paused = false;
}

void SampleHrRepeater::publishHr(uint8_t bpm) {
    _currentBpm = bpm;

    // If bpm is 0 ("no HR"), stop notifying
    if (bpm == 0) {
        return;
    }

    if (!_subscribed || _paused) {
        return;
    }

#if defined(ESP_PLATFORM) && defined(WITH_BLE)
    if (!_pHrChar) {
        return;
    }

    // Bluetooth SIG Heart Rate Measurement payload:
    // byte 0: flags (0x00 = UINT8 format, sensor contact not supported)
    // byte 1: heart rate in bpm
    uint8_t payload[2];
    payload[0] = 0x00;
    payload[1] = bpm;

    _pHrChar->setValue(payload, sizeof(payload));
    _pHrChar->notify();
#endif
}
