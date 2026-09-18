/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEStack.hpp
 * Description: Low-level NimBLE stack lifecycle wrapper. Forward-declares NimBLE
 *              types so no NimBLE headers leak into consumer interfaces.
 *              Authoritative: RFC #225 rev 2 §5.1, §16.
 *
 * Exported Classes:
 * - BLEStack: Host/controller initialization, own-address query, and server access
 */

#pragma once

#include "BLETypes.hpp"
#include <cstdint>
#include <cstring>

class NimBLEServer;
class NimBLEClient;
class NimBLEScan;

/**
 * @brief NimBLE stack abstraction isolating Bluetooth hardware initialization.
 *
 * All NimBLE header includes are strictly confined to BLEStack.cpp and guarded by
 * `#if defined(ESP_PLATFORM) && defined(WITH_BLE)`. In host test builds (env:native),
 * inline stubs provide predictable execution.
 */
class BLEStack {
public:
    BLEStack();
    ~BLEStack();

    /**
     * @brief Initialize the NimBLE Bluetooth controller and host stack.
     * @param deviceName Bluetooth device name.
     * @param eventQueue FreeRTOS queue handle for host-to-main event marshalling.
     * @return true on success, false on failure.
     */
    bool init(const char* deviceName, void* eventQueue = nullptr);

    /**
     * @brief Deinitialize the NimBLE stack and release controller resources.
     */
    void deinit();

    /**
     * @brief Check whether NimBLE stack has been successfully initialized.
     * @return true if initialized, false otherwise.
     */
    bool isInitialized() const;

    /**
     * @brief Retrieve this device's own Bluetooth MAC address (little-endian).
     * @param out Output BLEPeerAddress populated with own address.
     * @return true if successfully retrieved, false otherwise.
     */
    bool getOwnAddress(BLEPeerAddress& out) const;

    /**
     * @brief Get pointer to the shared NimBLEServer instance (if created).
     * @return NimBLEServer pointer, or nullptr if not created/initialized.
     */
    NimBLEServer* getServer() const;

private:
    bool          _initialized = false;
    NimBLEServer* _server      = nullptr;
    void*         _eventQueue  = nullptr;
};

#if !defined(ESP_PLATFORM) || !defined(WITH_BLE)
// Inline host stubs for native test runner
inline BLEStack::BLEStack() = default;
inline BLEStack::~BLEStack() = default;
inline bool BLEStack::init(const char* /*deviceName*/, void* eventQueue) {
    _initialized = true;
    _eventQueue  = eventQueue;
    return true;
}
inline void BLEStack::deinit() {
    _initialized = false;
    _server      = nullptr;
}
inline bool BLEStack::isInitialized() const {
    return _initialized;
}
inline bool BLEStack::getOwnAddress(BLEPeerAddress& out) const {
    out.type = BLEPeerAddress::Type::Public;
    // Deterministic dummy address for native host tests: BC:9A:78:56:1A:2B
    // In little-endian: bytes[0]=0x2B, bytes[1]=0x1A, bytes[5]=0xBC
    out.bytes[0] = 0x2B;
    out.bytes[1] = 0x1A;
    out.bytes[2] = 0x56;
    out.bytes[3] = 0x78;
    out.bytes[4] = 0x9A;
    out.bytes[5] = 0xBC;
    return true;
}
inline NimBLEServer* BLEStack::getServer() const {
    return nullptr;
}
#endif
