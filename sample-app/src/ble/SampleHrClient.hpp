/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/ble/SampleHrClient.hpp
 * Description: Sample Heart Rate Service (HRS, 0x180D) client provider.
 *              Receives heart rate measurements from a peripheral and feeds them to the repeater.
 */

#pragma once

#include "BLEManager/IBLEClientProvider.hpp"
#include "interfaces/ITickable.hpp"
#include <cstdint>

class SampleHrRepeater;

/**
 * @brief Sample Heart Rate Service client provider consuming HR measurements from a peripheral.
 */
class SampleHrClient : public IBLEClientProvider, public ITickable {
public:
    /**
     * @brief Construct with a pointer to the repeater that re-broadcasts received HR data.
     * @param repeater Repeater instance to feed heart rate values to.
     */
    explicit SampleHrClient(SampleHrRepeater* repeater) : _repeater(repeater) {}
    ~SampleHrClient() override = default;

    const char* getProviderId() const override { return "hr_client"; }
    const char* getDeviceTypeLabel() const override { return "Heart Rate Monitor"; }

    /** @brief Return the 16-bit HRS service UUID for discovery filtering. */
    size_t getTargetServiceUUIDs(uint16_t* out, size_t max) const override;
    /** @brief Return the HR Measurement characteristic subscription. */
    size_t getSubscriptions(BLESubscriptionSpec* out, size_t max) const override;

    /** @brief Handle successful connection to a heart rate peripheral. */
    void onConnected(const BLEStoredDevice& device) override;
    /** @brief Process an incoming heart rate measurement notification. */
    void onNotification(const BLENotification& notification) override;
    /** @brief Handle disconnection from the heart rate peripheral. */
    void onDisconnected(BLEDisconnectReason reason) override;

    /** @brief Periodic tick — manages HR data freshness tracking. */
    void tick(uint32_t currentMillis) override;

private:
    SampleHrRepeater* _repeater = nullptr;
    uint32_t _lastHrMs = 0;
    bool _connected = false;
};
