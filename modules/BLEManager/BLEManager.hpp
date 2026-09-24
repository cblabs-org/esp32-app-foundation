/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEManager.hpp
 * Description: Core managed Bluetooth Low Energy module. Coordinates shared GATT server,
 *              aggregated advertising, client discovery, peer bonding, and reconnects.
 *              Authoritative: RFC #225 rev 2 §6, §16.3.
 *
 * Exported Classes:
 * - BLEManager: Central coordination hub implementing ITickable, IConfigurable,
 *               IWebContributor, IMCPContributor, and IPreconditionContributor
 * - BLEManager::ScanInhibit: RAII token suppressing active scans during radio-heavy tasks
 */

#pragma once

#ifdef WITH_BLE

#if defined(ESP_PLATFORM)
  #include "sdkconfig.h"
  #include "soc/soc_caps.h"
  #if !SOC_BLE_SUPPORTED
    #error "WITH_BLE requires a SoC with BLE (ESP32-S2 has none)"
  #endif
  #if !CONFIG_BT_NIMBLE_ENABLED
    #error "WITH_BLE requires hardware/_common/sdkconfig.ble.defaults in SDKCONFIG_DEFAULTS"
  #endif
#endif

#include "BLETypes.hpp"
#include "BLEAdvertisingBuilder.hpp"
#include "BLEDeviceRegistry.hpp"
#include "BLEReconnectPolicy.hpp"
#include "IBLEServiceProvider.hpp"
#include "IBLEClientProvider.hpp"
#include "DeviceInfoServiceProvider.hpp"
#include "BLEEvents.hpp"
#include "BLEStack.hpp"
#include "BLERoutingTable.hpp"
#include "BLEAdvertisingDecision.hpp"
#include "BLEQueueMessage.hpp"

#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "interfaces/IPreconditionContributor.hpp"
#include "types/AppState.hpp"

#include <vector>
#include <string>
#include <atomic>
#include <memory>

// Forward declarations
class Application;
class EventBus;
class PsychicHttpServer;
class ITabRegistry;
class MCPToolRegistry;
class IPreconditionRegistry;

/**
 * @brief Managed Bluetooth Low Energy coordination module.
 */
class BLEManager : public ITickable,
                   public IConfigurable,
                   public IWebContributor,
                   public IMCPContributor,
                   public IPreconditionContributor {
public:
    using State = BLEManagerState;

    /**
     * @brief RAII token to inhibit BLE scanning.
     *
     * While any token is alive: no discovery or reconnect scans start, and
     * running scans stop. Peripheral connections and advertising are unaffected.
     */
    class ScanInhibit {
    public:
        ScanInhibit() = default;
        explicit ScanInhibit(BLEManager* mgr, const char* reason);
        ~ScanInhibit();

        ScanInhibit(const ScanInhibit&) = delete;
        ScanInhibit& operator=(const ScanInhibit&) = delete;
        ScanInhibit(ScanInhibit&& other) noexcept;
        ScanInhibit& operator=(ScanInhibit&& other) noexcept;

    private:
        BLEManager* _mgr    = nullptr;
        const char* _reason = nullptr;
    };

    /**
     * @brief Configuration parameters for BLEManager (schema v1).
     */
    struct BLEConfig {
        bool                  enabled                     = true;
        std::string           advertisedName              = "ESP32";
        bool                  nameSuffix                  = true;
        bool                  advertiseInConfiguring      = false;
        uint8_t               maxPeripheralConnections    = 3;
        BLEConnParams         peripheralPreferredConnParams = { 12, 24, 0, 400 }; // 15-30ms, 4s
        BLEConnParams         centralConnParams           = { 24, 60, 0, 600 };    // 30-75ms, 6s
        BLEScanParams         discoveryScan               = { 100, 50, true };
        BLEScanParams         reconnectScan               = { 160, 40, true };
        std::vector<uint32_t> reconnectFastScheduleS      = { 2, 4, 8, 16, 32 };
        uint32_t              slowRetryMs                 = 60000;
        bool                  otaDisconnectPeers          = true;
    };

    BLEManager();
    ~BLEManager() override;

    // --- Provider Registration (Tier 1 — pre-lock only) ---

    /**
     * @brief Register a GATT service provider (Peripheral role).
     * @param p Pointer to provider implementation.
     * @return true if registered, false if registration locked or null.
     */
    bool registerServiceProvider(IBLEServiceProvider* p);

    /**
     * @brief Register an external BLE client provider (Central role).
     * @param p Pointer to provider implementation.
     * @return true if registered, false if registration locked or null.
     */
    bool registerClientProvider(IBLEClientProvider* p);

    // --- Late Wiring ---

    /**
     * @brief Wire application hub and lock provider registration.
     * Called after app.begin().
     * @param app Reference to Application hub.
     */
    void begin(Application& app);

    // --- State and Diagnostics ---

    State  getState() const;
    bool   isScanning() const;
    bool   isAdvertising() const;
    size_t getPeripheralConnectionCount() const;
    const std::vector<BLEPeerInfo>& getActiveConnections() const { return _activeConnections; }
    bool   getPeerInfo(BLEConnHandle handle, BLEPeerInfo& out) const;
    bool   isClientConnected(const char* providerId) const;
    bool   getStoredDevice(const char* providerId, BLEStoredDevice& out) const;
    const BLEConfig& getConfig() const { return _config; }
    const BLEAdvPackResult& getLastPackResult() const { return _advPackResult; }
    uint32_t getDroppedEventsCount() const;
    uint32_t getScanInhibitCount() const { return _scanInhibitCount; }

    // --- Discovery and Pairing (User-initiated) ---

    bool   startDiscovery(const char* providerId, uint32_t durationMs = 30000);
    void   stopDiscovery();
    size_t getDiscoveryResults(BLEScanResult* out, size_t max) const;
    const std::vector<BLEScanResult>& getDiscoveryResults() const;
    bool   isDiscoveryActive() const;
    const char* getDiscoveryProviderId() const;
    uint32_t getDiscoveryDurationMs() const;
    const std::vector<IBLEClientProvider*>& getClientProviders() const { return _clientProviders; }
    bool   getReconnectPolicy(const char* providerId, BLEReconnectPolicy& out) const;
    bool   pair(const char* providerId, const BLEPeerAddress& addr);
    bool   forget(const char* providerId);
    // --- Coexistence and Lifecycle Control ---

    /// @brief Suppress all scanning (discovery + reconnect) while the token is alive.
    /// @note Must be called from the main Arduino loop task only. While the REST/MCP
    ///       thread safety work (O2) is deferred, calling from other tasks (e.g. the
    ///       DataManager upload task) is undefined behaviour. Acquire on the main loop
    ///       before starting the upload, then release when it completes.
    ScanInhibit inhibitScanning(const char* reason);
    void        refreshAdvertising();
    void        pause(const char* reason);
    void        resume();

    // --- Testing helpers ---
    void lockRegistration() { _locked = true; }
    void setStateForTesting(State s) { _state = s; }
    void setGattStartedForTesting(bool g) { _gattStarted = g; }
    void setProviderRegistrationFaultForTesting(bool f) { _providerRegistrationFault = f; }
    bool postTestQueueMessage(const BLEQueueMessage& msg);
    void handleOtaProgressForTesting(bool complete, bool success);
    AppState getEffectiveAppState() const;
    void setAppStateForTesting(AppState s) {
        _appStateForTesting = s;
        _hasAppStateForTesting = true;
    }


    // --- ITickable Interface ---
    void tick(uint32_t currentMillis) override;

    // --- IConfigurable Interface ---
    const char* getConfigNamespace() const override { return "ble"; }
    int         getSchemaVersion() const override { return 1; }
    void        applyConfig(const JsonObject& root) override;
    void        serializeConfig(JsonObject& root) const override;
    void        resetConfig() override;

    // --- IWebContributor Interface ---
    void        registerRoutes(PsychicHttpServer& server) override;
    void        registerTabs(ITabRegistry& registry) override;
    const char* getApiPrefix() const override { return "/api/ble"; }
    size_t      getMaxPostBody() const override { return 512; }

    // --- IMCPContributor Interface ---
    void registerMCPTools(MCPToolRegistry& registry) override;

    // --- IPreconditionContributor Interface ---
    void registerPreconditions(IPreconditionRegistry& registry) override;

private:
    friend class ScanInhibit;
    static void centralWorkerTask(void* arg);

    void releaseScanInhibit(const char* reason);

    void dispatchQueueEvents();
    void publishState(State state, const char* reason);
    void publishReady();

    BLEConfig                          _config;
    State                              _state = State::Disabled;
    bool                               _locked = false;
    Application*                       _app = nullptr;
    AppState                           _appStateForTesting = AppState::Boot;
    bool                               _hasAppStateForTesting = false;
    EventBus*                          _eventBus = nullptr;

    BLEStack                           _stack;
    BLEDeviceRegistry                  _deviceRegistry;
    BLEAdvPackResult                   _advPackResult;
    BLERoutingTable                    _routingTable;
    std::vector<BLEPeerInfo>           _activeConnections;

    std::vector<IBLEServiceProvider*>  _serviceProviders;
    std::vector<IBLEClientProvider*>   _clientProviders;
    std::map<std::string, BLEReconnectPolicy> _reconnectPolicies;
    std::map<std::string, bool>        _clientConnected;
    std::map<std::string, bool>        _clientConnecting;

    bool                               _gattStarted = false;
    bool                               _providerRegistrationFault = false;
    std::atomic<uint32_t>              _scanInhibitCount{0};  // O1: thread-safe for cross-task inhibit
    std::string                        _pauseReason;

    // --- Worker Task Queue ---
    struct WorkerCommand {
        enum class Type : uint8_t { Connect, Disconnect, Forget, Shutdown, Discover, StopDiscovery } type;
        char providerId[24] = {};  // R7: Owned copy, not a dangling const char*
        BLEPeerAddress addr;
        uint32_t durationMs;
        bool hasPeripherals = false;
    };
    void*                              _workerQueue = nullptr;
    void*                              _workerTask = nullptr;

    void*                              _eventQueue = nullptr;
    uint32_t                           _droppedEventsCount = 0;

#ifdef WITH_OTA
    static void _onOtaProgress(const char* topic, const void* payload, void* ctx);
#endif
    void handleOtaProgress(bool complete, bool success);

    // FALLBACK(#227): Atomic flags for OTA coupling.
    // BLEManager reacts only to the first ota.progress event (pause) and to complete && !success (resume).
    // No progress-percentage tracking. Mark all fallback code with // FALLBACK(#227).
    std::atomic<bool>                  _pendingOtaEvent{false};
    std::atomic<bool>                  _otaComplete{false};
    std::atomic<bool>                  _otaSuccess{false};
    bool                               _otaPaused = false;


    // Central state
    struct DiscoveryState {
        bool active = false;
        uint32_t startMs = 0;
        uint32_t durationMs = 0;
        char providerId[24] = {};  // R7: Owned copy, not a dangling const char*
        std::vector<BLEScanResult> results;
    } _discovery;

    // Ring buffers for EventBus published payloads
    static constexpr size_t RING_SIZE = 4;
    BLEStatePayload          _stateRing[RING_SIZE];
    uint8_t                  _stateRingHead = 0;

    BLEPeripheralConnPayload _peerConnRing[RING_SIZE];
    uint8_t                  _peerConnRingHead = 0;

    BLECentralConnPayload    _centralConnRing[RING_SIZE];
    uint8_t                  _centralConnRingHead = 0;

    BLEScanPayload           _scanRing[RING_SIZE];
    uint8_t                  _scanRingHead = 0;

    BLEDevicePayload         _deviceRing[RING_SIZE];
    uint8_t                  _deviceRingHead = 0;

    BLEAdvFaultPayload       _faultRing[RING_SIZE];
    uint8_t                  _faultRingHead = 0;

    // Built-in Device Information Service (DIS, 0x180A) provider
    std::unique_ptr<DeviceInfoServiceProvider> _disProvider;
    char                     _disSerial[12];
    char                     _disSoftwareRev[40];
};

#else // !defined(WITH_BLE)

/**
 * @brief Sentinel indicating BLEManager was included without WITH_BLE defined.
 */
struct BLEManager_requires_WITH_BLE {};

#endif // WITH_BLE
