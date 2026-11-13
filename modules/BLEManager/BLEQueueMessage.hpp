/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEQueueMessage.hpp
 * Description: Queue message structure for host-task to main-loop FreeRTOS queue marshalling.
 *              Authoritative: RFC #225 rev 2 §5.1, §6.
 */

#pragma once

#include "BLETypes.hpp"
#include <cstdint>

/**
 * @brief Thread-safe message passed through FreeRTOS queue from NimBLE host task
 *        or worker task to the main application loop.
 */
struct BLEQueueMessage {
    enum class Type : uint8_t {
        PeerConnected = 0,
        PeerDisconnected,
        SubscriptionChanged,
        MTUChanged,
        ConnParamsUpdated,
        CentralConnected,
        CentralDisconnected,
        Notification,
        ScanResult,
        ScanCompleted,
        AdvFault,
        DeviceForgotten      // R9: Worker→main-loop signal for forget completion
    } type = Type::PeerConnected;

    BLEConnHandle       handle          = BLE_CONN_HANDLE_NONE;
    BLEPeerAddress      address;
    BLEDisconnectReason reason          = BLEDisconnectReason::Other;
    uint16_t            attrHandle      = 0;
    uint16_t            uuid16          = 0;
    uint16_t            mtu             = 23;
    BLEConnParams       params;
    bool                boolVal1        = false; // notify (sub), encrypted (conn), or connectable (scan)
    bool                boolVal2        = false; // indicate (sub)
    char                providerId[24]  = {};
    uint8_t             rawBytes[64]    = {};
    uint8_t             rawLen          = 0;
    uint32_t            receivedAtMs    = 0;    ///< O3: host-task timestamp for latency measurement
    int8_t              rssi            = 0;    ///< R8: ScanResult RSSI, carried in its own field (not packed into handle)
    uint16_t            appearance      = 0;    ///< Issue #308: ScanResult appearance
    uint16_t            uuids16[6]      = {};   ///< Issue #308: ScanResult advertised 16-bit service UUIDs
    uint8_t             uuid16Count     = 0;    ///< Issue #308: ScanResult advertised 16-bit service UUID count
};
