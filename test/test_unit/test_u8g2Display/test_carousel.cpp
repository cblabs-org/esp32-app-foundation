/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2Display/test_carousel.cpp
 * Description: GoogleTest unit-test suite for the U8G2DisplayModule carousel
 *              engine. Validates runtime collection (add/remove/clear/contains),
 *              visibleBoard state, expired board eviction, carousel wrap-around,
 *              show() overrides, canAdvance() bounded-hold defer/force-rotation,
 *              nextBoard()/previousBoard() manual navigation, rotation timing,
 *              suspend/resume, and lifecycle hook (onActivate/onDeactivate)
 *              delivery on board transitions.
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
    bool advanceAllowed = true;     ///< Controls canAdvance() return value
    int renderCount = 0;            ///< Tracks how many times render() is called
    int activateCount = 0;          ///< Tracks how many times onActivate() is called
    int deactivateCount = 0;        ///< Tracks how many times onDeactivate() is called
    const char* name = "MockBoard"; ///< Board name for diagnostics

    /** @brief Render stub — increments counter. */
    void render(U8G2& /*u8g2*/) override {
        renderCount++;
    }

    /** @brief Return the expiry state configured by the test. */
    bool isExpired() const override {
        return expired;
    }

    /** @brief Return the hold state configured by the test. */
    bool canAdvance() const override {
        return advanceAllowed;
    }

    /** @brief Return the board name. */
    const char* getBoardName() const override {
        return name;
    }

    /** @brief Track activation lifecycle. */
    void onActivate() override { activateCount++; }

    /** @brief Track deactivation lifecycle. */
    void onDeactivate() override { deactivateCount++; }
};

/**
 * @brief Minimal IDisplayBoard mock that deliberately does NOT override
 *        canAdvance(), used to prove the IDisplayBoard base-class default
 *        (rotates freely) is unaffected by the hold feature.
 */
class DefaultAdvanceMockBoard : public IDisplayBoard {
public:
    void render(U8G2& /*u8g2*/) override {}
    bool isExpired() const override { return false; }
    const char* getBoardName() const override { return "DefaultAdvanceMockBoard"; }
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

// =============================================================================
// Test Group: canAdvance() bounded hold
// =============================================================================

class CarouselCanAdvanceTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** While the active board's canAdvance() returns false, rotation is deferred. */
TEST_F(CarouselCanAdvanceTest, Hold_CanAdvanceFalse_DefersRotation) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    boardA.advanceAllowed = false;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);

    display.tick(1000);  // interval elapsed, but boardA holds
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);
}

/** Once the held board's canAdvance() returns true, rotation proceeds. */
TEST_F(CarouselCanAdvanceTest, Hold_Released_Rotates) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    boardA.advanceAllowed = false;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);

    display.tick(1000);  // holds
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);

    boardA.advanceAllowed = true;
    display.tick(1500);  // hold released — rotates on next tick
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

/** A board that never releases its hold is forced to rotate once _maxHoldMs is exceeded. */
TEST_F(CarouselCanAdvanceTest, Hold_ExceedsMaxHold_ForcesRotation) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    boardA.advanceAllowed = false;  // never releases
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);
    display.carousel.setMaxHoldMs(500);

    display.tick(1000);  // held for interval only — within bound (<= 1500)
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);

    display.tick(1600);  // held 1600ms > interval(1000) + maxHold(500) — forced rotation
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

/** Expired boards are still evicted from the pool while the active board holds. */
TEST_F(CarouselCanAdvanceTest, Hold_ExpiredBoardStillEvictedDuringHold) {
    CarouselMockBoard boardA;  // active, holding
    CarouselMockBoard boardB;  // will expire
    boardA.advanceAllowed = false;
    boardB.expired = true;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);

    display.tick(1000);  // boardA holds; boardB must still be evicted

    EXPECT_EQ(display.carousel.boardCount(), 1);
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);
}

/** A board that does not override canAdvance() rotates exactly as before. */
TEST_F(CarouselCanAdvanceTest, DefaultBoard_NoOverride_RotatesNormally) {
    DefaultAdvanceMockBoard boardA;
    DefaultAdvanceMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);

    display.tick(1000);
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

// =============================================================================
// Test Group: Rotation timing (pre-existing coverage gap)
// =============================================================================

class CarouselRotationTimingTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** Ticking before interval has elapsed must NOT advance the carousel. */
TEST_F(CarouselRotationTimingTest, Tick_IntervalNotElapsed_NoRotation) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(5000);

    display.tick(4999);  // 1 ms short
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);
}

/** Ticking at or past the interval must advance the carousel. */
TEST_F(CarouselRotationTimingTest, Tick_IntervalElapsed_Rotates) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(5000);

    display.tick(5000);  // exactly at interval
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

// =============================================================================
// Test Group: Suspend / Resume (pre-existing coverage gap)
// =============================================================================

class CarouselSuspendResumeTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** suspend() must prevent rotation even when interval has elapsed. */
TEST_F(CarouselSuspendResumeTest, Suspend_PreventsRotation) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);
    display.carousel.suspend();

    display.tick(5000);  // well past interval
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);
}

/** resume() after suspend must allow rotation to proceed. */
TEST_F(CarouselSuspendResumeTest, Resume_AllowsRotation) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);
    display.carousel.suspend();

    display.tick(2000);  // frozen
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);

    display.carousel.resume();
    display.tick(3000);  // resumed, interval elapsed since lastRotation=0
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

// =============================================================================
// Test Group: nextBoard() / previousBoard() manual navigation
// =============================================================================

class CarouselNavigationTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** nextBoard() must advance to the next board. */
TEST_F(CarouselNavigationTest, NextBoard_AdvancesToNext) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    display.carousel.nextBoard(0);
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

/** nextBoard() at the last board must wrap to the first. */
TEST_F(CarouselNavigationTest, NextBoard_WrapsAround) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    display.carousel.nextBoard(0);  // → boardB
    display.carousel.nextBoard(0);  // → boardA (wrap)
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);
}

/** previousBoard() must return to the previous board. */
TEST_F(CarouselNavigationTest, PreviousBoard_ReturnsToPrevious) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    CarouselMockBoard boardC;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.add(&boardC);

    display.carousel.nextBoard(0);  // → boardB
    display.carousel.previousBoard(0);  // → boardA
    EXPECT_EQ(display.carousel.currentBoard(), &boardA);
}

/** previousBoard() at the first board must wrap to the last. */
TEST_F(CarouselNavigationTest, PreviousBoard_WrapsAround) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    CarouselMockBoard boardC;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.add(&boardC);

    display.carousel.previousBoard(0);  // → boardC (wrap)
    EXPECT_EQ(display.carousel.currentBoard(), &boardC);
}

/** nextBoard() must bypass a canAdvance() hold on the active board. */
TEST_F(CarouselNavigationTest, NextBoard_BypassesCanAdvanceHold) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    boardA.advanceAllowed = false;  // holding
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    display.carousel.nextBoard(0);  // must still advance
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);
}

/** nextBoard() must reset the rotation timer so full interval elapses before next auto-rotation. */
TEST_F(CarouselNavigationTest, NextBoard_ResetsRotationTimer) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    CarouselMockBoard boardC;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.add(&boardC);
    display.carousel.setInterval(1000);

    display.carousel.nextBoard(500);  // manual advance at t=500
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);

    display.tick(1400);  // only 900ms since manual advance — should NOT auto-rotate
    EXPECT_EQ(display.carousel.currentBoard(), &boardB);

    display.tick(1500);  // 1000ms since manual advance — should auto-rotate
    EXPECT_EQ(display.carousel.currentBoard(), &boardC);
}

// =============================================================================
// Test Group: Lifecycle hooks (onActivate / onDeactivate)
// =============================================================================

class CarouselLifecycleTest : public ::testing::Test {
protected:
    U8G2DisplayModule display;
};

/** Auto-rotation via _advance() must fire onDeactivate/onActivate. */
TEST_F(CarouselLifecycleTest, Advance_CallsOnDeactivateAndOnActivate) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);
    display.carousel.setInterval(1000);

    display.tick(1000);  // boardA → boardB

    EXPECT_EQ(boardA.deactivateCount, 1);
    EXPECT_EQ(boardB.activateCount, 1);
}

/** nextBoard() must fire lifecycle hooks. */
TEST_F(CarouselLifecycleTest, NextBoard_CallsLifecycleHooks) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    display.carousel.nextBoard(0);

    EXPECT_EQ(boardA.deactivateCount, 1);
    EXPECT_EQ(boardB.activateCount, 1);
}

/** show() to a different board must fire lifecycle hooks. */
TEST_F(CarouselLifecycleTest, Show_CallsLifecycleHooks) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    display.carousel.show(&boardB, 0);

    EXPECT_EQ(boardA.deactivateCount, 1);
    EXPECT_EQ(boardB.activateCount, 1);
}

/** show() with the already-active board must NOT fire lifecycle hooks. */
TEST_F(CarouselLifecycleTest, Show_SameBoard_NoLifecycleHooks) {
    CarouselMockBoard boardA;
    CarouselMockBoard boardB;
    display.carousel.add(&boardA);
    display.carousel.add(&boardB);

    display.carousel.show(&boardA, 0);  // already active

    EXPECT_EQ(boardA.activateCount, 0);
    EXPECT_EQ(boardA.deactivateCount, 0);
}
