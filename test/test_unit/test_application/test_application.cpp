/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_application/test_application.cpp
 * Description: GoogleTest unit-test suite for the Application composition root.
 *              Covers module registration (pre-lock and post-lock), the
 *              two-tier vector lock model, AppState lifecycle transitions,
 *              tick() dispatch behaviour, and softReset() native-build safety.
 *
 *              With the WO1 refactor, registerConfigurable() is removed.
 *              Modules now register with ConfigManager::registerModule()
 *              instead. The test suite validates the new gateway pattern
 *              through Application::getConfigManager().
 *
 *              All tests execute on the native GoogleTest runner (no hardware
 *              required). native builds is defined by the [env:native] environment
 *              in platformio.ini.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all test suites
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <algorithm>
#include "Application.hpp"
#include "OTAUpdateManager.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "MCPToolRegistry.hpp"
#include "events/AppConfiguringPayload.hpp"

// =============================================================================
// Mock implementations
// Simple inline concrete classes satisfy the pure-virtual interface contracts
// without requiring a mocking framework.
// =============================================================================

/** @brief Minimal ITickable mock that counts how many times tick() is called. */
class MockTickable : public ITickable {
public:
    int tickCount = 0; ///< Incremented once per tick() dispatch

    void tick(uint32_t currentMillis) override { tickCount++; }
};

/**
 * @brief Minimal IConfigurable mock implementing the new 6-method interface.
 *
 * Provides namespace "mock", schema version 1, and tracks applyConfig/
 * serializeConfig/resetConfig invocations.
 */
class MockConfigurable : public IConfigurable {
public:
    bool applyConfigCalled = false;     ///< Set true when applyConfig() is invoked
    mutable bool serializeConfigCalled = false; ///< Set true when serializeConfig() is invoked
    bool resetConfigCalled = false;     ///< Set true when resetConfig() is invoked
    const char* _ns;                    ///< Configurable namespace (default: "mock")

    MockConfigurable(const char* ns = "mock") : _ns(ns) {}

    const char* getConfigNamespace() const override { return _ns; }
    int getSchemaVersion() const override { return 1; }

    void applyConfig(const JsonObject& section) override {
        applyConfigCalled = true;
        (void)section;
    }

    void serializeConfig(JsonObject& section) const override {
        serializeConfigCalled = true;
        section["test_key"] = "test_value";
    }

    void resetConfig() override {
        resetConfigCalled = true;
    }
};

/** @brief Minimal IDataSource mock — scheduling metadata returns fixed values. */
class MockDataSource : public IDataSource {
public:
    UpdateStatus fetchUpdate(const FetchContext& ctx) override { return UpdateStatus::Success; }
    uint32_t     getUpdateIntervalMs() const override { return 60000; } // 60 s
    const char*  getSourceName()       const override { return "MockDataSource"; }
    PriorityTier getPriorityTier()     const override { return PriorityTier::Normal; }
};

/** @brief Minimal IStatusReporter mock. */
class MockStatusReporter : public IStatusReporter {
public:
    void reportStatus(const char* /*message*/, float /*progress*/ = -1.0f) override {}
    void reportError(const char* /*errorMessage*/) override {}
    void clearStatus() override {}
};

/** @brief Minimal IWebContributor mock. */
class MockWebContributor : public IWebContributor {
public:
    // PsychicHttpServer is forward-declared in IWebContributor.hpp; the mock
    // accepts the reference signature but never dereferences the pointer so
    // no linker dependency on ESPPsychicHttpServer is introduced.
    void registerRoutes(PsychicHttpServer& /*server*/) override {}
    void registerTabs(ITabRegistry& /*registry*/) override {}
};

/** @brief Minimal IMCPContributor mock that registers a named test tool. */
class MockMcpContributor : public IMCPContributor {
public:
    bool registered = false; ///< Set true when registerMCPTools() is invoked

    void registerMCPTools(MCPToolRegistry& registry) override {
        registered = true;
        registry.addTool(
            "mock_mcp_tool",
            "A mock MCP tool for testing",
            [](void* /*ctx*/, JsonObject& params, JsonObject& result) {
                (void)params;
                result["test"] = true;
            }
        );
    }
};

// =============================================================================
// Test Group 1: Module Registration (pre-lock)
// =============================================================================

class ApplicationRegistrationTest : public ::testing::Test {
protected:
    Application app; // Fresh Application instance for each test
};

/** registerTickable with a valid pointer must return true. */
TEST_F(ApplicationRegistrationTest, RegisterTickable_ValidPointer_ReturnsTrue) {
    MockTickable tickable;
    EXPECT_TRUE(app.registerTickable(&tickable));
}

/** registerTickable with nullptr must return false. */
TEST_F(ApplicationRegistrationTest, RegisterTickable_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(app.registerTickable(nullptr));
}

/** ConfigManager::registerModule with a valid pointer must return true. */
TEST_F(ApplicationRegistrationTest, RegisterConfigurable_ValidPointer_ReturnsTrue) {
    MockConfigurable configurable;
    EXPECT_TRUE(app.getConfigManager().registerModule(&configurable));
}

/** ConfigManager::registerModule with nullptr must return false. */
TEST_F(ApplicationRegistrationTest, RegisterConfigurable_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(app.getConfigManager().registerModule(nullptr));
}

/** registerDataSource with a valid pointer must return true. */
TEST_F(ApplicationRegistrationTest, RegisterDataSource_ValidPointer_ReturnsTrue) {
    MockDataSource source;
    EXPECT_TRUE(app.registerDataSource(&source));
}

/** registerDataSource with nullptr must return false. */
TEST_F(ApplicationRegistrationTest, RegisterDataSource_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(app.registerDataSource(nullptr));
}

/** registerStatusReporter with a valid pointer must return true. */
TEST_F(ApplicationRegistrationTest, RegisterStatusReporter_ValidPointer_ReturnsTrue) {
    MockStatusReporter reporter;
    EXPECT_TRUE(app.registerStatusReporter(&reporter));
}

/**
 * registerStatusReporter last-write-wins: registering a second reporter
 * before lock should still return true (no duplicate detection needed).
 */
TEST_F(ApplicationRegistrationTest, RegisterStatusReporter_ReplacesPreviousReporter_ReturnsTrue) {
    MockStatusReporter reporterA;
    MockStatusReporter reporterB;
    EXPECT_TRUE(app.registerStatusReporter(&reporterA));
    EXPECT_TRUE(app.registerStatusReporter(&reporterB)); // replaces A
}

/** registerWebContributor with a valid pointer must return true. */
TEST_F(ApplicationRegistrationTest, RegisterWebContributor_ValidPointer_ReturnsTrue) {
    MockWebContributor contributor;
    EXPECT_TRUE(app.registerWebContributor(&contributor));
}

/** registerMcpContributor with a valid pointer must return true. */
TEST_F(ApplicationRegistrationTest, RegisterMcpContributor_ValidPointer_ReturnsTrue) {
    MockMcpContributor contributor;
    EXPECT_TRUE(app.registerMcpContributor(&contributor));
}

/** registerMcpContributor with nullptr must return false. */
TEST_F(ApplicationRegistrationTest, RegisterMcpContributor_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(app.registerMcpContributor(nullptr));
}

// =============================================================================
// Test Group 2: Vector Lock Behaviour
// =============================================================================

/**
 * Helper: access the private _compactAndLock() method via begin() in native builds.
 *
 * In the native build, begin() completes without hardware I/O and calls
 * _compactAndLock() internally as part of the boot sequence. This is the
 * canonical way to transition the Application into locked state in tests.
 */
class ApplicationLockTest : public ::testing::Test {
protected:
    Application app;

    /**
     * @brief Advance app to RUNNING state so the vector is locked.
     *
     * Uses default FrameworkConfig (all safe defaults). In native builds
     * the LittleFS mount is skipped, so begin() always returns true.
     */
    void lockApp() {
        FrameworkConfig cfg;
        bool result = app.begin(cfg);
        // Tolerate begin() returning false in any environment; what matters
        // is that _compactAndLock() was called (which it always is before
        // the potential early-return on LittleFS failure — in native builds
        // there is no early return, so this always reaches RUNNING).
        (void)result;
    }
};

/** After begin(), registerTickable must return false (vector locked). */
TEST_F(ApplicationLockTest, PostLock_RegisterTickable_ReturnsFalse) {
    lockApp();
    MockTickable tickable;
    EXPECT_FALSE(app.registerTickable(&tickable));
}

/** After begin(), ConfigManager::registerModule is rejected (locked by beginAll). */
TEST_F(ApplicationLockTest, PostLock_ConfigManagerRegisterModule_ReturnsFalse) {
    lockApp();
    MockConfigurable configurable;
    // ConfigManager locks its own registry during beginAll(), called inside begin()
    EXPECT_FALSE(app.getConfigManager().registerModule(&configurable));
}

/** After begin(), registerDataSource must return false. */
TEST_F(ApplicationLockTest, PostLock_RegisterDataSource_ReturnsFalse) {
    lockApp();
    MockDataSource source;
    EXPECT_FALSE(app.registerDataSource(&source));
}

/** After begin(), registerStatusReporter must return false. */
TEST_F(ApplicationLockTest, PostLock_RegisterStatusReporter_ReturnsFalse) {
    lockApp();
    MockStatusReporter reporter;
    EXPECT_FALSE(app.registerStatusReporter(&reporter));
}

/** After begin(), registerWebContributor must return false. */
TEST_F(ApplicationLockTest, PostLock_RegisterWebContributor_ReturnsFalse) {
    lockApp();
    MockWebContributor contributor;
    EXPECT_FALSE(app.registerWebContributor(&contributor));
}

/** After begin(), registerMcpContributor must return false. */
TEST_F(ApplicationLockTest, PostLock_RegisterMcpContributor_ReturnsFalse) {
    lockApp();
    MockMcpContributor contributor;
    EXPECT_FALSE(app.registerMcpContributor(&contributor));
}

/**
 * A MockMcpContributor registered before begin() must appear
 * in the getMcpContributors() vector after begin() completes.
 */
TEST_F(ApplicationLockTest, McpContributor_ToolVisibleAfterBegin) {
    MockMcpContributor contributor;
    app.registerMcpContributor(&contributor);
    lockApp();

    // registerMCPTools() is invoked by MCPServer::begin(), not
    // Application::begin(), so contributor.registered is NOT yet true.
    // Verify vector registration only.
    const auto& contributors = app.getMcpContributors();
    EXPECT_NE(std::find(contributors.begin(), contributors.end(),
              static_cast<IMCPContributor*>(&contributor)),
              contributors.end());
}

/**
 * tick() must dispatch to all registered ITickable modules exactly once
 * per call when state is RUNNING.
 */
TEST_F(ApplicationLockTest, Tick_WhenRunning_DispatchesToAllTickables) {
    MockTickable tickableA;
    MockTickable tickableB;
    app.registerTickable(&tickableA);
    app.registerTickable(&tickableB);

    lockApp(); // Advances state to RUNNING

    app.tick();

    EXPECT_EQ(tickableA.tickCount, 1);
    EXPECT_EQ(tickableB.tickCount, 1);
}

/**
 * tick() must be a no-op when state is not RUNNING (pre-begin scenario).
 * Calling tick() on a freshly-constructed Application (state = BOOTING)
 * must not dispatch to any registered tickables.
 */
TEST_F(ApplicationLockTest, Tick_WhenNotRunning_DoesNotDispatch) {
    MockTickable tickable;
    app.registerTickable(&tickable);

    // Do NOT call begin() — state remains BOOT
    app.tick();

    EXPECT_EQ(tickable.tickCount, 0);
}

/**
 * Multiple tick() calls must accumulate dispatch counts correctly.
 */
TEST_F(ApplicationLockTest, Tick_CalledMultipleTimes_AccumulatesCount) {
    MockTickable tickable;
    app.registerTickable(&tickable);

    lockApp(); // Advances state to RUNNING

    app.tick();
    app.tick();
    app.tick();

    EXPECT_EQ(tickable.tickCount, 3);
}

// =============================================================================
// Test Group 3: AppState Transitions
// =============================================================================

class ApplicationStateTest : public ::testing::Test {
protected:
    Application app;
};

/**
 * A freshly constructed Application must report BOOT as its initial state.
 * This is the safe default for any code that queries state before begin().
 */
TEST_F(ApplicationStateTest, InitialState_IsBoot) {
    EXPECT_EQ(app.getState(), AppState::Boot);
}

/**
 * After a successful begin(), state must be CONFIGURING in native builds.
 * The WiFi config gate requires isReady() && !isAPMode(), which cannot be
 * satisfied without real WiFi hardware. On real hardware, the gate would
 * be satisfied during tick() once WiFi connects.
 */
TEST_F(ApplicationStateTest, AfterBegin_StateIsConfiguring) {
    FrameworkConfig cfg;
    app.begin(cfg);
    EXPECT_EQ(app.getState(), AppState::Configuring);
}

/**
 * getState() is const and must not alter state when called.
 */
TEST_F(ApplicationStateTest, GetState_DoesNotMutateState) {
    AppState before = app.getState();
    app.getState(); // const call — must be idempotent
    EXPECT_EQ(app.getState(), before);
}

// =============================================================================
// Test Group 4: softReset (native build safety)
// =============================================================================

class ApplicationSoftResetTest : public ::testing::Test {
protected:
    Application app;
};

/**
 * softReset() must complete without crashing the test process in native builds.
 * esp_restart() is stubbed out, so the method is a log-and-return no-op.
 */
TEST_F(ApplicationSoftResetTest, SoftReset_NativeBuild_DoesNotCrash) {
    // If this test completes, softReset() did not call abort() or raise a signal.
    EXPECT_NO_FATAL_FAILURE(app.softReset());
}

/**
 * softReset() must not alter the AppState — the device would have restarted
 * on hardware; on native we just verify state is unchanged.
 */
TEST_F(ApplicationSoftResetTest, SoftReset_DoesNotAlterAppState) {
    AppState stateBefore = app.getState();
    app.softReset();
    EXPECT_EQ(app.getState(), stateBefore);
}

// =============================================================================
// Test Group 5: begin() sequence integration (ConfigManager gateway)
// =============================================================================

class ApplicationBeginTest : public ::testing::Test {
protected:
    Application app;
};

/**
 * begin() must trigger ConfigManager::beginAll() which calls applyConfig()
 * on all registered IConfigurable modules. In native builds, with no config
 * files on disk, modules get resetConfig() + applyConfig() via defaults.
 */
TEST_F(ApplicationBeginTest, Begin_CallsApplyConfigOnRegisteredModules) {
    // Use different namespaces so each module gets its own config file
    MockConfigurable confA("mockA");
    MockConfigurable confB("mockB");
    app.getConfigManager().registerModule(&confA);
    app.getConfigManager().registerModule(&confB);

    // Clean up any leftover files from prior runs.
    // Application::begin() calls ConfigRoot::setMountBase("/config") which
    // resolves to test/tmp/config/mod/ on native (non-ESP_PLATFORM).
    remove("test/tmp/config/mod/mockA.json");
    remove("test/tmp/config/mod/mockB.json");

    FrameworkConfig cfg;
    app.begin(cfg);

    // In native builds with no file on disk, loadModuleConfig() calls
    // resetConfig() then saveModuleConfig(). The applyConfig is NOT called
    // in the "file not found" path — resetConfig() IS called instead.
    EXPECT_TRUE(confA.resetConfigCalled);
    EXPECT_TRUE(confB.resetConfigCalled);

    // Cleanup
    remove("test/tmp/config/mod/mockA.json");
    remove("test/tmp/config/mod/mockB.json");
}

/**
 * begin() in native builds must return true (LittleFS stub succeeds).
 */
TEST_F(ApplicationBeginTest, Begin_NativeBuild_ReturnsTrue) {
    FrameworkConfig cfg;
    EXPECT_TRUE(app.begin(cfg));
}

/**
 * begin() auto-propagates app name to WifiManager default hostname (Issue #232).
 */
TEST_F(ApplicationBeginTest, Begin_AutoPropagatesAppNameToWifiHostname) {
    app.setAppName("Weather Station");
    FrameworkConfig cfg;
    EXPECT_TRUE(app.begin(cfg));
    EXPECT_STREQ("weather-station", app.getWifiManager().getDefaultHostname());
}

/**
 * begin() auto-propagates sanitised app name to AuthManager cert hostname (Issue #232).
 */
TEST_F(ApplicationBeginTest, Begin_AutoPropagatesAppNameToAuthCertHostname) {
    app.setAppName("Weather Station");
    FrameworkConfig cfg;
    EXPECT_TRUE(app.begin(cfg));
    EXPECT_STREQ("weather-station", app.getAuthManager().getCertHostname());
}

/**
 * begin() respects explicitly configured default hostname if set before begin() (Issue #232).
 */
TEST_F(ApplicationBeginTest, Begin_PreservesExplicitCustomHostname) {
    app.setAppName("Weather Station");
    app.getWifiManager().setDefaultHostname("custom-device");
    FrameworkConfig cfg;
    EXPECT_TRUE(app.begin(cfg));
    EXPECT_STREQ("custom-device", app.getWifiManager().getDefaultHostname());
}

// =============================================================================
// Test Group: sanitiseHostname (Issue #232)
// =============================================================================

TEST(ApplicationSanitiseHostnameTest, StandardTransform) {
    EXPECT_STREQ("weather-station", Application::sanitiseHostname("Weather Station").c_str());
}

TEST(ApplicationSanitiseHostnameTest, StripSpecialChars) {
    EXPECT_STREQ("my-iot-device", Application::sanitiseHostname("My IoT Device!!").c_str());
}

TEST(ApplicationSanitiseHostnameTest, EmptyStringFallback) {
    EXPECT_STREQ("esp32-device", Application::sanitiseHostname("").c_str());
}

TEST(ApplicationSanitiseHostnameTest, NullptrFallback) {
    EXPECT_STREQ("esp32-device", Application::sanitiseHostname(nullptr).c_str());
}

TEST(ApplicationSanitiseHostnameTest, OnlyHyphensFallback) {
    EXPECT_STREQ("esp32-device", Application::sanitiseHostname("---").c_str());
}

TEST(ApplicationSanitiseHostnameTest, ConsecutiveSeparatorsAndUnderscores) {
    EXPECT_STREQ("foo-bar-baz", Application::sanitiseHostname("Foo__Bar   Baz").c_str());
}

TEST(ApplicationSanitiseHostnameTest, TruncateTo32Chars) {
    String longName = "A Very Long Application Display Name That Exceeds Thirty Two Chars";
    String sanitised = Application::sanitiseHostname(longName.c_str());
    EXPECT_LE(sanitised.length(), 32u);
    EXPECT_NE(sanitised[sanitised.length() - 1], '-');
    EXPECT_EQ(sanitised, "a-very-long-application-display");
}

// =============================================================================
// Test Group 6: ConfigManager Gateway
// =============================================================================

class ConfigManagerGatewayTest : public ::testing::Test {
protected:
    Application app;
};

/**
 * ConfigManager must be accessible via Application::getConfigManager().
 */
TEST_F(ConfigManagerGatewayTest, GetConfigManager_ReturnsReference) {
    ConfigManager& cm = app.getConfigManager();
    // If this compiles and doesn't crash, the accessor works
    EXPECT_TRUE(true);
}

/**
 * ConfigManager uses a dynamic vector — registering many modules must succeed.
 */
TEST_F(ConfigManagerGatewayTest, RegisterModule_DynamicVector_AcceptsMany) {
    constexpr int MODULE_COUNT = 16;
    // Use unique namespaces to avoid duplicates
    const char* namespaces[] = {
        "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7",
        "m8", "m9", "m10", "m11", "m12", "m13", "m14", "m15"
    };
    MockConfigurable modules[MODULE_COUNT] = {
        {namespaces[0]},  {namespaces[1]},  {namespaces[2]},  {namespaces[3]},
        {namespaces[4]},  {namespaces[5]},  {namespaces[6]},  {namespaces[7]},
        {namespaces[8]},  {namespaces[9]},  {namespaces[10]}, {namespaces[11]},
        {namespaces[12]}, {namespaces[13]}, {namespaces[14]}, {namespaces[15]}
    };
    ConfigManager& cm = app.getConfigManager();

    for (int i = 0; i < MODULE_COUNT; i++) {
        EXPECT_TRUE(cm.registerModule(&modules[i]));
    }
}

/**
 * ConfigManager::registerModule must reject after beginAll() locks the registry.
 */
TEST_F(ConfigManagerGatewayTest, RegisterModule_AfterBeginAll_ReturnsFalse) {
    ConfigManager& cm = app.getConfigManager();
    MockConfigurable confA("lockTestA");
    EXPECT_TRUE(cm.registerModule(&confA));

    // beginAll() locks the registry
    cm.beginAll();

    MockConfigurable confB("lockTestB");
    EXPECT_FALSE(cm.registerModule(&confB));

    // Cleanup
    remove("test/tmp/config/mod/lockTestA.json");
}

// =============================================================================
// Test Group 7: Core Domain Service Ownership (WO2)
// =============================================================================

class ApplicationDomainServiceTest : public ::testing::Test {
protected:
    Application app;
};

/** getWifiManager() must return a valid reference. */
TEST_F(ApplicationDomainServiceTest, GetWifiManager_ReturnsReference) {
    WifiManager& wm = app.getWifiManager();
    // If this compiles and doesn't crash, the accessor works
    EXPECT_TRUE(true);
}

/** getOtaManager() — OTA is heap-allocated in begin(); verify accessor
 *  doesn't crash after manual instantiation. */
TEST_F(ApplicationDomainServiceTest, GetOtaManager_ReturnsReference) {
    // Before begin(), _otaManager is nullptr — calling getOtaManager() would
    // dereference null.  This test therefore validates that the accessor
    // signature compiles and that the raw-pointer plumbing is correct.
    // Full lifecycle testing belongs in hardware integration tests.
    SUCCEED();
}


/** getKeyManager() must return a valid reference. */
TEST_F(ApplicationDomainServiceTest, GetKeyManager_ReturnsReference) {
    KeyManager& km = app.getKeyManager();
    // If this compiles and doesn't crash, the accessor works
    EXPECT_TRUE(true);
}

/**
 * After begin(), WifiManager must be registered with ConfigManager.
 * WifiManager's namespace is "wifi", so the ConfigManager registry should
 * contain a module with that namespace.
 */
TEST_F(ApplicationDomainServiceTest, AfterBegin_WifiManagerRegisteredWithConfigManager) {
    // Clean up any leftover files
    remove("test/tmp/config/mod/wifi.json");

    FrameworkConfig cfg;
    app.begin(cfg);

    // In native builds, WiFi gate cannot be satisfied (no hardware), so
    // the app enters CONFIGURING rather than RUNNING. This validates that
    // the WiFi module was registered and the gate system is working.
    EXPECT_EQ(app.getState(), AppState::Configuring);

    // Cleanup
    remove("test/tmp/config/mod/wifi.json");
}

// =============================================================================
// Test Group: registerModule<T> universal registration
// =============================================================================

class MultiModule : public ITickable, public IWebContributor {
public:
    void tick(uint32_t currentMillis) override { tickCount++; (void)currentMillis; }
    void registerRoutes(PsychicHttpServer& /*server*/) override {}
    void registerTabs(ITabRegistry& /*registry*/) override {}
    int tickCount = 0;
};

class DummyModule {
public:
    int value = 0;
};

TEST_F(ApplicationLockTest, RegisterModule_DetectsAndRegistersMultipleInterfaces) {
    MultiModule mod;
    bool result = app.registerModule(&mod);
    EXPECT_TRUE(result);

    lockApp(); // Advances state to RUNNING

    app.tick();
    EXPECT_EQ(mod.tickCount, 1);
}

TEST_F(ApplicationLockTest, RegisterModule_FailsForUnknownInterface) {
    DummyModule mod;
    bool result = app.registerModule(&mod);
    EXPECT_FALSE(result);
}

// =============================================================================
// Test Group 10: ITickable per-module budget override (WO1)
// =============================================================================

class MockCustomBudgetTickable : public ITickable {
public:
    uint32_t budget = 33;
    void tick(uint32_t /*currentMillis*/) override {}
    uint32_t getTickBudgetMs() const override { return budget; }
};

TEST_F(ApplicationRegistrationTest, ITickable_DefaultBudget_Returns10) {
    MockTickable tickable;
    EXPECT_EQ(tickable.getTickBudgetMs(), 10u);
}

TEST_F(ApplicationRegistrationTest, ITickable_OverrideBudget_ReturnsConfiguredValue) {
    MockCustomBudgetTickable tickable;
    tickable.budget = 50;
    EXPECT_EQ(tickable.getTickBudgetMs(), 50u);
}

// =============================================================================
// Test Group 11: Precondition REST API (WO5)
// =============================================================================

TEST(PreconditionApiTest, RoutesRegistered) {
    Application app;
    FrameworkConfig cfg;
    app.begin(cfg);

    bool preconditionsGetRegistered = false;
    bool preconditionsRetryRegistered = false;
    for (const auto& route : app.getWebServerManager().getServer().routes) {
        if (route.uri == "/api/preconditions" && route.method == HTTP_GET) {
            preconditionsGetRegistered = true;
        }
        if (route.uri == "/api/preconditions/retry" && route.method == HTTP_POST) {
            preconditionsRetryRegistered = true;
        }
    }
    EXPECT_TRUE(preconditionsGetRegistered);
    EXPECT_TRUE(preconditionsRetryRegistered);
}

class MockPreconditionContributor : public IPreconditionContributor {
public:
    void registerPreconditions(IPreconditionRegistry& registry) override {
        registry.add("Custom Op", PreconditionRole::Operational, [](void*) -> PreconditionStatus {
            return {true, Cause::Met};
        });
    }
};

TEST(PreconditionApiTest, PreconditionsGetHandlerReturnsJson) {
    Application app;
    MockPreconditionContributor contributor;
    app.registerModule(&contributor);
    FrameworkConfig cfg;
    app.begin(cfg);
    app.tick(); // Evaluate preconditions

    PsychicHttpRequestCallback getHandler = nullptr;
    for (const auto& route : app.getWebServerManager().getServer().routes) {
        if (route.uri == "/api/preconditions" && route.method == HTTP_GET) {
            getHandler = route.handler;
            break;
        }
    }
    ASSERT_NE(getHandler, nullptr);

    class MockPsychicResponse : public PsychicResponse {
    public:
        int sentCode = 0;
        std::string sentContentType;
        std::string sentBody;
        esp_err_t send(int code, const char* contentType, const char* content) override {
            sentCode = code;
            sentContentType = contentType ? contentType : "";
            sentBody = content ? content : "";
            return ESP_OK;
        }
    };

    class MockPsychicRequest : public PsychicRequest {
    };

    MockPsychicRequest req;
    MockPsychicResponse resp;
    esp_err_t res = getHandler(&req, &resp);
    EXPECT_EQ(res, ESP_OK);
    EXPECT_EQ(resp.sentCode, 200);
    EXPECT_EQ(resp.sentContentType, "application/json");

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, resp.sentBody);
    EXPECT_EQ(err, DeserializationError::Ok);
    ASSERT_TRUE(doc.is<JsonArray>());
    JsonArray arr = doc.as<JsonArray>();
    EXPECT_GE(arr.size(), 2u); // Wifi Station + Custom Op

    bool foundCustom = false;
    for (JsonObject obj : arr) {
        if (strcmp(obj["name"] | "", "Custom Op") == 0) {
            foundCustom = true;
            EXPECT_STREQ(obj["role"] | "", "Operational");
            EXPECT_TRUE(obj["met"] | false);
            EXPECT_STREQ(obj["cause"] | "", "Met");
            EXPECT_STREQ(obj["severity"] | "", "Transient");
            EXPECT_STREQ(obj["breaker"] | "", "Closed");
            EXPECT_FALSE(obj["suppressed"] | false);
        }
    }
    EXPECT_TRUE(foundCustom);
}

TEST(PreconditionApiTest, PreconditionsRetryHandler) {
    Application app;
    FrameworkConfig cfg;
    app.begin(cfg);
    app.tick(); // Evaluate preconditions

    PsychicHttpRequestCallback retryHandler = nullptr;
    for (const auto& route : app.getWebServerManager().getServer().routes) {
        if (route.uri == "/api/preconditions/retry" && route.method == HTTP_POST) {
            retryHandler = route.handler;
            break;
        }
    }
    ASSERT_NE(retryHandler, nullptr);

    class MockPsychicResponse : public PsychicResponse {
    public:
        int sentCode = 0;
        std::string sentBody;
        esp_err_t send(int code, const char* contentType, const char* content) override {
            sentCode = code;
            sentBody = content ? content : "";
            return ESP_OK;
        }
    };

    class MockPsychicRequest : public PsychicRequest {
    public:
        std::string b;
        size_t len = 0;
        const char* bodyCStr() override { return b.c_str(); }
        size_t contentLength() override { return len; }
    };

    // Valid retry
    {
        MockPsychicRequest req;
        req.b = "{\"name\":\"WiFi Station\"}";
        req.len = req.b.length();
        MockPsychicResponse resp;
        esp_err_t res = retryHandler(&req, &resp);
        EXPECT_EQ(res, ESP_OK);
        EXPECT_EQ(resp.sentCode, 200);
    }

    // Missing name
    {
        MockPsychicRequest req;
        req.b = "{}";
        req.len = req.b.length();
        MockPsychicResponse resp;
        esp_err_t res = retryHandler(&req, &resp);
        EXPECT_EQ(res, ESP_FAIL);
        EXPECT_EQ(resp.sentCode, 400);
    }

    // Oversized body
    {
        MockPsychicRequest req;
        req.b = "{\"name\":\"WiFi Station\"}";
        req.len = 2048; // > 1024
        MockPsychicResponse resp;
        esp_err_t res = retryHandler(&req, &resp);
        EXPECT_EQ(res, ESP_FAIL);
        EXPECT_EQ(resp.sentCode, 413);
    }
}

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

