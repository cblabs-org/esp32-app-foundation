/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_boards.cpp
 * Description: GoogleTest unit-test suite for the system display boards:
 *              BootBoard, WiFiWizardBoard, OtaProgressBoard,
 *              ErrorBoard, ScreensaverBoard, and ConfiguringStatusBoard.
 *              Validates board names, state transitions, and API contracts.
 *              All tests execute on the native GoogleTest runner (no hardware
 *              required).
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all board test suites
 */

#include <gtest/gtest.h>
#include "boards/bootBoard.hpp"
#include "boards/wifiWizardBoard.hpp"
#include "boards/otaProgressBoard.hpp"
#include "boards/errorBoard.hpp"
#include "boards/screensaverBoard.hpp"
#include "boards/configuringStatusBoard.hpp"

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

    void setSatisfied(bool val) { _satisfied = val; }

private:
    const char* _name;
    const char* _help;
    bool _satisfied;
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
// Test Group: OtaProgressBoard
// =============================================================================

class OtaProgressBoardTest : public ::testing::Test {
protected:
    OtaProgressBoard board{256, 64}; ///< Fresh instance for each test
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
// Test Group: ConfiguringStatusBoard
// =============================================================================

class ConfiguringStatusBoardTest : public ::testing::Test {
protected:
    ConfiguringStatusBoard board{256, 64}; ///< Fresh instance for each test

    // Mock gates for binding
    MockConfigGate gate1{"WiFi Station", "Connect to WiFi", true};
    MockConfigGate gate2{"API Keys", "Validate API keys", false};
    MockConfigGate gate3{"Time Sync", "Synchronize NTP", false};
    std::vector<IConfigGate*> gates;

    void SetUp() override {
        gates = {&gate1, &gate2, &gate3};
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

/** setGates must bind the gate list for counting. */
TEST_F(ConfiguringStatusBoardTest, SetGates_BindsGateList) {
    board.setGates(&gates);
    EXPECT_EQ(board.getTotalGateCount(), 3);
}

/** getSatisfiedCount must reflect gate satisfaction state. */
TEST_F(ConfiguringStatusBoardTest, GetSatisfiedCount_ReflectsState) {
    board.setGates(&gates);
    EXPECT_EQ(board.getSatisfiedCount(), 1); // Only gate1 is satisfied
}

/** getSatisfiedCount must update as gates become satisfied. */
TEST_F(ConfiguringStatusBoardTest, GetSatisfiedCount_UpdatesAsGatesSatisfied) {
    board.setGates(&gates);
    EXPECT_EQ(board.getSatisfiedCount(), 1);

    gate2.setSatisfied(true);
    EXPECT_EQ(board.getSatisfiedCount(), 2);

    gate3.setSatisfied(true);
    EXPECT_EQ(board.getSatisfiedCount(), 3);
}

/** getTotalGateCount must return 0 when no gates are bound. */
TEST_F(ConfiguringStatusBoardTest, GetTotalGateCount_NoGates_ReturnsZero) {
    EXPECT_EQ(board.getTotalGateCount(), 0);
}

/** getSatisfiedCount must return 0 when no gates are bound. */
TEST_F(ConfiguringStatusBoardTest, GetSatisfiedCount_NoGates_ReturnsZero) {
    EXPECT_EQ(board.getSatisfiedCount(), 0);
}

/** setGates(nullptr) must unbind the gate list safely. */
TEST_F(ConfiguringStatusBoardTest, SetGates_Nullptr_UnbindsGates) {
    board.setGates(&gates);
    EXPECT_EQ(board.getTotalGateCount(), 3);

    board.setGates(nullptr);
    EXPECT_EQ(board.getTotalGateCount(), 0);
    EXPECT_EQ(board.getSatisfiedCount(), 0);
}

/** dismiss must not affect gate counts. */
TEST_F(ConfiguringStatusBoardTest, Dismiss_DoesNotAffectGateCounts) {
    board.setGates(&gates);
    board.dismiss();
    EXPECT_TRUE(board.isExpired());
    EXPECT_EQ(board.getTotalGateCount(), 3);
    EXPECT_EQ(board.getSatisfiedCount(), 1);
}

/** All gates satisfied count must equal total gate count. */
TEST_F(ConfiguringStatusBoardTest, AllGatesSatisfied_CountMatchesTotal) {
    gate1.setSatisfied(true);
    gate2.setSatisfied(true);
    gate3.setSatisfied(true);
    board.setGates(&gates);
    EXPECT_EQ(board.getSatisfiedCount(), board.getTotalGateCount());
}

/** Empty gate vector must return zero counts. */
TEST_F(ConfiguringStatusBoardTest, EmptyGateVector_ReturnsZeroCounts) {
    std::vector<IConfigGate*> emptyGates;
    board.setGates(&emptyGates);
    EXPECT_EQ(board.getTotalGateCount(), 0);
    EXPECT_EQ(board.getSatisfiedCount(), 0);
}
