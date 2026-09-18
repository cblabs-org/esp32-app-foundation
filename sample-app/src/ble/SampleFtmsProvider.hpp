/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/ble/SampleFtmsProvider.hpp
 * Description: Sample Fitness Machine Service (FTMS, 0x1826) service provider.
 *              Generates 2 Hz synthetic indoor bike data when subscribed.
 *              Authoritative: RFC #225 rev 2 §7.4, WORKORDER WO-2.
 */

#pragma once

#include "BLEManager/IBLEServiceProvider.hpp"
#include "BLEManager/BLEAdvertisingBuilder.hpp"
#include "interfaces/ITickable.hpp"
#include <cstdint>

class NimBLECharacteristic;

class SampleFtmsProvider : public IBLEServiceProvider, public ITickable {
public:
    SampleFtmsProvider() = default;
    ~SampleFtmsProvider() override = default;

    const char* getProviderId() const override { return "ftms"; }

    bool registerServices(NimBLEServer& server) override;
    void contributeAdvertising(BLEAdvertisingBuilder& adv) override;

    void onPeerConnected(const BLEPeerInfo& peer) override;
    void onPeerDisconnected(BLEConnHandle handle, BLEDisconnectReason reason) override;
    void onSubscriptionChanged(BLEConnHandle handle, uint16_t attrHandle, uint16_t uuid16,
                               bool notify, bool indicate) override;
    void onStackPaused() override;
    void onStackResumed() override;

    void tick(uint32_t currentMillis) override;

private:
    NimBLECharacteristic* _pFeatureChar  = nullptr;
    NimBLECharacteristic* _pBikeDataChar = nullptr;
    NimBLECharacteristic* _pStatusChar   = nullptr;

    bool     _subscribed = false;
    bool     _paused     = false;
    uint32_t _lastTickMs = 0;

    // Synthetic metric state
    uint16_t _speedMphTimes100 = 1500; // 15.00 mph
    uint16_t _cadenceRpm       = 85;
    int16_t  _powerWatts       = 175;
    uint8_t  _hrBpm            = 130;
};
