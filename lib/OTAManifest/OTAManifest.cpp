/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Library: lib/OTAManifest/OTAManifest.cpp
 * Description: Implementation of the multi-architecture OTA manifest parser.
 *              Uses ArduinoJson to deserialise manifest.json and populates
 *              stack-allocated OTAManifest/OTAAsset structs. No heap allocation
 *              beyond ArduinoJson's own document pool.
 *
 * Exported Functions:
 * - parseManifest():     Deserialise raw JSON into an OTAManifest struct
 * - findAssetForBoard(): Linear scan for a board-matching asset
 */

#include "OTAManifest.hpp"
#include <ArduinoJson.h>
#include <cstring>

/**
 * @brief Parse a raw JSON buffer into an OTAManifest struct.
 *
 * Allocates a JsonDocument on the stack (~1024 bytes). Iterates the
 * "assets" array and copies up to kMaxBoards entries into the output
 * struct using strlcpy for safe null-terminated string copies.
 *
 * @param json    Pointer to the raw JSON buffer.
 * @param jsonLen Length of the JSON buffer in bytes.
 * @param out     Reference to the OTAManifest struct to populate.
 * @return true on success, false on parse error or missing required fields.
 */
bool parseManifest(const char* json, size_t jsonLen, OTAManifest& out) {
    // Zero the output struct to ensure clean state
    memset(&out, 0, sizeof(OTAManifest));

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json, jsonLen);
    if (err) {
        return false;
    }

    // --- Copy top-level fields ---
    const char* version = doc["version"] | "";
    strlcpy(out.version, version, sizeof(out.version));

    const char* channel = doc["channel"] | "";
    strlcpy(out.channel, channel, sizeof(out.channel));

    out.securityVersion = doc["securityVersion"] | 0u;

    // --- Iterate assets array ---
    JsonArray assets = doc["assets"];
    if (assets.isNull()) {
        // No assets key or null — valid manifest with zero assets
        out.assetCount = 0;
        return true;
    }

    uint8_t count = 0;
    for (JsonObject asset : assets) {
        if (count >= kMaxBoards) {
            break;  // Silently cap at kMaxBoards — no overflow
        }

        OTAAsset& entry = out.assets[count];

        const char* board     = asset["board"]     | "";
        const char* firmware  = asset["firmware"]  | "";
        const char* signature = asset["signature"] | "";
        const char* sha256    = asset["sha256"]    | "";

        strlcpy(entry.board,     board,     sizeof(entry.board));
        strlcpy(entry.firmware,  firmware,  sizeof(entry.firmware));
        strlcpy(entry.signature, signature, sizeof(entry.signature));
        strlcpy(entry.sha256,    sha256,    sizeof(entry.sha256));

        entry.size = asset["size"] | 0u;

        count++;
    }

    out.assetCount = count;
    return true;
}

/**
 * @brief Find the asset entry matching a given board ID.
 *
 * Linear scan through the manifest's populated asset entries.
 *
 * @param manifest Reference to a parsed OTAManifest.
 * @param boardId  Null-terminated board identifier to match.
 * @return Pointer to the matching OTAAsset, or nullptr if not found.
 */
const OTAAsset* findAssetForBoard(const OTAManifest& manifest, const char* boardId) {
    for (uint8_t i = 0; i < manifest.assetCount; i++) {
        if (strcmp(manifest.assets[i].board, boardId) == 0) {
            return &manifest.assets[i];
        }
    }
    return nullptr;
}
