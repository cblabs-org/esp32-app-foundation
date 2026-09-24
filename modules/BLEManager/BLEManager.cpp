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

#include "framework_version.h"
#include <esp_mac.h>

#if defined(ESP_PLATFORM)
#include <Arduino.h>
#else
#include <esp_system.h>
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

#include "types/AppState.hpp"
#include "EventBus.hpp"

#ifndef UNIT_TEST
#include "Application.hpp"
#else
class Application {
public:
    AppState getState() const { return _state; }
    void setState(AppState s) { _state = s; }
    EventBus& getEventBus() { return _bus; }
    const char* getManufacturer() const { return "CB Labs"; }
    const char* getAppName() const { return "ESP32 App Foundation"; }
private:
    AppState _state = AppState::Running;
    EventBus _bus;
};
#endif

#include "interfaces/IPreconditionRegistry.hpp"
#include "interfaces/ITabRegistry.hpp"
#include "MCPToolRegistry.hpp"
#include "BLESerialization.hpp"
#include <PsychicHttp.h>

#if defined(ESP_PLATFORM)
#include "utils/serveGzipAsset.hpp"
#include "portal_assets/ble_js.hpp"
#endif

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
void BLEManager::centralWorkerTask(void* arg) {
    auto* mgr = static_cast<BLEManager*>(arg);
    while (true) {
        BLEManager::WorkerCommand cmd;
        if (xQueueReceive(static_cast<QueueHandle_t>(mgr->_workerQueue), &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd.type) {
                case BLEManager::WorkerCommand::Type::Connect: {
                    bool bonded = false;
                    BLEStack::CentralConnectOptions opts;
                    opts.initiatorScan = mgr->_config.reconnectScan;
                    // Coexistence hardening: clamp reconnect scan to 25% duty when peripheral connections exist
                    clampScanParams(opts.initiatorScan, cmd.hasPeripherals);
                    opts.centralConnParams = mgr->_config.centralConnParams;
                    
                    IBLEClientProvider* provider = nullptr;
                    for (auto* p : mgr->_clientProviders) {
                        if (p && strcmp(p->getProviderId(), cmd.providerId) == 0) {
                            provider = p;
                            break;
                        }
                    }
                    if (!provider) {
                        break;
                    }
                    opts.provider = provider;
                    opts.hostEventQueue = mgr->_eventQueue;

                    mgr->_stack.executeCentralConnect(opts, cmd.addr, bonded);
                    break;
                }
                case BLEManager::WorkerCommand::Type::Disconnect:
                    mgr->_stack.executeCentralDisconnect(cmd.providerId);
                    break;
                case BLEManager::WorkerCommand::Type::Forget: {
                    // R9: Only do BLE stack work on the worker task.
                    // Registry, events, provider callbacks, and persistence run on the main loop.
                    mgr->_stack.executeCentralForget(cmd.providerId, cmd.addr);
                    // Send a DeviceForgotten message to the main loop
                    BLEQueueMessage forgetMsg;
                    forgetMsg.type = BLEQueueMessage::Type::DeviceForgotten;
                    strncpy(forgetMsg.providerId, cmd.providerId, sizeof(forgetMsg.providerId) - 1);
                    forgetMsg.address = cmd.addr;
                    xQueueSend(static_cast<QueueHandle_t>(mgr->_eventQueue), &forgetMsg, 0);
                    break;
                }
                case BLEManager::WorkerCommand::Type::Discover: {
                    IBLEClientProvider* provider = nullptr;
                    for (auto* p : mgr->_clientProviders) {
                        if (p && strcmp(p->getProviderId(), cmd.providerId) == 0) {
                            provider = p;
                            break;
                        }
                    }
                    if (provider) {
                        std::vector<BLEScanResult> results;
                        BLEScanParams discParams = mgr->_config.discoveryScan;
                        clampScanParams(discParams, false);
                        mgr->_stack.executeDiscovery(discParams, cmd.durationMs, provider, mgr->_eventQueue, results);
                    }
                    break;
                }
                case BLEManager::WorkerCommand::Type::StopDiscovery:
                    mgr->_stack.stopDiscovery();
                    break;
                case BLEManager::WorkerCommand::Type::Shutdown:
                    vTaskDelete(nullptr);
                    return;
            }
        }
    }
}

BLEManager::BLEManager() {
    // Y6: Queues are safe to create during static init (heap alloc only).
    // Worker task creation is deferred to begin() (needs RTOS scheduler).
    _eventQueue = xQueueCreate(QUEUE_CAPACITY, sizeof(BLEQueueMessage));
    _workerQueue = xQueueCreate(QUEUE_CAPACITY, sizeof(WorkerCommand));
    if (!_eventQueue || !_workerQueue) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS queues");
    }

    if (_config.enabled) {
        _state = State::Idle;
    } else {
        _state = State::Disabled;
    }
}

BLEManager::~BLEManager() {
    if (_workerTask) {
        vTaskDelete(static_cast<TaskHandle_t>(_workerTask));
        _workerTask = nullptr;
    }
    if (_workerQueue) {
        vQueueDelete(static_cast<QueueHandle_t>(_workerQueue));
        _workerQueue = nullptr;
    }
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

// Issue #232: mirrors WifiManager::setDefaultHostname()
void BLEManager::setDefaultAdvertisedName(const char* name) {
    if (name && name[0]) {
        _defaultAdvertisedName = name;
    }
}

void BLEManager::begin(Application& app) {
    // Issue #232: auto-wire advertised name from app identity.
    // If the consumer already called setDefaultAdvertisedName(), honour that.
    // Otherwise, use the app name (which the consumer set via app.setAppName()).
    if (_defaultAdvertisedName == "ESP32") {
        const char* appName = app.getAppName();
        if (appName && appName[0]) {
            _defaultAdvertisedName = appName;
        }
    }
    // If the persisted config still has the struct default, replace with the app default
    if (_config.advertisedName == "ESP32") {
        _config.advertisedName = _defaultAdvertisedName;
    }
    // Y6: Worker task needs RTOS scheduler — create here, not in constructor
#if defined(ESP_PLATFORM)
    if (_workerQueue && !_workerTask) {
        // Y6: 6 KB stack — NimBLE connect + service discovery + onClientReadyWorker need headroom
        xTaskCreate(centralWorkerTask, "ble_worker", 6144, this, 5, reinterpret_cast<TaskHandle_t*>(&_workerTask));
    }
#endif

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    snprintf(_disSerial, sizeof(_disSerial), "%02X:%02X:%02X", mac[3], mac[4], mac[5]);
    snprintf(_disSoftwareRev, sizeof(_disSoftwareRev), "App Foundation %s", FRAMEWORK_VERSION);

    _disProvider = std::make_unique<DeviceInfoServiceProvider>(
        app.getManufacturer(),
        app.getAppName(),
        _disSerial,
        BUILD_VERSION,
        ESP.getChipModel(),
        _disSoftwareRev
    );
    registerServiceProvider(_disProvider.get());

    _app = &app;
    _eventBus = &app.getEventBus();
    _locked = true;
    ESP_LOGI(TAG, "BLEManager wired to Application. Providers locked.");

#ifdef WITH_OTA
    if (_eventBus) {
        _eventBus->subscribe(ApplicationEvents::OTA_PROGRESS, _onOtaProgress, this);
    }
#endif
}

#ifdef WITH_OTA
void BLEManager::_onOtaProgress(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<BLEManager*>(ctx);
    if (!payload || !self) return;

    // FALLBACK(#227): Single-slot payload may be mutated/overwritten by publisher.
    // Copy complete and success immediately out of the payload.
    // BLEManager reacts only to the first ota.progress event (pause) and to complete && !success (resume).
    // No progress-percentage tracking. Mark all fallback code with // FALLBACK(#227).
    const auto* ota = static_cast<const ApplicationEvents::OtaProgressPayload*>(payload);
    self->handleOtaProgress(ota->complete, ota->success);
}
#endif

void BLEManager::handleOtaProgress(bool complete, bool success) {
    // FALLBACK(#227): Copy fields into atomics immediately from event callback context.
    // The callback runs on the esp_event task. No BLE calls happen here.
    _otaComplete.store(complete, std::memory_order_relaxed);
    _otaSuccess.store(success, std::memory_order_relaxed);
    _pendingOtaEvent.store(true, std::memory_order_release);
}

void BLEManager::handleOtaProgressForTesting(bool complete, bool success) {
    handleOtaProgress(complete, success);
}


bool BLEManager::postTestQueueMessage(const BLEQueueMessage& msg) {
    if (!_eventQueue) return false;
    return xQueueSend(static_cast<QueueHandle_t>(_eventQueue), &msg, 0) == pdTRUE;
}

AppState BLEManager::getEffectiveAppState() const {
    if (_hasAppStateForTesting) return _appStateForTesting;
    return _app ? _app->getState() : AppState::Running;
}

BLEManager::State BLEManager::getState() const {
    return _state;
}

bool BLEManager::isScanning() const {
    if (_discovery.active) return true;
    for (const auto& kv : _clientConnecting) {
        if (kv.second) return true;
    }
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
    if (!providerId) return false;
    auto it = _clientConnected.find(providerId);
    return it != _clientConnected.end() ? it->second : false;
}

bool BLEManager::getStoredDevice(const char* providerId, BLEStoredDevice& out) const {
    return _deviceRegistry.get(providerId, out);
}

// -----------------------------------------------------------------------------
// Discovery and Pairing Stubs (WO-3)
// -----------------------------------------------------------------------------

bool BLEManager::startDiscovery(const char* providerId, uint32_t durationMs) {
    // Coexistence hardening: Running-only rule (RFC §6.1, RFC §9)
    if (getEffectiveAppState() != AppState::Running) {
        ESP_LOGW(TAG, "startDiscovery rejected: only allowed in AppState::Running");
        return false;
    }
    if (_state != State::Active || _scanInhibitCount > 0 || _state == State::Paused || _discovery.active) {
        return false;
    }
    if (durationMs > 30000) durationMs = 30000;


    _discovery.active = true;
    _discovery.startMs = millis();
    _discovery.durationMs = durationMs;
    strncpy(_discovery.providerId, providerId, sizeof(_discovery.providerId) - 1);
    _discovery.results.clear();

    WorkerCommand cmd;
    cmd.type = WorkerCommand::Type::Discover;
    strncpy(cmd.providerId, providerId, sizeof(cmd.providerId) - 1);
    cmd.durationMs = durationMs;
    xQueueSend(static_cast<QueueHandle_t>(_workerQueue), &cmd, 0);

    _scanRing[_scanRingHead].kind = BLEScanPayload::Kind::Discovery;
    _scanRing[_scanRingHead].reason = "discovery";
    if (_eventBus) _eventBus->publish(BLEEvents::SCAN_STARTED, &_scanRing[_scanRingHead]);
    _scanRingHead = (_scanRingHead + 1) % RING_SIZE;

    return true;
}

void BLEManager::stopDiscovery() {
    if (_discovery.active) {
        _discovery.active = false;
        WorkerCommand cmd;
        cmd.type = WorkerCommand::Type::StopDiscovery;
        xQueueSend(static_cast<QueueHandle_t>(_workerQueue), &cmd, 0);
    }
}

size_t BLEManager::getDiscoveryResults(BLEScanResult* out, size_t max) const {
    if (!out || max == 0) {
        return _discovery.results.size();
    }
    size_t count = std::min(max, _discovery.results.size());
    for (size_t i = 0; i < count; ++i) {
        out[i] = _discovery.results[i];
    }
    return count;
}

const std::vector<BLEScanResult>& BLEManager::getDiscoveryResults() const {
    return _discovery.results;
}

bool BLEManager::isDiscoveryActive() const {
    return _discovery.active;
}

const char* BLEManager::getDiscoveryProviderId() const {
    return _discovery.providerId;
}

uint32_t BLEManager::getDiscoveryDurationMs() const {
    return _discovery.durationMs;
}

bool BLEManager::getReconnectPolicy(const char* providerId, BLEReconnectPolicy& out) const {
    if (!providerId) return false;
    auto it = _reconnectPolicies.find(providerId);
    if (it != _reconnectPolicies.end()) {
        out = it->second;
        return true;
    }
    return false;
}

bool BLEManager::pair(const char* providerId, const BLEPeerAddress& addr) {
    if (!providerId) return false;

    BLEStoredDevice device;
    device.address = addr;
    strncpy(device.providerId, providerId, sizeof(device.providerId) - 1);
    
    _deviceRegistry.set(device);
    // R8: Persist paired devices to flash so they survive reboot
#if defined(ESP_PLATFORM)
    if (_app) {
        _app->getConfigManager().saveModuleConfig(this);
    }
#endif

    WorkerCommand cmd;
    cmd.type = WorkerCommand::Type::Connect;
    strncpy(cmd.providerId, providerId, sizeof(cmd.providerId) - 1);
    cmd.addr = addr;
    xQueueSend(static_cast<QueueHandle_t>(_workerQueue), &cmd, 0);

    return true;
}

bool BLEManager::forget(const char* providerId) {
    if (!providerId) return false;

    // R9: Synchronously prevent the reconnect race — if tick() runs before
    // the worker processes Forget, it must NOT queue a Connect.
    _clientConnected.erase(providerId);
    _clientConnecting.erase(providerId);
    _reconnectPolicies.erase(providerId);

    WorkerCommand cmd;
    cmd.type = WorkerCommand::Type::Forget;
    strncpy(cmd.providerId, providerId, sizeof(cmd.providerId) - 1);

    // O9: Copy the stored address so the worker can delete the NVS bond
    // even after reboot (when no live client object exists).
    BLEStoredDevice stored;
    if (_deviceRegistry.get(providerId, stored)) {
        cmd.addr = stored.address;
    }

    xQueueSend(static_cast<QueueHandle_t>(_workerQueue), &cmd, 0);

    return true;
}

// -----------------------------------------------------------------------------
// Coexistence & Co-processor Control
// -----------------------------------------------------------------------------

BLEManager::ScanInhibit BLEManager::inhibitScanning(const char* reason) {
    auto guard = lockState();  // O2: safe to call from any task
    _scanInhibitCount++;
    ESP_LOGI(TAG, "Scan inhibited (reason: %s, active tokens: %u)",
             reason ? reason : "unspecified", _scanInhibitCount.load());

    if (_scanInhibitCount == 1) {
        if (_discovery.active) {
            stopDiscovery();
            _scanRing[_scanRingHead].kind = BLEScanPayload::Kind::Discovery;
            _scanRing[_scanRingHead].reason = "inhibited";
            if (_eventBus) _eventBus->publish(BLEEvents::SCAN_STOPPED, &_scanRing[_scanRingHead]);
            _scanRingHead = (_scanRingHead + 1) % RING_SIZE;
        }
        _stack.stopDiscovery();

        // Suspend reconnect policies
        for (auto& kv : _reconnectPolicies) {
            kv.second.suspend();
        }
    }

    return ScanInhibit(this, reason);
}

void BLEManager::releaseScanInhibit(const char* reason) {
    auto guard = lockState();  // O2: safe to call from any task
    if (_scanInhibitCount > 0) {
        _scanInhibitCount--;
        ESP_LOGI(TAG, "Scan inhibit released (reason: %s, active tokens: %u)",
                 reason ? reason : "unspecified", _scanInhibitCount.load());

        if (_scanInhibitCount == 0 && _state == State::Active) {
            uint32_t nowMs = millis();
            for (auto& kv : _reconnectPolicies) {
                kv.second.resume(nowMs);
            }
        }
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
        // O6: Don't return — still advertise with the faulting element dropped
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

        // Stop active scans while paused
        if (_discovery.active) {
            stopDiscovery();
            _scanRing[_scanRingHead].kind = BLEScanPayload::Kind::Discovery;
            _scanRing[_scanRingHead].reason = "paused";
            if (_eventBus) _eventBus->publish(BLEEvents::SCAN_STOPPED, &_scanRing[_scanRingHead]);
            _scanRingHead = (_scanRingHead + 1) % RING_SIZE;
        }
        _stack.stopDiscovery();

        // Suspend reconnect policies while paused
        for (auto& kv : _reconnectPolicies) {
            kv.second.suspend();
        }

        // Y4: Only disconnect peers if the pause reason is exactly "ota" (case-insensitive)
        if (_config.otaDisconnectPeers && (_pauseReason == "ota" || _pauseReason == "OTA")) {
            ESP_LOGI(TAG, "Disconnecting all peripheral peers due to OTA pause");
            for (const auto& peer : _activeConnections) {
                _stack.disconnect(peer.handle, 0x13);
            }
            for (const auto& kv : _clientConnected) {
                if (kv.second) {
                    WorkerCommand cmd;
                    cmd.type = WorkerCommand::Type::Disconnect;
                    strncpy(cmd.providerId, kv.first.c_str(), sizeof(cmd.providerId) - 1);
                    xQueueSend(static_cast<QueueHandle_t>(_workerQueue), &cmd, 0);
                }
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
        _otaPaused = false;
        ESP_LOGI(TAG, "BLE stack resumed");

        publishState(_state, "resumed");
        for (auto* p : _serviceProviders) {
            if (p) p->onStackResumed();
        }

        // Resume reconnect policies if not inhibited
        if (_scanInhibitCount == 0) {
            uint32_t nowMs = millis();
            for (auto& kv : _reconnectPolicies) {
                kv.second.resume(nowMs);
            }
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

    // 1.5 Process pending OTA event (RFC #225 rev 2 §6.1, PROJECT.md D8)
    if (_pendingOtaEvent.exchange(false, std::memory_order_acq_rel)) {
        // FALLBACK(#227): BLEManager reacts only to the first ota.progress event (pause)
        // and to complete && !success (resume), copying fields immediately.
        // No progress-percentage tracking. Mark all fallback code with // FALLBACK(#227).
        bool complete = _otaComplete.load(std::memory_order_relaxed);
        bool success = _otaSuccess.load(std::memory_order_relaxed);

        if (!complete) {
            if (!_otaPaused) {
                _otaPaused = true;
                pause("ota");
            }
        } else {
            _otaPaused = false;
            if (!success) {
                resume();
            }
            // on complete && success, do nothing (the device reboots)
        }
    }

    // 2. Lifecycle state machine progression (RFC §6.1, PROJECT.md D7)
    if (_state == State::Idle && (_app != nullptr || _hasAppStateForTesting)) {
        AppState appState = getEffectiveAppState();
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
            }
            // O6: Always attempt to advertise — the packer already dropped the
            // offending element. Making the device undiscoverable is worse than
            // advertising without a missing UUID.
            if (BLEAdvertisingDecision::shouldAdvertise(_activeConnections.size(),
                                                        _config.maxPeripheralConnections,
                                                        false,
                                                        _config.enabled)) {
                _stack.startAdvertising(_advPackResult, _config.peripheralPreferredConnParams);
            }

            _state = State::Active;
            publishReady();
            publishState(_state, "ready");
            ESP_LOGI(TAG, "BLE stack active and ready.");

            // Start reconnect policies for stored devices if in Running state
            if (getEffectiveAppState() == AppState::Running) {
                uint32_t nowMs = millis();
                for (auto* p : _clientProviders) {
                    if (p) {
                        BLEStoredDevice stored;
                        if (_deviceRegistry.get(p->getProviderId(), stored)) {
                            _reconnectPolicies[p->getProviderId()].start(nowMs);
                        }
                    }
                }
            }
        }
    }

    // Ensure reconnect policies start once entering Running state
    if (_state == State::Active && getEffectiveAppState() == AppState::Running) {
        uint32_t nowMs = millis();
        for (auto* p : _clientProviders) {
            if (p) {
                const char* pid = p->getProviderId();
                BLEStoredDevice stored;
                if (_deviceRegistry.get(pid, stored)) {
                    auto& policy = _reconnectPolicies[pid];
                    // O5: Apply config to newly created policies
                    policy.setFastSchedule(_config.reconnectFastScheduleS);
                    policy.setSlowRetryMs(_config.slowRetryMs);
                    if (policy.phase() == BLEReconnectPolicy::Phase::Idle && !_clientConnected[pid]) {
                        policy.start(nowMs);
                    }
                }
            }
        }
    }

    // Drive Reconnect Policies (Running-only rule: RFC §6.1, RFC §9)
    if (_state == State::Active &&
        getEffectiveAppState() == AppState::Running &&
        _scanInhibitCount == 0 &&
        !_discovery.active) {
        uint32_t nowMs = millis();
        for (auto* p : _clientProviders) {
            if (!p) continue;
            const char* pid = p->getProviderId();
            BLEStoredDevice stored;
            if (_deviceRegistry.get(pid, stored)) {
                auto& policy = _reconnectPolicies[pid];
                if (policy.phase() != BLEReconnectPolicy::Phase::Idle && policy.phase() != BLEReconnectPolicy::Phase::Exhausted) {
                    uint32_t nextAt = policy.nextAttemptAtMs(nowMs);
                    // A3: Use signed diff to survive 49.7-day millis() wrap
                    if (!_clientConnecting[pid] && nextAt > 0 && (int32_t)(nowMs - nextAt) >= 0) {
                        _clientConnecting[pid] = true;
                        
                        WorkerCommand cmd;
                        cmd.type = WorkerCommand::Type::Connect;
                        strncpy(cmd.providerId, pid, sizeof(cmd.providerId) - 1);
                        cmd.addr = stored.address;
                        cmd.hasPeripherals = (_activeConnections.size() > 0);
                        xQueueSend(static_cast<QueueHandle_t>(_workerQueue), &cmd, 0);
                        
                        // We do NOT advance the schedule here; onCentralDisconnected will do that
                    }
                }
            }
        }
    }
}

void BLEManager::dispatchQueueEvents() {
    if (!_eventQueue) return;
    auto guard = lockState();  // O2: hold while processing events

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
                    // O6: Advertise even with a packer fault — the faulting
                    // element was already dropped, remaining data is valid.
                    if (!_stack.isAdvertising()) {
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

            case BLEQueueMessage::Type::CentralConnected: {
                _clientConnected[msg.providerId] = true;
                _clientConnecting[msg.providerId] = false;
                _reconnectPolicies[msg.providerId].onConnected();
                IBLEClientProvider* prov = nullptr;
                for (auto* p : _clientProviders) {
                    if (p && strcmp(p->getProviderId(), msg.providerId) == 0) {
                        prov = p;
                        break;
                    }
                }
                // O7: Consume bond result — update stored device if bonded
                BLEStoredDevice stored;
                if (_deviceRegistry.get(msg.providerId, stored)) {
                    if (msg.boolVal1 && !stored.bonded) {
                        stored.bonded = true;
                        _deviceRegistry.set(stored);
                        #if defined(ESP_PLATFORM)
                        if (_app) _app->getConfigManager().saveModuleConfig(this);
                        #endif
                    }
                    // Also copy device name from discovery results if available
                    if (stored.name[0] == '\0' && !_discovery.results.empty()) {
                        for (size_t i = 0; i < _discovery.results.size(); ++i) {
                            if (_discovery.results[i].address == stored.address) {
                                strncpy(stored.name, _discovery.results[i].name, sizeof(stored.name) - 1);
                                _deviceRegistry.set(stored);
                                break;
                            }
                        }
                    }
                    if (prov) prov->onConnected(stored);
                }
                // O4: Publish ble.central.connected via ring buffer (not stack)
                if (_eventBus) {
                    auto& cp = _centralConnRing[_centralConnRingHead];
                    // N3: Use provider's getProviderId() (static lifetime) — not
                    // msg.providerId which is a stack-local that goes away.
                    cp.providerId = prov ? prov->getProviderId() : "";
                    if (_deviceRegistry.get(msg.providerId, stored)) {
                        stored.address.toRedactedString(cp.addrRedacted, sizeof(cp.addrRedacted));
                    }
                    cp.reason = BLEDisconnectReason::Other; // not a disconnect
                    cp.nextRetryMs = 0;
                    _eventBus->publish(BLEEvents::CENTRAL_CONNECTED, &cp);
                    _centralConnRingHead = (_centralConnRingHead + 1) % RING_SIZE;
                }
                break;
            }

            case BLEQueueMessage::Type::CentralDisconnected: {
                _clientConnected[msg.providerId] = false;
                _clientConnecting[msg.providerId] = false;
                IBLEClientProvider* prov = nullptr;
                for (auto* p : _clientProviders) {
                    if (p && strcmp(p->getProviderId(), msg.providerId) == 0) {
                        prov = p;
                        break;
                    }
                }
                if (prov) {
                    prov->onDisconnected(msg.reason);
                }
                _reconnectPolicies[msg.providerId].onAttemptFailed(millis());
                // O4: Publish via ring buffer, not stack payload
                if (_eventBus) {
                    auto& cp = _centralConnRing[_centralConnRingHead];
                    // N3: Use provider's getProviderId() (static lifetime)
                    cp.providerId = prov ? prov->getProviderId() : "";
                    BLEStoredDevice stored;
                    if (_deviceRegistry.get(msg.providerId, stored)) {
                        stored.address.toRedactedString(cp.addrRedacted, sizeof(cp.addrRedacted));
                    }
                    cp.reason = msg.reason;
                    uint32_t nowMs = millis();
                    uint32_t nextAt = _reconnectPolicies[msg.providerId].nextAttemptAtMs(nowMs);
                    cp.nextRetryMs = (nextAt > nowMs) ? (nextAt - nowMs) : 0; // relative delay
                    _eventBus->publish(BLEEvents::CENTRAL_DISCONNECTED, &cp);
                    _centralConnRingHead = (_centralConnRingHead + 1) % RING_SIZE;
                }
                break;
            }

            case BLEQueueMessage::Type::Notification: {
                IBLEClientProvider* prov = nullptr;
                for (auto* p : _clientProviders) {
                    if (p && strcmp(p->getProviderId(), msg.providerId) == 0) {
                        prov = p;
                        break;
                    }
                }
                if (prov) {
                    BLENotification n;
                    n.serviceUuid16 = msg.attrHandle; // packed in attrHandle
                    n.charUuid16 = msg.uuid16;
                    n.length = msg.rawLen;
                    n.truncated = msg.boolVal1;
                    // O3: Use the host-task timestamp from notifyCB
                    n.receivedAtMs = msg.receivedAtMs ? msg.receivedAtMs : millis();
                    memcpy(n.data, msg.rawBytes, n.length);
                    prov->onNotification(n);
                }
                break;
            }

            case BLEQueueMessage::Type::ScanResult: {
                IBLEClientProvider* prov = nullptr;
                for (auto* p : _clientProviders) {
                    if (p && _discovery.providerId[0] && strcmp(p->getProviderId(), _discovery.providerId) == 0) {
                        prov = p;
                        break;
                    }
                }
                if (prov) {
                    BLEScanResult res;
                    res.address = msg.address;
                    strncpy(res.name, msg.providerId, sizeof(res.name) - 1);
                    if (prov->acceptScanResult(res)) {
                        bool found = false;
                        for (auto& existing : _discovery.results) {
                            if (existing.address == res.address) {
                                existing = res;
                                found = true;
                                break;
                            }
                        }
                        if (!found && _discovery.results.size() < 16) { // Y3: RFC §8.1 cap
                            _discovery.results.push_back(res);
                        }
                    }
                }
                break;
            }

            case BLEQueueMessage::Type::ScanCompleted: {
                _discovery.active = false;
                _scanRing[_scanRingHead].kind = BLEScanPayload::Kind::Discovery;
                _scanRing[_scanRingHead].reason = "completed";
                if (_eventBus) _eventBus->publish(BLEEvents::SCAN_STOPPED, &_scanRing[_scanRingHead]);
                _scanRingHead = (_scanRingHead + 1) % RING_SIZE;
                break;
            }

            // R9: Handle DeviceForgotten on the main loop (thread-safe)
            case BLEQueueMessage::Type::DeviceForgotten: {
                _deviceRegistry.remove(msg.providerId);
                // Note: _clientConnected, _clientConnecting, _reconnectPolicies
                // already erased synchronously in forget() to prevent the reconnect race.
                // Persist after removal
#if defined(ESP_PLATFORM)
                if (_app) {
                    _app->getConfigManager().saveModuleConfig(this);
                }
#endif
                // Notify provider
                IBLEClientProvider* prov = nullptr;
                for (auto* p : _clientProviders) {
                    if (p && strcmp(p->getProviderId(), msg.providerId) == 0) {
                        prov = p;
                        break;
                    }
                }
                if (prov) prov->onDeviceForgotten();
                // Publish event via ring buffer
                if (_eventBus) {
                    auto& dp = _deviceRing[_deviceRingHead];
                    // N3: Use provider's getProviderId() (static lifetime)
                    dp.providerId = prov ? prov->getProviderId() : "";
                    msg.address.toRedactedString(dp.addrRedacted, sizeof(dp.addrRedacted));
                    _eventBus->publish(BLEEvents::DEVICE_FORGOTTEN, &dp);
                    _deviceRingHead = (_deviceRingHead + 1) % RING_SIZE;
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

    // O5: Apply reconnect schedule to all active policies
    for (auto& kv : _reconnectPolicies) {
        kv.second.setFastSchedule(_config.reconnectFastScheduleS);
        kv.second.setSlowRetryMs(_config.slowRetryMs);
    }

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

void BLEManager::registerTabs(ITabRegistry& registry) {
    registry.addTab(
        "Bluetooth",
        nullptr,
        15,   // was 60; WiFi=10, Security=50, System=900
        "<polyline points=\"6.5 6.5 17.5 17.5 12 23 12 1 17.5 6.5 6.5 17.5\"></polyline>",
        "/ble.js"
    );
}

void BLEManager::registerRoutes(PsychicHttpServer& server) {
#if defined(ESP_PLATFORM)
    // 1. Static Web Asset
    server.on("/ble.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        return serveGzipAsset(request, response, ble_js_gz, ble_js_gz_len, ble_js_gz_etag, "application/javascript");
    });
#endif

    // 2. GET /api/ble/status (Full unredacted addresses)
    server.on("/api/ble/status", HTTP_GET, [this](PsychicRequest* /*request*/, PsychicResponse* response) {
        auto guard = lockState();  // O2: serialize with tick()
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        BLESerialization::serializeStatus(root, *this, /*redactAddresses=*/false);
        String json;
        serializeJson(doc, json);
        return response->send(200, "application/json", json.c_str());
    });

    // 3. POST /api/ble/scan
    server.on("/api/ble/scan", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        auto guard = lockState();  // O2: serialize with tick()
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"empty body\"}");
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            return response->send(400, "application/json", "{\"error\":\"invalid json\"}");
        }
        const char* providerId = doc["providerId"] | "";
        if (!providerId || strlen(providerId) == 0) {
            return response->send(400, "application/json", "{\"error\":\"missing providerId\"}");
        }
        bool found = false;
        for (auto* p : _clientProviders) {
            if (p && strcmp(p->getProviderId(), providerId) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return response->send(400, "application/json", "{\"error\":\"unknown providerId\"}");
        }
        uint32_t durationMs = doc["durationMs"] | 30000;
        if (durationMs == 0 || durationMs > 30000) {
            return response->send(400, "application/json", "{\"error\":\"durationMs must be between 1 and 30000\"}");
        }
        if (_state != State::Active) {
            return response->send(409, "application/json", "{\"error\":\"BLE not active\"}");
        }
        if (_scanInhibitCount > 0) {
            return response->send(409, "application/json", "{\"error\":\"scanning inhibited\"}");
        }
        if (isScanning() || _discovery.active) {
            return response->send(409, "application/json", "{\"error\":\"scan already in progress\"}");
        }
        if (!startDiscovery(providerId, durationMs)) {
            return response->send(409, "application/json", "{\"error\":\"failed to start scan\"}");
        }
        return response->send(202, "application/json", "{\"status\":\"scanning\"}");
    });

    // 4. GET /api/ble/scan/results
    server.on("/api/ble/scan/results", HTTP_GET, [this](PsychicRequest* /*request*/, PsychicResponse* response) {
        auto guard = lockState();  // O2: serialize with tick()
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        BLESerialization::serializeScanResults(arr, _discovery.results.data(), _discovery.results.size(), /*redactAddresses=*/false);
        String json;
        serializeJson(doc, json);
        return response->send(200, "application/json", json.c_str());
    });

    // 5. POST /api/ble/pair
    server.on("/api/ble/pair", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        auto guard = lockState();  // O2: serialize with tick()
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"empty body\"}");
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            return response->send(400, "application/json", "{\"error\":\"invalid json\"}");
        }
        const char* providerId = doc["providerId"] | "";
        if (!providerId || strlen(providerId) == 0) {
            return response->send(400, "application/json", "{\"error\":\"missing providerId\"}");
        }
        bool found = false;
        for (auto* p : _clientProviders) {
            if (p && strcmp(p->getProviderId(), providerId) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return response->send(404, "application/json", "{\"error\":\"unknown providerId\"}");
        }
        const char* addrStr = doc["address"] | "";
        const char* typeStr = doc["addressType"] | "";
        BLEPeerAddress::Type addrType;
        if (!BLESerialization::parseAddressType(typeStr, addrType)) {
            return response->send(400, "application/json", "{\"error\":\"invalid addressType\"}");
        }
        BLEPeerAddress addr;
        if (!BLEPeerAddress::parse(addrStr, addrType, addr) || !addr.isValid()) {
            return response->send(400, "application/json", "{\"error\":\"invalid address\"}");
        }
        if (!pair(providerId, addr)) {
            return response->send(500, "application/json", "{\"error\":\"pair failed\"}");
        }
        return response->send(202, "application/json", "{\"status\":\"pairing\"}");
    });

    // 6. POST /api/ble/forget
    server.on("/api/ble/forget", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        auto guard = lockState();  // O2: serialize with tick()
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"empty body\"}");
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            return response->send(400, "application/json", "{\"error\":\"invalid json\"}");
        }
        const char* providerId = doc["providerId"] | "";
        if (!providerId || strlen(providerId) == 0) {
            return response->send(400, "application/json", "{\"error\":\"missing providerId\"}");
        }
        bool found = false;
        for (auto* p : _clientProviders) {
            if (p && strcmp(p->getProviderId(), providerId) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return response->send(404, "application/json", "{\"error\":\"unknown providerId\"}");
        }
        if (!forget(providerId)) {
            return response->send(500, "application/json", "{\"error\":\"forget failed\"}");
        }
        return response->send(200, "application/json", "{\"status\":\"forgotten\"}");
    });
}

void BLEManager::registerMCPTools(MCPToolRegistry& registry) {
    // 1. ble_status (Read-only, mutatesState = false, addresses REDACTED per RFC §15)
    registry.addTool(
        "ble_status",
        "Get Bluetooth Low Energy manager status, connection info, stored devices, and advertising state (addresses redacted for privacy)",
        [](void* ctx, JsonObject& /*params*/, JsonObject& result) {
            auto* mgr = static_cast<BLEManager*>(ctx);
            auto guard = mgr->lockState();  // O2: serialize with tick()
            BLESerialization::serializeStatus(result, *mgr, /*redactAddresses=*/true);
        },
        this,
        /*mutatesState=*/false
    );

    // 2. ble_scan (Mutating, mutatesState = true)
    JsonDocument scanSchema;
    scanSchema["type"] = "object";
    JsonObject scanProps = scanSchema["properties"].to<JsonObject>();
    scanProps["providerId"]["type"] = "string";
    scanProps["providerId"]["description"] = "Client provider identifier (e.g. 'hrm')";
    scanProps["durationMs"]["type"] = "integer";
    scanProps["durationMs"]["description"] = "Scan duration in milliseconds (max 30000, default 30000)";
    JsonArray scanReq = scanSchema["required"].to<JsonArray>();
    scanReq.add("providerId");

    registry.addTool(
        "ble_scan",
        "Start user-initiated BLE discovery scan for a central client provider",
        scanSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* mgr = static_cast<BLEManager*>(ctx);
            auto guard = mgr->lockState();  // O2: serialize with tick()
            const char* providerId = params["providerId"] | "";
            if (!providerId || strlen(providerId) == 0) {
                result["error"] = "missing providerId";
                result["isError"] = true;
                return;
            }
            bool found = false;
            for (auto* p : mgr->getClientProviders()) {
                if (p && strcmp(p->getProviderId(), providerId) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result["error"] = "unknown providerId";
                result["isError"] = true;
                return;
            }
            uint32_t durationMs = params["durationMs"] | 30000;
            if (durationMs == 0 || durationMs > 30000) {
                result["error"] = "durationMs must be between 1 and 30000";
                result["isError"] = true;
                return;
            }
            if (mgr->getState() != State::Active) {
                result["error"] = "BLE not active";
                result["isError"] = true;
                return;
            }
            if (mgr->getScanInhibitCount() > 0) {
                result["error"] = "scanning inhibited";
                result["isError"] = true;
                return;
            }
            if (mgr->isScanning() || mgr->isDiscoveryActive()) {
                result["error"] = "scan already in progress";
                result["isError"] = true;
                return;
            }
            if (!mgr->startDiscovery(providerId, durationMs)) {
                result["error"] = "failed to start scan";
                result["isError"] = true;
                return;
            }
            result["status"] = "scanning";
            result["providerId"] = providerId;
            result["durationMs"] = durationMs;
        },
        this,
        /*mutatesState=*/true
    );

    // 3. ble_pair (Mutating, mutatesState = true)
    JsonDocument pairSchema;
    pairSchema["type"] = "object";
    JsonObject pairProps = pairSchema["properties"].to<JsonObject>();
    pairProps["providerId"]["type"] = "string";
    pairProps["providerId"]["description"] = "Client provider identifier";
    pairProps["address"]["type"] = "string";
    pairProps["address"]["description"] = "6-byte MAC address (AA:BB:CC:DD:EE:FF)";
    pairProps["addressType"]["type"] = "string";
    JsonArray enumArr = pairProps["addressType"]["enum"].to<JsonArray>();
    enumArr.add("Public");
    enumArr.add("RandomStatic");
    enumArr.add("RandomResolvable");
    enumArr.add("RandomNonResolvable");
    pairProps["addressType"]["description"] = "Bluetooth address type classification";
    JsonArray pairReq = pairSchema["required"].to<JsonArray>();
    pairReq.add("providerId");
    pairReq.add("address");
    pairReq.add("addressType");

    registry.addTool(
        "ble_pair",
        "Pair and store a remote BLE device for a central client provider",
        pairSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* mgr = static_cast<BLEManager*>(ctx);
            auto guard = mgr->lockState();  // O2: serialize with tick()
            const char* providerId = params["providerId"] | "";
            if (!providerId || strlen(providerId) == 0) {
                result["error"] = "missing providerId";
                result["isError"] = true;
                return;
            }
            bool found = false;
            for (auto* p : mgr->getClientProviders()) {
                if (p && strcmp(p->getProviderId(), providerId) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result["error"] = "unknown providerId";
                result["isError"] = true;
                return;
            }
            const char* addrStr = params["address"] | "";
            const char* typeStr = params["addressType"] | "";
            BLEPeerAddress::Type addrType;
            if (!BLESerialization::parseAddressType(typeStr, addrType)) {
                result["error"] = "invalid addressType";
                result["isError"] = true;
                return;
            }
            BLEPeerAddress addr;
            if (!BLEPeerAddress::parse(addrStr, addrType, addr) || !addr.isValid()) {
                result["error"] = "invalid address";
                result["isError"] = true;
                return;
            }
            if (!mgr->pair(providerId, addr)) {
                result["error"] = "pair failed";
                result["isError"] = true;
                return;
            }
            result["status"] = "pairing";
            result["providerId"] = providerId;
        },
        this,
        /*mutatesState=*/true
    );

    // 4. ble_forget (Mutating, mutatesState = true)
    JsonDocument forgetSchema;
    forgetSchema["type"] = "object";
    JsonObject forgetProps = forgetSchema["properties"].to<JsonObject>();
    forgetProps["providerId"]["type"] = "string";
    forgetProps["providerId"]["description"] = "Client provider identifier";
    JsonArray forgetReq = forgetSchema["required"].to<JsonArray>();
    forgetReq.add("providerId");

    registry.addTool(
        "ble_forget",
        "Forget stored device identity and remove bond for a central client provider",
        forgetSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* mgr = static_cast<BLEManager*>(ctx);
            auto guard = mgr->lockState();  // O2: serialize with tick()
            const char* providerId = params["providerId"] | "";
            if (!providerId || strlen(providerId) == 0) {
                result["error"] = "missing providerId";
                result["isError"] = true;
                return;
            }
            bool found = false;
            for (auto* p : mgr->getClientProviders()) {
                if (p && strcmp(p->getProviderId(), providerId) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result["error"] = "unknown providerId";
                result["isError"] = true;
                return;
            }
            if (!mgr->forget(providerId)) {
                result["error"] = "forget failed";
                result["isError"] = true;
                return;
            }
            result["status"] = "forgotten";
            result["providerId"] = providerId;
        },
        this,
        /*mutatesState=*/true
    );
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

    // 3. Central Client Providers Preconditions
    for (auto* p : _clientProviders) {
        if (!p) continue;
        // Y5: Use static lifetime storage — provider labels are stable after lock
        static char preconditionNames[4][40] = {};
        static size_t preconditionIdx = 0;
        if (preconditionIdx < 4) {
            snprintf(preconditionNames[preconditionIdx], sizeof(preconditionNames[0]),
                     "BLE: %s", p->getDeviceTypeLabel());
        }
        
        const char* precName = (preconditionIdx < 4) ? preconditionNames[preconditionIdx] : "BLE: ?";
        if (preconditionIdx < 4) preconditionIdx++;

        registry.add(
            precName,
            PreconditionRole::Operational,
            [this, pid = p->getProviderId()]() -> PreconditionStatus {
                BLEStoredDevice stored;
                if (!_deviceRegistry.get(pid, stored)) {
                    return {false, Cause::NotConfigured};
                }
                
                if (isClientConnected(pid)) {
                    return {true, Cause::Met};
                }
                
                auto& policy = const_cast<BLEManager*>(this)->_reconnectPolicies[pid];
                if (policy.phase() == BLEReconnectPolicy::Phase::Fast) {
                    return {false, Cause::Connecting};
                } else if (policy.phase() == BLEReconnectPolicy::Phase::Slow || policy.phase() == BLEReconnectPolicy::Phase::Exhausted) {
                    if (stored.address.type == BLEPeerAddress::Type::RandomResolvable && !stored.bonded) {
                        return {false, Cause::Rejected};
                    }
                    return {false, Cause::Unreachable};
                }
                
                return {false, Cause::NotConfigured};
            },
            [pid = p->getProviderId()](Cause cause, char* buf, size_t len) {
                switch (cause) {
                    case Cause::NotConfigured: std::snprintf(buf, len, "No paired device"); break;
                    case Cause::Connecting: std::snprintf(buf, len, "Connecting to device..."); break;
                    case Cause::Unreachable: std::snprintf(buf, len, "Device unreachable"); break;
                    case Cause::Rejected: std::snprintf(buf, len, "Re-pair required (RPA rotation)"); break;
                    case Cause::Met: std::snprintf(buf, len, "Connected"); break;
                    default: std::snprintf(buf, len, "Unknown state"); break;
                }
            }
        );
    }
}

#endif // WITH_BLE
