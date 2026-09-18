/*
 * ESP32 App Foundation — BLE Build Spike Harness
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: src/ble_spike.cpp
 * Description: Temporary harness exercising every NimBLE API that RFC #225 §5
 *              depends on, so the linker keeps them and we can measure the real
 *              flash/RAM cost. This file is compiled only under -DBLE_SPIKE
 *              and -DWITH_BLE. It is committed for reference then removed.
 *
 * WARNING: This is throwaway spike code. It will be removed in a follow-up
 *          commit. Do not use as a reference for production patterns.
 */

#if defined(BLE_SPIKE) && defined(WITH_BLE) && defined(ESP_PLATFORM)

#include <NimBLEDevice.h>
#include <esp_log.h>

static const char* TAG = "ble_spike";

// ---- Server callbacks ----
class SpikeServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        ESP_LOGI(TAG, "Peripheral connected: %s", connInfo.getAddress().toString().c_str());
    }
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        ESP_LOGI(TAG, "Peripheral disconnected, reason=%d", reason);
    }
};

// ---- Characteristic callbacks (for subscribe-changed) ----
class SpikeCharCallbacks : public NimBLECharacteristicCallbacks {
    void onSubscribe(NimBLECharacteristic* pCharacteristic,
                     NimBLEConnInfo& connInfo,
                     uint16_t subValue) override {
        ESP_LOGI(TAG, "Subscribe changed: handle=0x%04x subValue=%u",
                 pCharacteristic->getHandle(), subValue);
    }
};

// ---- Client callbacks ----
class SpikeClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        ESP_LOGI(TAG, "Central connected to peer");
    }
    void onDisconnect(NimBLEClient* pClient, int reason) override {
        ESP_LOGI(TAG, "Central disconnected from peer, reason=%d", reason);
    }
};

// ---- Notification callback ----
static void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic,
                     uint8_t* pData, size_t length, bool isNotify) {
    ESP_LOGI(TAG, "Notification from handle=0x%04x len=%u",
             pRemoteCharacteristic->getHandle(), (unsigned)length);
}

/**
 * @brief Runs the full NimBLE API surface needed by RFC #225 §5.
 *
 * Called once from setup() under BLE_SPIKE. Exercises:
 * - Device init, MTU, security
 * - Server with two services (FTMS 0x1826, HR 0x180D)
 * - Legacy advertising with raw AD payload + scan response
 * - Preferred connection parameters (via NimBLEAdvertisementData)
 * - Passive + active scan
 * - Client direct connect with explicit address type
 * - Service discovery, subscribe
 * - Bonding, bond management
 * - Host task inspection
 */
void bleSpikeRun() {
    ESP_LOGI(TAG, "=== BLE Spike: Start ===");

    // --- Init ---
    NimBLEDevice::init("BLESpike");
    NimBLEDevice::setMTU(185);

    // --- Security: Just Works, bonding ---
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    // --- Server ---
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new SpikeServerCallbacks());

    // Service 1: FTMS (0x1826) with 3 characteristics
    NimBLEService* pFtms = pServer->createService("1826");
    // Feature (read)
    NimBLECharacteristic* pFeat = pFtms->createCharacteristic(
        "2ACC", NIMBLE_PROPERTY::READ);
    uint8_t featData[] = {0x02, 0x40, 0x00, 0x00, 0x0C, 0x20, 0x00, 0x00};
    pFeat->setValue(featData, sizeof(featData));
    // Indoor Bike Data (notify)
    NimBLECharacteristic* pBike = pFtms->createCharacteristic(
        "2AD2", NIMBLE_PROPERTY::NOTIFY);
    pBike->setCallbacks(new SpikeCharCallbacks());
    // Fitness Machine Status (notify)
    NimBLECharacteristic* pStatus = pFtms->createCharacteristic(
        "2ADA", NIMBLE_PROPERTY::NOTIFY);
    pStatus->setCallbacks(new SpikeCharCallbacks());

    // Service 2: HR (0x180D) with 1 notify characteristic
    NimBLEService* pHr = pServer->createService("180D");
    NimBLECharacteristic* pHrm = pHr->createCharacteristic(
        "2A37", NIMBLE_PROPERTY::NOTIFY);
    pHrm->setCallbacks(new SpikeCharCallbacks());

    // v2: services start automatically when server starts; no pFtms->start()

    // --- Advertising: raw AD payload + scan response ---
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();

    // Build raw primary AD payload
    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.addServiceUUID(NimBLEUUID((uint16_t)0x1826));
    advData.addServiceUUID(NimBLEUUID((uint16_t)0x180D));
    advData.setAppearance(0x0491); // Indoor Bike (example)
    // Service data for FTMS
    std::string svcData;
    svcData += (char)0x26; svcData += (char)0x18; // UUID little-endian
    svcData += (char)0x01; svcData += (char)0x20; svcData += (char)0x00; // flags
    advData.setServiceData(NimBLEUUID((uint16_t)0x1826), svcData);
    // Preferred connection parameters (min 15ms=12, max 30ms=24 in 1.25ms units)
    advData.setPreferredParams(12, 24);
    pAdv->setAdvertisementData(advData);

    // Build scan response with the name
    NimBLEAdvertisementData scanData;
    scanData.setName("BLESpike-Test");
    pAdv->setScanResponseData(scanData);

    // Legacy connectable advertising (default mode in v2)
    pAdv->setConnectableMode(BLE_GAP_CONN_MODE_UND);
    pAdv->start();
    ESP_LOGI(TAG, "Advertising started (legacy connectable)");

    // --- Scan (passive, then active) ---
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setInterval(160);   // 100 ms
    pScan->setWindow(40);      // 25 ms (< 50% of interval)
    pScan->setActiveScan(false);
    ESP_LOGI(TAG, "Starting passive scan (2s)...");
    NimBLEScanResults passiveResults = pScan->getResults(2000, false);
    ESP_LOGI(TAG, "Passive scan found %d devices", passiveResults.getCount());

    pScan->setActiveScan(true);
    ESP_LOGI(TAG, "Starting active scan (2s)...");
    NimBLEScanResults activeResults = pScan->getResults(2000, false);
    ESP_LOGI(TAG, "Active scan found %d devices", activeResults.getCount());

    // Log scan results — check address type availability
    for (int i = 0; i < activeResults.getCount(); i++) {
        const NimBLEAdvertisedDevice* dev = activeResults.getDevice(i);
        ESP_LOGI(TAG, "  Device: %s type=%d rssi=%d name='%s'",
                 dev->getAddress().toString().c_str(),
                 dev->getAddress().getType(),
                 dev->getRSSI(),
                 dev->getName().c_str());
    }

    // --- Client: direct connect with explicit address type ---
    // Use a dummy address — this will fail to connect (expected), but
    // exercises the API path.
    NimBLEAddress dummyAddr("AA:BB:CC:DD:EE:FF", BLE_ADDR_PUBLIC);
    ESP_LOGI(TAG, "Dummy address: %s type=%d", dummyAddr.toString().c_str(),
             dummyAddr.getType());

    NimBLEClient* pClient = NimBLEDevice::createClient(dummyAddr);
    pClient->setClientCallbacks(new SpikeClientCallbacks());
    pClient->setConnectionParams(12, 24, 0, 400);
    pClient->setConnectTimeout(5); // 5 seconds

    // Attempt connect (will timeout — that's fine, we're testing the API path)
    ESP_LOGI(TAG, "Attempting direct connect (will timeout)...");
    bool connected = pClient->connect(dummyAddr);
    if (connected) {
        ESP_LOGI(TAG, "Connected (unexpected with dummy addr)");
        // Service discovery — v2 returns a vector directly
        auto services = pClient->getServices(true);
        for (auto& svc : services) {
            ESP_LOGI(TAG, "  Service: %s", svc->getUUID().toString().c_str());
            auto chars = svc->getCharacteristics(true);
            for (auto& chr : chars) {
                ESP_LOGI(TAG, "    Char: %s handle=0x%04x",
                         chr->getUUID().toString().c_str(), chr->getHandle());
                // Subscribe if notify
                if (chr->canNotify()) {
                    chr->subscribe(true, notifyCB);
                }
            }
        }
        pClient->disconnect();
    } else {
        ESP_LOGW(TAG, "Connect failed (expected with dummy address)");
    }
    NimBLEDevice::deleteClient(pClient);

    // --- Bond management ---
    ESP_LOGI(TAG, "Bond count: %d", NimBLEDevice::getNumBonds());
    // deleteBond would be: NimBLEDevice::deleteBond(addr);

    // --- Host task info ---
    ESP_LOGI(TAG, "NimBLE host task is running on the default NimBLE host task");
    ESP_LOGI(TAG, "Own address: %s", NimBLEDevice::getAddress().toString().c_str());

    ESP_LOGI(TAG, "=== BLE Spike: Complete ===");
}

#endif // BLE_SPIKE && WITH_BLE && ESP_PLATFORM
