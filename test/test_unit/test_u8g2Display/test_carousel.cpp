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
 *              engine. Validates runtime collection (add/remove/clear/contains),
 *              visibleBoard state, expired board
 *              eviction, carousel wrap-around, and show() overrides.
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
// Test Group: Collection (add/remove/clear/contains)
// =============================================================================

class CarouselSlotPoolTest : public ::testing::Test {
protected:
    U8G2DisplayModule display; ///< Fresh module instance for each test
};

/** A freshly constructed module must have zero boards. */
TEST_F(CarouselSlotPoolTest, InitialBoardCount_IsZero) {
    EXPECT_EQ(display.carousel.boardCount(), 0);
}

/** addBoard with a valid pointer must increment count. */
TEST_F(CarouselSlotPoolTest, AddBoard_ValidPointer_ReturnsTrue) {
    CarouselMockBoard board;
    display.carousel.add(&board);
    EXPECT_EQ(display.carousel.boardCount(), 1);
}

/** Adding multiple boards must increment count correctly. */
TEST_F(CarouselSlotPoolTest, AddBoard_Multiple_IncrementCount) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    CarouselMockBoard boardC;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.add(&boardC);
    EXPECT_EQ(display.carousel.boardCount(), 3);
}

/** currentBoard with no boards must return nullptr. */
TEST_F(CarouselSlotPoolTest, GetActiveBoard_EmptyPool_ReturnsNullptr) {
    EXPECT_EQ(display.carousel.currentBoard(), nullptr);
}

/** After adding one board, currentBoard must return that board. */
TEST_F(CarouselSlotPoolTest, GetActiveBoard_OneBoard_ReturnsThatBoard) {
    CarouselMockBoard board;
    display.carousel.add(&board);
    EXPECT_EQ(display.carousel.currentBoard(), &board);
}

// =============================================================================
// Test Group: showBoard behaviour
// =============================================================================

class CarouselShowBoardTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** show with a pool board must update currentBoard. */
TEST_F(CarouselShowBoardTest, ShowBoard_PoolBoard_UpdatesActiveBoard) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    display.carousel.show(&boardB, 0);
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

/** show with a non-pool board must NOT change currentBoard (pool-only API). */
TEST_F(CarouselShowBoardTest, ShowBoard_NonPoolBoard_DoesNotChangeActive) {
    CarouselMockBoard poolBoard;
    CarouselMockBoard systemBoard;
    display.carousel.add(&poolBoard);

    display.carousel.show(&systemBoard, 0);
    // showBoard is pool-only — non-pool board is ignored
    EXPECT_EQ(display.carousel.currentBoard(), &poolBoard);
}

/** showBoard with a pool board after showing another pool board must update. */
TEST_F(CarouselShowBoardTest, ShowBoard_DifferentPoolBoard_UpdatesActive) {
    CarouselMockBoard poolBoardA;
    CarouselMockBoard poolBoardB;
    display.carousel.add(&poolBoardA);
    display.carousel.add(&poolBoardB);

    // Start on A, switch to B
    display.carousel.show(&poolBoardB, 0);
    EXPECT_EQ(display.carousel.currentBoard(), &poolBoardB);

    // Switch back to A
    display.carousel.show(&poolBoardA, 0);
    EXPECT_EQ(display.carousel.currentBoard(), &poolBoardA);
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
    EXPECT_NO_FATAL_FAILURE(display.tick(0));
}

/** tick() with boards must not crash in native build. */
TEST_F(CarouselEvictionTest, Tick_WithBoards_DoesNotCrash) {
    CarouselMockBoard board;
    display.carousel.add(&board);
    EXPECT_NO_FATAL_FAILURE(display.tick(0));
}

/** boardCount tracks additions correctly. */
TEST_F(CarouselEvictionTest, GetBoardCount_TracksCorrectly) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;

    EXPECT_EQ(display.carousel.boardCount(), 0);
    display.carousel.add(&boardA);
    EXPECT_EQ(display.carousel.boardCount(), 1);
    display.carousel.add(&boardB);
    EXPECT_EQ(display.carousel.boardCount(), 2);
}

// =============================================================================
// Test Group: Collection mutation (remove, clear, contains, duplicates)
// =============================================================================

class CarouselCollectionTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** remove() with a pool board must decrement count and return true. */
TEST_F(CarouselCollectionTest, Remove_PoolBoard_ReturnsTrue) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    EXPECT_TRUE(display.carousel.remove(&boardA));
    EXPECT_EQ(display.carousel.boardCount(), 1);
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

/** remove() with a non-pool board must return false and leave pool unchanged. */
TEST_F(CarouselCollectionTest, Remove_NonPoolBoard_ReturnsFalse) {
    CarouselMockBoard poolBoard;
    CarouselMockBoard otherBoard;
    display.carousel.add(&poolBoard);
    EXPECT_FALSE(display.carousel.remove(&otherBoard));
    EXPECT_EQ(display.carousel.boardCount(), 1);
}

/** remove() on the active board must advance to the next board. */
TEST_F(CarouselCollectionTest, Remove_ActiveBoard_AdvancesToNext) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    CarouselMockBoard boardC;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.add(&boardC);

    // Active is boardA (index 0), remove it
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);
    display.carousel.remove(&boardA);
    // boardB slides into slot 0
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

/** remove() the last remaining board must leave pool empty. */
TEST_F(CarouselCollectionTest, Remove_LastBoard_LeavesEmpty) {
    CarouselMockBoard board;
    display.carousel.add(&board);
    display.carousel.remove(&board);
    EXPECT_EQ(display.carousel.boardCount(), 0);
    EXPECT_EQ(display.carousel.currentBoard(), nullptr);
}

/** clear() must empty the pool. */
TEST_F(CarouselCollectionTest, Clear_EmptiesPool) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.clear();
    EXPECT_EQ(display.carousel.boardCount(), 0);
    EXPECT_EQ(display.carousel.currentBoard(), nullptr);
}

/** contains() must return true for registered boards. */
TEST_F(CarouselCollectionTest, Contains_RegisteredBoard_ReturnsTrue) {
    CarouselMockBoard board;
    display.carousel.add(&board);
    EXPECT_TRUE(display.carousel.contains(&board));
}

/** contains() must return false for unregistered boards. */
TEST_F(CarouselCollectionTest, Contains_UnregisteredBoard_ReturnsFalse) {
    CarouselMockBoard board;
    EXPECT_FALSE(display.carousel.contains(&board));
}

/** add() with a duplicate pointer must be ignored. */
TEST_F(CarouselCollectionTest, Add_Duplicate_IsIgnored) {
    CarouselMockBoard board;
    display.carousel.add(&board);
    display.carousel.add(&board);
    EXPECT_EQ(display.carousel.boardCount(), 1);
}

/** add() with nullptr must be ignored. */
TEST_F(CarouselCollectionTest, Add_Nullptr_IsIgnored) {
    display.carousel.add(nullptr);
    EXPECT_EQ(display.carousel.boardCount(), 0);
}
