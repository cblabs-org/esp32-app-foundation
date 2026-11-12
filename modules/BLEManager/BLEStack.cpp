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
#include "esp32-hal-bt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <cstring>
#include <algorithm>
#include <memory>
#include <map>
#include <new>

// ─── Arduino BLE-in-use override ────────────────────────────────────
// The Arduino core's initArduino() releases the BT controller's ROM
// data/bss regions to the heap when bleInUse() returns false.  The
// weak default returns false because esp-nimble-cpp doesn't include
// Arduino's esp32-hal-alloc-ble-mem.h.  Providing a strong definition
// here prevents the release so NimBLEDevice::init() finds its memory
// intact.  See: cores/esp32/esp32-hal-misc.c:353, esp32-hal-bt.c:52.
extern "C" bool bleInUse(void) { return true; }
#include <string>

static const char* TAG = "BLEStack";

namespace {

class ServerCallbacksHandler : public NimBLEServerCallbacks {
public:
    explicit ServerCallbacksHandler(BLEStack* stack, void* queue)
        : _stack(stack), _queue(queue) {}

    void onConnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& connInfo) override {
        ESP_LOGI(TAG, ">>> Central connected: handle=%d, addr=%s, itvl=%d (%.2fms), latency=%d, timeout=%d (%dms), MTU=%d",
                 connInfo.getConnHandle(), connInfo.getAddress().toString().c_str(),
                 connInfo.getConnInterval(), connInfo.getConnInterval() * 1.25f,
                 connInfo.getConnLatency(),
                 connInfo.getConnTimeout(), connInfo.getConnTimeout() * 10,
                 connInfo.getMTU());

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
        ESP_LOGW(TAG, "<<< Central disconnected: handle=%d, addr=%s, reason=0x%02x (%s)",
                 connInfo.getConnHandle(), connInfo.getAddress().toString().c_str(),
                 reason, NimBLEUtils::returnCodeToString(reason));

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
        ESP_LOGI(TAG, ">>> Central MTU changed: handle=%d, MTU=%u", connInfo.getConnHandle(), (unsigned)MTU);

        // Once MTU exchange is complete, if the central used a dangerously short
        // supervision timeout (< 2.0s, e.g. macOS default 720ms), update it to
        // 4.0s so radio coexistence bursts don't drop the connection.
        if (connInfo.getConnTimeout() < 200 && _stack) {
            BLEConnParams robustParams;
            robustParams.minIntervalUnits        = connInfo.getConnInterval();
            robustParams.maxIntervalUnits        = connInfo.getConnInterval();
            robustParams.latency                 = connInfo.getConnLatency();
            robustParams.supervisionTimeoutUnits = 400; // 4.0s
            ESP_LOGI(TAG, "Updating supervision timeout to 4.0s (was %dms)", connInfo.getConnTimeout() * 10);
            _stack->updateConnParams(connInfo.getConnHandle(), robustParams);
        }

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

// N1: ChainingCharCallbacks wraps any existing callback pointer (including
// provider-installed ones) and adds onSubscribe routing to BLEManager.
// NimBLE 2.5.0 always has a non-null default callback, so the old approach
// (skip if getCallbacks() != nullptr) silently broke onSubscribe for all chars.
class ChainingCharCallbacks : public NimBLECharacteristicCallbacks {
public:
    explicit ChainingCharCallbacks(NimBLECharacteristicCallbacks* prev, BLEStack* stack, void* queue)
        : _prev(prev), _stack(stack), _queue(queue) {}

    void onRead(NimBLECharacteristic* chr, NimBLEConnInfo& info) override {
        if (_prev) _prev->onRead(chr, info);
    }
    void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& info) override {
        if (_prev) _prev->onWrite(chr, info);
    }
    void onStatus(NimBLECharacteristic* chr, int code) override {
        if (_prev) _prev->onStatus(chr, code);
    }
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        // Forward to provider callback first (if any)
        if (_prev) _prev->onSubscribe(pCharacteristic, connInfo, subValue);

        // Then route to BLEManager for subscription tracking
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
    NimBLECharacteristicCallbacks* _prev;
    BLEStack* _stack;
    void*     _queue;
};

static std::unique_ptr<ServerCallbacksHandler>  s_serverCallbacks;
// N1: s_chrCallbacks is used only to hold the BLEStack* and queue* for
// creating ChainingCharCallbacks instances in startServer(). The actual
// chaining wrappers are allocated per-characteristic (once, at boot).
static BLEStack* s_chainingStack = nullptr;
static void*     s_chainingQueue = nullptr;

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

    // Fail-closed: if Arduino released BT controller memory at boot
    // (bleInUse() was false), NimBLEDevice::init() would write into
    // heap-owned regions and corrupt memory.  Abort with a clear error.
    if (btMemReleased(BT_MODE_BLE)) {
        ESP_LOGE(TAG, "BT controller memory was released by Arduino core. "
                       "Ensure bleInUse() returns true. Cannot init BLE.");
        return false;
    }

    NimBLEDevice::init(deviceName ? deviceName : "ESP32");
    NimBLEDevice::setMTU(185);

    // Default security: Just Works with bonding enabled (RFC §15)
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    // BLE TX power is set via CONFIG_BT_CTRL_DFT_TX_POWER_LEVEL_* in
    // sdkconfig.ble.defaults (default: +9 dBm / P9). Downstream consumers
    // can override in their own sdkconfig fragment — no code change needed.
    // See Issue #239: +20 dBm caused WiFi coexistence instability in soak testing.

    // Wi-Fi / BLE coexistence: leave at ESP-IDF default (PREFER_WIFI, enum 0).
    // PREFER_BT (dev.76) starved WiFi entirely; PREFER_BALANCE (dev.78) still
    // caused AUTH_EXPIRE during WiFi association.  The default has always worked
    // and BLE stability comes from +20 dBm TX power and deferred conn params.

    _server = NimBLEDevice::createServer();
    if (!_server) {
        ESP_LOGE(TAG, "Failed to create NimBLE server");
        NimBLEDevice::deinit(true);
        return false;
    }

    s_serverCallbacks = std::make_unique<ServerCallbacksHandler>(this, _eventQueue);
    s_chainingStack = this;
    s_chainingQueue = _eventQueue;

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
    s_chainingStack = nullptr;
    s_chainingQueue = nullptr;
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

    // N1: Install chaining callbacks on ALL characteristics. This wraps any
    // existing callback (provider-installed or NimBLE default) and adds
    // onSubscribe routing. NimBLE 2.5.0's getCallbacks() is never null
    // (returns &defaultCallback), so the old "skip if non-null" approach
    // silently prevented onSubscribe from ever firing.
    // One-time allocation per characteristic (~24 bytes each).
    for (auto* svc : getServerServices(_server)) {
        if (!svc) continue;
        for (auto* chr : svc->getCharacteristics()) {
            if (!chr) continue;
            auto* prev = chr->getCallbacks();
            auto* cb = new (std::nothrow) ChainingCharCallbacks(prev, s_chainingStack, s_chainingQueue);
            if (!cb) {
                ESP_LOGE(TAG, "Failed to allocate ChainingCharCallbacks");
                return false;
            }
            chr->setCallbacks(cb);
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

    // Fast connectable advertising interval: 50 ms (80 * 0.625 ms)
    pAdv->setMinInterval(80);
    pAdv->setMaxInterval(80);

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

    // A2: Don't call setPreferredParams() here — it adds AD bytes after the
    // packer has done its 31-byte accounting. The GAP PPCP characteristic
    // already exposes preferred params; let the central decide.

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
    // Preferred parameters are negotiated per connection via updateConnParams().
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

// --- Central Role (WO-3) ---

namespace {
    static void* s_hostQueue = nullptr;

    class ClientCallbacks : public NimBLEClientCallbacks {
    public:
        explicit ClientCallbacks(const char* providerId) {
            if (providerId) strncpy(_providerId, providerId, sizeof(_providerId) - 1);
        }
        void onConnect(NimBLEClient* /*pClient*/) override {}
        void onDisconnect(NimBLEClient* /*pClient*/, int reason) override {
            if (!s_hostQueue) return;
            BLEQueueMessage msg;
            msg.type = BLEQueueMessage::Type::CentralDisconnected;
            strncpy(msg.providerId, _providerId, sizeof(msg.providerId) - 1);
            switch (reason) {
                case 0x13: msg.reason = BLEDisconnectReason::PeerTerminated; break;
                case 0x16: msg.reason = BLEDisconnectReason::LocalTerminated; break;
                case 0x08: msg.reason = BLEDisconnectReason::SupervisionTimeout; break;
                case 0x3e: msg.reason = BLEDisconnectReason::ConnectFailed; break;
                default: msg.reason = BLEDisconnectReason::Other; break;
            }
            xQueueSend(static_cast<QueueHandle_t>(s_hostQueue), &msg, 0);
        }
    private:
        char _providerId[24] = {};
    };

    // N6: The old standalone notifyCB and s_clientProviderMap are removed.
    // Each subscription now captures providerId by value in its lambda,
    // eliminating the ordering gap (map populated after subscribe) and
    // the data race (map written on worker, read on NimBLE host task).

    class ScanCallbacks : public NimBLEScanCallbacks {
    public:
        // R8: Target service UUIDs for the active discovery, set by executeDiscovery()
        // before the scan starts. An empty list (targetUuidCount == 0) means no filtering.
        uint16_t targetUuids[6] = {};
        size_t   targetUuidCount = 0;

        void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
            if (!s_hostQueue) return;

            // R8: Filter by the provider's target service UUIDs (RFC #225 rev 2 §8.1).
            if (targetUuidCount > 0) {
                bool matches = false;
                for (size_t i = 0; i < targetUuidCount; ++i) {
                    if (advertisedDevice->isAdvertisingService(NimBLEUUID(targetUuids[i]))) {
                        matches = true;
                        break;
                    }
                }
                if (!matches) return;
            }

            BLEQueueMessage msg;
            msg.type = BLEQueueMessage::Type::ScanResult;

            NimBLEAddress addr = advertisedDevice->getAddress();
            BLEPeerAddress::parse(addr.toString().c_str(), static_cast<BLEPeerAddress::Type>(addr.getType()), msg.address);

            if (advertisedDevice->haveName()) {
                strncpy(msg.providerId, advertisedDevice->getName().c_str(), sizeof(msg.providerId) - 1);
            }

            // R8: RSSI now carried in its own dedicated field, not packed into handle.
            msg.rssi = (int8_t)advertisedDevice->getRSSI();
            // Issue #303: Carry connectable flag from NimBLE scan result to host loop.
            msg.boolVal1 = advertisedDevice->isConnectable();

            // Issue #308: Carry appearance and 16-bit service UUIDs across queue boundary.
            if (advertisedDevice->haveAppearance()) {
                msg.appearance = advertisedDevice->getAppearance();
            }

            uint8_t svcCount = advertisedDevice->getServiceUUIDCount();
            for (uint8_t i = 0; i < svcCount && msg.uuid16Count < 6; ++i) {
                NimBLEUUID u = advertisedDevice->getServiceUUID(i);
                if (u.bitSize() == 128) {
                    u.to16();
                }
                if (u.bitSize() == 16 && u.getBase() != nullptr) {
                    uint16_t val = reinterpret_cast<const ble_uuid16_t*>(u.getBase())->value;
                    bool exists = false;
                    for (uint8_t j = 0; j < msg.uuid16Count; ++j) {
                        if (msg.uuids16[j] == val) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        msg.uuids16[msg.uuid16Count++] = val;
                    }
                }
            }

            xQueueSend(static_cast<QueueHandle_t>(s_hostQueue), &msg, 0);
        }

        // R5: Use onScanEnd instead of sending ScanCompleted immediately
        void onScanEnd(const NimBLEScanResults& /*results*/, int /*reason*/) override {
            if (!s_hostQueue) return;
            BLEQueueMessage msg;
            msg.type = BLEQueueMessage::Type::ScanCompleted;
            xQueueSend(static_cast<QueueHandle_t>(s_hostQueue), &msg, 0);
        }
    };

    static std::map<std::string, NimBLEClient*> s_clients;

    // R8: Post a CentralDisconnected/ConnectFailed message to the main loop.
    // Factors the three identical failure-report blocks (createClient() nullptr,
    // connect() failure, required-subscription failure) into one helper.
    // R8 fix: the client is deliberately NOT deleted here — s_clients keeps one
    // client per provider for its lifetime, and executeCentralForget() is now the
    // only place a client is ever deleted, so s_clients can never hold a dangling
    // pointer to freed NimBLEClient memory.
    static void postCentralConnectFailure(void* hostEventQueue, const char* providerId, const BLEPeerAddress& addr) {
        if (!hostEventQueue) return;
        BLEQueueMessage failMsg;
        failMsg.type = BLEQueueMessage::Type::CentralDisconnected;
        failMsg.reason = BLEDisconnectReason::ConnectFailed;
        strncpy(failMsg.providerId, providerId, sizeof(failMsg.providerId) - 1);
        failMsg.address = addr;
        xQueueSend(static_cast<QueueHandle_t>(hostEventQueue), &failMsg, 0);
    }
}

bool BLEStack::executeCentralConnect(const CentralConnectOptions& opts, const BLEPeerAddress& addr, bool& bondedOut) {
    s_hostQueue = opts.hostEventQueue;
    const char* providerId = opts.provider->getProviderId();

    // R4/R8: One client per provider for its lifetime. Reuse the existing
    // client if we have one; only create (and set callbacks on) a new one
    // the first time. Connect/subscription failures below never delete this
    // client — executeCentralForget() is the sole deletion path — so s_clients
    // can never end up pointing at freed memory.
    NimBLEClient* pClient = nullptr;
    auto existingIt = s_clients.find(providerId);
    if (existingIt != s_clients.end() && existingIt->second) {
        pClient = existingIt->second;
        if (pClient->isConnected()) {
            pClient->disconnect();
        }
    } else {
        pClient = NimBLEDevice::createClient();
        if (!pClient) {
            postCentralConnectFailure(opts.hostEventQueue, providerId, addr);
            return false;
        }
        // N2: Set callbacks once at creation — NimBLE doesn't delete the old
        // callbacks on setClientCallbacks(), only in ~NimBLEClient(). Setting
        // on every reconnect leaks ~40 B per attempt.
        auto* cb = new (std::nothrow) ClientCallbacks(providerId);
        if (!cb) {
            ESP_LOGE(TAG, "Failed to allocate ClientCallbacks for %s", providerId);
            NimBLEDevice::deleteClient(pClient);
            postCentralConnectFailure(opts.hostEventQueue, providerId, addr);
            return false;
        }
        pClient->setClientCallbacks(cb, true);
    }

    // N6: Register client→provider mapping BEFORE subscribing so
    // early notifications can be routed. s_clients is also used for reuse.
    s_clients[providerId] = pClient;

    // R2: setConnectionParams takes (minInterval, maxInterval, latency, timeout) in 1.25ms units
    pClient->setConnectionParams(
        opts.centralConnParams.minIntervalUnits,
        opts.centralConnParams.maxIntervalUnits,
        opts.centralConnParams.latency,
        opts.centralConnParams.supervisionTimeoutUnits,
        // N4: Pass initiator scan params to avoid 100% duty default (16/16).
        // Convert ms → BLE units (0.625 ms per unit = ms * 1000 / 625).
        opts.initiatorScan.intervalMs * 1000 / 625,
        opts.initiatorScan.windowMs * 1000 / 625
    );
    
    // N9: BLEPeerAddress.bytes is little-endian (same byte order as ble_addr_t.val).
    // NimBLEAddress(uint8_t[], type) applies std::reverse_copy() expecting big-endian
    // esp_bd_addr_t — which would invert the address. Use the ble_addr_t constructor
    // which copies bytes directly without reversal.
    ble_addr_t rawAddr;
    rawAddr.type = static_cast<uint8_t>(addr.type);
    memcpy(rawAddr.val, addr.bytes, 6);
    NimBLEAddress bleAddr(rawAddr);
    if (!pClient->connect(bleAddr)) {
        // R3/R8: Report failure. The client is kept in s_clients for reuse on the
        // next attempt — it is never deleted here (see executeCentralForget()).
        postCentralConnectFailure(opts.hostEventQueue, providerId, addr);
        return false;
    }

    // N6: Subscribe BEFORE secureConnection() so HR data flows immediately.
    // SMP-silent peers would otherwise delay subscriptions by up to 30 s.
    // Capture providerId by value in the lambda — NimBLE 2.x notify_callback
    // is a std::function, so no shared map is needed.
    BLESubscriptionSpec subs[16];
    size_t count = opts.provider->getSubscriptions(subs, 16);
    bool requiredFailed = false;
    std::string provIdCapture(providerId);  // owned copy for lambda
    for (size_t i = 0; i < count; ++i) {
        NimBLERemoteService* pSvc = pClient->getService(subs[i].serviceUuid16);
        if (pSvc) {
            NimBLERemoteCharacteristic* pChr = pSvc->getCharacteristic(subs[i].charUuid16);
            if (pChr) {
                // N6: Lambda captures providerId by value — no shared map needed
                auto cb = [provIdCapture](NimBLERemoteCharacteristic* pRemChr,
                                           uint8_t* pData, size_t length, bool isNotify) {
                    if (!s_hostQueue || !pRemChr) return;
                    BLEQueueMessage msg;
                    msg.type = BLEQueueMessage::Type::Notification;
                    strncpy(msg.providerId, provIdCapture.c_str(), sizeof(msg.providerId) - 1);
                    // O3/N5: millis() for consistent time base
                    msg.receivedAtMs = (uint32_t)millis();

                    const NimBLEUUID& srvUuid = pRemChr->getRemoteService()->getUUID();
                    const NimBLEUUID& chrUuid = pRemChr->getUUID();
                    if (srvUuid.bitSize() == 16 && srvUuid.getBase() != nullptr) {
                        msg.attrHandle = reinterpret_cast<const ble_uuid16_t*>(srvUuid.getBase())->value;
                    }
                    if (chrUuid.bitSize() == 16 && chrUuid.getBase() != nullptr) {
                        msg.uuid16 = reinterpret_cast<const ble_uuid16_t*>(chrUuid.getBase())->value;
                    }

                    msg.rawLen = std::min(length, (size_t)64);
                    msg.boolVal1 = (length > 64); // truncated
                    memcpy(msg.rawBytes, pData, msg.rawLen);
                    xQueueSend(static_cast<QueueHandle_t>(s_hostQueue), &msg, 0);
                };
                bool ok = pChr->subscribe(!subs[i].indicate, cb, true);
                if (!ok && subs[i].required) {
                    ESP_LOGW("BLEStack", "Required subscription 0x%04X/0x%04X failed for '%s'",
                             subs[i].serviceUuid16, subs[i].charUuid16, providerId);
                    requiredFailed = true;
                }
            } else if (subs[i].required) {
                ESP_LOGW("BLEStack", "Required characteristic 0x%04X not found for '%s'",
                         subs[i].charUuid16, providerId);
                requiredFailed = true;
            }
        } else if (subs[i].required) {
            ESP_LOGW("BLEStack", "Required service 0x%04X not found for '%s'",
                     subs[i].serviceUuid16, providerId);
            requiredFailed = true;
        }
    }

    // O7: Opportunistic bonding — after subscribe so data flows before SMP
    bondedOut = pClient->secureConnection();

    // O8/R8: If a required subscription failed, disconnect and report failure.
    // The client is kept in s_clients for reuse — never deleted here.
    if (requiredFailed) {
        pClient->disconnect();
        postCentralConnectFailure(opts.hostEventQueue, providerId, addr);
        return false;
    }

    opts.provider->onClientReadyWorker(*pClient);

    // R4: s_clients[providerId] already set before subscribe (N6 fix)

    BLEQueueMessage msg;
    msg.type = BLEQueueMessage::Type::CentralConnected;
    msg.address = addr;
    msg.boolVal1 = bondedOut; // O7: carry bond result to main loop
    strncpy(msg.providerId, providerId, sizeof(msg.providerId) - 1);
    xQueueSend(static_cast<QueueHandle_t>(opts.hostEventQueue), &msg, 0);

    return true;
}

void BLEStack::executeCentralDisconnect(const char* providerId) {
    if (!providerId) return;
    auto it = s_clients.find(providerId);
    if (it != s_clients.end() && it->second) {
        it->second->disconnect();
    }
}

void BLEStack::executeCentralForget(const char* providerId, const BLEPeerAddress& addr) {
    if (!providerId) return;

    // O9: Delete the NVS bond unconditionally using the stored address.
    // This works even after reboot when no client object exists.
    // N9: Same byte-order fix as executeCentralConnect() — see comment there.
    ble_addr_t rawAddr;
    rawAddr.type = static_cast<uint8_t>(addr.type);
    memcpy(rawAddr.val, addr.bytes, 6);
    NimBLEAddress bleAddr(rawAddr);
    NimBLEDevice::deleteBond(bleAddr);

    // R8: This is the only place a client is ever deleted. Erase the s_clients
    // entry before deleting so no other code path can observe (let alone reuse)
    // a dangling pointer to freed NimBLEClient memory.
    auto it = s_clients.find(providerId);
    if (it != s_clients.end() && it->second) {
        NimBLEClient* pClient = it->second;
        s_clients.erase(it);
        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
    }
}

void BLEStack::executeDiscovery(const BLEScanParams& params, uint32_t durationMs, IBLEClientProvider* provider, void* hostEventQueue, std::vector<BLEScanResult>& /*resultsOut*/) {
    s_hostQueue = hostEventQueue;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    static ScanCallbacks scanCB;

    // R8: RFC #225 rev 2 §8.1 — filter scan results by the provider's target
    // service UUIDs. An empty list means no filtering (queue everything).
    scanCB.targetUuidCount = 0;
    if (provider) {
        scanCB.targetUuidCount = provider->getTargetServiceUUIDs(scanCB.targetUuids, 6);
    }

    pScan->setScanCallbacks(&scanCB, false);
    pScan->setInterval(params.intervalMs * 1000 / 625);
    pScan->setWindow(params.windowMs * 1000 / 625);
    pScan->setActiveScan(params.active);
    // R5: Don't let NimBLE accumulate scan results internally — we send each
    // one via the queue in onResult(). 0 = callbacks only.
    pScan->setMaxResults(0);
    // R5: NimBLE 2.x takes milliseconds directly — pass durationMs as-is.
    // ScanCompleted is sent by ScanCallbacks::onScanEnd, not here.
    pScan->start(durationMs, false);
}

void BLEStack::stopDiscovery() {
    NimBLEDevice::getScan()->stop();
}

bool BLEStack::isScanning() const {
    return NimBLEDevice::getScan()->isScanning();
}

void BLEStack::sendScanResult(void* /*hostEventQueue*/, const BLEScanResult& /*res*/) {
}
void BLEStack::sendScanComplete(void* /*hostEventQueue*/) {
}

#endif // defined(ESP_PLATFORM) && defined(WITH_BLE)
