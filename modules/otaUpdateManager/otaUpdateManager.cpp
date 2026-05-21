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
 *   - endUpdate(): Implementation of update finalization.
 *   - markAppValid(): Implementation of firmware blessing.
 *   - rollbackFirmware(): Implementation of firmware rollback.
 */

#include "otaUpdateManager.hpp"
#include <Update.h>
#include <esp_ota_ops.h>

otaUpdateManager otaManager; // Global instance of the OTA update manager

otaUpdateManager::otaUpdateManager() : expectedSize(0) {
}

bool otaUpdateManager::beginUpdate(size_t size, const char* expectedMD5) {
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
}

size_t otaUpdateManager::writeStream(Stream& dataStream) {
    // Write data from the provided stream directly into the OTA partition
    return Update.writeStream(dataStream);
}

bool otaUpdateManager::endUpdate() {
    // Finalize the update. This checks if the total bytes written 
    // match the expected size, and validates the MD5 hash.
    if (Update.end()) {
        return true;
    }
    return false;
}

void otaUpdateManager::markAppValid() {
    // Bless the application by cancelling the auto-rollback feature.
    // This indicates that the firmware successfully booted and passed basic checks.
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_mark_app_valid_cancel_rollback();
#endif
}

void otaUpdateManager::rollbackFirmware() {
    // Retrieve the partition we should rollback to
    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(NULL);
    
    if (updatePartition) {
        // Set the boot partition to the previous version
        esp_ota_set_boot_partition(updatePartition);
        
        // Reboot to load the previous firmware
        ESP.restart();
    }
}
