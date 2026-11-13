/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEEvents.hpp
 * Description: Topic string constants and payload structures published to EventBus
 *              by BLEManager. Authoritative: RFC #225 rev 2 §11.
 *
 * Exported Constants/Structs:
 * - BLEEvents: Namespace containing all "ble.*" topic name constants
 * - BLEStatePayload: Published on ble.ready and ble.state
 * - BLEPeripheralConnPayload: Published on ble.peripheral.connected / disconnected
 * - BLECentralConnPayload: Published on ble.central.connected / disconnected
 * - BLEScanPayload: Published on ble.scan.started / stopped
 * - BLEDevicePayload: Published on ble.device.paired / forgotten
 * - BLEAdvFaultPayload: Published on ble.advertising.fault
 */

#pragma once

#include "BLETypes.hpp"

namespace BLEEvents {
    inline constexpr const char* READY                   = "ble.ready";
    inline constexpr const char* STATE                   = "ble.state";
    inline constexpr const char* PERIPHERAL_CONNECTED    = "ble.peripheral.connected";
    inline constexpr const char* PERIPHERAL_DISCONNECTED = "ble.peripheral.disconnected";
    inline constexpr const char* CENTRAL_CONNECTED       = "ble.central.connected";
    inline constexpr const char* CENTRAL_DISCONNECTED    = "ble.central.disconnected";
    inline constexpr const char* SCAN_STARTED            = "ble.scan.started";
    inline constexpr const char* SCAN_STOPPED            = "ble.scan.stopped";
    inline constexpr const char* DEVICE_PAIRED           = "ble.device.paired";
    inline constexpr const char* DEVICE_FORGOTTEN        = "ble.device.forgotten";
    inline constexpr const char* ADVERTISING_FAULT       = "ble.advertising.fault";
}

/**
 * @brief Payload published on "ble.ready" and "ble.state".
 */
struct BLEStatePayload {
    BLEManagerState state  = BLEManagerState::Disabled;
    const char*     reason = nullptr;
};

/**
 * @brief Payload published on "ble.peripheral.connected" and "ble.peripheral.disconnected".
 */
struct BLEPeripheralConnPayload {
    BLEConnHandle handle              = BLE_CONN_HANDLE_NONE;
    char          addrRedacted[18]    = {};
    uint8_t       activeCount         = 0;
};

/**
 * @brief Payload published on "ble.central.connected" and "ble.central.disconnected".
 */
struct BLECentralConnPayload {
    const char*         providerId          = nullptr;
    char                addrRedacted[18]    = {};
    BLEDisconnectReason reason              = BLEDisconnectReason::Other;
    uint32_t            nextRetryMs         = 0;
};

/**
 * @brief Payload published on "ble.scan.started" and "ble.scan.stopped".
 */
struct BLEScanPayload {
    enum class Kind : uint8_t {
        Discovery = 0,
        Reconnect
    } kind = Kind::Discovery;

    const char* reason = nullptr;
};

/**
 * @brief Payload published on "ble.device.paired" and "ble.device.forgotten".
 */
struct BLEDevicePayload {
    const char* providerId          = nullptr;
    char        addrRedacted[18]    = {};
    char        name[32]            = {};
};

/**
 * @brief Payload published on "ble.advertising.fault".
 */
struct BLEAdvFaultPayload {
    const char* providerId   = nullptr;
    uint8_t     elementBytes = 0;
    uint8_t     freeBytes    = 0;
};
