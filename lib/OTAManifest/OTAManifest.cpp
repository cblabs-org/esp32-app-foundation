/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Library: lib/OTAManifest/OTAManifest.cpp
 * Description: Implementation of the multi-channel OTA manifest parser (v2).
 *              Uses ArduinoJson to deserialise the v2 manifest.json schema
 *              and populates stack-allocated OTAManifest/OTAChannel/OTAAsset
 *              structs. No heap allocation beyond ArduinoJson's own document
 *              pool.
 *
 *              The v2 manifest with 4 channels × 4 boards requires approximately
 *              4 KB. The JsonDocument should be sized to at least 4096 bytes.
 *
 * Exported Functions:
 * - parseManifest():     Deserialise raw JSON into an OTAManifest struct (v2 schema)
 * - findChannel():       Linear scan for a channel-matching entry
 * - findAssetForBoard(): Linear scan for a board-matching asset within a channel
 */

#include "OTAManifest.hpp"
#include <ArduinoJson.h>
#include <cstring>

/**
 * @brief Parse a raw JSON buffer into an OTAManifest struct (v2 schema).
 *
 * Allocates a JsonDocument on the stack. Reads top-level fields
 * (manifestVersion, manifestUrl), then iterates the "channels" object.
 * Each channel key becomes an OTAChannel entry with version,
 * securityVersion, channel name, and its assets array. Copies up to
 * kMaxChannels channels and kMaxBoards assets per channel using strlcpy
 * for safe null-terminated string copies.
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

    // --- Top-level fields ---
    out.manifestVersion = doc["manifestVersion"] | 0u;

    const char* manifestUrl = doc["manifestUrl"] | "";
    strlcpy(out.manifestUrl, manifestUrl, sizeof(out.manifestUrl));

    // --- Iterate channels object ---
    JsonObject channels = doc["channels"];
    if (channels.isNull()) {
        // No channels key or null — valid manifest with zero channels
        out.channelCount = 0;
        return true;
    }

    uint8_t channelIdx = 0;
    for (JsonPair kv : channels) {
        if (channelIdx >= kMaxChannels) {
            break;  // Silently cap at kMaxChannels — no overflow
        }

        OTAChannel& ch = out.channels[channelIdx];

        // Channel name comes from the JSON object key
        strlcpy(ch.name, kv.key().c_str(), sizeof(ch.name));

        JsonObject channelObj = kv.value();

        const char* version = channelObj["version"] | "";
        strlcpy(ch.version, version, sizeof(ch.version));

        ch.securityVersion = channelObj["securityVersion"] | 0u;

        // --- Iterate assets array within this channel ---
        JsonArray assets = channelObj["assets"];
        uint8_t assetIdx = 0;
        if (!assets.isNull()) {
            for (JsonObject asset : assets) {
                if (assetIdx >= kMaxBoards) {
                    break;  // Silently cap at kMaxBoards — no overflow
                }

                OTAAsset& entry = ch.assets[assetIdx];

                const char* board        = asset["board"]        | "";
                const char* firmwareUrl  = asset["firmwareUrl"]  | "";
                const char* signatureUrl = asset["signatureUrl"] | "";
                const char* sha256       = asset["sha256"]       | "";

                strlcpy(entry.board,        board,        sizeof(entry.board));
                strlcpy(entry.firmwareUrl,   firmwareUrl,  sizeof(entry.firmwareUrl));
                strlcpy(entry.signatureUrl,  signatureUrl, sizeof(entry.signatureUrl));
                strlcpy(entry.sha256,        sha256,       sizeof(entry.sha256));

                entry.size = asset["size"] | 0u;

                assetIdx++;
            }
        }
        ch.assetCount = assetIdx;

        channelIdx++;
    }

    out.channelCount = channelIdx;
    return true;
}

/**
 * @brief Find the channel entry matching a given channel name.
 *
 * Linear scan through the manifest's populated channel entries.
 *
 * @param manifest Reference to a parsed OTAManifest.
 * @param channelName Null-terminated channel name to match.
 * @return Pointer to the matching OTAChannel, or nullptr if not found.
 */
const OTAChannel* findChannel(const OTAManifest& manifest, const char* channelName) {
    for (uint8_t i = 0; i < manifest.channelCount; i++) {
        if (strcmp(manifest.channels[i].name, channelName) == 0) {
            return &manifest.channels[i];
        }
    }
    return nullptr;
}

/**
 * @brief Find the asset entry matching a given board ID within a channel.
 *
 * Linear scan through the channel's populated asset entries.
 *
 * @param channel Reference to a parsed OTAChannel.
 * @param boardId Null-terminated board identifier to match.
 * @return Pointer to the matching OTAAsset, or nullptr if not found.
 */
const OTAAsset* findAssetForBoard(const OTAChannel& channel, const char* boardId) {
    for (uint8_t i = 0; i < channel.assetCount; i++) {
        if (strcmp(channel.assets[i].board, boardId) == 0) {
            return &channel.assets[i];
        }
    }
    return nullptr;
}
