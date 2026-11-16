/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEStack.hpp
 * Description: Low-level NimBLE stack lifecycle wrapper. Forward-declares NimBLE
 *              types so no NimBLE headers leak into consumer interfaces.
 *              Authoritative: RFC #225 rev 2 §5.1, §16.
 *
 * Exported Classes:
 * - BLEStack: Host/controller initialization, own-address query, server, and advertising
 */

#pragma once

#include "BLETypes.hpp"
#include "BLEQueueMessage.hpp"
#include "BLEAdvertisingBuilder.hpp"
#include "IBLEClientProvider.hpp"
#include <cstdint>
#include <cstring>
#include <vector>

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
    /** @brief Metadata for a registered GATT characteristic (service index, handle, UUID). */
    struct CharacteristicEntry {
        size_t   serviceIndex = 0;
        uint16_t handle       = 0;
        uint16_t uuid16       = 0;
    };

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

    /**
     * @brief Start the GATT server, assign characteristic handles, and attach callbacks.
     * @return true on success, false on failure.
     */
    bool startServer();

    /**
     * @brief Configure and start legacy connectable advertising with packed PDU.
     * @param packResult Packed primary and scan response AD payloads.
     * @param preferredParams GAP connection timing parameters.
     * @return true on success, false on failure.
     */
    bool startAdvertising(const BLEAdvPackResult& packResult, const BLEConnParams& preferredParams);

    /**
     * @brief Stop advertising.
     * @return true on success, false on failure.
     */
    bool stopAdvertising();

    /**
     * @brief Query whether advertising is currently running.
     * @return true if active, false otherwise.
     */
    bool isAdvertising() const;

    /**
     * @brief Terminate an active connection with a peripheral peer.
     * @param handle Connection handle.
     * @param reason HCI reason code (e.g. 0x13).
     * @return true on success, false on failure.
     */
    bool disconnect(BLEConnHandle handle, uint8_t reason = 0x13);

    /**
     * @brief Request a connection timing parameter update from connected central.
     * @param handle Connection handle.
     * @param params Requested connection timing parameters.
     */
    void updateConnParams(BLEConnHandle handle, const BLEConnParams& params);

    /**
     * @brief Set GAP Peripheral Preferred Connection Parameters (PPCP).
     * @param params Connection parameters to expose in GAP service.
     */
    void setPreferredConnParams(const BLEConnParams& params);

    /**
     * @brief Count of services currently registered on the GATT server.
     * @return Number of services.
     */
    size_t getServiceCount() const;

    /**
     * @brief Enumerate all characteristics registered on the GATT server.
     * @return Vector of CharacteristicEntry records with handles and UUIDs.
     */
    std::vector<CharacteristicEntry> getRegisteredCharacteristics() const;

    /**
     * @brief Count of queue message drops due to a full FreeRTOS queue.
     * @return Number of dropped messages.
     */
    uint32_t getDroppedEventsCount() const;

    /**
     * @brief Increment dropped events counter.
     */
    void incrementDropCount() {
        _droppedEventsCount++;
    }

    // --- Central Role (WO-3) ---

    /** @brief Options bundle for initiating a Central role connection. */
    struct CentralConnectOptions {
        BLEScanParams initiatorScan;
        BLEConnParams centralConnParams;
        IBLEClientProvider* provider;
        void* hostEventQueue;
    };
    /**
     * @brief Initiate a Central role connection to a peripheral.
     * @param opts   Connection options (scan params, conn params, provider, queue).
     * @param addr   Bluetooth address of the target peripheral.
     * @param bondedOut Set to true if the connection is bonded.
     * @return true on success, false on failure.
     */
    bool executeCentralConnect(const CentralConnectOptions& opts, const BLEPeerAddress& addr, bool& bondedOut);
    /**
     * @brief Disconnect a Central role connection by provider ID.
     * @param providerId Provider whose connection is terminated.
     */
    void executeCentralDisconnect(const char* providerId);
    /**
     * @brief Forget (remove bond) for a Central role connection.
     * @param providerId Provider whose bond is removed.
     * @param addr       Bluetooth address of the bonded peer.
     */
    void executeCentralForget(const char* providerId, const BLEPeerAddress& addr);
    /**
     * @brief Execute a BLE discovery scan for a specific client provider.
     * @param params      Scan window/interval parameters.
     * @param durationMs  Maximum scan duration in milliseconds.
     * @param provider    Client provider whose service UUIDs filter results.
     * @param hostEventQueue FreeRTOS queue for scan result delivery.
     * @param resultsOut  Output vector populated with discovered peers.
     */
    void executeDiscovery(const BLEScanParams& params, uint32_t durationMs, IBLEClientProvider* provider, void* hostEventQueue, std::vector<BLEScanResult>& resultsOut);
    /** @brief Stop an active BLE discovery scan. */
    void stopDiscovery();
    /** @brief Returns true if a BLE scan is currently active. */
    bool isScanning() const;
    /**
     * @brief Marshal a scan result to the host event queue.
     * @param hostEventQueue FreeRTOS queue handle.
     * @param res            Scan result to send.
     */
    void sendScanResult(void* hostEventQueue, const BLEScanResult& res);
    /**
     * @brief Marshal a scan-complete event to the host event queue.
     * @param hostEventQueue FreeRTOS queue handle.
     */
    void sendScanComplete(void* hostEventQueue);

private:
    bool          _initialized        = false;
    NimBLEServer* _server             = nullptr;
    void*         _eventQueue         = nullptr;
    uint32_t      _droppedEventsCount = 0;
    bool          _advertisingStub    = false;
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
inline bool BLEStack::startServer() {
    return true;
}
inline bool BLEStack::startAdvertising(const BLEAdvPackResult& /*packResult*/, const BLEConnParams& /*preferredParams*/) {
    _advertisingStub = true;
    return true;
}
inline bool BLEStack::stopAdvertising() {
    _advertisingStub = false;
    return true;
}
inline bool BLEStack::isAdvertising() const {
    return _advertisingStub;
}
inline bool BLEStack::disconnect(BLEConnHandle /*handle*/, uint8_t /*reason*/) {
    return true;
}
inline void BLEStack::updateConnParams(BLEConnHandle /*handle*/, const BLEConnParams& /*params*/) {}
inline void BLEStack::setPreferredConnParams(const BLEConnParams& /*params*/) {}
inline size_t BLEStack::getServiceCount() const {
    return 0;
}
inline std::vector<BLEStack::CharacteristicEntry> BLEStack::getRegisteredCharacteristics() const {
    return {};
}
inline uint32_t BLEStack::getDroppedEventsCount() const {
    return _droppedEventsCount;
}

inline bool BLEStack::executeCentralConnect(const CentralConnectOptions& /*opts*/, const BLEPeerAddress& /*addr*/, bool& bondedOut) {
    bondedOut = false;
    return true;
}
inline void BLEStack::executeCentralDisconnect(const char* /*providerId*/) {}
inline void BLEStack::executeCentralForget(const char* /*providerId*/, const BLEPeerAddress& /*addr*/) {}
inline void BLEStack::executeDiscovery(const BLEScanParams& /*params*/, uint32_t /*durationMs*/, IBLEClientProvider* /*provider*/, void* /*hostEventQueue*/, std::vector<BLEScanResult>& /*resultsOut*/) {}
inline void BLEStack::stopDiscovery() {}
inline bool BLEStack::isScanning() const { return false; }
inline void BLEStack::sendScanResult(void* /*hostEventQueue*/, const BLEScanResult& /*res*/) {}
inline void BLEStack::sendScanComplete(void* /*hostEventQueue*/) {}
#endif
