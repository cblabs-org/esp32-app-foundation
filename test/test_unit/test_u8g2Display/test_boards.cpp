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
 *              BootSplashBoard, LoadingBoard, and WiFiWizardBoard. Validates
 *              board names, state transitions, and API contracts. All tests
 *              execute on the native GoogleTest runner (no hardware required).
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all board test suites
 */

#include <gtest/gtest.h>
#include "boards/bootSplashBoard.hpp"
#include "boards/loadingBoard.hpp"
#include "boards/wifiWizardBoard.hpp"

// =============================================================================
// Test Group: BootSplashBoard
// =============================================================================

class BootSplashBoardTest : public ::testing::Test {
protected:
    BootSplashBoard board; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(BootSplashBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: Boot Splash");
}

/** isExpired must return false before activate() is called. */
TEST_F(BootSplashBoardTest, IsExpired_BeforeActivate_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** isExpired must return false immediately after activate() (native build). */
TEST_F(BootSplashBoardTest, IsExpired_AfterActivate_ReturnsFalse) {
    board.activate("1.0.0");
    // In NATIVE_BUILD, millis() is unavailable — isExpired() always returns false
    EXPECT_FALSE(board.isExpired());
}

/** activate must not crash with a nullptr version. */
TEST_F(BootSplashBoardTest, Activate_NullVersion_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.activate(nullptr));
}

/** activate must not crash with an empty string. */
TEST_F(BootSplashBoardTest, Activate_EmptyVersion_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.activate(""));
}

// =============================================================================
// Test Group: LoadingBoard
// =============================================================================

class LoadingBoardTest : public ::testing::Test {
protected:
    LoadingBoard board; ///< Fresh instance for each test
};

/** getBoardName must return the expected system board identifier. */
TEST_F(LoadingBoardTest, GetBoardName_ReturnsCorrectName) {
    EXPECT_STREQ(board.getBoardName(), "SYS: Loading");
}

/** isExpired must return false initially (loading not complete). */
TEST_F(LoadingBoardTest, IsExpired_Initially_ReturnsFalse) {
    EXPECT_FALSE(board.isExpired());
}

/** setComplete must cause isExpired to return true. */
TEST_F(LoadingBoardTest, SetComplete_CausesExpiry) {
    board.setComplete();
    EXPECT_TRUE(board.isExpired());
}

/** setStatus must not crash with valid message and progress. */
TEST_F(LoadingBoardTest, SetStatus_ValidInput_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.setStatus("Connecting...", 0.5f));
}

/** setStatus must not crash with nullptr message. */
TEST_F(LoadingBoardTest, SetStatus_NullMessage_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.setStatus(nullptr, 0.0f));
}

/** setStatus with indeterminate progress must not crash. */
TEST_F(LoadingBoardTest, SetStatus_IndeterminateProgress_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(board.setStatus("Loading...", -1.0f));
}

/** isExpired remains false after setStatus (loading not complete). */
TEST_F(LoadingBoardTest, SetStatus_DoesNotCauseExpiry) {
    board.setStatus("Step 1", 0.25f);
    EXPECT_FALSE(board.isExpired());
}

// =============================================================================
// Test Group: WiFiWizardBoard
// =============================================================================

class WiFiWizardBoardTest : public ::testing::Test {
protected:
    WiFiWizardBoard board; ///< Fresh instance for each test
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
