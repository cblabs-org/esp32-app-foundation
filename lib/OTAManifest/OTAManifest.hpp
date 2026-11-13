/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Library: lib/OTAManifest/OTAManifest.hpp
 * Description: Multi-channel OTA manifest parser (v2 schema). Defines the
 *              schema for manifest.json containing absolute download URLs
 *              for all release channels. Each channel maps board IDs to
 *              their firmware URL, signature URL, size, and SHA-256 hash.
 *              All storage is stack-allocated using fixed-size char arrays —
 *              no heap, no Arduino String.
 *
 * Exported Symbols:
 * - OTAAsset:           Per-board firmware asset descriptor (board, firmwareUrl, signatureUrl, size, sha256)
 * - OTAChannel:         Per-channel release descriptor (version, name, securityVersion, assets)
 * - OTAManifest:        Top-level manifest with manifestVersion, manifestUrl, and channel array
 * - parseManifest():    Parse raw JSON into an OTAManifest struct
 * - findChannel():      Linear scan for a channel-matching entry in a parsed manifest
 * - findAssetForBoard(): Linear scan for a board-matching asset in a parsed channel
 */

#pragma once

#include <cstdint>
#include <cstddef>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr size_t kMaxBoards        = 4;     ///< Maximum number of board entries per channel
static constexpr size_t kMaxChannels      = 4;     ///< Maximum number of channels (dev, beta, rc, stable)
static constexpr size_t kMaxUrlLen        = 256;   ///< Maximum length for absolute URL fields
static constexpr size_t kMaxBoardIdLen    = 24;    ///< Maximum length for board ID strings
static constexpr size_t kMaxSha256Len     = 65;    ///< SHA-256 hex string length (64 hex chars + null)
static constexpr size_t kMaxVersionTagLen = 32;    ///< Maximum length for version tag strings

// ---------------------------------------------------------------------------
// OTAAsset — per-board firmware asset descriptor
// ---------------------------------------------------------------------------

/**
 * @brief Describes a single firmware asset for a specific board.
 *
 * Maps a board identifier to its firmware binary URL, signature URL,
 * binary size, and SHA-256 checksum. All fields are fixed-size char
 * arrays for stack allocation. URLs are absolute — no URL construction
 * needed on-device.
 */
struct OTAAsset {
    char     board[kMaxBoardIdLen];       ///< Board identifier (e.g. "esp32dev")
    char     firmwareUrl[kMaxUrlLen];     ///< Absolute firmware binary URL
    char     signatureUrl[kMaxUrlLen];    ///< Absolute firmware signature URL
    uint32_t size;                        ///< Firmware binary size in bytes
    char     sha256[kMaxSha256Len];       ///< SHA-256 hex digest of the firmware binary
};

// ---------------------------------------------------------------------------
// OTAChannel — per-channel release descriptor
// ---------------------------------------------------------------------------

/**
 * @brief Describes a single release channel within the manifest.
 *
 * Each channel (e.g. "stable", "dev", "beta", "rc") contains its own
 * version tag, security version counter, and an array of per-board
 * OTAAsset entries. The channel name is copied from the JSON object key.
 */
struct OTAChannel {
    char     version[kMaxVersionTagLen];  ///< Release version tag for this channel (e.g. "v3.1.0")
    char     name[16];                    ///< Channel name (e.g. "stable", "dev", "beta", "rc")
    uint32_t securityVersion;             ///< Anti-rollback security version counter
    OTAAsset assets[kMaxBoards];          ///< Per-board asset descriptors
    uint8_t  assetCount;                  ///< Number of valid entries in the assets array
};

// ---------------------------------------------------------------------------
// OTAManifest — top-level manifest container (v2)
// ---------------------------------------------------------------------------

/**
 * @brief Top-level manifest describing all firmware channels and assets.
 *
 * Parsed from the v2 manifest.json. Contains the schema version, a
 * self-referencing manifest URL (for future migration), and an array
 * of OTAChannel entries keyed by channel name.
 *
 * Buffer sizing: the v2 manifest with 4 channels × 4 boards requires
 * approximately 4 KB. The static parse buffer should be at least 4096 bytes.
 */
struct OTAManifest {
    uint8_t    manifestVersion;           ///< Schema version (currently 1)
    char       manifestUrl[kMaxUrlLen];   ///< Self-referencing URL for future migration
    OTAChannel channels[kMaxChannels];    ///< Per-channel release descriptors
    uint8_t    channelCount;              ///< Number of valid entries in the channels array
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Parse a raw JSON buffer into an OTAManifest struct (v2 schema).
 *
 * Uses ArduinoJson internally (dependency hidden from the header). Parses
 * the v2 manifest structure: reads manifestVersion, manifestUrl, then
 * iterates the "channels" object (keyed by channel name). For each channel,
 * populates an OTAChannel entry with version, securityVersion, and assets.
 * Copies all string fields with strlcpy to ensure null termination. If the
 * JSON contains more than kMaxChannels channels or kMaxBoards assets per
 * channel, excess entries are silently capped.
 *
 * @param json    Pointer to the raw JSON character buffer.
 * @param jsonLen Length of the JSON buffer in bytes.
 * @param out     Reference to the OTAManifest struct to populate.
 * @return true on successful parse, false on JSON error or missing fields.
 */
bool parseManifest(const char* json, size_t jsonLen, OTAManifest& out);

/**
 * @brief Find the channel entry matching a given channel name.
 *
 * Performs a linear scan of the manifest's channels array, comparing each
 * entry's name field to the provided channelName using strcmp().
 *
 * @param manifest Reference to a parsed OTAManifest.
 * @param channelName Null-terminated channel name string to search for.
 * @return Pointer to the matching OTAChannel within the manifest, or nullptr if not found.
 */
const OTAChannel* findChannel(const OTAManifest& manifest, const char* channelName);

/**
 * @brief Find the asset entry matching a given board ID within a channel.
 *
 * Performs a linear scan of the channel's assets array, comparing each
 * entry's board field to the provided boardId using strcmp().
 *
 * @param channel  Reference to a parsed OTAChannel.
 * @param boardId  Null-terminated board identifier string to search for.
 * @return Pointer to the matching OTAAsset within the channel, or nullptr if not found.
 */
const OTAAsset* findAssetForBoard(const OTAChannel& channel, const char* boardId);
