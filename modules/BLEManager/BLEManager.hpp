/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
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

#include <array>
#include <vector>
#include <string>
#include <atomic>
#include <memory>
#include <mutex>

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
        /**
         * @brief Construct a scan-inhibit token that suppresses scanning for its lifetime.
         * @param mgr  Owning BLEManager instance.
         * @param reason Human-readable reason string (static lifetime).
         */
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
     * @brief Set the factory-default advertised name (issue #232).
     *
     * Mirrors WifiManager::setDefaultHostname(). Call before begin().
     * If the user has not persisted a custom advertisedName in config,
     * this value is used instead of the hardcoded "ESP32" default.
     * The MAC-based suffix (-XXXX) is still appended if nameSuffix is true.
     *
     * @param name  App-supplied default (static lifetime or copied before return).
     */
    void setDefaultAdvertisedName(const char* name);

    /**
     * @brief Wire application hub and lock provider registration.
     * Called after app.begin().
     * @param app Reference to Application hub.
     */
    void begin(Application& app);

    // --- State and Diagnostics ---

    /** @brief Returns the current BLE manager lifecycle state. */
    State  getState() const;
    /** @brief Returns true if a BLE scan (discovery or reconnect) is active. */
    bool   isScanning() const;
    /** @brief Returns true if BLE advertising is currently running. */
    bool   isAdvertising() const;
    /** @brief Returns the number of active peripheral connections. */
    size_t getPeripheralConnectionCount() const;
    const std::vector<BLEPeerInfo>& getActiveConnections() const { return _activeConnections; }
    /**
     * @brief Look up a connected peer by connection handle.
     * @param handle Connection handle to query.
     * @param out    Output peer info populated on success.
     * @return true if found, false otherwise.
     */
    bool   getPeerInfo(BLEConnHandle handle, BLEPeerInfo& out) const;
    /**
     * @brief Check whether a client provider's paired device is currently connected.
     * @param providerId Provider identifier to query.
     * @return true if the provider's device is connected.
     */
    bool   isClientConnected(const char* providerId) const;
    /**
     * @brief Retrieve the stored (bonded) device for a client provider.
     * @param providerId Provider identifier to query.
     * @param out        Output stored device populated on success.
     * @return true if a stored device exists for this provider.
     */
    bool   getStoredDevice(const char* providerId, BLEStoredDevice& out) const;
    const BLEConfig& getConfig() const { return _config; }
    const BLEAdvPackResult& getLastPackResult() const { return _advPackResult; }
    /** @brief Returns the number of queue messages dropped due to a full event queue. */
    uint32_t getDroppedEventsCount() const;
    /** @brief Returns the number of times scan inhibition has been requested. */
    uint32_t getScanInhibitCount() const { return _scanInhibitCount; }

    // --- Discovery and Pairing (User-initiated) ---

    /**
     * @brief Start a user-initiated BLE discovery scan for a specific client provider.
     * @param providerId Provider whose target service UUIDs filter scan results.
     * @param durationMs Maximum scan duration in milliseconds (default 30 s).
     * @return true if discovery started, false if already active or provider not found.
     */
    bool   startDiscovery(const char* providerId, uint32_t durationMs = 30000);
    /**
     * @brief Stop an active discovery scan.
     * @return true if a scan was active and stopped, false otherwise.
     */
    bool   stopDiscovery();
    /**
     * @brief Copy discovery results into a caller-supplied buffer.
     * @param out Buffer to receive scan results.
     * @param max Maximum number of results to copy.
     * @return Number of results written.
     */
    size_t getDiscoveryResults(BLEScanResult* out, size_t max) const;
    const std::vector<BLEScanResult>& getDiscoveryResults() const;
    /** @brief Returns true if a user-initiated discovery scan is in progress. */
    bool   isDiscoveryActive() const;
    const char* getDiscoveryProviderId() const;
    /** @brief Returns the configured duration of the current/last discovery scan in ms. */
    uint32_t getDiscoveryDurationMs() const;
    const std::vector<IBLEClientProvider*>& getClientProviders() const { return _clientProviders; }
    /**
     * @brief Retrieve the reconnect policy for a client provider.
     * @param providerId Provider identifier.
     * @param out        Output policy populated on success.
     * @return true if a policy exists for this provider.
     */
    bool   getReconnectPolicy(const char* providerId, BLEReconnectPolicy& out) const;
    /**
     * @brief Pair with a discovered BLE peripheral for a given client provider.
     * @param providerId Provider that will own the connection.
     * @param addr       Bluetooth address of the peer to pair with.
     * @param name       Optional device name (if omitted or null, looks up from discovery results).
     * @return true if pairing was initiated, false on error.
     */
    bool   pair(const char* providerId, const BLEPeerAddress& addr, const char* name = nullptr);
    /**
     * @brief Forget (unpair) the stored device for a client provider.
     * @param providerId Provider whose stored device is removed.
     * @return true if a device was forgotten, false if none was stored.
     */
    bool   forget(const char* providerId);
    // --- Coexistence and Lifecycle Control ---

    /// @brief Suppress all scanning (discovery + reconnect) while the token is alive.
    /// @note Thread-safe (O2): protected by _stateMutex, safe to call from any task
    ///       (e.g. the DataManager upload task or the httpd task).
    ScanInhibit inhibitScanning(const char* reason);
    /** @brief Rebuild and restart advertising (e.g. after provider list changes). */
    void        refreshAdvertising();
    /**
     * @brief Pause the BLE stack (stops advertising and scanning).
     * @param reason Human-readable reason for the pause (e.g. "OTA download").
     */
    void        pause(const char* reason);
    /** @brief Resume the BLE stack after a pause. */
    void        resume();

    // --- Testing helpers ---
    /** @brief Lock provider registration (prevents further registerServiceProvider/registerClientProvider calls). */
    void lockRegistration() { _locked = true; }
    /** @brief Override BLE manager state for unit testing. */
    void setStateForTesting(State s) { _state = s; }
    /** @brief Override GATT-started flag for unit testing. */
    void setGattStartedForTesting(bool g) { _gattStarted = g; }
    /** @brief Override provider-registration-fault flag for unit testing. */
    void setProviderRegistrationFaultForTesting(bool f) { _providerRegistrationFault = f; }
    /**
     * @brief Post a queue message directly for unit testing.
     * @param msg Message to enqueue.
     * @return true if posted, false if queue is full.
     */
    bool postTestQueueMessage(const BLEQueueMessage& msg);
    /**
     * @brief Simulate OTA progress event for unit testing.
     * @param complete True if OTA is finished.
     * @param success  True if OTA succeeded.
     */
    void handleOtaProgressForTesting(bool complete, bool success);
    /** @brief Returns the effective AppState (test override or real). */
    AppState getEffectiveAppState() const;
    /**
     * @brief Override the AppState for unit testing.
     * @param s AppState to inject.
     */
    void setAppStateForTesting(AppState s) {
        _appStateForTesting = s;
        _hasAppStateForTesting = true;
    }
    /** @brief Set the EventBus instance for testing. */
    void setEventBusForTesting(EventBus* bus) {
        _eventBus = bus;
    }


    // --- ITickable Interface ---
    /** @brief Advance BLE manager state machine, dispatch queued events. */
    void tick(uint32_t currentMillis) override;

    // --- IConfigurable Interface ---
    const char* getConfigNamespace() const override { return "ble"; }
    /** @brief Returns the BLE configuration schema version. */
    int         getSchemaVersion() const override { return 1; }
    /** @brief Apply BLE configuration from a parsed JSON object. */
    void        applyConfig(const JsonObject& root) override;
    /** @brief Serialize current BLE configuration to a JSON object. */
    void        serializeConfig(JsonObject& root) const override;
    /** @brief Reset BLE configuration to factory defaults. */
    void        resetConfig() override;

    // --- IWebContributor Interface ---
    /** @brief Register BLE REST API routes on the web server. */
    void        registerRoutes(PsychicHttpServer& server) override;
    /** @brief Register BLE web portal tabs. */
    void        registerTabs(ITabRegistry& registry) override;
    const char* getApiPrefix() const override { return "/api/ble"; }
    /** @brief Returns the maximum POST body size for BLE API endpoints. */
    size_t      getMaxPostBody() const override { return 512; }

    // --- IMCPContributor Interface ---
    /** @brief Register BLE MCP tools (ble_status, ble_scan, ble_pair, ble_forget). */
    void registerMCPTools(MCPToolRegistry& registry) override;

    // --- IPreconditionContributor Interface ---
    /** @brief Register BLE-related preconditions (GATT health, provider faults). */
    void registerPreconditions(IPreconditionRegistry& registry) override;

    /// @brief Acquire a scoped lock on BLEManager state.
    /// REST/MCP handlers MUST hold this while reading or mutating state to
    /// avoid racing with tick()/dispatchQueueEvents() on the main loop.
    /// Uses a recursive mutex so nested calls within the same task are safe.
    [[nodiscard]] std::unique_lock<std::recursive_mutex> lockState() const {
        return std::unique_lock<std::recursive_mutex>(_stateMutex);
    }

private:
    friend class ScanInhibit;
    mutable std::recursive_mutex _stateMutex;  // O2: protects state vs httpd task
    static void centralWorkerTask(void* arg);

    void releaseScanInhibit(const char* reason);

    void dispatchQueueEvents();
    void publishState(State state, const char* reason);
    void publishReady();

    BLEConfig                          _config;
    std::string                        _defaultAdvertisedName = "ESP32"; ///< App-supplied factory default (issue #232)
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

    struct ClientProviderPrecContext {
        BLEManager* mgr = nullptr;
        const char* providerId = nullptr;
    };
    std::array<ClientProviderPrecContext, 4> _clientProviderPrecContexts{};

    std::map<std::string, BLEReconnectPolicy> _reconnectPolicies;
    std::map<std::string, bool>        _clientConnected;
    std::map<std::string, bool>        _clientConnecting;

    bool                               _gattStarted = false;
    bool                               _providerRegistrationFault = false;
    std::atomic<uint32_t>              _scanInhibitCount{0};  // O1: thread-safe for cross-task inhibit
    std::string                        _pauseReason;

    // WO-9: Set (under _stateMutex) whenever config changes while locked; the
    // flash write itself happens at the end of tick(), after the lock is
    // released, so it never blocks the httpd task while holding _stateMutex.
    std::atomic<bool>                  _configDirty{false};

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
    static constexpr size_t RING_SIZE = 4; ///< Capacity of EventBus payload ring buffers
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
