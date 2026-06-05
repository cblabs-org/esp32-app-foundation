/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Library: lib/OTAManifest/OTAManifest.hpp
 * Description: Multi-architecture OTA manifest parser. Defines the schema for
 *              manifest.json files shipped alongside GitHub Release assets.
 *              Each manifest maps board IDs to their firmware binary, signature
 *              file, size, and SHA-256 hash. All storage is stack-allocated
 *              using fixed-size char arrays — no heap, no Arduino String.
 *
 * Exported Symbols:
 * - OTAAsset:           Per-board firmware asset descriptor (board, firmware, signature, size, sha256)
 * - OTAManifest:        Top-level manifest with version, channel, securityVersion, and asset array
 * - parseManifest():    Parse raw JSON into an OTAManifest struct
 * - findAssetForBoard(): Linear scan for a board-matching asset in a parsed manifest
 */

#pragma once

#include <cstdint>
#include <cstddef>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr size_t kMaxBoards        = 4;    ///< Maximum number of board entries in a manifest
static constexpr size_t kMaxFilenameLen   = 64;   ///< Maximum length for firmware/signature filenames
static constexpr size_t kMaxBoardIdLen    = 24;   ///< Maximum length for board ID strings
static constexpr size_t kMaxSha256Len     = 65;   ///< SHA-256 hex string length (64 hex chars + null)
static constexpr size_t kMaxVersionTagLen = 32;   ///< Maximum length for version tag strings

// ---------------------------------------------------------------------------
// OTAAsset — per-board firmware asset descriptor
// ---------------------------------------------------------------------------

/**
 * @brief Describes a single firmware asset for a specific board.
 *
 * Maps a board identifier to its firmware binary filename, signature
 * filename, binary size, and SHA-256 checksum. All fields are fixed-size
 * char arrays for stack allocation.
 */
struct OTAAsset {
    char     board[kMaxBoardIdLen];       ///< Board identifier (e.g. "esp32dev")
    char     firmware[kMaxFilenameLen];   ///< Firmware binary filename (e.g. "firmware-esp32dev.bin")
    char     signature[kMaxFilenameLen];  ///< Signature filename (e.g. "firmware-esp32dev.bin.sig")
    uint32_t size;                        ///< Firmware binary size in bytes
    char     sha256[kMaxSha256Len];       ///< SHA-256 hex digest of the firmware binary
};

// ---------------------------------------------------------------------------
// OTAManifest — top-level manifest container
// ---------------------------------------------------------------------------

/**
 * @brief Top-level manifest describing all firmware assets in a GitHub Release.
 *
 * Parsed from the manifest.json asset in a release. Contains the release
 * version tag, channel, security version counter, and an array of per-board
 * OTAAsset entries.
 */
struct OTAManifest {
    char     version[kMaxVersionTagLen];  ///< Release version tag (e.g. "v3.1.0")
    char     channel[16];                 ///< Release channel (e.g. "stable", "beta")
    uint32_t securityVersion;             ///< Anti-rollback security version counter
    OTAAsset assets[kMaxBoards];          ///< Per-board asset descriptors
    uint8_t  assetCount;                  ///< Number of valid entries in the assets array
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Parse a raw JSON buffer into an OTAManifest struct.
 *
 * Uses ArduinoJson internally (dependency hidden from the header). Copies
 * all string fields with strlcpy to ensure null termination. If the JSON
 * contains more than kMaxBoards assets, only the first kMaxBoards are stored.
 *
 * @param json    Pointer to the raw JSON character buffer.
 * @param jsonLen Length of the JSON buffer in bytes.
 * @param out     Reference to the OTAManifest struct to populate.
 * @return true on successful parse, false on JSON error or missing fields.
 */
bool parseManifest(const char* json, size_t jsonLen, OTAManifest& out);

/**
 * @brief Find the asset entry matching a given board ID.
 *
 * Performs a linear scan of the manifest's assets array, comparing each
 * entry's board field to the provided boardId using strcmp().
 *
 * @param manifest Reference to a parsed OTAManifest.
 * @param boardId  Null-terminated board identifier string to search for.
 * @return Pointer to the matching OTAAsset within the manifest, or nullptr if not found.
 */
const OTAAsset* findAssetForBoard(const OTAManifest& manifest, const char* boardId);
