/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEAdvertisingBuilder.hpp
 * Description: Declarative advertising payload builder and deterministic packing
 *              engine. Implements RFC #225 rev 2 §5.4 and §7. Pure C++, fully
 *              host-testable without NimBLE dependencies.
 *
 * Exported Types/Classes:
 * - BLEAdvertisingBuilder: Builder interface for providers contributing AD elements
 * - BLEAdvPayload: 31-byte raw BLE PDU payload buffer
 * - BLEAdvFault: Fault description when a Required element cannot fit in either PDU
 * - BLEAdvBudget: Byte usage breakdown for primary PDU and scan response
 * - BLEAdvPackResult: Complete packing result containing both PDUs, faults, and budget
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

/// Maximum raw payload capacity of a legacy BLE advertising or scan-response PDU.
constexpr size_t BLE_ADV_MAX_PAYLOAD = 31;

/**
 * @brief Raw byte payload for a BLE advertising or scan response PDU (up to 31 bytes).
 */
struct BLEAdvPayload {
    uint8_t data[BLE_ADV_MAX_PAYLOAD] = {};
    uint8_t length                    = 0;
};

/**
 * @brief Record of a Required element that could not fit into either PDU.
 */
struct BLEAdvFault {
    char    description[64] = {};
    uint8_t requiredBytes   = 0;
    uint8_t availableBytes  = 0;
};

/**
 * @brief Byte budget accounting for primary advertising and scan response PDUs.
 */
struct BLEAdvBudget {
    uint8_t advUsed      = 0;
    uint8_t advMax       = BLE_ADV_MAX_PAYLOAD;
    uint8_t scanRespUsed = 0;
    uint8_t scanRespMax  = BLE_ADV_MAX_PAYLOAD;
};

/**
 * @brief Result of deterministic advertising payload packing.
 */
struct BLEAdvPackResult {
    BLEAdvPayload            primaryAdv;
    BLEAdvPayload            scanResponse;
    std::vector<BLEAdvFault> faults;
    BLEAdvBudget             budget;

    /** @brief True if any Required element overflowed and raised a fault. */
    bool hasFault() const { return !faults.empty(); }
};

/**
 * @brief Declarative BLE advertising builder and packing engine.
 *
 * Implements the deterministic packing algorithm from RFC #225 rev 2 §7:
 * 1. Flags (3 B) always first in the primary PDU.
 * 2. Scan response always starts with Complete Local Name (advertisedName + optional "-XXXX" suffix).
 * 3. 16-bit Service UUIDs from all providers are merged into a single Complete List (AD type 0x03).
 * 4. Elements are prioritized (Required -> High -> Normal) and packed into primary PDU first,
 *    then spilled into scan response.
 * 5. If a Required element fits in neither PDU, a fault is generated and the element is omitted.
 */
class BLEAdvertisingBuilder {
public:
    /**
     * @brief Priority classification controlling packing order and overflow fault handling.
     */
    enum class Priority : uint8_t {
        Required = 0, ///< Must be present; if it fits nowhere, raises a precondition fault
        High     = 1, ///< High priority; packed before Normal
        Normal   = 2  ///< Best effort; packed last, drops gracefully on overflow
    };

    BLEAdvertisingBuilder() = default;

    /**
     * @brief Add a 16-bit Service UUID to the Complete List of 16-bit Service UUIDs.
     * @param uuid 16-bit Bluetooth SIG assigned UUID (e.g. 0x1826 for FTMS, 0x180D for Heart Rate).
     * @param priority Packing priority.
     */
    void addServiceUuid16(uint16_t uuid, Priority priority = Priority::Required);

    /**
     * @brief Add a 128-bit custom Service UUID.
     * @param uuid 16-byte UUID array in little-endian order.
     * @param priority Packing priority.
     */
    void addServiceUuid128(const uint8_t uuid[16], Priority priority = Priority::Normal);

    /**
     * @brief Add Service Data associated with a 16-bit Service UUID (AD type 0x16).
     * @param uuid 16-bit Service UUID.
     * @param data Payload bytes (up to 20 bytes).
     * @param len Number of bytes in data.
     * @param priority Packing priority.
     */
    void addServiceData16(uint16_t uuid, const uint8_t* data, uint8_t len,
                          Priority priority = Priority::Required);

    /**
     * @brief Set Bluetooth Appearance value (AD type 0x19).
     *
     * Last writer wins. If called multiple times with conflicting non-zero values,
     * a warning is logged and the last value is used.
     *
     * @param appearance 16-bit Bluetooth Appearance code.
     */
    void setAppearance(uint16_t appearance);

    /**
     * @brief Add Manufacturer-Specific Data (AD type 0xFF).
     * @param companyId 16-bit Bluetooth SIG Company Identifier.
     * @param data Payload data bytes.
     * @param len Number of data bytes.
     * @param priority Packing priority.
     */
    void addManufacturerData(uint16_t companyId, const uint8_t* data, uint8_t len,
                             Priority priority = Priority::Normal);

    /**
     * @brief Clear all accumulated elements and reset builder state.
     */
    void clear();

    /**
     * @brief Pack all contributions into primary advertising and scan response PDUs.
     *
     * @param advertisedName Device name to advertise in scan response (can be nullptr).
     * @param nameSuffix If true, append "-XXXX" (last 2 bytes of MAC) to advertisedName.
     * @param ownMac 6-byte little-endian Bluetooth MAC address (bytes[0]=LSB, bytes[5]=MSB).
     * @return BLEAdvPackResult containing primary PDU, scan response, faults, and byte budget.
     */
    BLEAdvPackResult pack(const char* advertisedName = nullptr,
                          bool nameSuffix = true,
                          const uint8_t* ownMac = nullptr) const;

private:
    struct Element {
        Priority             priority;
        uint32_t             order;
        std::string          description;
        std::vector<uint8_t> bytes; // Full AD structure: [Length, AD_Type, Data...]
    };

    uint32_t              _orderCounter = 0;
    std::vector<uint16_t> _uuids16;
    Priority              _uuids16Priority = Priority::Normal;
    uint32_t              _uuids16Order    = 0;
    bool                  _hasUuids16      = false;

    uint16_t              _appearance      = 0;
    bool                  _hasAppearance   = false;
    uint32_t              _appearanceOrder = 0;

    std::vector<Element>  _elements;
};
