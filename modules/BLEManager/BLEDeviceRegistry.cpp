/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEDeviceRegistry.cpp
 * Description: Implementation of BLE peer identity storage and JSON serialization.
 *              Authoritative: RFC #225 rev 2 §8.2, §10.
 *
 * Exported Methods:
 * - BLEDeviceRegistry::set(): Stores or updates stored device entry
 * - BLEDeviceRegistry::get(): Retrieves stored device entry by provider ID
 * - BLEDeviceRegistry::remove(): Removes stored device entry
 * - BLEDeviceRegistry::list(): Copies stored devices to array
 * - BLEDeviceRegistry::count(): Returns number of stored entries
 * - BLEDeviceRegistry::clear(): Clears all stored entries
 * - BLEDeviceRegistry::serialize(): Serializes entries to ArduinoJson object
 * - BLEDeviceRegistry::deserialize(): Restores entries from ArduinoJson object
 */

#include "BLEDeviceRegistry.hpp"
#include <cstring>

bool BLEDeviceRegistry::set(const BLEStoredDevice& device) {
    if (device.providerId[0] == '\0') {
        return false;
    }
    _devices[device.providerId] = device;
    return true;
}

bool BLEDeviceRegistry::get(const char* providerId, BLEStoredDevice& out) const {
    if (!providerId) return false;
    auto it = _devices.find(providerId);
    if (it == _devices.end()) {
        return false;
    }
    out = it->second;
    return true;
}

bool BLEDeviceRegistry::remove(const char* providerId) {
    if (!providerId) return false;
    return _devices.erase(providerId) > 0;
}

size_t BLEDeviceRegistry::list(BLEStoredDevice* out, size_t max) const {
    if (!out || max == 0) return 0;
    size_t count = 0;
    for (const auto& pair : _devices) {
        if (count >= max) break;
        out[count++] = pair.second;
    }
    return count;
}

size_t BLEDeviceRegistry::count() const {
    return _devices.size();
}

void BLEDeviceRegistry::clear() {
    _devices.clear();
}

void BLEDeviceRegistry::serialize(JsonObject obj) const {
    for (const auto& pair : _devices) {
        JsonObject devObj = obj[pair.first].to<JsonObject>();
        char addrStr[18] = {};
        pair.second.address.toString(addrStr, sizeof(addrStr));
        devObj["addr"]     = addrStr;
        devObj["addrType"] = static_cast<uint8_t>(pair.second.address.type);
        devObj["name"]     = pair.second.name;
        devObj["bonded"]   = pair.second.bonded;
    }
}

bool BLEDeviceRegistry::deserialize(JsonObjectConst obj) {
    clear();
    for (JsonPairConst kv : obj) {
        const char* providerId = kv.key().c_str();
        JsonObjectConst devObj = kv.value().as<JsonObjectConst>();
        if (devObj.isNull()) continue;

        BLEStoredDevice dev;
        std::strncpy(dev.providerId, providerId, sizeof(dev.providerId) - 1);
        dev.providerId[sizeof(dev.providerId) - 1] = '\0';

        const char* addrStr = devObj["addr"] | "";
        uint8_t addrType = devObj["addrType"] | 0;
        BLEPeerAddress::parse(addrStr, static_cast<BLEPeerAddress::Type>(addrType), dev.address);

        const char* nameStr = devObj["name"] | "";
        std::strncpy(dev.name, nameStr, sizeof(dev.name) - 1);
        dev.name[sizeof(dev.name) - 1] = '\0';

        dev.bonded = devObj["bonded"] | false;

        _devices[providerId] = dev;
    }
    return true;
}
