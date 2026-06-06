/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Library: lib/SemVer/SemVer.cpp
 * Description: Implementation of the SemVer 2.0 engine. All parsing uses
 *              stack buffers, strtoul(), and strncmp() — zero heap allocation.
 */

#include "SemVer.hpp"
#include <cstdlib>   // strtoul
#include <cstring>   // strncmp, strcmp

// ---------------------------------------------------------------------------
// packVersion
// ---------------------------------------------------------------------------

uint32_t packVersion(const FirmwareVersion& v) {
    return (uint32_t(v.major)   << 24)
         | (uint32_t(v.minor)   << 16)
         | (uint32_t(v.patch)   << 8)
         | (uint32_t(v.channel) << 6)   // 2 bits: bits 7-6 (DEV=0,BETA=1,RC=2,STABLE=3)
         | uint32_t(v.preNum);          // 6 bits: bits 5-0 (max 63)
}

// ---------------------------------------------------------------------------
// parseVersionTag
// ---------------------------------------------------------------------------

FirmwareVersion parseVersionTag(const char* tag) {
    FirmwareVersion v;
    if (tag == nullptr) {
        return v;
    }

    const char* p = tag;

    // Skip leading 'v' or 'V'
    if (*p == 'v' || *p == 'V') {
        ++p;
    }

    // Parse MAJOR
    char* end = nullptr;
    v.major = static_cast<uint8_t>(strtoul(p, &end, 10));
    if (end == p || *end != '.') {
        return v;  // malformed — return zeroed struct
    }
    p = end + 1;  // skip '.'

    // Parse MINOR
    v.minor = static_cast<uint8_t>(strtoul(p, &end, 10));
    if (end == p || *end != '.') {
        return v;
    }
    p = end + 1;  // skip '.'

    // Parse PATCH
    v.patch = static_cast<uint8_t>(strtoul(p, &end, 10));
    p = end;

    // Default to STABLE with no pre-release
    v.channel = Channel::STABLE;
    v.preNum  = 0;

    // Check for pre-release suffix: '-dev', '-beta', '-rc'
    if (*p == '-') {
        ++p;  // skip '-'

        if (strncmp(p, "dev", 3) == 0) {
            v.channel = Channel::DEV;
            p += 3;
        } else if (strncmp(p, "beta", 4) == 0) {
            v.channel = Channel::BETA;
            p += 4;
        } else if (strncmp(p, "rc", 2) == 0) {
            v.channel = Channel::RC;
            p += 2;
        }

        // Parse pre-release number after '.'
        if (*p == '.') {
            ++p;
            v.preNum = static_cast<uint8_t>(strtoul(p, &end, 10));
        }
    }

    return v;
}

// ---------------------------------------------------------------------------
// channelToString / stringToChannel
// ---------------------------------------------------------------------------

const char* channelToString(Channel ch) {
    switch (ch) {
        case Channel::DEV:    return "dev";
        case Channel::BETA:   return "beta";
        case Channel::RC:     return "rc";
        case Channel::STABLE: return "stable";
        default:              return "stable";
    }
}

Channel stringToChannel(const char* str) {
    if (str == nullptr) {
        return Channel::STABLE;
    }
    if (strcmp(str, "dev") == 0)    return Channel::DEV;
    if (strcmp(str, "beta") == 0)   return Channel::BETA;
    if (strcmp(str, "rc") == 0)     return Channel::RC;
    return Channel::STABLE;
}

// ---------------------------------------------------------------------------
// Comparison operators
// ---------------------------------------------------------------------------

bool operator<(const FirmwareVersion& a, const FirmwareVersion& b) {
    return packVersion(a) < packVersion(b);
}

bool operator>(const FirmwareVersion& a, const FirmwareVersion& b) {
    return packVersion(a) > packVersion(b);
}

bool operator==(const FirmwareVersion& a, const FirmwareVersion& b) {
    return packVersion(a) == packVersion(b);
}

bool operator!=(const FirmwareVersion& a, const FirmwareVersion& b) {
    return packVersion(a) != packVersion(b);
}

bool operator<=(const FirmwareVersion& a, const FirmwareVersion& b) {
    return packVersion(a) <= packVersion(b);
}

bool operator>=(const FirmwareVersion& a, const FirmwareVersion& b) {
    return packVersion(a) >= packVersion(b);
}
