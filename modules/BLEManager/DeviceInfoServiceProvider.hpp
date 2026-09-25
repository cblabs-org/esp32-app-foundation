/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/DeviceInfoServiceProvider.hpp
 * Description: Built-in Bluetooth SIG Device Information Service (DIS, 0x180A)
 *              provider. Exposes standard read-only device identity characteristics.
 *              Authoritative: RFC #225 rev 2 §6, WORKORDER WO6-DIS.
 *
 * Exported Classes:
 * - DeviceInfoServiceProvider: IBLEServiceProvider implementation for standard DIS (0x180A)
 *
 *   Lifecycle:
 *   - DeviceInfoServiceProvider(): Constructor taking 6 identity strings
 *   - ~DeviceInfoServiceProvider(): Virtual destructor
 *
 *   IBLEServiceProvider:
 *   - getProviderId():       Return "dis" identifier
 *   - registerServices():    Register 0x180A service and 6 read-only characteristics
 */

#pragma once

#include "IBLEServiceProvider.hpp"

#if defined(ESP_PLATFORM) && defined(WITH_BLE)
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEService.h>
#include <NimBLECharacteristic.h>
#include "esp_log.h"
#endif

#include <cstdint>

/**
 * @brief Standard Bluetooth SIG Device Information Service (DIS, 0x180A) provider.
 *
 * Exposes 6 standard read-only characteristics to identify the device over GATT:
 * - 0x2A29: Manufacturer Name String
 * - 0x2A24: Model Number String
 * - 0x2A25: Serial Number String
 * - 0x2A26: Firmware Revision String
 * - 0x2A27: Hardware Revision String
 * - 0x2A28: Software Revision String
 */
class DeviceInfoServiceProvider : public IBLEServiceProvider {
public:
    /**
     * @brief Construct a DeviceInfoServiceProvider.
     *
     * All parameter pointers must refer to static or long-lived strings that remain
     * valid throughout the lifetime of the BLEManager.
     *
     * @param manufacturer Manufacturer name string (0x2A29).
     * @param model        Model number string (0x2A24).
     * @param serial       Serial number string (0x2A25).
     * @param firmwareRev  Firmware revision string (0x2A26).
     * @param hardwareRev  Hardware revision string (0x2A27).
     * @param softwareRev  Software revision string (0x2A28).
     */
    DeviceInfoServiceProvider(const char* manufacturer,
                              const char* model,
                              const char* serial,
                              const char* firmwareRev,
                              const char* hardwareRev,
                              const char* softwareRev)
        : _manufacturer(manufacturer),
          _model(model),
          _serial(serial),
          _firmwareRev(firmwareRev),
          _hardwareRev(hardwareRev),
          _softwareRev(softwareRev) {}

    ~DeviceInfoServiceProvider() override = default;

    /** @brief Stable identifier for logs, status, and config keys. */
    const char* getProviderId() const override { return "dis"; }

    /**
     * @brief Create DIS service (0x180A) and its 6 read-only characteristics.
     *
     * @param server Shared NimBLEServer instance.
     * @return true on success, false if service creation failed.
     */
    bool registerServices(NimBLEServer& server) override {
#if defined(ESP_PLATFORM) && defined(WITH_BLE)
        NimBLEService* pDis = server.createService(NimBLEUUID((uint16_t)0x180A));
        if (!pDis) {
            ESP_LOGE("DeviceInfoService", "Failed to create DIS service (0x180A)");
            return false;
        }

        auto addChar = [pDis](uint16_t uuid16, const char* value) {
            NimBLECharacteristic* pChar = pDis->createCharacteristic(
                NimBLEUUID(uuid16),
                NIMBLE_PROPERTY::READ
            );
            if (pChar && value) {
                pChar->setValue(value);
            }
        };

        addChar(0x2A29, _manufacturer);  // Manufacturer Name String
        addChar(0x2A24, _model);         // Model Number String
        addChar(0x2A25, _serial);        // Serial Number String
        addChar(0x2A26, _firmwareRev);   // Firmware Revision String
        addChar(0x2A27, _hardwareRev);   // Hardware Revision String
        addChar(0x2A28, _softwareRev);   // Software Revision String

        ESP_LOGI("DeviceInfoService", "Device Information Service (0x180A) registered");
        return true;
#else
        (void)server;
        return true;
#endif
    }

private:
    const char* _manufacturer = nullptr;
    const char* _model        = nullptr;
    const char* _serial       = nullptr;
    const char* _firmwareRev  = nullptr;
    const char* _hardwareRev  = nullptr;
    const char* _softwareRev  = nullptr;
};
