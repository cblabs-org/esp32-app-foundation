/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLESerialization.hpp
 * Description: Pure serialization and validation functions for BLE REST endpoints
 *              and MCP tools.
 *              Authoritative: RFC #225 rev 2 §12, §13, §15.
 *
 * Exported Functions:
 * - stateToString(): Convert BLEManagerState to string
 * - addressTypeToString(): Convert BLEPeerAddress::Type to string
 * - parseAddressType(): Parse BLEPeerAddress::Type from string
 * - reconnectPhaseToString(): Convert BLEReconnectPolicy::Phase to string
 * - serializeStatus(): Serialize full or redacted status to JsonObject
 * - serializeScanResults(): Serialize scan results to JsonArray
 */

#pragma once

#include <ArduinoJson.h>
#include "BLETypes.hpp"
#include "BLEReconnectPolicy.hpp"
#include "BLEAdvertisingBuilder.hpp"

class BLEManager;

namespace BLESerialization {

/**
 * @brief Convert BLEManagerState enum to human-readable string.
 */
const char* stateToString(BLEManagerState state);

/**
 * @brief Convert BLEPeerAddress::Type enum to string representation.
 */
const char* addressTypeToString(BLEPeerAddress::Type type);

/**
 * @brief Parse BLEPeerAddress::Type from string representation.
 * @return true if valid type parsed, false otherwise.
 */
bool parseAddressType(const char* str, BLEPeerAddress::Type& out);

/**
 * @brief Convert BLEReconnectPolicy::Phase enum to string representation.
 */
const char* reconnectPhaseToString(BLEReconnectPolicy::Phase phase);

/**
 * @brief Serialize full or redacted BLEManager status to a JsonObject.
 *
 * Populates state, scan/adv status, advertised name, advertising packet budget & hex,
 * active peripheral connections (with negotiated connection parameters), client
 * provider stored devices and reconnect status, and queue drop / scan inhibit counters.
 *
 * @param root Target JsonObject to populate.
 * @param mgr Reference to the BLEManager instance.
 * @param redactAddresses If true, peer and stored device MAC addresses are redacted
 *                        (for read-only MCP ble_status). If false, unredacted MAC
 *                        addresses are emitted (for authenticated REST /api/ble/status).
 */
void serializeStatus(JsonObject root, const BLEManager& mgr, bool redactAddresses);

/**
 * @brief Serialize a list of BLEScanResult items to a JsonArray.
 *
 * @param root Target JsonArray to populate.
 * @param results Pointer to array of BLEScanResult objects.
 * @param count Number of results in the array.
 * @param redactAddresses If true, addresses are redacted.
 */
void serializeScanResults(JsonArray root, const BLEScanResult* results, size_t count, bool redactAddresses = false);

} // namespace BLESerialization
