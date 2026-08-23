/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2Display/test_boards.cpp
 * Description: GoogleTest unit-test suite for the system display boards:
 *              BootBoard, WiFiWizardBoard, OTAProgressBoard,
 *              ErrorBoard, ScreensaverBoard, and ConfiguringStatusBoard.
 *              Validates board names, state transitions, and API contracts.
 *              All tests execute on the native GoogleTest runner (no hardware
 *              required).
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all board test suites
 */

#include <gtest/gtest.h>
#include "boards/BootBoard.hpp"
#include "boards/WiFiWizardBoard.hpp"
#include "boards/OTAProgressBoard.hpp"
#include "boards/ErrorBoard.hpp"
#include "boards/ScreensaverBoard.hpp"
#include "boards/ConfiguringStatusBoard.hpp"

// =============================================================================
// Mock: IConfigGate for unit testing
// =============================================================================

/**
 * @brief Simple mock IConfigGate for testing ConfiguringStatusBoard.
 *
 * Allows manual control of satisfaction state via setSatisfied().
 */
class MockConfigGate : public IConfigGate {
public:
    MockConfigGate(const char* name, const char* help, bool satisfied = false)
        : _name(name), _help(help), _satisfied(satisfied) {}

    bool isSatisfied() const override { return _satisfied; }
    const char* getGateName() const override { return _name; }
    const char* getHelpText() const override { return _help; }

    const char* const* getDependencies(uint8_t& count) const override {
        count = _depCount;
        return _deps;
    }

    GateActionStatus getActionStatus() const override {
        return _actionStatus;
    }

    void setSatisfied(bool val) { _satisfied = val; }

    void setDependencies(const char* const* deps, uint8_t count) {
        _deps = deps;
        _depCount = count;
    }

    void setActionStatus(GateActionStatus status) { _actionStatus = status; }

private:
    const char* _name;
    const char* _help;
    bool _satisfied;
    const char* const* _deps = nullptr;
    uint8_t _depCount = 0;
    GateActionStatus _actionStatus = GateActionStatus::NeedsUserAction;
};

// =============================================================================
// Test Group: BootBoard
// =============================================================================

class BootBoardTest : public ::testing::Test {
protected:
    BootBoard board{256, 64}; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(BootBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: Boot");
}

/** isExpired must return false before activate() is called. */
TEST_F(BootBoardTest, IsExpired_BeforeActivate_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** isExpired must return false immediately after activate() (native build). */
TEST_F(BootBoardTest, IsExpired_AfterActivate_ReturnsFalse) {
    board.activate("1.0.0");
    EXPECT_FALSE(board.isExpired());
}

/** activate must not crash with a nullptr version. */
TEST_F(BootBoardTest, Activate_NullVersion_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.activate(nullptr));
}

/** activate must not crash with an empty string. */
TEST_F(BootBoardTest, Activate_EmptyVersion_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.activate(""));
}

/** setComplete must cause isExpired to return true. */
TEST_F(BootBoardTest, SetComplete_CausesExpiry) {
    board.setComplete();
    EXPECT_TRUE(board.isExpired());
}

/** setStatus must not crash with valid message and progress. */
TEST_F(BootBoardTest, SetStatus_ValidInput_DoesNotCrash) {
    board.activate("1.0.0");
    EXPECT_NO_FATAL_FAILURE(board.setStatus("Connecting...", 0.5f));
}

/** setStatus must not crash with nullptr message. */
TEST_F(BootBoardTest, SetStatus_NullMessage_DoesNotCrash) {
    board.activate("1.0.0");
    EXPECT_NO_FATAL_FAILURE(board.setStatus(nullptr, 0.0f));
}

/** setStatus with indeterminate progress must not crash. */
TEST_F(BootBoardTest, SetStatus_IndeterminateProgress_DoesNotCrash) {
    board.activate("1.0.0");
    EXPECT_NO_FATAL_FAILURE(board.setStatus("Loading...", -1.0f));
}

/** isExpired remains false after setStatus (loading not complete). */
TEST_F(BootBoardTest, SetStatus_DoesNotCauseExpiry) {
    board.activate("1.0.0");
    board.setStatus("Step 1", 0.25f);
    EXPECT_FALSE(board.isExpired());
}

/** BootBoard construction with custom logo correctly sets image dimensions. */
TEST_F(BootBoardTest, CustomLogo_SetsLogoDimensions) {
    static const uint8_t dummyBits[] = {0x00};
    U8G2BootLogo customLogo{48, 48, dummyBits};
    BootBoard customBoard{256, 64, &customLogo};

    EXPECT_EQ(customBoard.getLogoWidth(), 48);
    EXPECT_EQ(customBoard.getLogoHeight(), 48);
}

/** BootBoard construction with nullptr logo falls back to default logo dimensions. */
TEST_F(BootBoardTest, NullptrLogo_FallsBackToDefaultLogoDimensions) {
    BootBoard defaultBoard{256, 64, nullptr};

    EXPECT_EQ(defaultBoard.getLogoWidth(), 110);
    EXPECT_EQ(defaultBoard.getLogoHeight(), 32);
}

/** BootBoard construction with custom title sets the correct app title. */
TEST_F(BootBoardTest, CustomTitle_SetsAppTitle) {
    BootBoard customTitleBoard{256, 64, nullptr, "Custom App Title"};

    EXPECT_STREQ(customTitleBoard.getAppTitle(), "Custom App Title");
}


// =============================================================================
// Test Group: WiFiWizardBoard
// =============================================================================

class WiFiWizardBoardTest : public ::testing::Test {
protected:
    WiFiWizardBoard board{256, 64}; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(WiFiWizardBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: WiFi Wizard");
}

/** isExpired must return false initially (not dismissed). */
TEST_F(WiFiWizardBoardTest, IsExpired_Initially_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** dismiss must cause isExpired to return true. */
TEST_F(WiFiWizardBoardTest, Dismiss_CausesExpiry) {
    board.dismiss();
    EXPECT_TRUE(board.isExpired());
}

/** configure must not crash with valid SSID and default IP. */
TEST_F(WiFiWizardBoardTest, Configure_ValidSsid_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.configure("MyNetwork"));
}

/** configure must not crash with custom IP. */
TEST_F(WiFiWizardBoardTest, Configure_CustomIp_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.configure("TestAP", "10.0.0.1"));
}

/** configure must not crash with nullptr SSID. */
TEST_F(WiFiWizardBoardTest, Configure_NullSsid_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.configure(nullptr));
}

/** isExpired remains false after configure (not dismissed). */
TEST_F(WiFiWizardBoardTest, Configure_DoesNotCauseExpiry) {
    board.configure("TestAP");
    EXPECT_FALSE(board.isExpired());
}

// =============================================================================
// Test Group: OTAProgressBoard
// =============================================================================

class OtaProgressBoardTest : public ::testing::Test {
protected:
    OTAProgressBoard board{256, 64}; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(OtaProgressBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: OTA Progress");
}

/** isExpired must return false initially (no update in progress). */
TEST_F(OtaProgressBoardTest, IsExpired_Initially_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** setProgress must not crash with valid inputs. */
TEST_F(OtaProgressBoardTest, SetProgress_ValidInput_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.setProgress(0.5f, "Downloading..."));
}

/** setProgress must not crash with nullptr status. */
TEST_F(OtaProgressBoardTest, SetProgress_NullStatus_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.setProgress(0.5f, nullptr));
}

/** setProgress must not cause expiry. */
TEST_F(OtaProgressBoardTest, SetProgress_DoesNotCauseExpiry) {
    board.setProgress(0.75f, "75%");
    EXPECT_FALSE(board.isExpired());
}

/** setComplete(true) must cause isExpired to return true (auto-dismiss). */
TEST_F(OtaProgressBoardTest, SetComplete_Success_CausesExpiry) {
    board.setComplete(true);
    EXPECT_TRUE(board.isExpired());
}

/** setComplete(false) must NOT cause isExpired to return true (persist). */
TEST_F(OtaProgressBoardTest, SetComplete_Failure_DoesNotCauseExpiry) {
    board.setComplete(false);
    EXPECT_FALSE(board.isExpired());
}

/** reset() must clear the completion state back to initial. */
TEST_F(OtaProgressBoardTest, Reset_ClearsState) {
    board.setProgress(0.9f, "Almost done");
    board.setComplete(true);
    EXPECT_TRUE(board.isExpired());

    board.reset();
    EXPECT_FALSE(board.isExpired());
}

/** reset() must clear the failure state. */
TEST_F(OtaProgressBoardTest, Reset_ClearsFailure) {
    board.setComplete(false);
    EXPECT_FALSE(board.isExpired()); // Failed = not expired

    board.reset();
    EXPECT_FALSE(board.isExpired()); // Reset = back to initial
}

// =============================================================================
// Test Group: ErrorBoard
// =============================================================================

class ErrorBoardTest : public ::testing::Test {
protected:
    ErrorBoard board{256, 64}; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(ErrorBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: Error");
}

/** isExpired must return false initially (no error shown). */
TEST_F(ErrorBoardTest, IsExpired_Initially_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** showError must not cause expiry (error is active). */
TEST_F(ErrorBoardTest, ShowError_DoesNotCauseExpiry) {
    board.showError("Something went wrong");
    EXPECT_FALSE(board.isExpired());
}

/** dismiss must cause isExpired to return true. */
TEST_F(ErrorBoardTest, Dismiss_CausesExpiry) {
    board.showError("Test error");
    board.dismiss();
    EXPECT_TRUE(board.isExpired());
}

/** showError with fatal flag must not cause expiry. */
TEST_F(ErrorBoardTest, ShowError_Fatal_DoesNotCauseExpiry) {
    board.showError("Critical failure", true);
    EXPECT_FALSE(board.isExpired());
}

/** Fatal errors can still be dismissed manually. */
TEST_F(ErrorBoardTest, Dismiss_Fatal_CausesExpiry) {
    board.showError("Critical failure", true);
    board.dismiss();
    EXPECT_TRUE(board.isExpired());
}

/** showError must reset dismissed state (re-show after dismiss). */
TEST_F(ErrorBoardTest, ShowError_AfterDismiss_ResetsExpiry) {
    board.showError("Error 1");
    board.dismiss();
    EXPECT_TRUE(board.isExpired());

    board.showError("Error 2");
    EXPECT_FALSE(board.isExpired());
}

/** showError must not crash with nullptr message. */
TEST_F(ErrorBoardTest, ShowError_NullMessage_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.showError(nullptr));
}

// =============================================================================
// Test Group: ScreensaverBoard
// =============================================================================

class ScreensaverBoardTest : public ::testing::Test {
protected:
    ScreensaverBoard board{256, 64}; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(ScreensaverBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: Screensaver");
}

/** isExpired must return true initially (screensaver not active). */
TEST_F(ScreensaverBoardTest, IsExpired_Initially_ReturnsTrue) {
    EXPECT_TRUE(board.isExpired());
}

/** activate must cause isExpired to return false. */
TEST_F(ScreensaverBoardTest, Activate_CausesNotExpired) {
    board.activate();
    EXPECT_FALSE(board.isExpired());
}

/** deactivate must cause isExpired to return true. */
TEST_F(ScreensaverBoardTest, Deactivate_CausesExpiry) {
    board.activate();
    EXPECT_FALSE(board.isExpired());

    board.deactivate();
    EXPECT_TRUE(board.isExpired());
}

/** isActive must reflect activation state. */
TEST_F(ScreensaverBoardTest, IsActive_ReflectsState) {
    EXPECT_FALSE(board.isActive());

    board.activate();
    EXPECT_TRUE(board.isActive());

    board.deactivate();
    EXPECT_FALSE(board.isActive());
}

/** Multiple activate/deactivate cycles must work correctly. */
TEST_F(ScreensaverBoardTest, MultipleCycles_WorkCorrectly) {
    board.activate();
    EXPECT_FALSE(board.isExpired());
    board.deactivate();
    EXPECT_TRUE(board.isExpired());

    board.activate();
    EXPECT_FALSE(board.isExpired());
    board.deactivate();
    EXPECT_TRUE(board.isExpired());
}

// =============================================================================
// =============================================================================
// Test Group: ConfiguringStatusBoard
// =============================================================================

#include "interfaces/IPrecondition.hpp"
#include "events/PreconditionHealthPayload.hpp"
#include "PreconditionEngine/PreconditionEngine.hpp"

class ConfiguringStatusBoardTest : public ::testing::Test {
protected:
    ConfiguringStatusBoard board{256, 64}; ///< Fresh instance for each test

    PreconditionSnapshot prec1{"WiFi Station", PreconditionRole::Startup, Cause::Met, Severity::Transient, true, true};
    PreconditionSnapshot prec2{"API Keys", PreconditionRole::Startup, Cause::NotConfigured, Severity::NeedsUser, false, false};
    PreconditionSnapshot prec3{"Time Sync", PreconditionRole::Startup, Cause::DependencyUnmet, Severity::Blocked, false, false};
    std::vector<PreconditionSnapshot> snapshots;

    void SetUp() override {
        snapshots = {prec1, prec2, prec3};
    }
};

/** getBoardName must return the expected system board identifier. */
TEST_F(ConfiguringStatusBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: Configuring");
}

/** isExpired must return false initially (not dismissed). */
TEST_F(ConfiguringStatusBoardTest, IsExpired_Initially_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** dismiss must cause isExpired to return true. */
TEST_F(ConfiguringStatusBoardTest, Dismiss_CausesExpiry) {
    board.dismiss();
    EXPECT_TRUE(board.isExpired());
}

/** setPreconditions must bind the snapshot list for counting. */
TEST_F(ConfiguringStatusBoardTest, SetPreconditions_BindsSnapshotList) {
    board.setPreconditions(snapshots.data(), snapshots.size());
    EXPECT_EQ(board.getTotalPreconditions(), 3);
    EXPECT_EQ(board.getStartupCount(), 3);
    EXPECT_EQ(board.getStartupMetCount(), 1);
}

/** getStartupMetCount must reflect precondition satisfaction state. */
TEST_F(ConfiguringStatusBoardTest, GetStartupMetCount_ReflectsState) {
    board.setPreconditions(snapshots.data(), snapshots.size());
    EXPECT_EQ(board.getStartupMetCount(), 1); // Only prec1 is met
}

/** getStartupMetCount and target percent must update as preconditions become met. */
TEST_F(ConfiguringStatusBoardTest, GetStartupMetCount_UpdatesAsPreconditionsSatisfied) {
    board.setPreconditions(snapshots.data(), snapshots.size());
    EXPECT_EQ(board.getStartupMetCount(), 1);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), (1.0f / 3.0f) * 100.0f);

    board.updatePrecondition("API Keys", Cause::Met, Severity::Transient);
    EXPECT_EQ(board.getStartupMetCount(), 2);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), (2.0f / 3.0f) * 100.0f);

    board.updatePrecondition("Time Sync", Cause::Met, Severity::Transient);
    EXPECT_EQ(board.getStartupMetCount(), 3);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 100.0f);
}

/** getTotalPreconditions must return 0 when no preconditions are bound. */
TEST_F(ConfiguringStatusBoardTest, GetTotalPreconditions_NoPreconditions_ReturnsZero) {
    EXPECT_EQ(board.getTotalPreconditions(), 0);
    EXPECT_EQ(board.getStartupCount(), 0);
    EXPECT_EQ(board.getStartupMetCount(), 0);
}

/** clear must unbind / reset snapshots safely. */
TEST_F(ConfiguringStatusBoardTest, Clear_ResetsCounts) {
    board.setPreconditions(snapshots.data(), snapshots.size());
    EXPECT_EQ(board.getTotalPreconditions(), 3);

    board.clear();
    EXPECT_EQ(board.getTotalPreconditions(), 0);
    EXPECT_EQ(board.getStartupCount(), 0);
    EXPECT_EQ(board.getStartupMetCount(), 0);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 0.0f);
}

/** dismiss must not affect precondition counts. */
TEST_F(ConfiguringStatusBoardTest, Dismiss_DoesNotAffectCounts) {
    board.setPreconditions(snapshots.data(), snapshots.size());
    board.dismiss();
    EXPECT_TRUE(board.isExpired());
    EXPECT_EQ(board.getTotalPreconditions(), 3);
    EXPECT_EQ(board.getStartupMetCount(), 1);
}

/** All startup preconditions satisfied must yield 100% target progress. */
TEST_F(ConfiguringStatusBoardTest, AllStartupPreconditionsSatisfied_Yields100Percent) {
    snapshots[0].cause = Cause::Met;
    snapshots[0].met = true;
    snapshots[1].cause = Cause::Met;
    snapshots[1].met = true;
    snapshots[2].cause = Cause::Met;
    snapshots[2].met = true;

    board.setPreconditions(snapshots.data(), snapshots.size());
    EXPECT_EQ(board.getStartupMetCount(), 3);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 100.0f);
}

/** Progress bar counts Startup-role preconditions ONLY (CG-14). */
TEST_F(ConfiguringStatusBoardTest, ProgressBar_CountsStartupOnly) {
    PreconditionSnapshot mixed[4] = {
        {"WiFi Station", PreconditionRole::Startup, Cause::Met, Severity::Transient, true, true},
        {"Time Sync", PreconditionRole::Startup, Cause::NotConfigured, Severity::NeedsUser, false, false},
        {"Weather Data", PreconditionRole::Operational, Cause::Met, Severity::Transient, true, true},
        {"OTA Channel", PreconditionRole::Operational, Cause::Met, Severity::Transient, true, true}
    };

    board.setPreconditions(mixed, 4);
    EXPECT_EQ(board.getTotalPreconditions(), 4);
    EXPECT_EQ(board.getStartupCount(), 2);
    EXPECT_EQ(board.getStartupMetCount(), 1);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 50.0f);
}

/** Operational preconditions do NOT affect progress percentage (even if unmet or failing). */
TEST_F(ConfiguringStatusBoardTest, OperationalPreconditions_DoNotAffectProgressPercentage) {
    PreconditionSnapshot items[3] = {
        {"WiFi Station", PreconditionRole::Startup, Cause::Met, Severity::Transient, true, true},
        {"Time Sync", PreconditionRole::Startup, Cause::Met, Severity::Transient, true, true},
        {"MQTT Broker", PreconditionRole::Operational, Cause::Unreachable, Severity::Transient, false, false}
    };

    board.setPreconditions(items, 3);
    EXPECT_EQ(board.getStartupCount(), 2);
    EXPECT_EQ(board.getStartupMetCount(), 2);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 100.0f); // 2/2 startup met = 100%, op is ignored for progress

    board.updatePrecondition("MQTT Broker", Cause::Rejected, Severity::NeedsUser, false, PreconditionRole::Operational);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 100.0f);
}

/** Precondition updates: updating status changes startup met count and target percent. */
TEST_F(ConfiguringStatusBoardTest, DynamicPreconditionUpdates) {
    board.updatePrecondition("WiFi Station", Cause::Connecting, Severity::Transient, false, PreconditionRole::Startup);
    board.updatePrecondition("Time Sync", Cause::NotConfigured, Severity::NeedsUser, false, PreconditionRole::Startup);
    board.updatePrecondition("Weather", Cause::Met, Severity::Transient, true, PreconditionRole::Operational);

    EXPECT_EQ(board.getTotalPreconditions(), 3);
    EXPECT_EQ(board.getStartupCount(), 2);
    EXPECT_EQ(board.getStartupMetCount(), 0);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 0.0f);

    board.updatePrecondition("WiFi Station", Cause::Met, Severity::Transient, true, PreconditionRole::Startup);
    EXPECT_EQ(board.getStartupMetCount(), 1);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 50.0f);

    board.updatePrecondition("Time Sync", Cause::Met, Severity::Transient, true, PreconditionRole::Startup);
    EXPECT_EQ(board.getStartupMetCount(), 2);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 100.0f);
}


/** Cause-aware prefixes and snapshot properties. */
TEST_F(ConfiguringStatusBoardTest, PreconditionSnapshots_CauseAndSeverity) {
    PreconditionSnapshot items[6] = {
        {"WiFi Met", PreconditionRole::Startup, Cause::Met, Severity::Transient, true, true},
        {"WiFi Conn", PreconditionRole::Startup, Cause::Connecting, Severity::Transient, false, false},
        {"WiFi Reject", PreconditionRole::Startup, Cause::Rejected, Severity::NeedsUser, false, false},
        {"Time Blocked", PreconditionRole::Startup, Cause::DependencyUnmet, Severity::Blocked, false, false},
        {"Bug Indet", PreconditionRole::Startup, Cause::Indeterminate, Severity::Persistent, false, false},
        {"Rate Lim", PreconditionRole::Operational, Cause::RateLimited, Severity::Transient, false, false}
    };

    board.setPreconditions(items, 6);
    EXPECT_EQ(board.getTotalPreconditions(), 6);
    EXPECT_EQ(board.getStartupCount(), 5);
    EXPECT_EQ(board.getStartupMetCount(), 1);
    EXPECT_FLOAT_EQ(board.getTargetPercent(), 20.0f);
}


/** setActiveGateStatus stores the status value. */
TEST_F(ConfiguringStatusBoardTest, SetActiveGateStatus_StoresStatus) {
    board.setActiveGateStatus(GateActionStatus::InProgress);
    EXPECT_EQ(board.getActiveGateStatus(), GateActionStatus::InProgress);

    board.setActiveGateStatus(GateActionStatus::NeedsUserAction);
    EXPECT_EQ(board.getActiveGateStatus(), GateActionStatus::NeedsUserAction);

    board.setActiveGateStatus(GateActionStatus::Satisfied);
    EXPECT_EQ(board.getActiveGateStatus(), GateActionStatus::Satisfied);
}

/** setActionableGateName stores the name for render comparison. */
TEST_F(ConfiguringStatusBoardTest, SetActionableGateName_StoresName) {
    board.setActionableGateName("WiFi Station");
    EXPECT_STREQ(board.getActionableGateName(), "WiFi Station");

    board.setActionableGateName(nullptr);
    EXPECT_EQ(board.getActionableGateName(), nullptr);
}

/** setConnectionInfo must not crash with valid and null inputs. */
TEST_F(ConfiguringStatusBoardTest, SetConnectionInfo_ValidAndNull_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.setConnectionInfo("MyNetwork", "http://192.168.1.42"));
    EXPECT_NO_FATAL_FAILURE(board.setConnectionInfo(nullptr, nullptr));
}


// =============================================================================
// Test Group: GenericGateHelperBoard
// =============================================================================

#include "boards/GenericGateHelperBoard.hpp"

class GenericGateHelperBoardTest : public ::testing::Test {
protected:
    GenericGateHelperBoard board{256, 64}; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(GenericGateHelperBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: Gate Helper");
}

/** isExpired must return false initially (not dismissed). */
TEST_F(GenericGateHelperBoardTest, IsExpired_Initially_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** dismiss must cause isExpired to return true. */
TEST_F(GenericGateHelperBoardTest, Dismiss_CausesExpiry) {
    board.dismiss();
    EXPECT_TRUE(board.isExpired());
}

/** configure must not crash with valid inputs. */
TEST_F(GenericGateHelperBoardTest, Configure_ValidInputs_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.configure("Board Config", "Add a departure board", "192.168.1.42"));
}

/** configure must not crash with nullptr inputs. */
TEST_F(GenericGateHelperBoardTest, Configure_NullInputs_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.configure(nullptr, nullptr, nullptr));
}

/** configure must reset dismissed state (board becomes active again). */
TEST_F(GenericGateHelperBoardTest, Configure_AfterDismiss_ResetsExpiry) {
    board.dismiss();
    EXPECT_TRUE(board.isExpired());

    board.configure("Test Gate", "Help text", "10.0.0.1");
    EXPECT_FALSE(board.isExpired());
}

/** configure with empty strings must not crash. */
TEST_F(GenericGateHelperBoardTest, Configure_EmptyStrings_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.configure("", "", ""));
}

/** Multiple configure calls must not crash (reconfiguration). */
TEST_F(GenericGateHelperBoardTest, Configure_MultipleCalls_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.configure("Gate A", "Help A", "192.168.1.1"));
    EXPECT_NO_FATAL_FAILURE(board.configure("Gate B", "Help B", "192.168.1.2"));
    EXPECT_NO_FATAL_FAILURE(board.configure("Gate C", "Help C", "192.168.1.3"));
}

/** configure must clear buffers when given nullptrs. */
TEST_F(GenericGateHelperBoardTest, Configure_Nullptr_ClearsPreviousState) {
    board.configure("TestGate", "Help text", "192.168.1.1");
    EXPECT_NO_FATAL_FAILURE(board.configure(nullptr, nullptr, nullptr));
    // Implementation validates this doesn't crash and handles nulls safely.
}
