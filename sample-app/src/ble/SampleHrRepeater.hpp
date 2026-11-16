/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/ble/SampleHrRepeater.hpp
 * Description: Sample Heart Rate Service (HRS, 0x180D) service provider.
 *              Repeats heart rate measurements received from central/client.
 *              Authoritative: RFC #225 rev 2 §7.4, WORKORDER WO-2.
 */

#pragma once

#include "BLEManager/IBLEServiceProvider.hpp"
#include "BLEManager/BLEAdvertisingBuilder.hpp"
#include <cstdint>

class NimBLECharacteristic;

/**
 * @brief Sample Heart Rate Service provider that re-broadcasts HR measurements from a client.
 */
class SampleHrRepeater : public IBLEServiceProvider {
public:
    SampleHrRepeater() = default;
    ~SampleHrRepeater() override = default;

    const char* getProviderId() const override { return "hr_repeater"; }

    /** @brief Create HRS service and HR Measurement characteristic. */
    bool registerServices(NimBLEServer& server) override;
    /** @brief Add HRS service UUID to advertising data. */
    void contributeAdvertising(BLEAdvertisingBuilder& adv) override;

    /** @brief Handle central peer connection event. */
    void onPeerConnected(const BLEPeerInfo& peer) override;
    /** @brief Handle central peer disconnection event. */
    void onPeerDisconnected(BLEConnHandle handle, BLEDisconnectReason reason) override;
    /** @brief Track subscription state for HR Measurement characteristic. */
    void onSubscriptionChanged(BLEConnHandle handle, uint16_t attrHandle, uint16_t uuid16,
                               bool notify, bool indicate) override;
    /** @brief Suspend HR notifications while stack is paused. */
    void onStackPaused() override;
    /** @brief Resume HR notifications after stack resumes. */
    void onStackResumed() override;

    /**
     * @brief Publish a new heart rate measurement in BPM.
     * @param bpm Heart rate value in BPM. 0 indicates no HR / sensor off, stopping notifications.
     */
    void publishHr(uint8_t bpm);

    /**
     * @brief Query whether a central is actively subscribed to HR notifications.
     */
    bool isSubscribed() const { return _subscribed; }

private:
    NimBLECharacteristic* _pHrChar    = nullptr;
    bool                  _subscribed = false;
    bool                  _paused     = false;
    uint8_t               _currentBpm = 0;
};
