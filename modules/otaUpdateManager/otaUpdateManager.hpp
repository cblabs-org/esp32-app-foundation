/*
 * ESP32 App Foundation
 * (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/otaUpdateManager/otaUpdateManager.hpp
 * Description: Kernel service for managing Over-The-Air (OTA) firmware updates.
 *              Handles streaming MD5 validation, OTA rollback, and firmware blessing.
 *
 * Exported Functions/Classes:
 * - otaUpdateManager: Service class for OTA lifecycle management.
 *   - beginUpdate(): Initiates the update process with size and MD5.
 *   - writeStream(): Streams the data to the flash partition.
 *   - endUpdate(): Finalizes the update.
 *   - markAppValid(): Blesses the running firmware.
 *   - rollbackFirmware(): Reverts to the previous firmware partition.
 */

#pragma once

#include <Arduino.h>
#include <Stream.h>

class otaUpdateManager {
private:
    size_t expectedSize; // Tracks the total expected size of the update
    
public:
    /**
     * @brief Default constructor.
     */
    otaUpdateManager();

    /**
     * @brief Initiates the OTA update process.
     * @param size The total size of the firmware binary.
     * @param expectedMD5 The expected MD5 hash of the firmware for validation.
     * @return True if initialization succeeds.
     */
    bool beginUpdate(size_t size, const char* expectedMD5);

    /**
     * @brief Writes data from a stream to the OTA partition.
     * @param dataStream The stream containing the firmware binary.
     * @return The number of bytes written.
     */
    size_t writeStream(Stream& dataStream);

    /**
     * @brief Finalizes the OTA update and checks for MD5 validation success.
     * @return True if the update is complete and valid.
     */
    bool endUpdate();

    /**
     * @brief Blesses the current running firmware, cancelling any pending rollback.
     *        Should be called after critical subsystems (WiFi, FS) are operational.
     */
    void markAppValid();

    /**
     * @brief Triggers a rollback to the previous firmware partition and reboots.
     */
    void rollbackFirmware();
};

extern otaUpdateManager otaManager;
