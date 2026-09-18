/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEManager.cpp
 * Description: Implementation of managed Bluetooth Low Energy module.
 *              Authoritative: RFC #225 rev 2 §6, §10, §11, §14.
 *
 * Exported Classes/Methods:
 * - BLEManager::registerServiceProvider(): Registers Peripheral service contributor
 * - BLEManager::registerClientProvider(): Registers Central client contributor
 * - BLEManager::begin(): Wires Application and EventBus, locks registration
 * - BLEManager::tick(): Polling state machine and FreeRTOS queue drain
 * - BLEManager::inhibitScanning(): RAII scan suppression token
 * - BLEManager::pause() / resume(): Administrative pause controls
 * - BLEManager::applyConfig() / serializeConfig(): JSON persistence
 * - BLEManager::registerPreconditions(): Registers BLE Stack and Advertising preconditions
 */

#include "BLEManager.hpp"

#ifdef WITH_BLE

#include "types/AppState.hpp"
#include "EventBus.hpp"

#ifndef UNIT_TEST
class Application {
public:
    AppState getState() const;
    EventBus& getEventBus();
};
#else
class Application {
public:
    AppState getState() const { return _state; }
    void setState(AppState s) { _state = s; }
    EventBus& getEventBus() { return _bus; }
private:
    AppState _state = AppState::Running;
    EventBus _bus;
};
#endif

#include "interfaces/IPreconditionRegistry.hpp"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

static const char* TAG = "BLEManager";
static constexpr size_t QUEUE_CAPACITY = 32;
static constexpr size_t MAX_EVENTS_PER_TICK = 16;

// -----------------------------------------------------------------------------
// ScanInhibit RAII Implementation
// -----------------------------------------------------------------------------

BLEManager::ScanInhibit::ScanInhibit(BLEManager* mgr, const char* reason)
    : _mgr(mgr), _reason(reason) {}

BLEManager::ScanInhibit::~ScanInhibit() {
    if (_mgr) {
        _mgr->releaseScanInhibit(_reason);
    }
}

BLEManager::ScanInhibit::ScanInhibit(ScanInhibit&& other) noexcept
    : _mgr(other._mgr), _reason(other._reason) {
    other._mgr = nullptr;
    other._reason = nullptr;
}

BLEManager::ScanInhibit& BLEManager::ScanInhibit::operator=(ScanInhibit&& other) noexcept {
    if (this != &other) {
        if (_mgr) {
            _mgr->releaseScanInhibit(_reason);
        }
        _mgr = other._mgr;
        _reason = other._reason;
        other._mgr = nullptr;
        other._reason = nullptr;
    }
    return *this;
}

// -----------------------------------------------------------------------------
// BLEManager Lifecycle & Registration
// -----------------------------------------------------------------------------

BLEManager::BLEManager() {
    _eventQueue = xQueueCreate(QUEUE_CAPACITY, sizeof(BLEQueueMessage));
    if (!_eventQueue) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS event queue");
    }

    if (_config.enabled) {
        _state = State::Idle;
    } else {
        _state = State::Disabled;
    }
}

BLEManager::~BLEManager() {
    if (_eventQueue) {
        vQueueDelete(static_cast<QueueHandle_t>(_eventQueue));
        _eventQueue = nullptr;
    }
}

bool BLEManager::registerServiceProvider(IBLEServiceProvider* p) {
    if (_locked || _gattStarted || !p) {
        if (_locked || _gattStarted) {
            ESP_LOGW(TAG, "registerServiceProvider: rejected after registration locked or GATT started");
        }
        return false;
    }
    if (std::find(_serviceProviders.begin(), _serviceProviders.end(), p) == _serviceProviders.end()) {
        _serviceProviders.push_back(p);
    }
    return true;
}

bool BLEManager::registerClientProvider(IBLEClientProvider* p) {
    if (_locked || !p) {
        if (_locked) {
            ESP_LOGW(TAG, "registerClientProvider: rejected after registration locked");
        }
        return false;
    }
    if (std::find(_clientProviders.begin(), _clientProviders.end(), p) == _clientProviders.end()) {
        _clientProviders.push_back(p);
    }
    return true;
}

void BLEManager::begin(Application& app) {
    _app = &app;
    _eventBus = &app.getEventBus();
    _locked = true;
    ESP_LOGI(TAG, "BLEManager wired to Application. Providers locked.");
}

bool BLEManager::postTestQueueMessage(const BLEQueueMessage& msg) {
    if (!_eventQueue) return false;
    return xQueueSend(static_cast<QueueHandle_t>(_eventQueue), &msg, 0) == pdTRUE;
}

BLEManager::State BLEManager::getState() const {
    return _state;
}

bool BLEManager::isScanning() const {
    // WO-3: Will query active discovery or reconnect scan state
    return false;
}

bool BLEManager::isAdvertising() const {
    return _stack.isAdvertising();
}

size_t BLEManager::getPeripheralConnectionCount() const {
    return _activeConnections.size();
}

bool BLEManager::getPeerInfo(BLEConnHandle handle, BLEPeerInfo& out) const {
    auto it = std::find_if(_activeConnections.begin(), _activeConnections.end(),
                           [handle](const BLEPeerInfo& p) { return p.handle == handle; });
    if (it != _activeConnections.end()) {
        out = *it;
        return true;
    }
    return false;
}

uint32_t BLEManager::getDroppedEventsCount() const {
    return _droppedEventsCount + _stack.getDroppedEventsCount();
}

bool BLEManager::isClientConnected(const char* providerId) const {
    // WO-3: Will return connection status for client provider
    (void)providerId;
    return false;
}

bool BLEManager::getStoredDevice(const char* providerId, BLEStoredDevice& out) const {
    return _deviceRegistry.get(providerId, out);
}

// -----------------------------------------------------------------------------
// Discovery and Pairing Stubs (WO-3)
// -----------------------------------------------------------------------------

bool BLEManager::startDiscovery(const char* providerId, uint32_t durationMs) {
    (void)providerId;
    (void)durationMs;
    return false; // WO-3
}

void BLEManager::stopDiscovery() {
    // WO-3
}

size_t BLEManager::getDiscoveryResults(BLEScanResult* out, size_t max) const {
    (void)out;
    (void)max;
    return 0; // WO-3
}

bool BLEManager::pair(const char* providerId, const BLEPeerAddress& addr) {
    (void)providerId;
    (void)addr;
    return false; // WO-3
}

bool BLEManager::forget(const char* providerId) {
    (void)providerId;
    return false; // WO-3
}

// -----------------------------------------------------------------------------
// Coexistence & Co-processor Control
// -----------------------------------------------------------------------------

BLEManager::ScanInhibit BLEManager::inhibitScanning(const char* reason) {
    _scanInhibitCount++;
    ESP_LOGI(TAG, "Scan inhibited (reason: %s, active tokens: %u)",
             reason ? reason : "unspecified", _scanInhibitCount);
    return ScanInhibit(this, reason);
}

void BLEManager::releaseScanInhibit(const char* reason) {
    if (_scanInhibitCount > 0) {
        _scanInhibitCount--;
        ESP_LOGI(TAG, "Scan inhibit released (reason: %s, active tokens: %u)",
                 reason ? reason : "unspecified", _scanInhibitCount);
    }
}

void BLEManager::refreshAdvertising() {
    if (_state != State::Active && _state != State::Paused) {
        return;
    }

    BLEPeerAddress ownAddr;
    _stack.getOwnAddress(ownAddr);
    BLEAdvertisingBuilder advBuilder;
    for (auto* p : _serviceProviders) {
        if (p) p->contributeAdvertising(advBuilder);
    }

    _advPackResult = advBuilder.pack(_config.advertisedName.c_str(), _config.nameSuffix, ownAddr.bytes);
    if (_advPackResult.hasFault()) {
        ESP_LOGE(TAG, "BLE advertising pack fault during refresh: required element exceeds PDU capacity");
        const auto& fault = _advPackResult.faults.front();
        _faultRing[_faultRingHead].providerId   = "packer";
        _faultRing[_faultRingHead].elementBytes = fault.requiredBytes;
        _faultRing[_faultRingHead].freeBytes    = fault.availableBytes;
        if (_eventBus) {
            _eventBus->publish(BLEEvents::ADVERTISING_FAULT, &_faultRing[_faultRingHead]);
        }
        _faultRingHead = (_faultRingHead + 1) % RING_SIZE;
        return;
    }

    if (_state == State::Active &&
        BLEAdvertisingDecision::shouldAdvertise(_activeConnections.size(),
                                                _config.maxPeripheralConnections,
                                                false,
                                                _config.enabled)) {
        _stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams);
    }
}

void BLEManager::pause(const char* reason) {
    if (_state == State::Active) {
        _state = State::Paused;
        _pauseReason = reason ? reason : "administrative";
        ESP_LOGI(TAG, "BLE stack paused: %s", _pauseReason.c_str());

        // Stop advertising while paused
        _stack.stopAdvertising();

        // Disconnect peers if reason is OTA and otaDisconnectPeers is configured
        if (_config.otaDisconnectPeers && _pauseReason.find("ota") != std::string::npos) {
            ESP_LOGI(TAG, "Disconnecting all peripheral peers due to OTA pause");
            for (const auto& peer : _activeConnections) {
                _stack.disconnect(peer.handle, 0x13);
            }
        }

        publishState(_state, _pauseReason.c_str());
        for (auto* p : _serviceProviders) {
            if (p) p->onStackPaused();
        }
    }
}

void BLEManager::resume() {
    if (_state == State::Paused) {
        _state = State::Active;
        _pauseReason.clear();
        ESP_LOGI(TAG, "BLE stack resumed");

        publishState(_state, "resumed");
        for (auto* p : _serviceProviders) {
            if (p) p->onStackResumed();
        }

        if (BLEAdvertisingDecision::shouldAdvertise(_activeConnections.size(),
                                                _config.maxPeripheralConnections,
                                                false,
                                                _config.enabled)) {
            _stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams);
        }
    }
}

// -----------------------------------------------------------------------------
// Periodic Update Hook (ITickable)
// -----------------------------------------------------------------------------

void BLEManager::tick(uint32_t /*currentMillis*/) {
    // 1. Drain incoming messages from FreeRTOS queue (host -> main loop)
    dispatchQueueEvents();

    // 2. Lifecycle state machine progression (RFC §6.1, PROJECT.md D7)
    if (_state == State::Idle && _app != nullptr) {
        AppState appState = _app->getState();
        bool startNow = (appState == AppState::Running) ||
                        (appState == AppState::Configuring && _config.advertiseInConfiguring);

        if (startNow) {
            _state = State::Starting;
            publishState(_state, "starting");
            ESP_LOGI(TAG, "Transitioning Idle -> Starting. Initializing stack...");

            char nameBuf[40] = {};
            std::strncpy(nameBuf, _config.advertisedName.c_str(), sizeof(nameBuf) - 1);

            if (!_stack.init(nameBuf, _eventQueue)) {
                _state = State::Error;
                publishState(_state, "init_failed");
                ESP_LOGE(TAG, "BLE stack failed to initialize. State -> Error");
                return;
            }

            // GATT registration (done once per boot, static thereafter)
            if (!_gattStarted) {
                _gattStarted = true;
                NimBLEServer* server = _stack.getServer();
                std::vector<IBLEServiceProvider*> serviceOwners;

                for (auto* p : _serviceProviders) {
                    if (!p) continue;
                    size_t startIdx = _stack.getServiceCount();
                    bool ok = true;
                    if (server) {
                        ok = p->registerServices(*server);
                    }
                    size_t endIdx = _stack.getServiceCount();
                    for (size_t i = startIdx; i < endIdx; ++i) {
                        if (i >= serviceOwners.size()) {
                            serviceOwners.resize(i + 1, nullptr);
                        }
                        serviceOwners[i] = p;
                    }
                    if (!ok) {
                        ESP_LOGE(TAG, "Provider %s failed service registration", p->getProviderId());
                        _providerRegistrationFault = true;
                    }
                }

                if (!_stack.startServer()) {
                    ESP_LOGE(TAG, "BLE GATT start failed");
                    _providerRegistrationFault = true;
                }

                auto chars = _stack.getRegisteredCharacteristics();
                for (const auto& entry : chars) {
                    if (entry.serviceIndex < serviceOwners.size() && serviceOwners[entry.serviceIndex]) {
                        _routingTable.addRoute(entry.handle, serviceOwners[entry.serviceIndex]);
                    }
                }
            }

            // Aggregate advertising contributions and pack AD payloads
            BLEPeerAddress ownAddr;
            _stack.getOwnAddress(ownAddr);
            BLEAdvertisingBuilder advBuilder;
            for (auto* p : _serviceProviders) {
                if (p) p->contributeAdvertising(advBuilder);
            }
            _advPackResult = advBuilder.pack(_config.advertisedName.c_str(), _config.nameSuffix, ownAddr.bytes);
            if (_advPackResult.hasFault()) {
                ESP_LOGE(TAG, "BLE advertising pack fault: required element exceeds PDU capacity");
                const auto& fault = _advPackResult.faults.front();
                _faultRing[_faultRingHead].providerId   = "packer";
                _faultRing[_faultRingHead].elementBytes = fault.requiredBytes;
                _faultRing[_faultRingHead].freeBytes    = fault.availableBytes;
                if (_eventBus) {
                    _eventBus->publish(BLEEvents::ADVERTISING_FAULT, &_faultRing[_faultRingHead]);
                }
                _faultRingHead = (_faultRingHead + 1) % RING_SIZE;
            } else {
                if (BLEAdvertisingDecision::shouldAdvertise(_activeConnections.size(),
                                                            _config.maxPeripheralConnections,
                                                            false,
                                                            _config.enabled)) {
                    _stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams);
                }
            }

            _state = State::Active;
            publishReady();
            publishState(_state, "ready");
            ESP_LOGI(TAG, "BLE stack active and ready.");
        }
    }
}

void BLEManager::dispatchQueueEvents() {
    if (!_eventQueue) return;

    BLEQueueMessage msg;
    size_t processed = 0;
    auto queue = static_cast<QueueHandle_t>(_eventQueue);

    while (processed < MAX_EVENTS_PER_TICK && xQueueReceive(queue, &msg, 0) == pdTRUE) {
        processed++;

        switch (msg.type) {
            case BLEQueueMessage::Type::PeerConnected: {
                BLEPeerInfo peer;
                peer.handle = msg.handle;
                peer.address = msg.address;
                peer.mtu = msg.mtu;
                peer.params = msg.params;
                peer.encrypted = msg.boolVal1;

                auto it = std::find_if(_activeConnections.begin(), _activeConnections.end(),
                                       [h = msg.handle](const BLEPeerInfo& p) { return p.handle == h; });
                if (it != _activeConnections.end()) {
                    *it = peer;
                } else {
                    _activeConnections.push_back(peer);
                }

                // Request connection parameter update once after connect (RFC §9)
                _stack.updateConnParams(msg.handle, _config.peripheralPreferredConnParams);

                for (auto* p : _serviceProviders) {
                    if (p) p->onPeerConnected(peer);
                }

                // Stop advertising if reached cap
                if (!BLEAdvertisingDecision::shouldAdvertise(_activeConnections.size(),
                                                            _config.maxPeripheralConnections,
                                                            _state == State::Paused,
                                                            _config.enabled)) {
                    _stack.stopAdvertising();
                }

                _peerConnRing[_peerConnRingHead].handle = peer.handle;
                peer.address.toRedactedString(_peerConnRing[_peerConnRingHead].addrRedacted,
                                              sizeof(_peerConnRing[_peerConnRingHead].addrRedacted));
                _peerConnRing[_peerConnRingHead].activeCount = static_cast<uint8_t>(_activeConnections.size());
                if (_eventBus) {
                    _eventBus->publish(BLEEvents::PERIPHERAL_CONNECTED, &_peerConnRing[_peerConnRingHead]);
                }
                _peerConnRingHead = (_peerConnRingHead + 1) % RING_SIZE;
                break;
            }

            case BLEQueueMessage::Type::PeerDisconnected: {
                char redacted[18] = {};
                msg.address.toRedactedString(redacted, sizeof(redacted));

                auto it = std::find_if(_activeConnections.begin(), _activeConnections.end(),
                                       [h = msg.handle](const BLEPeerInfo& p) { return p.handle == h; });
                if (it != _activeConnections.end()) {
                    _activeConnections.erase(it);
                }

                for (auto* p : _serviceProviders) {
                    if (p) p->onPeerDisconnected(msg.handle, msg.reason);
                }

                // Restart advertising if below cap and active
                if (_state == State::Active &&
                    BLEAdvertisingDecision::shouldAdvertise(_activeConnections.size(),
                                                            _config.maxPeripheralConnections,
                                                            false,
                                                            _config.enabled)) {
                    if (!_stack.isAdvertising() && !_advPackResult.hasFault()) {
                        _stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams);
                    }
                }

                _peerConnRing[_peerConnRingHead].handle = msg.handle;
                std::memcpy(_peerConnRing[_peerConnRingHead].addrRedacted, redacted, sizeof(redacted));
                _peerConnRing[_peerConnRingHead].activeCount = static_cast<uint8_t>(_activeConnections.size());
                if (_eventBus) {
                    _eventBus->publish(BLEEvents::PERIPHERAL_DISCONNECTED, &_peerConnRing[_peerConnRingHead]);
                }
                _peerConnRingHead = (_peerConnRingHead + 1) % RING_SIZE;
                break;
            }

            case BLEQueueMessage::Type::SubscriptionChanged: {
                IBLEServiceProvider* prov = _routingTable.findProvider(msg.attrHandle);
                if (prov) {
                    prov->onSubscriptionChanged(msg.handle, msg.attrHandle, msg.uuid16, msg.boolVal1, msg.boolVal2);
                }
                break;
            }

            case BLEQueueMessage::Type::MTUChanged: {
                auto it = std::find_if(_activeConnections.begin(), _activeConnections.end(),
                                       [h = msg.handle](const BLEPeerInfo& p) { return p.handle == h; });
                if (it != _activeConnections.end()) {
                    it->mtu = msg.mtu;
                }
                break;
            }

            case BLEQueueMessage::Type::ConnParamsUpdated: {
                auto it = std::find_if(_activeConnections.begin(), _activeConnections.end(),
                                       [h = msg.handle](const BLEPeerInfo& p) { return p.handle == h; });
                if (it != _activeConnections.end()) {
                    it->params = msg.params;
                    it->encrypted = msg.boolVal1;
                }
                break;
            }

            default:
                break;
        }
    }
}

void BLEManager::publishState(State state, const char* reason) {
    _stateRing[_stateRingHead].state  = state;
    _stateRing[_stateRingHead].reason = reason;
    if (_eventBus) {
        _eventBus->publish(BLEEvents::STATE, &_stateRing[_stateRingHead]);
    }
    _stateRingHead = (_stateRingHead + 1) % RING_SIZE;
}

void BLEManager::publishReady() {
    _stateRing[_stateRingHead].state  = State::Active;
    _stateRing[_stateRingHead].reason = "ready";
    if (_eventBus) {
        _eventBus->publish(BLEEvents::READY, &_stateRing[_stateRingHead]);
    }
    _stateRingHead = (_stateRingHead + 1) % RING_SIZE;
}

// -----------------------------------------------------------------------------
// Configuration (IConfigurable)
// -----------------------------------------------------------------------------

void BLEManager::applyConfig(const JsonObject& root) {
    _config.enabled                = root["enabled"] | _config.enabled;
    _config.advertisedName         = root["advertisedName"] | _config.advertisedName;
    _config.nameSuffix             = root["nameSuffix"] | _config.nameSuffix;
    _config.advertiseInConfiguring = root["advertiseInConfiguring"] | _config.advertiseInConfiguring;
    _config.maxPeripheralConnections = root["maxPeripheralConnections"] | _config.maxPeripheralConnections;

    if (root["peripheralPreferredConnParams"].is<JsonObjectConst>()) {
        JsonObjectConst p = root["peripheralPreferredConnParams"].as<JsonObjectConst>();
        _config.peripheralPreferredConnParams.minIntervalUnits        = p["minIntervalUnits"] | _config.peripheralPreferredConnParams.minIntervalUnits;
        _config.peripheralPreferredConnParams.maxIntervalUnits        = p["maxIntervalUnits"] | _config.peripheralPreferredConnParams.maxIntervalUnits;
        _config.peripheralPreferredConnParams.latency                 = p["latency"] | _config.peripheralPreferredConnParams.latency;
        _config.peripheralPreferredConnParams.supervisionTimeoutUnits = p["supervisionTimeoutUnits"] | _config.peripheralPreferredConnParams.supervisionTimeoutUnits;
    }

    if (root["centralConnParams"].is<JsonObjectConst>()) {
        JsonObjectConst c = root["centralConnParams"].as<JsonObjectConst>();
        _config.centralConnParams.minIntervalUnits        = c["minIntervalUnits"] | _config.centralConnParams.minIntervalUnits;
        _config.centralConnParams.maxIntervalUnits        = c["maxIntervalUnits"] | _config.centralConnParams.maxIntervalUnits;
        _config.centralConnParams.latency                 = c["latency"] | _config.centralConnParams.latency;
        _config.centralConnParams.supervisionTimeoutUnits = c["supervisionTimeoutUnits"] | _config.centralConnParams.supervisionTimeoutUnits;
    }

    if (root["discoveryScan"].is<JsonObjectConst>()) {
        JsonObjectConst d = root["discoveryScan"].as<JsonObjectConst>();
        _config.discoveryScan.intervalMs = d["intervalMs"] | _config.discoveryScan.intervalMs;
        _config.discoveryScan.windowMs   = d["windowMs"] | _config.discoveryScan.windowMs;
        _config.discoveryScan.active     = d["active"] | _config.discoveryScan.active;
    }
    clampScanParams(_config.discoveryScan);

    if (root["reconnectScan"].is<JsonObjectConst>()) {
        JsonObjectConst r = root["reconnectScan"].as<JsonObjectConst>();
        _config.reconnectScan.intervalMs = r["intervalMs"] | _config.reconnectScan.intervalMs;
        _config.reconnectScan.windowMs   = r["windowMs"] | _config.reconnectScan.windowMs;
        _config.reconnectScan.active     = r["active"] | _config.reconnectScan.active;
    }
    clampScanParams(_config.reconnectScan);

    if (root["reconnectFastScheduleS"].is<JsonArrayConst>()) {
        _config.reconnectFastScheduleS.clear();
        for (uint32_t sec : root["reconnectFastScheduleS"].as<JsonArrayConst>()) {
            _config.reconnectFastScheduleS.push_back(sec);
        }
    }

    _config.slowRetryMs        = root["slowRetryMs"] | _config.slowRetryMs;
    _config.otaDisconnectPeers = root["otaDisconnectPeers"] | _config.otaDisconnectPeers;

    if (root["devices"].is<JsonObjectConst>()) {
        _deviceRegistry.deserialize(root["devices"].as<JsonObjectConst>());
    }
}

void BLEManager::serializeConfig(JsonObject& root) const {
    root["enabled"]                  = _config.enabled;
    root["advertisedName"]           = _config.advertisedName;
    root["nameSuffix"]               = _config.nameSuffix;
    root["advertiseInConfiguring"]   = _config.advertiseInConfiguring;
    root["maxPeripheralConnections"] = _config.maxPeripheralConnections;

    JsonObject pPref = root["peripheralPreferredConnParams"].to<JsonObject>();
    pPref["minIntervalUnits"]        = _config.peripheralPreferredConnParams.minIntervalUnits;
    pPref["maxIntervalUnits"]        = _config.peripheralPreferredConnParams.maxIntervalUnits;
    pPref["latency"]                 = _config.peripheralPreferredConnParams.latency;
    pPref["supervisionTimeoutUnits"] = _config.peripheralPreferredConnParams.supervisionTimeoutUnits;

    JsonObject cPref = root["centralConnParams"].to<JsonObject>();
    cPref["minIntervalUnits"]        = _config.centralConnParams.minIntervalUnits;
    cPref["maxIntervalUnits"]        = _config.centralConnParams.maxIntervalUnits;
    cPref["latency"]                 = _config.centralConnParams.latency;
    cPref["supervisionTimeoutUnits"] = _config.centralConnParams.supervisionTimeoutUnits;

    JsonObject disc = root["discoveryScan"].to<JsonObject>();
    disc["intervalMs"] = _config.discoveryScan.intervalMs;
    disc["windowMs"]   = _config.discoveryScan.windowMs;
    disc["active"]     = _config.discoveryScan.active;

    JsonObject reco = root["reconnectScan"].to<JsonObject>();
    reco["intervalMs"] = _config.reconnectScan.intervalMs;
    reco["windowMs"]   = _config.reconnectScan.windowMs;
    reco["active"]     = _config.reconnectScan.active;

    JsonArray sched = root["reconnectFastScheduleS"].to<JsonArray>();
    for (uint32_t s : _config.reconnectFastScheduleS) {
        sched.add(s);
    }

    root["slowRetryMs"]        = _config.slowRetryMs;
    root["otaDisconnectPeers"] = _config.otaDisconnectPeers;

    JsonObject devs = root["devices"].to<JsonObject>();
    _deviceRegistry.serialize(devs);
}

void BLEManager::resetConfig() {
    _config = BLEConfig();
    _deviceRegistry.clear();
}

// -----------------------------------------------------------------------------
// Web & MCP Extension Points (Stubs for WO-4)
// -----------------------------------------------------------------------------

void BLEManager::registerRoutes(PsychicHttpServer& server) {
    (void)server;
    // WO-4
}

void BLEManager::registerTabs(ITabRegistry& registry) {
    (void)registry;
    // WO-4
}

void BLEManager::registerMCPTools(MCPToolRegistry& registry) {
    (void)registry;
    // WO-4
}

// -----------------------------------------------------------------------------
// Preconditions (IPreconditionContributor)
// -----------------------------------------------------------------------------

void BLEManager::registerPreconditions(IPreconditionRegistry& registry) {
    // 1. "BLE Stack" Precondition (Operational)
    registry.add(
        "BLE Stack",
        PreconditionRole::Operational,
        [this]() -> PreconditionStatus {
            if (_state == State::Disabled) {
                return {false, Cause::Unavailable};
            }
            if (_state == State::Error || _providerRegistrationFault) {
                return {false, Cause::InternalError};
            }
            return {true, Cause::Met};
        },
        [](Cause cause, char* buf, size_t len) {
            switch (cause) {
                case Cause::Unavailable:
                    std::snprintf(buf, len, "BLE disabled in configuration");
                    break;
                case Cause::InternalError:
                    std::snprintf(buf, len, "BLE stack or provider GATT registration failed");
                    break;
                default:
                    std::snprintf(buf, len, "BLE stack operational");
                    break;
            }
        }
    );

    // 2. "BLE Advertising" Precondition (Operational)
    registry.add(
        "BLE Advertising",
        PreconditionRole::Operational,
        [this]() -> PreconditionStatus {
            if (_advPackResult.hasFault()) {
                return {false, Cause::Implausible};
            }
            return {true, Cause::Met};
        },
        [](Cause cause, char* buf, size_t len) {
            switch (cause) {
                case Cause::Implausible:
                    std::snprintf(buf, len, "Required BLE advertising element exceeded PDU capacity");
                    break;
                default:
                    std::snprintf(buf, len, "BLE advertising normal");
                    break;
            }
        }
    );

    // WO-3: register per-client-provider "BLE: <label>" preconditions here
}

#endif // WITH_BLE
