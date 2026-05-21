/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/keyManager/keyManager.hpp
 * Description: Kernel service that provides an encrypted registry for storing
 *              and retrieving sensitive credentials (e.g. API keys).
 *
 * Exported Functions/Classes:
 * - KeyManager: [Class] Service provider for secure credential storage.
 *   - begin(): Initializes the KeyManager with a dependency-injected DeviceCrypto instance.
 *   - addKey(): Encrypts and stores a credential securely.
 *   - getKeyById(): Retrieves and decrypts a credential.
 *   - deleteKey(): Removes a credential from the registry.
 */

#pragma once

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <LittleFS.h>
#else
#include <string>
#include <stdint.h>
#endif

#include <memory>
#include <ArduinoJson.h>
#include "deviceCrypto.hpp"

#ifndef NATIVE_BUILD
#define KEY_MANAGER_PATH "/etc/keys.json"
#else
#define KEY_MANAGER_PATH "etc_keys.json"
#endif

/**
 * @brief Kernel service providing an encrypted registry for credentials.
 */
class KeyManager {
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
     * @return true if successfully stored, false on failure.
     */
    bool addKey(const char* id, const char* secret);

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
};
