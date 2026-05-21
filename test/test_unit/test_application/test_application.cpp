/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_application/test_application.cpp
 * Description: GoogleTest unit-test suite for the Application composition root.
 *              Covers module registration (pre-lock and post-lock), the
 *              two-tier vector lock model, AppState lifecycle transitions,
 *              tick() dispatch behaviour, and softReset() native-build safety.
 *
 *              All tests execute on the native GoogleTest runner (no hardware
 *              required). NATIVE_BUILD is defined by the [env:native] environment
 *              in platformio.ini.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all test suites
 */

#include <gtest/gtest.h>
#include "Application.hpp"
#include "interfaces/IMcpContributor.hpp"
#include "mcpToolRegistry.hpp"

// =============================================================================
// Mock implementations
// Simple inline concrete classes satisfy the pure-virtual interface contracts
// without requiring a mocking framework.
// =============================================================================

/** @brief Minimal ITickable mock that counts how many times tick() is called. */
class MockTickable : public ITickable {
public:
    int tickCount = 0; ///< Incremented once per tick() dispatch

    void tick() override { tickCount++; }
};

/** @brief Minimal iConfigurable mock with all three methods returning success. */
class MockConfigurable : public iConfigurable {
public:
    bool loadConfigCalled = false; ///< Set true when loadConfig() is invoked

    bool loadConfig() override {
        loadConfigCalled = true;
        return true;
    }

    bool saveConfig() override { return true; }

    void resetConfig() override {}
};

/** @brief Minimal iDataSource mock — scheduling metadata returns fixed values. */
class MockDataSource : public iDataSource {
public:
    UpdateStatus fetchUpdate() override               { return UpdateStatus::SUCCESS; }
    uint32_t     getUpdateIntervalMs() const override { return 60000; } // 60 s
    const char*  getSourceName()       const override { return "MockDataSource"; }
    PriorityTier getPriorityTier()     const override { return PriorityTier::PRIORITY_NORMAL; }
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
    // AsyncWebServer is forward-declared in IWebContributor.hpp; the mock
    // accepts the reference signature but never dereferences the pointer so
    // no linker dependency on ESPAsyncWebServer is introduced.
    void registerRoutes(AsyncWebServer& /*server*/) override {}
};

/** @brief Minimal IMcpContributor mock that registers a named test tool. */
class MockMcpContributor : public IMcpContributor {
public:
    bool registered = false; ///< Set true when registerMcpTools() is invoked

    void registerMcpTools(McpToolRegistry& registry) override {
        registered = true;
        registry.addTool(
            "mock_mcp_tool",
            "A mock MCP tool for testing",
            [](JsonObject& params, JsonObject& result) {
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

/** registerConfigurable with a valid pointer must return true. */
TEST_F(ApplicationRegistrationTest, RegisterConfigurable_ValidPointer_ReturnsTrue) {
    MockConfigurable configurable;
    EXPECT_TRUE(app.registerConfigurable(&configurable));
}

/** registerConfigurable with nullptr must return false. */
TEST_F(ApplicationRegistrationTest, RegisterConfigurable_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(app.registerConfigurable(nullptr));
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
 * Helper: access the private _compactAndLock() method via begin() in NATIVE_BUILD.
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
     * Uses default FrameworkConfig (all safe defaults). In NATIVE_BUILD
     * the LittleFS mount is skipped, so begin() always returns true.
     */
    void lockApp() {
        FrameworkConfig cfg;
        bool result = app.begin(cfg);
        // Tolerate begin() returning false in any environment; what matters
        // is that _compactAndLock() was called (which it always is before
        // the potential early-return on LittleFS failure — in NATIVE_BUILD
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

/** After begin(), registerConfigurable must return false. */
TEST_F(ApplicationLockTest, PostLock_RegisterConfigurable_ReturnsFalse) {
    lockApp();
    MockConfigurable configurable;
    EXPECT_FALSE(app.registerConfigurable(&configurable));
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
 * A MockMcpContributor registered before begin() must have its tool
 * visible in the McpServer's registry after begin() completes.
 */
TEST_F(ApplicationLockTest, McpContributor_ToolVisibleAfterBegin) {
    MockMcpContributor contributor;
    app.registerMcpContributor(&contributor);
    lockApp();

    EXPECT_TRUE(contributor.registered);
    EXPECT_NE(app.getMcpServer().getRegistry().findTool("mock_mcp_tool"), nullptr);
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

    // Do NOT call begin() — state remains BOOTING
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
 * A freshly constructed Application must report BOOTING as its initial state.
 * This is the safe default for any code that queries state before begin().
 */
TEST_F(ApplicationStateTest, InitialState_IsBooting) {
    EXPECT_EQ(app.getState(), AppState::BOOTING);
}

/**
 * After a successful begin(), state must be RUNNING.
 * In NATIVE_BUILD, LittleFS is skipped so begin() always returns true
 * and the state reaches RUNNING.
 */
TEST_F(ApplicationStateTest, AfterBegin_StateIsRunning) {
    FrameworkConfig cfg;
    app.begin(cfg);
    EXPECT_EQ(app.getState(), AppState::RUNNING);
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
 * softReset() must complete without crashing the test process in NATIVE_BUILD.
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
// Test Group 5: begin() sequence integration (loadConfig dispatch)
// =============================================================================

class ApplicationBeginTest : public ::testing::Test {
protected:
    Application app;
};

/**
 * begin() must call loadConfig() on all registered iConfigurable modules
 * during the APP_SETUP phase.
 */
TEST_F(ApplicationBeginTest, Begin_CallsLoadConfigOnAllConfigurables) {
    MockConfigurable confA;
    MockConfigurable confB;
    app.registerConfigurable(&confA);
    app.registerConfigurable(&confB);

    FrameworkConfig cfg;
    app.begin(cfg);

    EXPECT_TRUE(confA.loadConfigCalled);
    EXPECT_TRUE(confB.loadConfigCalled);
}

/**
 * begin() in NATIVE_BUILD must return true (LittleFS stub succeeds).
 */
TEST_F(ApplicationBeginTest, Begin_NativeBuild_ReturnsTrue) {
    FrameworkConfig cfg;
    EXPECT_TRUE(app.begin(cfg));
}

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
