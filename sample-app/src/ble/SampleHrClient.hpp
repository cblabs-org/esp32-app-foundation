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

class SampleHrClient : public IBLEClientProvider, public ITickable {
public:
    explicit SampleHrClient(SampleHrRepeater* repeater) : _repeater(repeater) {}
    ~SampleHrClient() override = default;

    const char* getProviderId() const override { return "hr_client"; }
    const char* getDeviceTypeLabel() const override { return "Heart Rate Monitor"; }

    size_t getTargetServiceUUIDs(uint16_t* out, size_t max) const override;
    size_t getSubscriptions(BLESubscriptionSpec* out, size_t max) const override;

    void onConnected(const BLEStoredDevice& device) override;
    void onNotification(const BLENotification& notification) override;
    void onDisconnected(BLEDisconnectReason reason) override;

    void tick(uint32_t currentMillis) override;

private:
    SampleHrRepeater* _repeater = nullptr;
    uint32_t _lastHrMs = 0;
    bool _connected = false;
};
