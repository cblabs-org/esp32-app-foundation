/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/IBLEServiceProvider.hpp
 * Description: Pure interface for modules contributing GATT services and advertising
 *              elements in the BLE Peripheral role. Authoritative: RFC #225 rev 2 §5.2.
 *
 * Exported Classes:
 * - IBLEServiceProvider: Abstract base class for Peripheral GATT service contributors
 */

#pragma once

#include "BLETypes.hpp"

class NimBLEServer;            ///< Forward declaration only; included by providers' .cpp files
class BLEAdvertisingBuilder;

/**
 * @brief Extension point for modules that expose GATT services (Peripheral role).
 *
 * Lifecycle (RFC §6):
 * - registerServices(): Called once when the stack starts; providers create services/characteristics here.
 * - contributeAdvertising(): Called once at start, and again on bleManager.refreshAdvertising().
 * - onPeerConnected / onPeerDisconnected / onSubscriptionChanged: Called on main loop from tick().
 * - onStackPaused / onStackResumed: Called on main loop when stack is paused/resumed.
 *
 * GATT is STATIC after registerServices() returns. Services that are conditionally present
 * must always exist and signal absence in data (RFC §5.2).
 */
class IBLEServiceProvider {
public:
    virtual ~IBLEServiceProvider() = default;

    /**
     * @brief Stable identifier for logs, status, and config keys (static lifetime, <= 23 chars).
     * @return Null-terminated string literal.
     */
    virtual const char* getProviderId() const = 0;

    /**
     * @brief Create services and characteristics on the shared server.
     *
     * Do NOT start the server or advertising; BLEManager starts them after all
     * providers have returned.
     *
     * @param server Shared NimBLEServer instance.
     * @return false to signal failure; raises the "BLE GATT" precondition fault.
     */
    virtual bool registerServices(NimBLEServer& server) = 0;

    /**
     * @brief Add advertising elements. The manager owns Flags, Name, and packing.
     * @param adv Reference to BLEAdvertisingBuilder.
     */
    virtual void contributeAdvertising(BLEAdvertisingBuilder& adv) { (void)adv; }

    /**
     * @brief A central connected to the shared GATT server.
     *
     * Every provider is notified of every connection; filter by subscriptions if needed.
     * Runs on the main loop from BLEManager::tick().
     *
     * @param peer Active peer connection metadata.
     */
    virtual void onPeerConnected(const BLEPeerInfo& peer) { (void)peer; }

    /**
     * @brief A central disconnected from the shared GATT server.
     * Runs on the main loop from BLEManager::tick().
     *
     * @param handle Connection handle of the disconnected peer.
     * @param reason Disconnection reason.
     */
    virtual void onPeerDisconnected(BLEConnHandle handle, BLEDisconnectReason reason) {
        (void)handle;
        (void)reason;
    }

    /**
     * @brief A peer enabled or disabled notifications or indications on a characteristic.
     *
     * Allows providers to notify only when someone is actively listening.
     * Runs on the main loop from BLEManager::tick().
     *
     * @param handle Connection handle of the peer.
     * @param charHandle Attribute handle of the characteristic.
     * @param charUuid16 16-bit UUID, or 0 for 128-bit (use charHandle).
     * @param notify True if notifications were enabled.
     * @param indicate True if indications were enabled.
     */
    virtual void onSubscriptionChanged(BLEConnHandle handle, uint16_t charHandle,
                                       uint16_t charUuid16, bool notify, bool indicate) {
        (void)handle;
        (void)charHandle;
        (void)charUuid16;
        (void)notify;
        (void)indicate;
    }

    /**
     * @brief Stack paused (e.g. OTA download, administrative pause).
     * Providers should suspend timers that send notifications.
     */
    virtual void onStackPaused() {}

    /**
     * @brief Stack resumed after pause.
     */
    virtual void onStackResumed() {}
};
