/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLETypes.hpp
 * Description: Core value types, connection parameters, scan structures, and
 *              address representations for Bluetooth Low Energy management.
 *              Authoritative source: RFC #225 rev 2 §4.
 *
 * Exported Types/Classes:
 * - BLEPeerAddress: 6-byte on-air little-endian address with type and redaction helpers
 * - BLEConnHandle: Connection handle scalar (0xFFFF = none)
 * - BLEConnParams: Connection interval, latency, and supervision timeout parameters
 * - BLEScanParams: Scan interval, window, and active scanning mode
 * - BLEScanResult: Advertised peer information discovered during scanning
 * - BLEStoredDevice: Persisted peer bond identity and metadata
 * - BLEPeerInfo: Active peer connection metadata
 * - BLEDisconnectReason: Standardized disconnection reason enumeration
 * - clampScanParams(): Scan parameter validation and safety clamping helper
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Bluetooth Low Energy Manager lifecycle state machine enumeration.
 * Authoritative: RFC #225 rev 2 §6.
 */
enum class BLEManagerState : uint8_t {
    Disabled = 0, ///< BLE disabled by configuration
    Idle,         ///< Stack initialized but waiting for application start conditions
    Starting,     ///< Initializing hardware stack, GATT server, and advertising
    Active,       ///< Normal operational state
    Paused,       ///< Temporarily paused (e.g. during OTA download)
    Error         ///< Stack failed to initialize or encountered fatal error
};

/**
 * @brief Bluetooth connection handle scalar.
 */
using BLEConnHandle = uint16_t;

/**
 * @brief Sentinel value indicating no active BLE connection handle.
 */
constexpr BLEConnHandle BLE_CONN_HANDLE_NONE = 0xFFFF;

/**
 * @brief Bluetooth Device Address representation.
 *
 * Stores the 6-byte address in on-air little-endian byte order (bytes[0] is LSB,
 * bytes[5] is MSB). Provides serialization to standard colon-separated hex strings,
 * privacy-preserving redacted strings, and parsing utilities.
 */
struct BLEPeerAddress {
    /**
     * @brief Bluetooth address type classification.
     */
    enum class Type : uint8_t {
        Public = 0,
        RandomStatic,
        RandomResolvable,     ///< RPA — needs a bond (IRK) to reconnect after rotation
        RandomNonResolvable,
        Unknown
    };

    uint8_t bytes[6] = {};    ///< Little-endian, as on-air (bytes[0]=LSB, bytes[5]=MSB)
    Type    type     = Type::Unknown;

    /**
     * @brief Determine if the address represents a valid, non-zero address with a known type.
     * @return true if valid, false otherwise.
     */
    bool isValid() const;

    /**
     * @brief Equality comparison operator.
     * @param other Address to compare against.
     * @return true if both type and all 6 bytes match.
     */
    bool operator==(const BLEPeerAddress& other) const;

    /**
     * @brief Inequality comparison operator.
     * @param other Address to compare against.
     * @return true if type or any byte differs.
     */
    bool operator!=(const BLEPeerAddress& other) const { return !(*this == other); }

    /**
     * @brief Format address as a full standard string ("AA:BB:CC:DD:EE:FF").
     * @param buf Output destination buffer.
     * @param len Destination buffer size (must be >= 18 bytes).
     */
    void toString(char* buf, size_t len) const;

    /**
     * @brief Format address as a privacy-safe redacted string ("**:**:**:**:EE:FF").
     * @param buf Output destination buffer.
     * @param len Destination buffer size (must be >= 18 bytes).
     */
    void toRedactedString(char* buf, size_t len) const;

    /**
     * @brief Parse a standard colon-separated or hyphen-separated hex string into a BLEPeerAddress.
     * @param str Input null-terminated MAC string ("AA:BB:CC:DD:EE:FF" or "AA-BB-CC-DD-EE-FF").
     * @param type Address type classification to associate.
     * @param out Parsed output object.
     * @return true if 6 hex bytes were successfully parsed, false otherwise.
     */
    static bool parse(const char* str, Type type, BLEPeerAddress& out);
};

/**
 * @brief Bluetooth connection timing parameters.
 */
struct BLEConnParams {
    uint16_t minIntervalUnits        = 24;   ///< 1.25 ms units (6..3200, 24 = 30 ms)
    uint16_t maxIntervalUnits        = 40;   ///< 1.25 ms units (6..3200, 40 = 50 ms)
    uint16_t latency                 = 0;    ///< Slave latency (number of connection events)
    uint16_t supervisionTimeoutUnits = 400;  ///< 10 ms units (10..3200, 400 = 4000 ms)
};

/**
 * @brief Bluetooth Low Energy scan timing parameters.
 */
struct BLEScanParams {
    uint16_t intervalMs = 100;  ///< Scan interval in milliseconds
    uint16_t windowMs   = 50;   ///< Scan window in milliseconds (clamped <= 50% of interval)
    bool     active     = true; ///< True for active scanning (requests scan response), false for passive
};

/**
 * @brief Advertised peer information discovered during a scan.
 */
struct BLEScanResult {
    BLEPeerAddress address;
    char           name[32]       = {};
    int8_t         rssi           = 0;
    uint16_t       appearance     = 0;
    uint16_t       uuids16[6]     = {};
    uint8_t        uuid16Count    = 0;
    bool           connectable    = false;
};

/**
 * @brief Persisted identity and bond metadata for a known peer.
 */
struct BLEStoredDevice {
    char           providerId[24] = {};  ///< Client provider identifier (RFC §8.2)
    BLEPeerAddress address;
    char           name[32]       = {};
    bool           bonded         = false;
};

/**
 * @brief Connection metadata for an active BLE peer link.
 */
struct BLEPeerInfo {
    BLEConnHandle  handle    = BLE_CONN_HANDLE_NONE;
    BLEPeerAddress address;
    uint16_t       mtu       = 23;
    BLEConnParams  params;
    bool           encrypted = false;
};

/**
 * @brief Standardized BLE disconnection reason classification.
 */
enum class BLEDisconnectReason : uint8_t {
    PeerTerminated = 0,
    LocalTerminated,
    SupervisionTimeout,
    ConnectFailed,
    Paused,
    Other
};

/**
 * @brief Clamp scan window to at most 50% of scan interval.
 * @param params Scan parameters structure to validate and clamp in-place.
 */
void clampScanParams(BLEScanParams& params);

/**
 * @brief Clamp scan window to at most 50% of scan interval.
 * @param intervalMs Reference to interval in milliseconds.
 * @param windowMs Reference to window in milliseconds (clamped in-place).
 */
void clampScanParams(uint16_t& intervalMs, uint16_t& windowMs);
