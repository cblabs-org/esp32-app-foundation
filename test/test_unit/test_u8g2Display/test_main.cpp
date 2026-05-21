/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_main.cpp
 * Description: GoogleTest unit-test suite for the U8g2DisplayModule skeleton.
 *              Validates the board slot pool (add/count), carousel state,
 *              showBoard behaviour, and IStatusReporter / iConfigurable stubs.
 *
 *              All tests execute on the native GoogleTest runner (no hardware
 *              required). NATIVE_BUILD is defined by the [env:native] environment
 *              in platformio.ini.
 *
 *              NOTE: begin(Application&) requires a full Application instance
 *              which pulls in Arduino.h via webServer — a pre-existing native
 *              build limitation. Lock-after-begin() is therefore NOT tested here;
 *              it will be covered in hardware integration tests (WO4+).
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all test suites
 */

#include <gtest/gtest.h>
#include "u8g2DisplayModule.hpp"

// =============================================================================
// Mock IDisplayBoard — minimal concrete implementation for testing
// =============================================================================

/** @brief Minimal IDisplayBoard mock for unit testing the slot pool and carousel. */
class MockDisplayBoard : public IDisplayBoard {
public:
    bool expired = false;          ///< Controls isExpired() return value
    int renderCount = 0;           ///< Tracks how many times render() is called
    const char* name = "MockBoard"; ///< Board name for diagnostics

    /**
     * @brief Render stub — increments counter (no actual U8G2 drawing in native).
     * @param u8g2 Reference to the U8G2 driver (unused in native build).
     */
    void render(U8G2& /*u8g2*/) override {
        renderCount++;
    }

    /**
     * @brief Return the expiry state configured by the test.
     * @return true if this board should be evicted from the pool.
     */
    bool isExpired() const override {
        return expired;
    }

    /**
     * @brief Return the board name for logging.
     * @return Null-terminated string identifier.
     */
    const char* getBoardName() const override {
        return name;
    }
};

// =============================================================================
// Test Group 1: Board Slot Pool (pre-lock)
// =============================================================================

class U8g2DisplaySlotPoolTest : public ::testing::Test {
protected:
    U8g2DisplayModule display; ///< Fresh module instance for each test
};

/** A freshly constructed module must have zero boards. */
TEST_F(U8g2DisplaySlotPoolTest, InitialBoardCount_IsZero) {
    EXPECT_EQ(display.getBoardCount(), 0);
}

/** addBoard with a valid pointer must return true and increment count. */
TEST_F(U8g2DisplaySlotPoolTest, AddBoard_ValidPointer_ReturnsTrue) {
    MockDisplayBoard board;
    EXPECT_TRUE(display.addBoard(&board));
    EXPECT_EQ(display.getBoardCount(), 1);
}

/** addBoard with nullptr must return false and not change count. */
TEST_F(U8g2DisplaySlotPoolTest, AddBoard_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(display.addBoard(nullptr));
    EXPECT_EQ(display.getBoardCount(), 0);
}

/** Adding multiple boards must increment count correctly. */
TEST_F(U8g2DisplaySlotPoolTest, AddBoard_Multiple_IncrementCount) {
    MockDisplayBoard boardA;
    MockDisplayBoard boardB;
    MockDisplayBoard boardC;
    EXPECT_TRUE(display.addBoard(&boardA));
    EXPECT_TRUE(display.addBoard(&boardB));
    EXPECT_TRUE(display.addBoard(&boardC));
    EXPECT_EQ(display.getBoardCount(), 3);
}

/** getActiveBoard with no boards must return nullptr. */
TEST_F(U8g2DisplaySlotPoolTest, GetActiveBoard_EmptyPool_ReturnsNullptr) {
    EXPECT_EQ(display.getActiveBoard(), nullptr);
}

/** After adding one board, getActiveBoard must return that board. */
TEST_F(U8g2DisplaySlotPoolTest, GetActiveBoard_OneBoard_ReturnsThatBoard) {
    MockDisplayBoard board;
    display.addBoard(&board);
    EXPECT_EQ(display.getActiveBoard(), &board);
}

// =============================================================================
// Test Group 2: IStatusReporter stubs
// =============================================================================

class U8g2DisplayStatusReporterTest : public ::testing::Test {
protected:
    U8g2DisplayModule display;
};

/** reportStatus stub must not crash. */
TEST_F(U8g2DisplayStatusReporterTest, ReportStatus_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportStatus("Booting...", 0.5f));
}

/** reportError stub must not crash. */
TEST_F(U8g2DisplayStatusReporterTest, ReportError_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.reportError("Fatal error"));
}

/** clearStatus stub must not crash. */
TEST_F(U8g2DisplayStatusReporterTest, ClearStatus_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.clearStatus());
}

// =============================================================================
// Test Group 3: iConfigurable stubs
// =============================================================================

class U8g2DisplayConfigurableTest : public ::testing::Test {
protected:
    U8g2DisplayModule display;
};

/** loadConfig stub must return true. */
TEST_F(U8g2DisplayConfigurableTest, LoadConfig_ReturnsTrue) {
    EXPECT_TRUE(display.loadConfig());
}

/** saveConfig stub must return true. */
TEST_F(U8g2DisplayConfigurableTest, SaveConfig_ReturnsTrue) {
    EXPECT_TRUE(display.saveConfig());
}

/** resetConfig stub must not crash. */
TEST_F(U8g2DisplayConfigurableTest, ResetConfig_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.resetConfig());
}

// =============================================================================
// Test Group 4: showBoard
// =============================================================================

class U8g2DisplayShowBoardTest : public ::testing::Test {
protected:
    U8g2DisplayModule display;
};

/** showBoard with a pool board must update getActiveBoard. */
TEST_F(U8g2DisplayShowBoardTest, ShowBoard_PoolBoard_UpdatesActiveBoard) {
    MockDisplayBoard boardA;
    MockDisplayBoard boardB;
    display.addBoard(&boardA);
    display.addBoard(&boardB);

    display.showBoard(&boardB);
    EXPECT_EQ(display.getActiveBoard(), &boardB);
}

/** showBoard with a non-pool board must set override active board. */
TEST_F(U8g2DisplayShowBoardTest, ShowBoard_NonPoolBoard_SetsOverride) {
    MockDisplayBoard poolBoard;
    MockDisplayBoard systemBoard;
    display.addBoard(&poolBoard);

    display.showBoard(&systemBoard);
    EXPECT_EQ(display.getActiveBoard(), &systemBoard);
}

// =============================================================================
// Test Group 5: Tick behaviour (native — no hardware rendering)
// =============================================================================

class U8g2DisplayTickTest : public ::testing::Test {
protected:
    U8g2DisplayModule display;
};

/** tick() with no boards must not crash. */
TEST_F(U8g2DisplayTickTest, Tick_EmptyPool_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.tick());
}

/** tick() with boards must not crash in native build. */
TEST_F(U8g2DisplayTickTest, Tick_WithBoards_DoesNotCrash) {
    MockDisplayBoard board;
    display.addBoard(&board);
    EXPECT_NO_FATAL_FAILURE(display.tick());
}

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
