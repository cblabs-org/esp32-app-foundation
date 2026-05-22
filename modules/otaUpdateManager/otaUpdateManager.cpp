/*
 * ESP32 App Foundation
 * (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/otaUpdateManager/otaUpdateManager.cpp
 * Description: Implementation of the OTA Update Manager service.
 *              Provides methods for streaming MD5 validation, OTA rollback,
 *              and blessing the new firmware image.
 *
 * Exported Functions/Classes:
 * - otaUpdateManager: [Class implementation]
 *   - beginUpdate(): Implementation of update start.
 *   - writeStream(): Implementation of streaming data to partition.
 *   - endUpdate(): Implementation of finalization.
 *   - markAppValid(): Implementation of firmware blessing.
 *   - rollbackFirmware(): Implementation of firmware rollback.
 *   - registerRoutes(): Implementation of API routes for OTA management.
 */

#include "otaUpdateManager.hpp"
#ifndef NATIVE_BUILD
#include <ESPAsyncWebServer.h>
#endif
#include <ArduinoJson.h>

#ifndef NATIVE_BUILD
#include <Update.h>
#include <esp_ota_ops.h>
#endif

otaUpdateManager otaManager; // Global instance of the OTA update manager

otaUpdateManager::otaUpdateManager() : expectedSize(0) {
}

bool otaUpdateManager::beginUpdate(size_t size, const char* expectedMD5) {
#ifndef NATIVE_BUILD
    // --- Step 1: Initialize the Update library ---
    // Start the update process with the expected size
    if (!Update.begin(size)) {
        return false;
    }
    
    // --- Step 2: Set the expected MD5 hash ---
    // The Update library will automatically hash the written data
    // and compare it to this string when Update.end() is called.
    if (expectedMD5 != nullptr) {
        Update.setMD5(expectedMD5);
    }
    
    expectedSize = size;
    return true;
#else
    return false;
#endif
}

size_t otaUpdateManager::writeStream(Stream& dataStream) {
#ifndef NATIVE_BUILD
    // Write data from the provided stream directly into the OTA partition
    return Update.writeStream(dataStream);
#else
    return 0;
#endif
}

bool otaUpdateManager::endUpdate() {
#ifndef NATIVE_BUILD
    // Finalize the update. This checks if the total bytes written 
    // match the expected size, and validates the MD5 hash.
    if (Update.end()) {
        return true;
    }
    return false;
#else
    return false;
#endif
}

void otaUpdateManager::markAppValid() {
    // Bless the application by cancelling the auto-rollback feature.
    // This indicates that the firmware successfully booted and passed basic checks.
#ifndef NATIVE_BUILD
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_mark_app_valid_cancel_rollback();
#endif
#endif
}

void otaUpdateManager::rollbackFirmware() {
#ifndef NATIVE_BUILD
    // Retrieve the partition we should rollback to
    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(NULL);
    
    if (updatePartition) {
        // Set the boot partition to the previous version
        esp_ota_set_boot_partition(updatePartition);
        
        // Reboot to load the previous firmware
        ESP.restart();
    }
#endif
}

void otaUpdateManager::registerRoutes(AsyncWebServer& server) {
#ifndef NATIVE_BUILD
    server.on("/api/ota", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["status"] = "ready";
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/ota", HTTP_POST, 
        [](AsyncWebServerRequest *request) {},
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            if (!error) {
                const char* action = doc["action"] | "";
                if (strcmp(action, "rollback") == 0) {
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    this->rollbackFirmware();
                    return;
                } else if (strcmp(action, "markValid") == 0) {
                    this->markAppValid();
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                    return;
                }
            }
            request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid action\"}");
        }
    );
#endif
}
