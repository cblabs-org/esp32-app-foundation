/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/IBLEClientProvider.hpp
 * Description: Pure interface for modules connecting to remote BLE peripherals
 *              in the Central role. Authoritative: RFC #225 rev 2 §5.3.
 *
 * Exported Types/Classes:
 * - BLESubscriptionSpec: Specification of characteristic to subscribe to
 * - BLENotification: Inbound notification payload copied for main-loop delivery
 * - IBLEClientProvider: Abstract base class for Central role client contributors
 */

#pragma once

#include "BLETypes.hpp"

class NimBLEClient;

/**
 * @brief Characteristic subscription specification for Central role connections.
 */
struct BLESubscriptionSpec {
    uint16_t serviceUuid16 = 0;     ///< 16-bit Service UUID (e.g. 0x180D)
    uint16_t charUuid16    = 0;     ///< 16-bit Characteristic UUID (e.g. 0x2A37 for HR Measurement)
    bool     indicate      = false; ///< false = notify, true = indicate
    bool     required      = false; ///< If true and missing, connection is treated as failed
};

/**
 * @brief Inbound notification payload copied for safe main-loop delivery.
 */
struct BLENotification {
    uint16_t serviceUuid16 = 0;
    uint16_t charUuid16    = 0;
    uint8_t  data[64]      = {};   ///< Truncated to 64 B; truncated flags when shortened
    uint8_t  length        = 0;
    bool     truncated     = false;
    uint32_t receivedAtMs  = 0;    ///< millis() when host task received it (latency metric)
};

/**
 * @brief Extension point for modules that connect to external BLE devices (Central role).
 *
 * BLEManager manages: scanning, device selection, stored identity, bonding,
 * reconnection, GATT service discovery, and subscriptions. The provider declares
 * WHAT it requires and receives data on the main loop. It never sees NimBLEClient
 * unless it opts into onClientReadyWorker().
 */
class IBLEClientProvider {
public:
    /** @brief Virtual destructor. */
    virtual ~IBLEClientProvider() = default;

    /**
     * @brief Stable identifier; also the key for stored device identity (static lifetime, <= 23 chars).
     * @return Null-terminated string literal.
     */
    virtual const char* getProviderId() const = 0;

    /**
     * @brief Human-readable device category label for the web portal (e.g. "Heart Rate Monitor").
     * @return Null-terminated string literal.
     */
    virtual const char* getDeviceTypeLabel() const = 0;

    /**
     * @brief Service UUIDs used to filter discovery scan results (any match offers the device).
     * @param out Destination buffer for 16-bit UUIDs.
     * @param max Maximum number of UUIDs that fit in out.
     * @return Number of UUIDs written.
     */
    virtual size_t getTargetServiceUUIDs(uint16_t* out, size_t max) const = 0;

    /**
     * @brief Characteristics to subscribe to after connection and service discovery.
     * @param out Destination buffer for subscription specifications.
     * @param max Maximum number of specifications that fit in out.
     * @return Number of specifications written.
     */
    virtual size_t getSubscriptions(BLESubscriptionSpec* out, size_t max) const = 0;

    /**
     * @brief Optional extra filter for discovery scan results (e.g. name prefix).
     * Runs on the main loop.
     *
     * @param result Discovered peer scan result.
     * @return true to accept result into discovery list, false to filter out.
     */
    virtual bool acceptScanResult(const BLEScanResult& result) const {
        (void)result;
        return true;
    }

    /**
     * @brief Connected to peer; discovery and subscriptions complete.
     * Runs on the main loop from BLEManager::tick().
     *
     * @param device Persisted stored device identity.
     */
    virtual void onConnected(const BLEStoredDevice& device) = 0;

    /**
     * @brief Notification received from a subscribed characteristic.
     * Runs on the main loop from BLEManager::tick().
     *
     * @param notification Decoded notification payload.
     */
    virtual void onNotification(const BLENotification& notification) = 0;

    /**
     * @brief Connection lost or failed. BLEManager automatically schedules reconnection.
     * Runs on the main loop from BLEManager::tick().
     *
     * @param reason Disconnection reason.
     */
    virtual void onDisconnected(BLEDisconnectReason reason) = 0;

    /**
     * @brief User paired a new device (via web portal or MCP tool).
     * Runs on the main loop.
     *
     * @param device Stored device metadata.
     */
    virtual void onDevicePaired(const BLEStoredDevice& device) { (void)device; }

    /**
     * @brief User forgot the stored device (via web portal or MCP tool).
     * Runs on the main loop.
     */
    virtual void onDeviceForgotten() {}

    /**
     * @brief Opt-in worker hook executed on the BLEManager worker task after subscriptions succeed.
     *
     * Use for one-off blocking reads (e.g. Battery Level, Body Sensor Location).
     * Must not touch main-loop state without synchronization.
     *
     * @param client Active connected NimBLEClient instance.
     */
    virtual void onClientReadyWorker(NimBLEClient& client) { (void)client; }
};
