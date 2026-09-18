/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEStack.cpp
 * Description: Low-level NimBLE stack initialization, deinitialization, and host task.
 *              Guarded by ESP_PLATFORM && WITH_BLE; compiles to empty on native.
 *              Authoritative: RFC #225 rev 2 §5.1, §16.
 *
 * Exported Methods:
 * - BLEStack::init(): Initializes NimBLE controller and creates server
 * - BLEStack::deinit(): Deinitializes NimBLE stack
 * - BLEStack::isInitialized(): Checks stack status
 * - BLEStack::getOwnAddress(): Queries own BT MAC address
 * - BLEStack::getServer(): Returns server instance
 */

#include "BLEStack.hpp"

#if defined(ESP_PLATFORM) && defined(WITH_BLE)

#include <NimBLEDevice.h>
#include <cstring>
#include "esp_log.h"

static const char* TAG = "BLEStack";

BLEStack::BLEStack() = default;

BLEStack::~BLEStack() {
    if (_initialized) {
        deinit();
    }
}

bool BLEStack::init(const char* deviceName, void* eventQueue) {
    if (_initialized) {
        return true;
    }
    _eventQueue = eventQueue;

    ESP_LOGI(TAG, "Initializing NimBLE stack as '%s'...", deviceName ? deviceName : "ESP32");
    NimBLEDevice::init(deviceName ? deviceName : "ESP32");
    NimBLEDevice::setMTU(185);

    // Default security: Just Works with bonding enabled (RFC §15)
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    _server = NimBLEDevice::createServer();
    if (!_server) {
        ESP_LOGE(TAG, "Failed to create NimBLE server");
        NimBLEDevice::deinit(true);
        return false;
    }

    _initialized = true;
    ESP_LOGI(TAG, "NimBLE stack initialized successfully. Host task running.");
    return true;
}

void BLEStack::deinit() {
    if (!_initialized) {
        return;
    }
    ESP_LOGI(TAG, "Deinitializing NimBLE stack...");
    _server = nullptr;
    NimBLEDevice::deinit(true);
    _initialized = false;
}

bool BLEStack::isInitialized() const {
    return _initialized;
}

bool BLEStack::getOwnAddress(BLEPeerAddress& out) const {
    if (!_initialized) {
        return false;
    }
    NimBLEAddress addr = NimBLEDevice::getAddress();
    return BLEPeerAddress::parse(addr.toString().c_str(),
                                 static_cast<BLEPeerAddress::Type>(addr.getType()),
                                 out);
}

NimBLEServer* BLEStack::getServer() const {
    return _server;
}

#endif // defined(ESP_PLATFORM) && defined(WITH_BLE)
