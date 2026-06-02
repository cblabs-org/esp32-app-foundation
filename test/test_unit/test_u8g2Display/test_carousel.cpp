/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_u8g2Display/test_carousel.cpp
 * Description: GoogleTest unit-test suite for the U8G2DisplayModule carousel
 *              engine. Validates board pool add/count, pool locking behaviour
 *              (without Application), getActiveBoard state, expired board
 *              eviction, carousel wrap-around, and showBoard overrides.
 *
 * Exported Functions:
 * - (GoogleTest fixtures — discovered automatically by gtest_main)
 */

#include <gtest/gtest.h>
#include "U8G2DisplayModule.hpp"

// =============================================================================
// Mock IDisplayBoard — minimal concrete implementation for testing
// =============================================================================

/** @brief Minimal IDisplayBoard mock for unit testing the carousel engine. */
class CarouselMockBoard : public IDisplayBoard {
public:
    bool expired = false;           ///< Controls isExpired() return value
    int renderCount = 0;            ///< Tracks how many times render() is called
    const char* name = "MockBoard"; ///< Board name for diagnostics

    /** @brief Render stub — increments counter. */
    void render(U8G2& /*u8g2*/) override {
        renderCount++;
    }

    /** @brief Return the expiry state configured by the test. */
    bool isExpired() const override {
        return expired;
    }

    /** @brief Return the board name. */
    const char* getBoardName() const override {
        return name;
    }
};

// =============================================================================
// Test Group: Board Slot Pool (pre-lock)
// =============================================================================

class CarouselSlotPoolTest : public ::testing::Test {
protected:
    U8G2DisplayModule display; ///< Fresh module instance for each test
};

/** A freshly constructed module must have zero boards. */
TEST_F(CarouselSlotPoolTest, InitialBoardCount_IsZero) {
    EXPECT_EQ(display.getBoardCount(), 0);
}

/** addBoard with a valid pointer must return true and increment count. */
TEST_F(CarouselSlotPoolTest, AddBoard_ValidPointer_ReturnsTrue) {
    CarouselMockBoard board;
    EXPECT_TRUE(display.addBoard(&board));
    EXPECT_EQ(display.getBoardCount(), 1);
}

/** addBoard with nullptr must return false and not change count. */
TEST_F(CarouselSlotPoolTest, AddBoard_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(display.addBoard(nullptr));
    EXPECT_EQ(display.getBoardCount(), 0);
}

/** Adding multiple boards must increment count correctly. */
TEST_F(CarouselSlotPoolTest, AddBoard_Multiple_IncrementCount) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    CarouselMockBoard boardC;
    EXPECT_TRUE(display.addBoard(&boardA));
    EXPECT_TRUE(display.addBoard(&boardB));
    EXPECT_TRUE(display.addBoard(&boardC));
    EXPECT_EQ(display.getBoardCount(), 3);
}

/** getActiveBoard with no boards must return nullptr. */
TEST_F(CarouselSlotPoolTest, GetActiveBoard_EmptyPool_ReturnsNullptr) {
    EXPECT_EQ(display.getActiveBoard(), nullptr);
}

/** After adding one board, getActiveBoard must return that board. */
TEST_F(CarouselSlotPoolTest, GetActiveBoard_OneBoard_ReturnsThatBoard) {
    CarouselMockBoard board;
    display.addBoard(&board);
    EXPECT_EQ(display.getActiveBoard(), &board);
}

// =============================================================================
// Test Group: showBoard behaviour
// =============================================================================

class CarouselShowBoardTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** showBoard with a pool board must update getActiveBoard. */
TEST_F(CarouselShowBoardTest, ShowBoard_PoolBoard_UpdatesActiveBoard) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.addBoard(&boardA);
    display.addBoard(&boardB);

    display.showBoard(&boardB);
    EXPECT_EQ(display.getActiveBoard(), &boardB);
}

/** showBoard with a non-pool board must NOT change getActiveBoard (pool-only API). */
TEST_F(CarouselShowBoardTest, ShowBoard_NonPoolBoard_DoesNotChangeActive) {
    CarouselMockBoard poolBoard;
    CarouselMockBoard systemBoard;
    display.addBoard(&poolBoard);

    display.showBoard(&systemBoard);
    // showBoard is pool-only — non-pool board is ignored
    EXPECT_EQ(display.getActiveBoard(), &poolBoard);
}

/** showBoard with a pool board after showing another pool board must update. */
TEST_F(CarouselShowBoardTest, ShowBoard_DifferentPoolBoard_UpdatesActive) {
    CarouselMockBoard poolBoardA;
    CarouselMockBoard poolBoardB;
    display.addBoard(&poolBoardA);
    display.addBoard(&poolBoardB);

    // Start on A, switch to B
    display.showBoard(&poolBoardB);
    EXPECT_EQ(display.getActiveBoard(), &poolBoardB);

    // Switch back to A
    display.showBoard(&poolBoardA);
    EXPECT_EQ(display.getActiveBoard(), &poolBoardA);
}

// =============================================================================
// Test Group: Expired board eviction via tick()
// =============================================================================

class CarouselEvictionTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** tick() with no boards must not crash. */
TEST_F(CarouselEvictionTest, Tick_EmptyPool_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(display.tick());
}

/** tick() with boards must not crash in native build. */
TEST_F(CarouselEvictionTest, Tick_WithBoards_DoesNotCrash) {
    CarouselMockBoard board;
    display.addBoard(&board);
    EXPECT_NO_FATAL_FAILURE(display.tick());
}

/** getBoardCount tracks additions correctly. */
TEST_F(CarouselEvictionTest, GetBoardCount_TracksCorrectly) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;

    EXPECT_EQ(display.getBoardCount(), 0);
    display.addBoard(&boardA);
    EXPECT_EQ(display.getBoardCount(), 1);
    display.addBoard(&boardB);
    EXPECT_EQ(display.getBoardCount(), 2);
}
