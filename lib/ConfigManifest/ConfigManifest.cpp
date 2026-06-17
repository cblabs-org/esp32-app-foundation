/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Library: lib/ConfigManifest/ConfigManifest.cpp
 * Description: Implementation of the Three-Tier config manifest. Handles JSON
 *              serialization/deserialization of the manifest file and provides
 *              the slot management logic for Active/Previous/Golden tiers.
 *
 * Exported Classes:
 * - ConfigManifest: see ConfigManifest.hpp
 */

#include "ConfigManifest.hpp"
#include <ArduinoJson.h>
#include "platform/LogShim.h"

#ifndef NATIVE_BUILD
#include <LittleFS.h>
#else
#include <fstream>
#include <sstream>
#endif

static constexpr const char* TAG = "ConfigManifest";

/// Manifest file path on LittleFS (hardware) or working directory (native).
#ifndef NATIVE_BUILD
static constexpr const char* kManifestPath = "/config_manifest.json";
#else
static constexpr const char* kManifestPath = "test/tmp/config_manifest.json";
#endif

// =============================================================================
// Persistence
// =============================================================================

/**
 * @brief Load the manifest from the JSON file.
 *
 * Parses the "modules" array from the manifest file and populates the
 * internal slots. Returns true on success or if the file does not exist
 * (first boot scenario — starts with an empty manifest).
 *
 * @return true if loaded or file not found; false on parse error.
 */
bool ConfigManifest::load() {
    _slotCount = 0;
    memset(_slots, 0, sizeof(_slots));

    JsonDocument doc;

#ifndef NATIVE_BUILD
    if (!LittleFS.exists(kManifestPath)) {
        ESP_LOGI(TAG, "Manifest file not found — first boot, starting empty");
        return true;
    }

    File file = LittleFS.open(kManifestPath, "r");
    if (!file) {
        ESP_LOGW(TAG, "Failed to open %s for reading", kManifestPath);
        return false;
    }

    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        ESP_LOGE(TAG, "Failed to parse %s: %s", kManifestPath, err.c_str());
        return false;
    }
#else
    std::ifstream file(kManifestPath);
    if (!file.is_open()) {
        ESP_LOGI(TAG, "Manifest file not found — first boot, starting empty");
        return true;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();
    file.close();

    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) {
        ESP_LOGE(TAG, "Failed to parse %s: %s", kManifestPath, err.c_str());
        return false;
    }
#endif

    // Parse the "modules" array
    JsonArray modules = doc["modules"].as<JsonArray>();
    if (modules.isNull()) {
        ESP_LOGW(TAG, "No 'modules' array in manifest — starting empty");
        return true;
    }

    for (JsonObject entry : modules) {
        if (_slotCount >= kMaxModules) {
            ESP_LOGW(TAG, "Manifest truncated — exceeds kMaxModules (%d)", (int)kMaxModules);
            break;
        }

        const char* ns = entry["ns"] | "";
        if (ns[0] == '\0') continue;

        ModuleSlot& slot = _slots[_slotCount];
        strlcpy(slot.ns, ns, kMaxNamespaceLen);
        slot.activeVersion   = entry["active"]   | 0;
        slot.previousVersion = entry["previous"] | 0;
        slot.goldenVersion   = entry["golden"]   | 0;
        _slotCount++;
    }

    ESP_LOGI(TAG, "Loaded manifest with %d module slot(s)", (int)_slotCount);
    return true;
}

/**
 * @brief Save the manifest to the JSON file.
 *
 * Serializes all tracked module slots into a "modules" JSON array
 * and writes the file. Creates the file if it does not exist.
 *
 * @return true if saved successfully; false on write error.
 */
bool ConfigManifest::save() const {
    JsonDocument doc;
    JsonArray modules = doc["modules"].to<JsonArray>();

    for (uint8_t i = 0; i < _slotCount; i++) {
        const ModuleSlot& slot = _slots[i];
        JsonObject entry = modules.add<JsonObject>();
        entry["ns"]       = slot.ns;
        entry["active"]   = slot.activeVersion;
        entry["previous"] = slot.previousVersion;
        entry["golden"]   = slot.goldenVersion;
    }

#ifndef NATIVE_BUILD
    File file = LittleFS.open(kManifestPath, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open %s for writing", kManifestPath);
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        ESP_LOGE(TAG, "Failed to write manifest to %s", kManifestPath);
        file.close();
        return false;
    }
    file.close();
#else
    std::ofstream file(kManifestPath);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open %s for writing", kManifestPath);
        return false;
    }
    std::string jsonStr;
    serializeJson(doc, jsonStr);
    file << jsonStr;
    file.close();
#endif

    ESP_LOGI(TAG, "Manifest saved with %d module slot(s)", (int)_slotCount);
    return true;
}

// =============================================================================
// Slot Management
// =============================================================================

/**
 * @brief Find an existing slot by namespace, or create a new one.
 *
 * Linear search by strcmp(). If not found and _slotCount < kMaxModules,
 * creates a new slot with zeroed versions.
 *
 * @param ns Module namespace string.
 * @return Pointer to the matching or new slot, or nullptr if full.
 */
ModuleSlot* ConfigManifest::findOrCreate(const char* ns) {
    if (ns == nullptr || ns[0] == '\0') {
        ESP_LOGW(TAG, "findOrCreate() called with null/empty namespace");
        return nullptr;
    }

    // Search existing slots
    for (uint8_t i = 0; i < _slotCount; i++) {
        if (strcmp(_slots[i].ns, ns) == 0) {
            return &_slots[i];
        }
    }

    // Create new slot if capacity remains
    if (_slotCount >= kMaxModules) {
        ESP_LOGE(TAG, "Cannot create slot for '%s' — manifest full (%d/%d)",
                 ns, (int)_slotCount, (int)kMaxModules);
        return nullptr;
    }

    ModuleSlot& slot = _slots[_slotCount];
    memset(&slot, 0, sizeof(ModuleSlot));
    strlcpy(slot.ns, ns, kMaxNamespaceLen);
    _slotCount++;

    ESP_LOGD(TAG, "Created manifest slot for '%s' (slot %d)", ns, (int)_slotCount);
    return &slot;
}

// =============================================================================
// Update Methods
// =============================================================================

/**
 * @brief Set the Active schema version for a module namespace.
 *
 * @param ns            Module namespace string.
 * @param schemaVersion The schema version to record.
 */
void ConfigManifest::updateActive(const char* ns, int schemaVersion) {
    ModuleSlot* slot = findOrCreate(ns);
    if (slot != nullptr) {
        slot->activeVersion = schemaVersion;
    }
}

/**
 * @brief Set the Previous schema version for a module namespace.
 *
 * @param ns            Module namespace string.
 * @param schemaVersion The schema version to record.
 */
void ConfigManifest::updatePrevious(const char* ns, int schemaVersion) {
    ModuleSlot* slot = findOrCreate(ns);
    if (slot != nullptr) {
        slot->previousVersion = schemaVersion;
    }
}

/**
 * @brief Set the Golden schema version for a module namespace.
 *
 * @param ns            Module namespace string.
 * @param schemaVersion The schema version to record.
 */
void ConfigManifest::updateGolden(const char* ns, int schemaVersion) {
    ModuleSlot* slot = findOrCreate(ns);
    if (slot != nullptr) {
        slot->goldenVersion = schemaVersion;
    }
}

// =============================================================================
// Query Methods
// =============================================================================

/**
 * @brief Get the full ModuleSlot for a given namespace.
 *
 * @param ns Module namespace string.
 * @return Pointer to the matching ModuleSlot, or nullptr if not found.
 */
const ModuleSlot* ConfigManifest::getSlot(const char* ns) const {
    if (ns == nullptr) return nullptr;

    for (uint8_t i = 0; i < _slotCount; i++) {
        if (strcmp(_slots[i].ns, ns) == 0) {
            return &_slots[i];
        }
    }
    return nullptr;
}

/**
 * @brief Get the Active schema version for a namespace.
 * @return Active version, or -1 if not found.
 */
int ConfigManifest::getActiveVersion(const char* ns) const {
    const ModuleSlot* slot = getSlot(ns);
    return slot ? slot->activeVersion : -1;
}

/**
 * @brief Get the Previous schema version for a namespace.
 * @return Previous version, or -1 if not found.
 */
int ConfigManifest::getPreviousVersion(const char* ns) const {
    const ModuleSlot* slot = getSlot(ns);
    return slot ? slot->previousVersion : -1;
}

/**
 * @brief Get the Golden schema version for a namespace.
 * @return Golden version, or -1 if not found.
 */
int ConfigManifest::getGoldenVersion(const char* ns) const {
    const ModuleSlot* slot = getSlot(ns);
    return slot ? slot->goldenVersion : -1;
}

// =============================================================================
// Snapshot Operations
// =============================================================================

/**
 * @brief Copy all Active versions to Previous for every tracked slot.
 *
 * Iterates all slots and copies activeVersion → previousVersion.
 * Called before an OTA download to preserve the pre-update config state.
 */
void ConfigManifest::snapshotActiveToPrevious() {
    for (uint8_t i = 0; i < _slotCount; i++) {
        _slots[i].previousVersion = _slots[i].activeVersion;
    }
    ESP_LOGI(TAG, "Snapshot: Active → Previous for %d slot(s)", (int)_slotCount);
}

/**
 * @brief Copy all Active versions to Golden for every tracked slot.
 *
 * Iterates all slots and copies activeVersion → goldenVersion.
 * Called after firmware is blessed to record the last-known-good state.
 */
void ConfigManifest::snapshotActiveToGolden() {
    for (uint8_t i = 0; i < _slotCount; i++) {
        _slots[i].goldenVersion = _slots[i].activeVersion;
    }
    ESP_LOGI(TAG, "Snapshot: Active → Golden for %d slot(s)", (int)_slotCount);
}
