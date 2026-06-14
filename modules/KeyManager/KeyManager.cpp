/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/KeyManager/keyManager.cpp
 * Description: Kernel service implementation for storing and retrieving
 *              encrypted credentials via LittleFS.
 */

#include "KeyManager.hpp"
#include "../Logger/Logger.hpp"
#include <fstream>
#include <sstream>

#ifndef NATIVE_BUILD
#include <PsychicHttp.h>
#else
#include "../WebServer/WebServerManager.hpp"
#endif

#include "platform/LogShim.h"

static const char* TAG = "KeyManager";

void KeyManager::begin(DeviceCrypto* crypto) {
    _crypto = crypto;
    ESP_LOGI(TAG, "Initialized with DeviceCrypto reference.");
}

bool KeyManager::loadRegistry(JsonDocument& doc) const {
#ifndef NATIVE_BUILD
    if (!LittleFS.exists(KEY_MANAGER_PATH)) {
        return true; // Empty registry is valid initially
    }
    File file = LittleFS.open(KEY_MANAGER_PATH, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open registry for reading.");
        return false;
    }
    DeserializationError error = deserializeJson(doc, file);
    file.close();
#else
    std::ifstream file(KEY_MANAGER_PATH);
    if (!file.is_open()) {
        return true; // Empty registry
    }
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (jsonStr.empty()) {
        return true; // Empty file is valid
    }
    DeserializationError error = deserializeJson(doc, jsonStr);
    file.close();
#endif

    if (error) {
        ESP_LOGE(TAG, "Failed to parse registry: %s", error.c_str());
        return false;
    }
    return true;
}

bool KeyManager::saveRegistry(const JsonDocument& doc) const {
#ifndef NATIVE_BUILD
    // Ensure parent directory exists (fresh filesystem won't have /etc/)
    if (!LittleFS.exists("/etc")) {
        LittleFS.mkdir("/etc");
    }
    File file = LittleFS.open(KEY_MANAGER_PATH, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open registry for writing.");
        return false;
    }
    serializeJson(doc, file);
    file.close();
#else
    std::ofstream file(KEY_MANAGER_PATH);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open local registry file for writing.");
        return false;
    }
    std::string jsonStr;
    serializeJson(doc, jsonStr);
    file << jsonStr;
    file.close();
#endif
    return true;
}

bool KeyManager::addKey(const char* id, const char* secret) {
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

    doc[id] = encrypted.get();

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

    if (!doc.containsKey(id)) {
        ESP_LOGW(TAG, "Key not found: %s", id);
        return "";
    }

    const char* ciphertext = doc[id];
    size_t outLen = 0;
    std::unique_ptr<char[]> decrypted = _crypto->decrypt(ciphertext, strlen(ciphertext), &outLen);
    
    if (!decrypted || outLen == 0) {
        ESP_LOGE(TAG, "Decryption failed for id: %s", id);
        return "";
    }

    std::string result(decrypted.get(), outLen);
    Logger::registerSecret(String(result.c_str()));
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

    if (!doc.containsKey(id)) {
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
// Key Listing
// =============================================================================

/**
 * @brief List all stored key IDs without exposing plaintext values.
 *
 * Loads the registry and iterates over all key-value pairs, extracting
 * only the key names. This is used by the GET /api/keys endpoint.
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

// =============================================================================
// IWebContributor — REST API Routes
// =============================================================================

/**
 * @brief Register key management REST API routes with the web server.
 *
 * Registers three endpoints under /api/keys for CRUD operations on the
 * encrypted key vault. All route handler bodies are guarded by
 * #ifndef NATIVE_BUILD since ESPAsyncWebServer is unavailable in the
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
#ifndef NATIVE_BUILD
    // --- GET /api/keys — list all stored key IDs ---
    server.on("/api/keys", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        JsonArray keysArr = doc.to<JsonArray>();

        std::vector<std::string> ids = listKeys();
        for (const auto& id : ids) {
            JsonObject entry = keysArr.add<JsonObject>();
            entry["id"] = id;
            entry["hasValue"] = true; // All stored keys have a value
        }

        String output;
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

        String id = request->getParam("id")->value();
        if (id.length() == 0) {
            response->send(400, "application/json", "{\"error\":\"Empty 'id' parameter\"}");
            return ESP_FAIL;
        }

        if (deleteKey(id.c_str())) {
            response->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            response->send(404, "application/json", "{\"error\":\"Key not found\"}");
        }
        return ESP_OK;
    });
#else
    // Native build: register stub routes for compilation
    server.on("/api/keys", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) { return ESP_OK; });
    server.on("/api/keys", HTTP_POST, [](PsychicRequest* request, PsychicResponse* response) { return ESP_OK; });
    server.on("/api/keys", HTTP_DELETE, [](PsychicRequest* request, PsychicResponse* response) { return ESP_OK; });
#endif
}
