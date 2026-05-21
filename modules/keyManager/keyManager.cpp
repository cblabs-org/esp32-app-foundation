/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/keyManager/keyManager.cpp
 * Description: Kernel service implementation for storing and retrieving
 *              encrypted credentials via LittleFS.
 */

#include "keyManager.hpp"
#include <fstream>
#include <sstream>

#ifndef NATIVE_BUILD
#define LOG_KEYMGR(fmt, ...) Serial.printf("[KEY_MGR] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_KEYMGR(fmt, ...) printf("[KEY_MGR] " fmt "\n", ##__VA_ARGS__)
#endif

void KeyManager::begin(DeviceCrypto* crypto) {
    _crypto = crypto;
    LOG_KEYMGR("Initialized with DeviceCrypto reference.");
}

bool KeyManager::loadRegistry(JsonDocument& doc) const {
#ifndef NATIVE_BUILD
    if (!LittleFS.exists(KEY_MANAGER_PATH)) {
        return true; // Empty registry is valid initially
    }
    File file = LittleFS.open(KEY_MANAGER_PATH, "r");
    if (!file) {
        LOG_KEYMGR("Failed to open registry for reading.");
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
        LOG_KEYMGR("Failed to parse registry: %s", error.c_str());
        return false;
    }
    return true;
}

bool KeyManager::saveRegistry(const JsonDocument& doc) const {
#ifndef NATIVE_BUILD
    File file = LittleFS.open(KEY_MANAGER_PATH, "w");
    if (!file) {
        LOG_KEYMGR("Failed to open registry for writing.");
        return false;
    }
    serializeJson(doc, file);
    file.close();
#else
    std::ofstream file(KEY_MANAGER_PATH);
    if (!file.is_open()) {
        LOG_KEYMGR("Failed to open local registry file for writing.");
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
        LOG_KEYMGR("Invalid ID or Secret.");
        return false;
    }
    if (!_crypto) {
        LOG_KEYMGR("DeviceCrypto not initialized.");
        return false;
    }
    
    size_t outLen = 0;
    std::unique_ptr<char[]> encrypted = _crypto->encrypt(secret, strlen(secret), &outLen);
    if (!encrypted || outLen == 0) {
        LOG_KEYMGR("Encryption failed for id: %s", id);
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

    LOG_KEYMGR("Successfully stored encrypted key: %s", id);
    return true;
}

std::string KeyManager::getKeyById(const char* id) const {
    if (!id || strlen(id) == 0) {
        return "";
    }
    if (!_crypto) {
        LOG_KEYMGR("DeviceCrypto not initialized.");
        return "";
    }

    JsonDocument doc;
    if (!loadRegistry(doc)) {
        return "";
    }

    if (!doc.containsKey(id)) {
        LOG_KEYMGR("Key not found: %s", id);
        return "";
    }

    const char* ciphertext = doc[id];
    size_t outLen = 0;
    std::unique_ptr<char[]> decrypted = _crypto->decrypt(ciphertext, strlen(ciphertext), &outLen);
    
    if (!decrypted || outLen == 0) {
        LOG_KEYMGR("Decryption failed for id: %s", id);
        return "";
    }

    return std::string(decrypted.get(), outLen);
}

bool KeyManager::deleteKey(const char* id) {
    if (!id || strlen(id) == 0) {
        return false;
    }
    JsonDocument doc;
    if (!loadRegistry(doc)) {
        return false;
    }

    if (!doc.containsKey(id)) {
        LOG_KEYMGR("Key not found to delete: %s", id);
        return false;
    }

    doc.remove(id);

    if (!saveRegistry(doc)) {
        return false;
    }

    LOG_KEYMGR("Successfully deleted key: %s", id);
    return true;
}
