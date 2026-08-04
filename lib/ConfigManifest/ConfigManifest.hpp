/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Library: lib/ConfigManifest/ConfigManifest.hpp
 * Description: Three-Tier config lifecycle manifest that tracks Active, Previous,
 *              and Golden schema versions per module namespace. The manifest is
 *              persisted as /config_manifest.json and operates alongside the
 *              per-module config files at /mod/<namespace>.json.
 *
 *              - Active:   the currently running config schema version
 *              - Previous: the config schema version before the last OTA update
 *              - Golden:   the last-known-good config (snapshotted when firmware is blessed)
 *
 * Exported Classes:
 * - ModuleSlot:      Per-module slot tracking Active/Previous/Golden schema versions
 * - ConfigManifest:  Manifest manager for the Three-Tier config lifecycle
 *
 *   ConfigManifest API:
 *   - load():                      Load manifest from /config_manifest.json
 *   - save():                      Save manifest to /config_manifest.json
 *   - updateActive():              Set the Active schema version for a namespace
 *   - updatePrevious():            Set the Previous schema version for a namespace
 *   - updateGolden():              Set the Golden schema version for a namespace
 *   - getSlot():                   Get the full ModuleSlot for a namespace
 *   - getActiveVersion():          Get the Active schema version for a namespace
 *   - getPreviousVersion():        Get the Previous schema version for a namespace
 *   - getGoldenVersion():          Get the Golden schema version for a namespace
 *   - snapshotActiveToPrevious():  Copy all Active → Previous slots
 *   - snapshotActiveToGolden():    Copy all Active → Golden slots
 */

#pragma once

#include <cstdint>
#include <cstring>

/// Maximum number of module namespaces the manifest can track.
static constexpr size_t kMaxModules = 16;

/// Maximum length of a module namespace string (including null terminator).
static constexpr size_t kMaxNamespaceLen = 24;

/**
 * @brief Per-module slot tracking the Three-Tier config lifecycle versions.
 *
 * Each slot records the schema version in the Active, Previous, and Golden
 * tiers for a single module namespace. A version of 0 indicates "not yet set".
 */
struct ModuleSlot {
    char ns[kMaxNamespaceLen];   ///< Module namespace (e.g., "ota", "wifi")
    int activeVersion;           ///< Schema version of the Active config
    int previousVersion;         ///< Schema version before last OTA
    int goldenVersion;           ///< Schema version of last-known-good config
};

/**
 * @brief Manifest manager for the Three-Tier config lifecycle.
 *
 * Tracks Active, Previous, and Golden schema versions for up to kMaxModules
 * module namespaces. Persisted as /config_manifest.json on LittleFS (hardware)
 * or config_manifest.json in the working directory (native builds).
 *
 * This manifest does NOT replace per-module config files — it operates alongside
 * them to enable config rollback and pruning of orphaned config files.
 */
class ConfigManifest {
public:
    /**
     * @brief Load the manifest from /config_manifest.json.
     *
     * On hardware: reads from LittleFS. On native: reads from the working
     * directory. Returns false gracefully if the file does not exist (first
     * boot scenario).
     *
     * @return true if loaded successfully or file not found; false on parse error.
     */
    bool load();

    /**
     * @brief Save the manifest to /config_manifest.json.
     *
     * Serializes all tracked module slots to JSON and writes the file.
     *
     * @return true if saved successfully; false on write error.
     */
    bool save() const;

    /**
     * @brief Set the Active schema version for a module namespace.
     *
     * Creates a new slot if the namespace is not yet tracked (up to kMaxModules).
     *
     * @param ns            Module namespace string (e.g., "ota").
     * @param schemaVersion The schema version to record.
     */
    void updateActive(const char* ns, int schemaVersion);

    /**
     * @brief Set the Previous schema version for a module namespace.
     *
     * Creates a new slot if the namespace is not yet tracked (up to kMaxModules).
     *
     * @param ns            Module namespace string.
     * @param schemaVersion The schema version to record.
     */
    void updatePrevious(const char* ns, int schemaVersion);

    /**
     * @brief Set the Golden schema version for a module namespace.
     *
     * Creates a new slot if the namespace is not yet tracked (up to kMaxModules).
     *
     * @param ns            Module namespace string.
     * @param schemaVersion The schema version to record.
     */
    void updateGolden(const char* ns, int schemaVersion);

    /**
     * @brief Get the full ModuleSlot for a given namespace.
     *
     * @param ns Module namespace string.
     * @return Pointer to the matching ModuleSlot, or nullptr if not found.
     */
    const ModuleSlot* getSlot(const char* ns) const;

    /**
     * @brief Get the Active schema version for a namespace.
     * @param ns Module namespace string.
     * @return Active version, or -1 if the namespace is not tracked.
     */
    int getActiveVersion(const char* ns) const;

    /**
     * @brief Get the Previous schema version for a namespace.
     * @param ns Module namespace string.
     * @return Previous version, or -1 if the namespace is not tracked.
     */
    int getPreviousVersion(const char* ns) const;

    /**
     * @brief Get the Golden schema version for a namespace.
     * @param ns Module namespace string.
     * @return Golden version, or -1 if the namespace is not tracked.
     */
    int getGoldenVersion(const char* ns) const;

    /**
     * @brief Copy all Active versions to Previous for every tracked slot.
     *
     * Called before an OTA download begins to preserve the pre-update state.
     */
    void snapshotActiveToPrevious();

    /**
     * @brief Copy all Active versions to Golden for every tracked slot.
     *
     * Called after firmware is blessed (markAppValid) to record the
     * last-known-good configuration state.
     */
    void snapshotActiveToGolden();

private:
    ModuleSlot _slots[kMaxModules] = {};   ///< Fixed-size array of module slots
    uint8_t _slotCount = 0;                ///< Number of active slots

    /**
     * @brief Find an existing slot by namespace, or create a new one.
     *
     * Linear search by strcmp(). If not found and capacity remains, creates
     * a new slot with zeroed versions.
     *
     * @param ns Module namespace string.
     * @return Pointer to the matching or new slot, or nullptr if full.
     */
    ModuleSlot* findOrCreate(const char* ns);
};
