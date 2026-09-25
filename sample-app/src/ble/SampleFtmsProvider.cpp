/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/ble/SampleFtmsProvider.cpp
 * Description: Sample FTMS provider implementation.
 *              Authoritative: RFC #225 rev 2 §7.4, WORKORDER WO-2.
 */

#include "SampleFtmsProvider.hpp"

#if defined(ESP_PLATFORM) && defined(WITH_BLE)
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLECharacteristic.h>
#include "esp_log.h"
#endif

static const char* TAG = "SampleFtms";

bool SampleFtmsProvider::registerServices(NimBLEServer& server) {
#if defined(ESP_PLATFORM) && defined(WITH_BLE)
    ESP_LOGI(TAG, "Registering FTMS GATT services...");

    // 1. FTMS Service (0x1826)
    NimBLEService* pFtms = server.createService(NimBLEUUID((uint16_t)0x1826));
    if (!pFtms) {
        ESP_LOGE(TAG, "Failed to create FTMS service");
        return false;
    }

    // Fitness Machine Feature: 0x2ACC (Read)
    // FTMS Table 4.3 — bits for Indoor Bike Data fields this sample provides:
    //   bit 1  = Cadence Supported
    //   bit 7  = Resistance Level Supported
    //   bit 10 = Heart Rate Measurement Supported
    //   bit 14 = Power Measurement Supported
    // = 0x00004482.  (bit 2 = Total Distance is NOT set — Indoor Bike Data
    //   doesn't carry it.)
    // Plus 32-bit Target Setting Features = 0x00000000
    uint8_t featureVal[8] = {};
    uint32_t machineFeatures = 0x00004482;
    memcpy(featureVal, &machineFeatures, 4);  // Machine Features
    // featureVal[4..7] = 0 — no target settings
    _pFeatureChar = pFtms->createCharacteristic(
        NimBLEUUID((uint16_t)0x2ACC),
        NIMBLE_PROPERTY::READ
    );
    if (_pFeatureChar) {
        _pFeatureChar->setValue(featureVal, sizeof(featureVal));
    }

    // Indoor Bike Data: 0x2AD2 (Notify)
    _pBikeDataChar = pFtms->createCharacteristic(
        NimBLEUUID((uint16_t)0x2AD2),
        NIMBLE_PROPERTY::NOTIFY
    );

    // Fitness Machine Status: 0x2ADA (Notify)
    _pStatusChar = pFtms->createCharacteristic(
        NimBLEUUID((uint16_t)0x2ADA),
        NIMBLE_PROPERTY::NOTIFY
    );

    // Y8: Don't create a second DIS — the framework auto-registers one in begin().

    return true;
#else
    (void)server;
    return true;
#endif
}

void SampleFtmsProvider::contributeAdvertising(BLEAdvertisingBuilder& adv) {
    // Service UUID — FTMS apps discover via this, not Appearance
    adv.addServiceUuid16(0x1826, BLEAdvertisingBuilder::Priority::Required);

    // Service Data (Indoor Bike flag 0x0020, 3 bytes)
    const uint8_t serviceData[3] = {0x01, 0x20, 0x00};
    adv.addServiceData16(0x1826, serviceData, sizeof(serviceData), BLEAdvertisingBuilder::Priority::Required);

    // NOTE: Appearance is consumer-specific. The SIG Assigned Numbers define
    // Cycling sub-categories only for Computer, Speed, Cadence, Power, and
    // Speed+Cadence sensors — there is no Indoor Bike value. Consumer apps
    // should set their own appearance via contributeAdvertising() if needed
    // (e.g. adv.setAppearance(0x0480) for generic Cycling).
}

void SampleFtmsProvider::onPeerConnected(const BLEPeerInfo& /*peer*/) {
    // Peers track subscription independently
}

void SampleFtmsProvider::onPeerDisconnected(BLEConnHandle /*handle*/, BLEDisconnectReason /*reason*/) {
    // Central disconnected: if no subscribers remain, notifications will be idle
}

void SampleFtmsProvider::onSubscriptionChanged(BLEConnHandle /*handle*/, uint16_t /*attrHandle*/,
                                              uint16_t uuid16, bool notify, bool /*indicate*/) {
    if (uuid16 == 0x2AD2 || uuid16 == 0) {
        _subscribed = notify;
#if defined(ESP_PLATFORM) && defined(WITH_BLE)
        ESP_LOGI(TAG, "FTMS 0x2AD2 subscription state changed: %s", notify ? "SUBSCRIBED" : "UNSUBSCRIBED");
#endif
    }
}

void SampleFtmsProvider::onStackPaused() {
    _paused = true;
}

void SampleFtmsProvider::onStackResumed() {
    _paused = false;
}

void SampleFtmsProvider::tick(uint32_t currentMillis) {
    if (!_subscribed || _paused) {
        return;
    }

    // 2 Hz transmission (every 500 ms)
    if (currentMillis - _lastTickMs < 500) {
        return;
    }
    _lastTickMs = currentMillis;

#if defined(ESP_PLATFORM) && defined(WITH_BLE)
    if (!_pBikeDataChar) {
        return;
    }

    // FTMS Indoor Bike Data packet (Bluetooth SIG Fitness Machine Service v1.0, Indoor Bike Data):
    // Flags (16-bit):
    //   bit 0 = 0 (Instantaneous Speed present)
    //   bit 2 = 1 (Instantaneous Cadence present)
    //   bit 6 = 1 (Instantaneous Power present)
    //   bit 9 = 1 (Heart Rate present)
    // Value = (1 << 2) | (1 << 6) | (1 << 9) = 0x0244
    uint16_t flags = 0x0244;
    uint16_t speed = 2500;                 // 25.00 km/h (0.01 km/h)
    uint16_t cadence = _cadenceRpm * 2;    // 0.5 rpm resolution
    int16_t  power = _powerWatts;          // 1 watt resolution
    uint8_t  hr = _hrBpm;                  // 1 bpm

    uint8_t packet[9];
    packet[0] = static_cast<uint8_t>(flags & 0xFF);
    packet[1] = static_cast<uint8_t>((flags >> 8) & 0xFF);
    packet[2] = static_cast<uint8_t>(speed & 0xFF);
    packet[3] = static_cast<uint8_t>((speed >> 8) & 0xFF);
    packet[4] = static_cast<uint8_t>(cadence & 0xFF);
    packet[5] = static_cast<uint8_t>((cadence >> 8) & 0xFF);
    packet[6] = static_cast<uint8_t>(power & 0xFF);
    packet[7] = static_cast<uint8_t>((power >> 8) & 0xFF);
    packet[8] = hr;

    _pBikeDataChar->setValue(packet, sizeof(packet));
    _pBikeDataChar->notify();
#else
    (void)currentMillis;
#endif
}
