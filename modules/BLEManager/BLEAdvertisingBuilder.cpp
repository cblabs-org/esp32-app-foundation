/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEAdvertisingBuilder.cpp
 * Description: Implementation of advertising builder and deterministic packing engine.
 *              Authoritative: RFC #225 rev 2 §5.4, §7.
 *
 * Exported Methods:
 * - BLEAdvertisingBuilder::addServiceUuid16(): Adds/merges 16-bit UUID
 * - BLEAdvertisingBuilder::addServiceUuid128(): Adds 128-bit custom service UUID
 * - BLEAdvertisingBuilder::addServiceData16(): Adds 16-bit service data AD structure
 * - BLEAdvertisingBuilder::setAppearance(): Sets Bluetooth appearance with conflict logging
 * - BLEAdvertisingBuilder::addManufacturerData(): Adds manufacturer-specific data AD structure
 * - BLEAdvertisingBuilder::pack(): Deterministically packs primary PDU and scan response
 * - BLEAdvertisingBuilder::clear(): Resets accumulated elements
 */

#include "BLEAdvertisingBuilder.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "esp_log.h"

static const char* TAG = "BLEAdvBuilder";

void BLEAdvertisingBuilder::addServiceUuid16(uint16_t uuid, Priority priority) {
    if (!_hasUuids16) {
        _hasUuids16 = true;
        _uuids16Order = _orderCounter++;
        _uuids16Priority = priority;
    } else {
        if (static_cast<uint8_t>(priority) < static_cast<uint8_t>(_uuids16Priority)) {
            _uuids16Priority = priority;
        }
    }

    if (std::find(_uuids16.begin(), _uuids16.end(), uuid) == _uuids16.end()) {
        _uuids16.push_back(uuid);
    }
}

void BLEAdvertisingBuilder::addServiceUuid128(const uint8_t uuid[16], Priority priority) {
    if (!uuid) return;
    Element elem;
    elem.priority = priority;
    elem.order = _orderCounter++;
    elem.description = "128-bit Service UUID";
    elem.bytes.resize(18);
    elem.bytes[0] = 17;   // Length (1 byte type + 16 bytes data)
    elem.bytes[1] = 0x07; // Complete List of 128-bit Service Class UUIDs
    std::memcpy(&elem.bytes[2], uuid, 16);
    _elements.push_back(std::move(elem));
}

void BLEAdvertisingBuilder::addServiceData16(uint16_t uuid, const uint8_t* data, uint8_t len, Priority priority) {
    Element elem;
    elem.priority = priority;
    elem.order = _orderCounter++;
    char desc[48];
    std::snprintf(desc, sizeof(desc), "Service Data 0x%04X", uuid);
    elem.description = desc;

    elem.bytes.resize(4 + len);
    elem.bytes[0] = static_cast<uint8_t>(1 + 2 + len); // Length
    elem.bytes[1] = 0x16;                              // Service Data - 16-bit UUID
    elem.bytes[2] = static_cast<uint8_t>(uuid & 0xFF);
    elem.bytes[3] = static_cast<uint8_t>((uuid >> 8) & 0xFF);
    if (len > 0 && data) {
        std::memcpy(&elem.bytes[4], data, len);
    }
    _elements.push_back(std::move(elem));
}

void BLEAdvertisingBuilder::setAppearance(uint16_t appearance) {
    if (_hasAppearance && _appearance != appearance && _appearance != 0) {
        ESP_LOGW(TAG, "Appearance conflict: replacing 0x%04X with 0x%04X", _appearance, appearance);
    }
    _appearance = appearance;
    _hasAppearance = true;
    _appearanceOrder = _orderCounter++;
}

void BLEAdvertisingBuilder::addManufacturerData(uint16_t companyId, const uint8_t* data, uint8_t len, Priority priority) {
    Element elem;
    elem.priority = priority;
    elem.order = _orderCounter++;
    char desc[48];
    std::snprintf(desc, sizeof(desc), "Manufacturer Data 0x%04X", companyId);
    elem.description = desc;

    elem.bytes.resize(4 + len);
    elem.bytes[0] = static_cast<uint8_t>(1 + 2 + len); // Length
    elem.bytes[1] = 0xFF;                              // Manufacturer Specific Data
    elem.bytes[2] = static_cast<uint8_t>(companyId & 0xFF);
    elem.bytes[3] = static_cast<uint8_t>((companyId >> 8) & 0xFF);
    if (len > 0 && data) {
        std::memcpy(&elem.bytes[4], data, len);
    }
    _elements.push_back(std::move(elem));
}

void BLEAdvertisingBuilder::clear() {
    _orderCounter = 0;
    _uuids16.clear();
    _uuids16Priority = Priority::Normal;
    _uuids16Order = 0;
    _hasUuids16 = false;

    _appearance = 0;
    _hasAppearance = false;
    _appearanceOrder = 0;

    _elements.clear();
}

BLEAdvPackResult BLEAdvertisingBuilder::pack(const char* advertisedName,
                                            bool nameSuffix,
                                            const uint8_t* ownMac) const {
    BLEAdvPackResult result;

    // 1. Flags (3 B): always first in primary PDU
    // 0x02: Length 2; 0x01: AD Type Flags; 0x06: LE General Discoverable | BR/EDR Not Supported
    result.primaryAdv.data[0] = 0x02;
    result.primaryAdv.data[1] = 0x01;
    result.primaryAdv.data[2] = 0x06;
    result.primaryAdv.length  = 3;

    // 2. Complete Local Name: always in scan response first (RFC §7 rule 6)
    if (advertisedName && advertisedName[0] != '\0') {
        char fullName[40] = {};
        if (nameSuffix && ownMac) {
            // "-XXXX" suffix from last 2 bytes of MAC (bytes[1] and bytes[0] in little-endian)
            std::snprintf(fullName, sizeof(fullName), "%s-%02X%02X",
                          advertisedName, ownMac[1], ownMac[0]);
        } else {
            std::snprintf(fullName, sizeof(fullName), "%s", advertisedName);
        }

        size_t nameLen = std::strlen(fullName);
        if (nameLen + 2 <= BLE_ADV_MAX_PAYLOAD) {
            result.scanResponse.data[0] = static_cast<uint8_t>(nameLen + 1); // Length
            result.scanResponse.data[1] = 0x09;                              // Complete Local Name
            std::memcpy(&result.scanResponse.data[2], fullName, nameLen);
            result.scanResponse.length = static_cast<uint8_t>(nameLen + 2);
        } else {
            // Truncate if exceeds max payload
            size_t fitLen = BLE_ADV_MAX_PAYLOAD - 2;
            result.scanResponse.data[0] = static_cast<uint8_t>(fitLen + 1);
            result.scanResponse.data[1] = 0x08;  // Y2: Shortened Local Name (truncated)
            std::memcpy(&result.scanResponse.data[2], fullName, fitLen);
            result.scanResponse.length = static_cast<uint8_t>(BLE_ADV_MAX_PAYLOAD);
        }
    }

    // 3. Collect all elements to pack
    std::vector<Element> candidates = _elements;

    // Add merged 16-bit UUID list if present
    if (_hasUuids16 && !_uuids16.empty()) {
        Element uuidElem;
        uuidElem.priority = _uuids16Priority;
        uuidElem.order = _uuids16Order;
        uuidElem.description = "16-bit Service UUID List";

        size_t dataLen = 2 * _uuids16.size();
        uuidElem.bytes.resize(2 + dataLen);
        uuidElem.bytes[0] = static_cast<uint8_t>(1 + dataLen); // Length
        uuidElem.bytes[1] = 0x03;                              // Complete List of 16-bit Service Class UUIDs
        for (size_t i = 0; i < _uuids16.size(); ++i) {
            uuidElem.bytes[2 + 2 * i]     = static_cast<uint8_t>(_uuids16[i] & 0xFF);
            uuidElem.bytes[2 + 2 * i + 1] = static_cast<uint8_t>((_uuids16[i] >> 8) & 0xFF);
        }
        candidates.push_back(std::move(uuidElem));
    }

    // Add Appearance if set
    if (_hasAppearance && _appearance != 0) {
        Element appElem;
        appElem.priority = Priority::Required;
        appElem.order = _appearanceOrder;
        appElem.description = "Appearance";
        appElem.bytes.resize(4);
        appElem.bytes[0] = 0x03; // Length
        appElem.bytes[1] = 0x19; // Appearance
        appElem.bytes[2] = static_cast<uint8_t>(_appearance & 0xFF);
        appElem.bytes[3] = static_cast<uint8_t>((_appearance >> 8) & 0xFF);
        candidates.push_back(std::move(appElem));
    }

    // 4. Sort by priority, then by insertion order (RFC §7 rule 2)
    std::sort(candidates.begin(), candidates.end(), [](const Element& a, const Element& b) {
        if (a.priority != b.priority) {
            return static_cast<uint8_t>(a.priority) < static_cast<uint8_t>(b.priority);
        }
        return a.order < b.order;
    });

    // 5. Pack into primary PDU first; spill over to scan response; record faults on Required overflow
    for (const auto& elem : candidates) {
        size_t elemSize = elem.bytes.size();
        if (result.primaryAdv.length + elemSize <= BLE_ADV_MAX_PAYLOAD) {
            std::memcpy(&result.primaryAdv.data[result.primaryAdv.length], elem.bytes.data(), elemSize);
            result.primaryAdv.length += static_cast<uint8_t>(elemSize);
        } else if (result.scanResponse.length + elemSize <= BLE_ADV_MAX_PAYLOAD) {
            std::memcpy(&result.scanResponse.data[result.scanResponse.length], elem.bytes.data(), elemSize);
            result.scanResponse.length += static_cast<uint8_t>(elemSize);
        } else {
            // Fits in neither
            if (elem.priority == Priority::Required) {
                BLEAdvFault fault;
                std::snprintf(fault.description, sizeof(fault.description), "%s", elem.description.c_str());
                fault.requiredBytes = static_cast<uint8_t>(elemSize);
                uint8_t availPrimary = static_cast<uint8_t>(BLE_ADV_MAX_PAYLOAD - result.primaryAdv.length);
                uint8_t availScan    = static_cast<uint8_t>(BLE_ADV_MAX_PAYLOAD - result.scanResponse.length);
                fault.availableBytes = std::max(availPrimary, availScan);
                result.faults.push_back(fault);

                ESP_LOGE(TAG, "Required advertising element '%s' (%zu B) fits in neither PDU (avail: prim=%u, scan=%u)",
                         elem.description.c_str(), elemSize, availPrimary, availScan);
            } else {
                ESP_LOGW(TAG, "Optional advertising element '%s' (%zu B) dropped due to PDU overflow",
                         elem.description.c_str(), elemSize);
            }
        }
    }

    // 6. Byte budget summary
    result.budget.advUsed      = result.primaryAdv.length;
    result.budget.advMax       = BLE_ADV_MAX_PAYLOAD;
    result.budget.scanRespUsed = result.scanResponse.length;
    result.budget.scanRespMax  = BLE_ADV_MAX_PAYLOAD;

    return result;
}
