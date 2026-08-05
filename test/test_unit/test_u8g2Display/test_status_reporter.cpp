/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2Display/test_status_reporter.cpp
 * Description: GoogleTest unit-test suite for the U8G2DisplayModule's
 *              IStatusReporter implementation. Validates that reportStatus()
 *              delegates to BootBoard, reportError() delegates to ErrorBoard,
 *              and clearStatus() marks BootBoard as complete.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "U8G2DisplayModule.hpp"

// =============================================================================
// Test Group: IStatusReporter delegation
// =============================================================================

class StatusReporterTest : public ::testing::Test {
protected:
    U8G2DisplayModule display; ///< Fresh module instance for each test
};

/** reportStatus must not crash and must delegate to BootBoard. */
TEST_F(StatusReporterTest, ReportStatus_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportStatus("Booting...", 0.5f));
}

/** reportStatus must show the boot board as active. */
TEST_F(StatusReporterTest, ReportStatus_ShowsBootBoard) {
    display.reportStatus("Connecting...", 0.3f);
    EXPECT_NE(display.visibleBoard(), nullptr);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: Boot");
}

/** reportStatus with indeterminate progress must work. */
TEST_F(StatusReporterTest, ReportStatus_IndeterminateProgress) {
    EXPECT_NO_FATAL_FAILURE(display.reportStatus("Working...", -1.0f));
    EXPECT_NE(display.visibleBoard(), nullptr);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: Boot");
}

/** reportStatus with nullptr message must not crash. */
TEST_F(StatusReporterTest, ReportStatus_NullMessage_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportStatus(nullptr, 0.0f));
}

/** Multiple reportStatus calls must keep the boot board active. */
TEST_F(StatusReporterTest, ReportStatus_MultipleCalls_KeepsBootBoardActive) {
    display.reportStatus("Step 1", 0.25f);
    display.reportStatus("Step 2", 0.50f);
    display.reportStatus("Step 3", 0.75f);
    EXPECT_NE(display.visibleBoard(), nullptr);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: Boot");
}

/** reportError must not crash. */
TEST_F(StatusReporterTest, ReportError_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportError("Fatal error"));
}

/** reportError must show the error board as active. */
TEST_F(StatusReporterTest, ReportError_ShowsErrorBoard) {
    display.reportError("Critical failure");
    EXPECT_EQ(display.visibleBoard(), &display.boards.error);
}

/** reportError with nullptr must not crash. */
TEST_F(StatusReporterTest, ReportError_NullMessage_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportError(nullptr));
}

/** clearStatus must not crash. */
TEST_F(StatusReporterTest, ClearStatus_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.clearStatus());
}

/** clearStatus must mark BootBoard as complete (expired) and remove it from modal layer. */
TEST_F(StatusReporterTest, ClearStatus_MarksBootComplete) {
    display.reportStatus("Loading...", 0.5f);
    display.clearStatus();
    EXPECT_EQ(display.visibleBoard(), nullptr); // Or whatever the base board is, but not BootBoard
}

/** After reportStatus then reportError, error board should be active. */
TEST_F(StatusReporterTest, ReportError_OverridesBootBoard) {
    display.reportStatus("Loading...", 0.5f);
    EXPECT_NE(display.visibleBoard(), nullptr);
    EXPECT_STREQ(display.visibleBoard()->getBoardName(), "SYS: Boot");

    display.reportError("Something failed");
    EXPECT_EQ(display.visibleBoard(), &display.boards.error);
}
