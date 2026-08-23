/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/KeyManager/KeyManager.hpp
 * Description: Kernel service that provides an encrypted registry for storing
 *              and retrieving sensitive credentials (e.g. API keys).
 *              Each key entry carries optional ownership metadata
 *              (managedBy / exclusive) so the REST API and portal UI
 *              can enforce write-protection for module-managed keys.
 *              Implements IWebContributor for REST API routes under
 *              System → Keys sub-pill. Exposes GET/POST/DELETE /api/keys
 *              endpoints; GET never returns plaintext values.
 *
 * Exported Functions/Classes:
 * - KeyInfo: [Struct] Metadata descriptor returned by listKeysWithMetadata().
 * - KeyManager: [Class] Service provider for secure credential storage.
 *
 *   Lifecycle:
 *   - begin():                Initializes the KeyManager with a dependency-injected DeviceCrypto instance.
 *
 *   Credential Registry:
 *   - addKey():               Encrypts and stores a credential with optional ownership metadata.
 *   - getKeyById():           Retrieves and decrypts a credential.
 *   - deleteKey():            Removes a credential from the registry.
 *   - listKeys():             Returns a list of stored key IDs (no values).
 *   - listKeysWithMetadata(): Returns KeyInfo structs with id, hasValue, managedBy, exclusive.
 *   - isExclusive():          Checks whether a key is exclusively owned by a module.
 *
 *   IWebContributor (tab: "Keys", parent: "System", order: 20):
 *   - registerRoutes():       Register REST API routes
 *   - getTabName():           Return "Keys" for portal tab
 *   - getParentTab():         Return "System" for navigation hierarchy
 *   - getTabOrder():          Return 20 for pill ordering
 */

#pragma once

#include <Arduino.h>
#include <string>
#include <stdint.h>
#include <memory>
#include <vector>
#include <ArduinoJson.h>
#include "DeviceCrypto.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "ConfigRoot.hpp"

#define KEY_MANAGER_PATH (ConfigRoot::getEtcFilePath("keys.json").c_str())

/**
 * @brief Metadata descriptor for a single key entry.
 *
 * Returned by KeyManager::listKeysWithMetadata() for the GET /api/keys
 * response. Contains the key ID, whether a value exists, and optional
 * ownership attributes (managedBy / exclusive / systemLocked).
 */
struct KeyInfo {
    std::string id;        ///< Unique key identifier.
    bool hasValue;         ///< True if an encrypted value is stored.
    std::string managedBy; ///< Owning module name (empty if user-created).
    bool exclusive;        ///< True if the key is write-protected by its owner.
    bool systemLocked;     ///< True if the key is system-locked (un-bypassable over MCP/REST).
};

/**
 * @brief Kernel service providing an encrypted registry for credentials.
 *
 * Implements IWebContributor to register REST API routes for managing keys
 * via the web portal. Implements IMCPContributor to expose MCP tools over JSON-RPC.
 * GET /api/keys returns key metadata (never plaintext values). Keys may
 * carry ownership metadata (managedBy / exclusive / systemLocked) so the API and MCP server
 * can enforce write-protection for module-managed entries.
 */
class KeyManager : public IWebContributor, public IMCPContributor {
private:
    DeviceCrypto* _crypto = nullptr; ///< Pointer to injected DeviceCrypto service.
    
    /**
     * @brief Loads the JSON registry from the filesystem into a JsonDocument.
     * @param doc The JsonDocument to populate.
     * @return true if successful or if file doesn't exist yet, false on read error.
     */
    bool loadRegistry(JsonDocument& doc) const;
    
    /**
     * @brief Saves the populated JsonDocument to the filesystem.
     * @param doc The JsonDocument to save.
     * @return true on success, false on write error.
     */
    bool saveRegistry(const JsonDocument& doc) const;

public:
    KeyManager() = default;
    ~KeyManager() = default;

    /**
     * @brief Initializes the KeyManager.
     * @param crypto Pointer to the DeviceCrypto service used for encryption/decryption.
     */
    void begin(DeviceCrypto* crypto);

    /**
     * @brief Encrypts and stores a new credential in the registry.
     * @param id The unique identifier for the key.
     * @param secret The plaintext credential to store.
     * @param managedBy Optional owning module name (e.g. "WiFi", "Security").
     * @param exclusive If true, the key is write-protected via the REST API.
     * @param systemLocked If true, the key is un-bypassable over MCP/REST.
     * @return true if successfully stored, false on failure.
     */
    bool addKey(const char* id, const char* secret,
                const char* managedBy = nullptr, bool exclusive = false,
                bool systemLocked = false);

    /**
     * @brief Retrieves and decrypts a credential from the registry.
     * @param id The unique identifier for the key.
     * @return A standard string containing the decrypted plaintext. Empty if not found or error.
     */
    std::string getKeyById(const char* id) const;

    /**
     * @brief Deletes a credential from the registry.
     * @param id The unique identifier for the key.
     * @return true if successfully removed, false if not found or error.
     */
    bool deleteKey(const char* id);

    /**
     * @brief List all stored key IDs (never exposes plaintext values).
     * @return Vector of key ID strings.
     */
    std::vector<std::string> listKeys() const;

    /**
     * @brief List all stored keys with full ownership metadata.
     *
     * Used by the GET /api/keys endpoint to provide the portal UI with
     * managedBy / exclusive / systemLocked attributes for badge rendering.
     *
     * @return Vector of KeyInfo structs (empty if registry is empty or unreadable).
     */
    std::vector<KeyInfo> listKeysWithMetadata() const;

    /**
     * @brief Check whether a key is exclusively owned by a module.
     * @param id The unique identifier for the key.
     * @return true if the key exists and has exclusive=true, false otherwise.
     */
    bool isExclusive(const char* id) const;

    /**
     * @brief Get the owner (managedBy module name) of a stored key.
     * @param id The unique identifier for the key.
     * @return The managedBy string, or empty string if not set or key not found.
     */
    std::string getOwner(const char* id) const;

    /**
     * @brief Check whether a key is system-locked.
     * @param id The unique identifier for the key.
     * @return true if the key exists and has systemLocked=true, false otherwise.
     */
    bool isSystemLocked(const char* id) const;

    /**
     * @brief Retrieves the metadata for a single key without returning its plaintext value.
     * @param id The unique identifier for the key.
     * @param outInfo Reference to a KeyInfo struct to populate.
     * @return true if the key exists and metadata was populated, false otherwise.
     */
    bool getKeyMetadata(const char* id, KeyInfo& outInfo) const;

    // -------------------------------------------------------------------------
    // IMCPContributor Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Register key management MCP tools with the tool registry.
     * @param registry Reference to the MCPToolRegistry.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

    // -------------------------------------------------------------------------
    // IWebContributor Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Register all REST API route handlers with the web server.
     * @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /**
     * @brief Register the Keys tab in the System sub-navigation.
     * @param registry  The TabRegistry to register tabs with.
     */
    void registerTabs(TabRegistry& registry) override {
        registry.addTab("Keys", "System", 20);
    }

    /**
     * @brief Return the API path prefix for body-size middleware matching.
     * @return "/api/keys"
     */
    const char* getApiPrefix() const override { return "/api/keys"; }
};
