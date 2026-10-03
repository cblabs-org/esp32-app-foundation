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

/**
 * @brief Sample Fitness Machine Service (FTMS) provider generating synthetic indoor bike data.
 */
class SampleFtmsProvider : public IBLEServiceProvider, public ITickable {
public:
    SampleFtmsProvider() = default;
    ~SampleFtmsProvider() override = default;

    const char* getProviderId() const override { return "ftms"; }

    /** @brief Create FTMS service and characteristics on the shared GATT server. */
    bool registerServices(NimBLEServer& server) override;
    /** @brief Add FTMS service UUID to advertising data. */
    void contributeAdvertising(BLEAdvertisingBuilder& adv) override;

    /** @brief Handle central peer connection event. */
    void onPeerConnected(const BLEPeerInfo& peer) override;
    /** @brief Handle central peer disconnection event. */
    void onPeerDisconnected(BLEConnHandle handle, BLEDisconnectReason reason) override;
    /** @brief Handle subscription state change for FTMS characteristics. */
    void onSubscriptionChanged(BLEConnHandle handle, uint16_t attrHandle, uint16_t uuid16,
                               bool notify, bool indicate) override;
    /** @brief Suspend synthetic data generation while stack is paused. */
    void onStackPaused() override;
    /** @brief Resume synthetic data generation after stack resumes. */
    void onStackResumed() override;

    /** @brief Generate and notify synthetic bike data at 2 Hz when subscribed. */
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
