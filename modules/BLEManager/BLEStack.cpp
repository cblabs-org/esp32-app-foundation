/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEStack.cpp
 * Description: Low-level NimBLE stack initialization, deinitialization, and host task callbacks.
 *              Guarded by ESP_PLATFORM && WITH_BLE; compiles to empty on native.
 *              Authoritative: RFC #225 rev 2 §5.1, §6, §16.
 */

#include "BLEStack.hpp"

#if defined(ESP_PLATFORM) && defined(WITH_BLE)

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLECharacteristic.h>
#include <NimBLEAdvertising.h>
#include <NimBLEAdvertisementData.h>
#include "services/gap/ble_svc_gap.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <cstring>
#include <algorithm>
#include <memory>

static const char* TAG = "BLEStack";

namespace {

class ServerCallbacksHandler : public NimBLEServerCallbacks {
public:
    explicit ServerCallbacksHandler(BLEStack* stack, void* queue)
        : _stack(stack), _queue(queue) {}

    void onConnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& connInfo) override {
        if (!_queue) return;

        BLEQueueMessage msg;
        msg.type = BLEQueueMessage::Type::PeerConnected;
        msg.handle = connInfo.getConnHandle();

        NimBLEAddress addr = connInfo.getAddress();
        BLEPeerAddress::parse(addr.toString().c_str(),
                              static_cast<BLEPeerAddress::Type>(addr.getType()),
                              msg.address);

        msg.mtu = connInfo.getMTU();
        msg.params.minIntervalUnits        = connInfo.getConnInterval();
        msg.params.maxIntervalUnits        = connInfo.getConnInterval();
        msg.params.latency                 = connInfo.getConnLatency();
        msg.params.supervisionTimeoutUnits = connInfo.getConnTimeout();
        msg.boolVal1                       = connInfo.isEncrypted();

        if (xQueueSend(static_cast<QueueHandle_t>(_queue), &msg, 0) != pdTRUE) {
            if (_stack) _stack->incrementDropCount();
        }
    }

    void onDisconnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& connInfo, int reason) override {
        if (!_queue) return;

        BLEQueueMessage msg;
        msg.type = BLEQueueMessage::Type::PeerDisconnected;
        msg.handle = connInfo.getConnHandle();

        NimBLEAddress addr = connInfo.getAddress();
        BLEPeerAddress::parse(addr.toString().c_str(),
                              static_cast<BLEPeerAddress::Type>(addr.getType()),
                              msg.address);

        switch (reason) {
            case 0x13: // BLE_ERR_REM_USER_CONN_TERM
                msg.reason = BLEDisconnectReason::PeerTerminated;
                break;
            case 0x16: // BLE_ERR_CONN_TERM_LOCAL
                msg.reason = BLEDisconnectReason::LocalTerminated;
                break;
            case 0x08: // BLE_ERR_CONN_HNDL_TO
                msg.reason = BLEDisconnectReason::SupervisionTimeout;
                break;
            case 0x3e: // BLE_ERR_CONN_ESTABL_FAIL
                msg.reason = BLEDisconnectReason::ConnectFailed;
                break;
            default:
                msg.reason = BLEDisconnectReason::Other;
                break;
        }

        if (xQueueSend(static_cast<QueueHandle_t>(_queue), &msg, 0) != pdTRUE) {
            if (_stack) _stack->incrementDropCount();
        }
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        if (!_queue) return;

        BLEQueueMessage msg;
        msg.type = BLEQueueMessage::Type::MTUChanged;
        msg.handle = connInfo.getConnHandle();
        msg.mtu = MTU;

        if (xQueueSend(static_cast<QueueHandle_t>(_queue), &msg, 0) != pdTRUE) {
            if (_stack) _stack->incrementDropCount();
        }
    }

    void onConnParamsUpdate(NimBLEConnInfo& connInfo) override {
        if (!_queue) return;

        BLEQueueMessage msg;
        msg.type = BLEQueueMessage::Type::ConnParamsUpdated;
        msg.handle = connInfo.getConnHandle();
        msg.params.minIntervalUnits        = connInfo.getConnInterval();
        msg.params.maxIntervalUnits        = connInfo.getConnInterval();
        msg.params.latency                 = connInfo.getConnLatency();
        msg.params.supervisionTimeoutUnits = connInfo.getConnTimeout();
        msg.boolVal1                       = connInfo.isEncrypted();

        if (xQueueSend(static_cast<QueueHandle_t>(_queue), &msg, 0) != pdTRUE) {
            if (_stack) _stack->incrementDropCount();
        }
    }

private:
    BLEStack* _stack;
    void*     _queue;
};

class CharacteristicCallbacksHandler : public NimBLECharacteristicCallbacks {
public:
    explicit CharacteristicCallbacksHandler(BLEStack* stack, void* queue)
        : _stack(stack), _queue(queue) {}

    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        if (!_queue || !pCharacteristic) return;

        BLEQueueMessage msg;
        msg.type = BLEQueueMessage::Type::SubscriptionChanged;
        msg.handle = connInfo.getConnHandle();
        msg.attrHandle = pCharacteristic->getHandle();

        const NimBLEUUID& uuid = pCharacteristic->getUUID();
        if (uuid.bitSize() == 16 && uuid.getBase() != nullptr) {
            msg.uuid16 = reinterpret_cast<const ble_uuid16_t*>(uuid.getBase())->value;
        } else {
            msg.uuid16 = 0;
        }

        msg.boolVal1 = (subValue & 0x01) != 0; // notify
        msg.boolVal2 = (subValue & 0x02) != 0; // indicate

        if (xQueueSend(static_cast<QueueHandle_t>(_queue), &msg, 0) != pdTRUE) {
            if (_stack) _stack->incrementDropCount();
        }
    }

private:
    BLEStack* _stack;
    void*     _queue;
};

static std::unique_ptr<ServerCallbacksHandler>         s_serverCallbacks;
static std::unique_ptr<CharacteristicCallbacksHandler> s_chrCallbacks;

} // namespace

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

    s_serverCallbacks = std::make_unique<ServerCallbacksHandler>(this, _eventQueue);
    s_chrCallbacks    = std::make_unique<CharacteristicCallbacksHandler>(this, _eventQueue);

    _initialized = true;
    ESP_LOGI(TAG, "NimBLE stack initialized successfully. Host task running.");
    return true;
}

void BLEStack::deinit() {
    if (!_initialized) {
        return;
    }
    ESP_LOGI(TAG, "Deinitializing NimBLE stack...");
    s_serverCallbacks.reset();
    s_chrCallbacks.reset();
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

#if defined(ESP_PLATFORM) && defined(WITH_BLE)
// Herb Sutter template friend trick to access private member NimBLEServer::m_svcVec
template<typename Tag, typename Tag::type M>
struct Rob {
    friend typename Tag::type get(Tag) {
        return M;
    }
};

struct NimBLEServer_svcVec {
    typedef std::vector<NimBLEService*> NimBLEServer::*type;
    friend type get(NimBLEServer_svcVec);
};

template struct Rob<NimBLEServer_svcVec, &NimBLEServer::m_svcVec>;

static const std::vector<NimBLEService*>& getServerServices(const NimBLEServer* server) {
    return server->*get(NimBLEServer_svcVec());
}
#endif

bool BLEStack::startServer() {
    if (!_server) return false;

    // Attach server callbacks
    _server->setCallbacks(s_serverCallbacks.get(), false);

    // Start GATT server (starts all services and allocates attribute handles)
    if (!_server->start()) {
        ESP_LOGE(TAG, "NimBLEServer::start() failed");
        return false;
    }

    // Attach characteristic callbacks to all characteristics on all services
    for (auto* svc : getServerServices(_server)) {
        if (!svc) continue;
        for (auto* chr : svc->getCharacteristics()) {
            if (!chr) continue;
            chr->setCallbacks(s_chrCallbacks.get());
        }
    }

    return true;
}

bool BLEStack::startAdvertising(const BLEAdvPackResult& packResult, const BLEConnParams& preferredParams) {
    if (!_initialized) return false;

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (!pAdv) return false;

    pAdv->stop();
    pAdv->clearData();
    pAdv->setConnectableMode(BLE_GAP_CONN_MODE_UND);

    // Set GAP PPCP characteristic
    setPreferredConnParams(preferredParams);

    // Primary AD payload
    if (packResult.primaryAdv.length > 0) {
        NimBLEAdvertisementData primaryData;
        primaryData.addData(packResult.primaryAdv.data, packResult.primaryAdv.length);
        pAdv->setAdvertisementData(primaryData);
    }

    // Scan Response AD payload
    if (packResult.scanResponse.length > 0) {
        NimBLEAdvertisementData scanData;
        scanData.addData(packResult.scanResponse.data, packResult.scanResponse.length);
        pAdv->setScanResponseData(scanData);
        pAdv->enableScanResponse(true);
    } else {
        pAdv->enableScanResponse(false);
    }

    // Preferred params in advert data
    pAdv->setPreferredParams(preferredParams.minIntervalUnits, preferredParams.maxIntervalUnits);

    return pAdv->start();
}

bool BLEStack::stopAdvertising() {
    if (!_initialized) return false;
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (!pAdv) return false;
    return pAdv->stop();
}

bool BLEStack::isAdvertising() const {
    if (!_initialized) return false;
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    return pAdv ? pAdv->isAdvertising() : false;
}

bool BLEStack::disconnect(BLEConnHandle handle, uint8_t reason) {
    if (!_server) return false;
    return _server->disconnect(handle, reason);
}

void BLEStack::updateConnParams(BLEConnHandle handle, const BLEConnParams& params) {
    if (!_server) return;
    _server->updateConnParams(handle,
                              params.minIntervalUnits,
                              params.maxIntervalUnits,
                              params.latency,
                              params.supervisionTimeoutUnits);
}

void BLEStack::setPreferredConnParams(const BLEConnParams& /*params*/) {
    // Preferred parameters are advertised via pAdv->setPreferredParams()
    // and negotiated per connection via updateConnParams().
}

size_t BLEStack::getServiceCount() const {
    if (!_server) return 0;
    return getServerServices(_server).size();
}

std::vector<BLEStack::CharacteristicEntry> BLEStack::getRegisteredCharacteristics() const {
    std::vector<CharacteristicEntry> result;
    if (!_server) return result;

    const auto& services = getServerServices(_server);
    for (size_t sIdx = 0; sIdx < services.size(); ++sIdx) {
        auto* svc = services[sIdx];
        if (!svc) continue;

        for (auto* chr : svc->getCharacteristics()) {
            if (!chr) continue;

            CharacteristicEntry entry;
            entry.serviceIndex = sIdx;
            entry.handle = chr->getHandle();

            const NimBLEUUID& uuid = chr->getUUID();
            if (uuid.bitSize() == 16 && uuid.getBase() != nullptr) {
                entry.uuid16 = reinterpret_cast<const ble_uuid16_t*>(uuid.getBase())->value;
            } else {
                entry.uuid16 = 0;
            }

            result.push_back(entry);
        }
    }
    return result;
}

uint32_t BLEStack::getDroppedEventsCount() const {
    return _droppedEventsCount;
}

#endif // defined(ESP_PLATFORM) && defined(WITH_BLE)
