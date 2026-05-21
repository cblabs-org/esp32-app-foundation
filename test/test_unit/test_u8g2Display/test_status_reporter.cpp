/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_status_reporter.cpp
 * Description: GoogleTest unit-test suite for the U8g2DisplayModule's
 *              IStatusReporter implementation. Validates that reportStatus()
 *              delegates to LoadingBoard, reportError() delegates to ErrorBoard,
 *              and clearStatus() marks LoadingBoard as complete.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "u8g2DisplayModule.hpp"

// =============================================================================
// Test Group: IStatusReporter delegation
// =============================================================================

class StatusReporterTest : public ::testing::Test {
protected:
    U8g2DisplayModule display; ///< Fresh module instance for each test
};

/** reportStatus must not crash and must delegate to LoadingBoard. */
TEST_F(StatusReporterTest, ReportStatus_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportStatus("Booting...", 0.5f));
}

/** reportStatus must show the loading board as active. */
TEST_F(StatusReporterTest, ReportStatus_ShowsLoadingBoard) {
    display.reportStatus("Connecting...", 0.3f);
    EXPECT_EQ(display.getActiveBoard(), &display.getLoading());
}

/** reportStatus with indeterminate progress must work. */
TEST_F(StatusReporterTest, ReportStatus_IndeterminateProgress) {
    EXPECT_NO_FATAL_FAILURE(display.reportStatus("Working...", -1.0f));
    EXPECT_EQ(display.getActiveBoard(), &display.getLoading());
}

/** reportStatus with nullptr message must not crash. */
TEST_F(StatusReporterTest, ReportStatus_NullMessage_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportStatus(nullptr, 0.0f));
}

/** Multiple reportStatus calls must keep the loading board active. */
TEST_F(StatusReporterTest, ReportStatus_MultipleCalls_KeepsLoadingActive) {
    display.reportStatus("Step 1", 0.25f);
    display.reportStatus("Step 2", 0.50f);
    display.reportStatus("Step 3", 0.75f);
    EXPECT_EQ(display.getActiveBoard(), &display.getLoading());
}

/** reportError must not crash. */
TEST_F(StatusReporterTest, ReportError_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportError("Fatal error"));
}

/** reportError must show the error board as active. */
TEST_F(StatusReporterTest, ReportError_ShowsErrorBoard) {
    display.reportError("Critical failure");
    EXPECT_EQ(display.getActiveBoard(), &display.getError());
}

/** reportError with nullptr must not crash. */
TEST_F(StatusReporterTest, ReportError_NullMessage_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportError(nullptr));
}

/** clearStatus must not crash. */
TEST_F(StatusReporterTest, ClearStatus_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.clearStatus());
}

/** clearStatus must mark LoadingBoard as complete (expired). */
TEST_F(StatusReporterTest, ClearStatus_MarksLoadingComplete) {
    display.reportStatus("Loading...", 0.5f);
    display.clearStatus();
    EXPECT_TRUE(display.getLoading().isExpired());
}

/** After reportStatus then reportError, error board should be active. */
TEST_F(StatusReporterTest, ReportError_OverridesLoadingBoard) {
    display.reportStatus("Loading...", 0.5f);
    EXPECT_EQ(display.getActiveBoard(), &display.getLoading());

    display.reportError("Something failed");
    EXPECT_EQ(display.getActiveBoard(), &display.getError());
}
