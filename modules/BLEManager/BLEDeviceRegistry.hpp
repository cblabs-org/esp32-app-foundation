/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEDeviceRegistry.hpp
 * Description: In-memory registry and ArduinoJson serializer for paired and bonded
 *              BLE peer devices keyed by client provider identifier.
 *              Authoritative: RFC #225 rev 2 §8.2, §10.
 *
 * Exported Classes:
 * - BLEDeviceRegistry: CRUD operations and JSON persistence helper for BLEStoredDevice
 */

#pragma once

#include "BLETypes.hpp"
#include <ArduinoJson.h>
#include <cstddef>
#include <map>
#include <string>

/**
 * @brief Registry of bonded and stored BLE peers keyed by client provider identifier.
 *
 * Provides thread-safe in-memory lookup, update, removal, and serialization
 * for BLE stored devices. Address types (Public, RandomStatic, etc.) are strictly
 * preserved across serialization cycles.
 */
class BLEDeviceRegistry {
public:
    BLEDeviceRegistry() = default;

    /**
     * @brief Store or update a peer device for a client provider.
     * @param device Device metadata including provider ID, address, and bond status.
     * @return true if successfully stored, false if provider ID is invalid or empty.
     */
    bool set(const BLEStoredDevice& device);

    /**
     * @brief Retrieve stored device metadata for a given provider identifier.
     * @param providerId Client provider identifier (e.g. "hrm").
     * @param out Destination struct to copy metadata into.
     * @return true if found, false otherwise.
     */
    bool get(const char* providerId, BLEStoredDevice& out) const;

    /**
     * @brief Remove stored device for a given provider identifier.
     * @param providerId Client provider identifier.
     * @return true if device was present and removed, false if not found.
     */
    bool remove(const char* providerId);

    /**
     * @brief Populate an array with all currently stored devices.
     * @param out Destination array pointer.
     * @param max Maximum number of entries that fit in out.
     * @return Total number of entries copied into out.
     */
    size_t list(BLEStoredDevice* out, size_t max) const;

    /**
     * @brief Get count of stored devices.
     * @return Number of stored device entries.
     */
    size_t count() const;

    /**
     * @brief Remove all stored devices.
     */
    void clear();

    /**
     * @brief Serialize all stored devices into a JSON object.
     *
     * Produces key-value pairs where key is providerId and value contains
     * addr, addrType, name, and bonded fields.
     *
     * @param obj Target ArduinoJson object to populate.
     */
    void serialize(JsonObject obj) const;

    /**
     * @brief Deserialize stored devices from a JSON object.
     * @param obj Source ArduinoJson object.
     * @return true on successful deserialization.
     */
    bool deserialize(JsonObjectConst obj);

private:
    std::map<std::string, BLEStoredDevice> _devices;
};
