/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Library: lib/SemVer/SemVer.hpp
 * Description: SemVer 2.0 engine for firmware version comparison on ESP32.
 *              Uses packed 32-bit integers for zero-allocation comparison on MCU,
 *              with a stack-based tag parser for git release tags.
 *
 * Exported Symbols:
 * - Channel:           Enum class for release channels (DEV, BETA, RC, STABLE)
 * - FirmwareVersion:   Struct holding parsed version components
 * - packVersion():     Encodes FirmwareVersion to a 32-bit integer for comparison
 * - parseVersionTag(): Parses a git tag string into a FirmwareVersion struct
 * - channelToString(): Converts Channel enum to its string representation
 * - stringToChannel(): Parses a channel string into a Channel enum value
 */

#pragma once

#include <cstdint>

/**
 * @brief Release channel classification for firmware versions.
 *
 * Values are contiguous so they pack into exactly 2 bits and the
 * ordering invariant DEV < BETA < RC < STABLE is preserved in the
 * packed 32-bit integer comparison performed by packVersion().
 */
enum class Channel : uint8_t {
    DEV    = 0,   ///< Development / nightly builds
    BETA   = 1,   ///< Beta pre-releases
    RC     = 2,   ///< Release candidates
    STABLE = 3,   ///< Production releases
};

/**
 * @brief Parsed semantic version with channel and pre-release number.
 *
 * Designed for stack allocation — no heap, no String objects.
 * Comparison operators delegate to packVersion() for single-instruction
 * integer comparison on MCU.
 */
struct FirmwareVersion {
    uint8_t major   = 0;
    uint8_t minor   = 0;
    uint8_t patch   = 0;
    Channel channel = Channel::STABLE;
    uint8_t preNum  = 0;
};

/**
 * @brief Encode a FirmwareVersion into a packed 32-bit integer.
 *
 * Bit layout: MAJOR(8) | MINOR(8) | PATCH(8) | CHANNEL(2) | PRE_NUM(6)
 *
 * Channel uses bits 7-6 (2 bits, values 0-3 — exact fit for 4 channels).
 * PRE_NUM uses bits 5-0 (6 bits, values 0-63 — raised from the old 4-bit
 * cap of 15 which silently corrupted at beta.16+).
 *
 * @param v The FirmwareVersion to pack.
 * @return Packed 32-bit representation suitable for integer comparison.
 */
uint32_t packVersion(const FirmwareVersion& v);

/**
 * @brief Parse a git tag string into a FirmwareVersion struct.
 *
 * Accepts tags like "v3.1.0", "3.1.0", "v3.1.0-beta.1", "v3.1.0-dev.3",
 * "v3.1.0-rc.1". Uses strtoul() and strncmp() — zero heap allocation.
 *
 * @param tag The null-terminated git tag string to parse.
 * @return Parsed FirmwareVersion (by value, stack-allocated).
 */
FirmwareVersion parseVersionTag(const char* tag);

/**
 * @brief Convert a Channel enum value to its string representation.
 *
 * @param ch The Channel enum value.
 * @return Pointer to a static string: "dev", "beta", "rc", or "stable".
 */
const char* channelToString(Channel ch);

/**
 * @brief Parse a channel string into a Channel enum value.
 *
 * Accepted inputs: "dev", "beta", "rc", "stable" (case-sensitive).
 * Unrecognised strings default to Channel::STABLE.
 *
 * @param str The null-terminated channel string to parse.
 * @return The corresponding Channel enum value.
 */
Channel stringToChannel(const char* str);

// ---------------------------------------------------------------------------
// Comparison operators — delegate to packVersion() for MCU-friendly compare
// ---------------------------------------------------------------------------

/**
 * @brief Less-than comparison for FirmwareVersion.
 */
bool operator<(const FirmwareVersion& a, const FirmwareVersion& b);

/**
 * @brief Greater-than comparison for FirmwareVersion.
 */
bool operator>(const FirmwareVersion& a, const FirmwareVersion& b);

/**
 * @brief Equality comparison for FirmwareVersion.
 */
bool operator==(const FirmwareVersion& a, const FirmwareVersion& b);

/**
 * @brief Inequality comparison for FirmwareVersion.
 */
bool operator!=(const FirmwareVersion& a, const FirmwareVersion& b);

/**
 * @brief Less-than-or-equal comparison for FirmwareVersion.
 */
bool operator<=(const FirmwareVersion& a, const FirmwareVersion& b);

/**
 * @brief Greater-than-or-equal comparison for FirmwareVersion.
 */
bool operator>=(const FirmwareVersion& a, const FirmwareVersion& b);
