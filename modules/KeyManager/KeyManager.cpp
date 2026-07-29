/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/KeyManager/KeyManager.cpp
 * Description: Kernel service implementation for storing and retrieving
 *              encrypted credentials via LittleFS.
 */

#include "KeyManager.hpp"
#include "../Logger/Logger.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#ifndef ESP_PLATFORM
#include <filesystem>
#endif

#include <PsychicHttp.h>

#include "platform/LogShim.h"

static constexpr const char* TAG = "KeyManager";

void KeyManager::begin(DeviceCrypto* crypto) {
    _crypto = crypto;
    ESP_LOGI(TAG, "Initialized with DeviceCrypto reference.");
}

bool KeyManager::loadRegistry(JsonDocument& doc) const {
    struct stat st;
    if (stat(KEY_MANAGER_PATH, &st) != 0) {
        return true; // Empty registry is valid initially
    }
    FILE* file = fopen(KEY_MANAGER_PATH, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open registry for reading.");
        return false;
    }
    std::vector<char> buf(st.st_size);
    fread(buf.data(), 1, st.st_size, file);
    fclose(file);
    DeserializationError error = deserializeJson(doc, buf.data(), st.st_size);

    if (error) {
        ESP_LOGE(TAG, "Failed to parse registry: %s", error.c_str());
        return false;
    }
    return true;
}

bool KeyManager::saveRegistry(const JsonDocument& doc) const {
    // Ensure parent directory exists (fresh filesystem won't have system config dir)
#ifdef ESP_PLATFORM
    struct stat st;
    if (stat(ConfigRoot::getEtcPath(), &st) != 0) {
        mkdir(ConfigRoot::getEtcPath(), 0777);
    }
#else
    std::error_code ec;
    std::filesystem::create_directories(ConfigRoot::getEtcPath(), ec);
#endif
    FILE* file = fopen(KEY_MANAGER_PATH, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open registry for writing.");
        return false;
    }
    std::string out;
    serializeJson(doc, out);
    fwrite(out.c_str(), 1, out.length(), file);
    fclose(file);
    return true;
}

bool KeyManager::addKey(const char* id, const char* secret,
                        const char* managedBy, bool exclusive) {
    if (!id || strlen(id) == 0 || !secret || strlen(secret) == 0) {
        ESP_LOGE(TAG, "Invalid ID or Secret.");
        return false;
    }
    if (!_crypto) {
        ESP_LOGE(TAG, "DeviceCrypto not initialized.");
        return false;
    }
    
    size_t outLen = 0;
    std::unique_ptr<char[]> encrypted = _crypto->encrypt(secret, strlen(secret), &outLen);
    if (!encrypted || outLen == 0) {
        ESP_LOGE(TAG, "Encryption failed for id: %s", id);
        return false;
    }

    JsonDocument doc;
    if (!loadRegistry(doc)) {
        return false;
    }

    // Write nested object: {"value": "...", "managedBy": "...", "exclusive": true}
    JsonObject entry = doc[id].to<JsonObject>();
    entry["value"] = encrypted.get();
    if (managedBy && strlen(managedBy) > 0) {
        entry["managedBy"] = managedBy;
        if (exclusive) {
            entry["exclusive"] = true;
        }
    }

    if (!saveRegistry(doc)) {
        return false;
    }

    ESP_LOGI(TAG, "Successfully stored encrypted key: %s", id);
    return true;
}

std::string KeyManager::getKeyById(const char* id) const {
    if (!id || strlen(id) == 0) {
        ESP_LOGW(TAG, "getKeyById() called with null/empty id");
        return "";
    }
    if (!_crypto) {
        ESP_LOGE(TAG, "DeviceCrypto not initialized.");
        return "";
    }

    JsonDocument doc;
    if (!loadRegistry(doc)) {
        return "";
    }

    if (!doc[id].is<JsonVariant>()) {
        ESP_LOGW(TAG, "Key not found: %s", id);
        return "";
    }

    // Read ciphertext from nested object format: {"value": "..."}
    JsonVariant entry = doc[id];
    const char* ciphertext = entry.is<JsonObject>()
        ? entry["value"].as<const char*>()
        : entry.as<const char*>();

    if (!ciphertext || strlen(ciphertext) == 0) {
        ESP_LOGE(TAG, "No ciphertext found for id: %s", id);
        return "";
    }

    size_t outLen = 0;
    std::unique_ptr<char[]> decrypted = _crypto->decrypt(ciphertext, strlen(ciphertext), &outLen);
    
    if (!decrypted || outLen == 0) {
        ESP_LOGE(TAG, "Decryption failed for id: %s", id);
        return "";
    }

    std::string result(decrypted.get(), outLen);
    Logger::registerSecret(std::string(result.c_str()));
    return result;
}

bool KeyManager::deleteKey(const char* id) {
    if (!id || strlen(id) == 0) {
        ESP_LOGW(TAG, "deleteKey() called with null/empty id");
        return false;
    }
    JsonDocument doc;
    if (!loadRegistry(doc)) {
        return false;
    }

    if (!doc[id].is<JsonVariant>()) {
        ESP_LOGW(TAG, "Key not found to delete: %s", id);
        return false;
    }

    doc.remove(id);

    if (!saveRegistry(doc)) {
        return false;
    }

    ESP_LOGI(TAG, "Successfully deleted key: %s", id);
    return true;
}

// =============================================================================
// Key Listing & Metadata Queries
// =============================================================================

/**
 * @brief List all stored key IDs without exposing plaintext values.
 *
 * Loads the registry and iterates over all key-value pairs, extracting
 * only the key names. Retained for backward compatibility with modules
 * that only need IDs.
 *
 * @return Vector of key ID strings (empty if registry is empty or unreadable).
 */
std::vector<std::string> KeyManager::listKeys() const {
    std::vector<std::string> ids;
    JsonDocument doc;
    if (!loadRegistry(doc)) {
        return ids;
    }

    JsonObject obj = doc.as<JsonObject>();
    for (JsonPair pair : obj) {
        ids.push_back(std::string(pair.key().c_str()));
    }
    return ids;
}

/**
 * @brief List all stored keys with full ownership metadata.
 *
 * Iterates the registry extracting id, hasValue, managedBy, and exclusive
 * for each entry.  Used by the GET /api/keys REST handler to populate
 * the portal Keys tab with badge and protection information.
 *
 * @return Vector of KeyInfo structs (empty if registry is empty or unreadable).
 */
std::vector<KeyInfo> KeyManager::listKeysWithMetadata() const {
    std::vector<KeyInfo> result;
    JsonDocument doc;
    if (!loadRegistry(doc)) {
        return result;
    }

    JsonObject obj = doc.as<JsonObject>();
    for (JsonPair pair : obj) {
        KeyInfo info;
        info.id = std::string(pair.key().c_str());

        JsonVariant entry = pair.value();
        if (entry.is<JsonObject>()) {
            // Nested format: {"value": "...", "managedBy": "...", "exclusive": true}
            const char* val = entry["value"] | static_cast<const char*>(nullptr);
            info.hasValue = (val != nullptr && strlen(val) > 0);
            const char* mb = entry["managedBy"] | static_cast<const char*>(nullptr);
            info.managedBy = mb ? std::string(mb) : "";
            info.exclusive = entry["exclusive"] | false;
        } else {
            // Flat legacy string (shouldn't occur with no migration, but safe)
            info.hasValue = true;
            info.managedBy = "";
            info.exclusive = false;
        }

        result.push_back(info);
    }
    return result;
}

/**
 * @brief Check whether a key is exclusively owned by a module.
 *
 * Loads the registry and inspects the exclusive flag on the entry.
 * Used by POST and DELETE REST handlers to reject writes to
 * module-managed keys with 403.
 *
 * @param id The unique identifier for the key.
 * @return true if the key exists and has exclusive=true, false otherwise.
 */
bool KeyManager::isExclusive(const char* id) const {
    if (!id || strlen(id) == 0) return false;

    JsonDocument doc;
    if (!loadRegistry(doc)) return false;
    if (!doc[id].is<JsonVariant>()) return false;

    JsonVariant entry = doc[id];
    if (!entry.is<JsonObject>()) return false;
    return entry["exclusive"] | false;
}

// =============================================================================
// IWebContributor — REST API Routes
// =============================================================================

/**
 * @brief Register key management REST API routes with the web server.
 *
 * Registers three endpoints under /api/keys for CRUD operations on the
 * encrypted key vault. All route handler bodies are guarded by
 * since ESPAsyncWebServer is unavailable in the
 * native GoogleTest runner.
 *
 * Routes:
 * - GET    /api/keys        → list key IDs with hasValue flag (never plaintext)
 * - POST   /api/keys        → add or update a key {id, value} — 1KB body limit
 * - DELETE /api/keys?id=xxx → delete a key by ID
 *
 * @param server Reference to the AsyncWebServer instance.
 */
void KeyManager::registerRoutes(PsychicHttpServer& server) {
    // --- GET /api/keys — list all stored keys with ownership metadata ---
    server.on("/api/keys", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        JsonArray keysArr = doc.to<JsonArray>();

        std::vector<KeyInfo> keys = listKeysWithMetadata();
        for (const auto& ki : keys) {
            JsonObject entry = keysArr.add<JsonObject>();
            entry["id"] = ki.id;
            entry["hasValue"] = ki.hasValue;
            if (!ki.managedBy.empty()) {
                entry["managedBy"] = ki.managedBy;
            }
            if (ki.exclusive) {
                entry["exclusive"] = true;
            }
        }

        std::string output;
        serializeJson(doc, output);
        response->send(200, "application/json", output.c_str());
        return ESP_OK;
    });

    // --- POST /api/keys — add or update a key ---
    server.on("/api/keys", HTTP_POST,
        [this](PsychicRequest* request, PsychicResponse* response) {
            if (request->contentLength() > 1024) {
                response->send(413, "application/json", "{\"error\":\"Payload too large\"}");
                return ESP_FAIL;
            }

            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"Missing body\"}");
                return ESP_FAIL;
            }

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, body);

            if (err) {
                response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return ESP_FAIL;
            }

            const char* id = doc["id"] | static_cast<const char*>(nullptr);
            const char* value = doc["value"] | static_cast<const char*>(nullptr);

            if (!id || strlen(id) == 0 || !value || strlen(value) == 0) {
                response->send(400, "application/json", "{\"error\":\"Missing 'id' or 'value'\"}");
                return ESP_FAIL;
            }

            // Key name length guard — prevent excessively long identifiers
            if (strlen(id) > 64) {
                response->send(400, "application/json", "{\"error\":\"Key name too long (max 64 chars)\"}");
                return ESP_FAIL;
            }

            // Reject writes to exclusively-managed keys
            if (isExclusive(id)) {
                response->send(403, "application/json",
                    "{\"error\":\"Key is exclusively managed by its owning module\"}");
                return ESP_FAIL;
            }

            if (addKey(id, value)) {
                response->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                response->send(500, "application/json", "{\"error\":\"Failed to store key\"}");
            }
            return ESP_OK;
        }
    );

    // --- DELETE /api/keys?id=xxx — delete a key by ID ---
    server.on("/api/keys", HTTP_DELETE, [this](PsychicRequest* request, PsychicResponse* response) {
        if (!request->hasParam("id")) {
            response->send(400, "application/json", "{\"error\":\"Missing 'id' parameter\"}");
            return ESP_FAIL;
        }

        std::string id = request->getParam("id")->value().c_str();
        if (id.empty()) {
            response->send(400, "application/json", "{\"error\":\"Empty 'id' parameter\"}");
            return ESP_FAIL;
        }

        // Reject deletion of exclusively-managed keys
        if (isExclusive(id.c_str())) {
            response->send(403, "application/json",
                "{\"error\":\"Key is exclusively managed by its owning module\"}");
            return ESP_FAIL;
        }

        if (deleteKey(id.c_str())) {
            response->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            response->send(404, "application/json", "{\"error\":\"Key not found\"}");
        }
        return ESP_OK;
    });
}
