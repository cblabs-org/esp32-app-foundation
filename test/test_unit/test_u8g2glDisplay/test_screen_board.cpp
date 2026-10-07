/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_u8g2glDisplay/test_screen_board.cpp
 * Description: U8G2GLScreenBoard is generic over a u8g2gl_screen, so it is tested
 *              with hand-made screens whose lifecycle functions count their calls.
 *              Covers the carousel contract (activate, deactivate, tick, render,
 *              name, expiry) and the two things the board owes the legacy
 *              display: the active screen is released only if it is still this
 *              board's, and the font mode the runtime leaves changed is put back.
 *
 * Exported Functions:
 * - TEST_F(ScreenBoardTest, ...): lifecycle and state-restoration tests
 */

#include <gtest/gtest.h>

#include <NativeU8G2DisplayType.h>
#include <u8g2gl/u8g2gl_core.h>

#include "U8G2GLScreenBoard.hpp"

namespace {

/** @brief Call counters for one fake screen. */
struct Counters {
    int activate = 0;                ///< Times the screen's activate_fn ran
    int deactivate = 0;              ///< Times the screen's deactivate_fn ran
    int tick = 0;                    ///< Times the screen's tick_fn ran
    int render = 0;                  ///< Times the screen's render_fn ran
    uint32_t lastTickMs = 0;         ///< The time passed to the most recent tick_fn
    int fontModeDuringRender = -1;   ///< is_transparent as seen inside render_fn; -1 if never rendered
};

Counters g_a;   ///< Counters of screen A
Counters g_b;   ///< Counters of screen B

/** @brief Screen A's activate_fn: counts the call. */
void activateA() { ++g_a.activate; }
/** @brief Screen A's deactivate_fn: counts the call. */
void deactivateA() { ++g_a.deactivate; }
/** @brief Screen A's tick_fn: counts the call, records the time, reports no deadline. */
uint32_t tickA(uint32_t now) { ++g_a.tick; g_a.lastTickMs = now; return U8G2GL_IDLE; }
/** @brief Draws nothing but behaves like a runtime label: sets the font mode to solid and leaves it. */
void renderA(U8G2& display) {
    ++g_a.render;
    display.setFontMode(0);
    g_a.fontModeDuringRender = display.getU8g2()->font_decode.is_transparent;
}
/** @brief Screen B's activate_fn: counts the call. */
void activateB() { ++g_b.activate; }
/** @brief Screen B's deactivate_fn: counts the call. */
void deactivateB() { ++g_b.deactivate; }

/** @brief A screen with no objects and the given lifecycle functions. */
u8g2gl_screen makeScreen(void (*activate)(), void (*deactivate)(),
                         uint32_t (*tick)(uint32_t), void (*render)(U8G2&)) {
    return u8g2gl_screen{nullptr, nullptr, 0, nullptr, activate, deactivate, tick, render, false};
}

}  // namespace

/** Fixture: no screen active and zeroed counters at the start of every test. */
class ScreenBoardTest : public ::testing::Test {
protected:
    void SetUp() override {
        u8g2gl_set_active_screen(nullptr);
        g_a = Counters{};
        g_b = Counters{};
    }
    void TearDown() override { u8g2gl_set_active_screen(nullptr); }
};

/** The name given at construction is the board name; the board never expires. */
TEST_F(ScreenBoardTest, NameAndExpiry) {
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "TEST: Screen");
    EXPECT_STREQ(board.getBoardName(), "TEST: Screen");
    EXPECT_FALSE(board.isExpired());
}

/** onActivate() makes the screen the runtime's active one and runs its activate_fn exactly once. */
TEST_F(ScreenBoardTest, OnActivateMakesTheScreenActive) {
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "A");
    board.onActivate();
    EXPECT_EQ(u8g2gl_get_active_screen(), &screen);
    EXPECT_EQ(g_a.activate, 1);   // Once: u8g2gl_set_active_screen() already calls it
    EXPECT_TRUE(screen.dirty);    // The new screen is marked dirty so it paints at once
}

/** onDeactivate() releases the screen and runs its deactivate_fn exactly once. */
TEST_F(ScreenBoardTest, OnDeactivateReleasesTheScreen) {
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "A");
    board.onActivate();
    board.onDeactivate();
    EXPECT_EQ(u8g2gl_get_active_screen(), nullptr);
    EXPECT_EQ(g_a.deactivate, 1);
}

/** If another screen has already taken over, a late onDeactivate() leaves it alone. */
TEST_F(ScreenBoardTest, OnDeactivateDoesNotCancelASuccessor) {
    u8g2gl_screen screenA = makeScreen(activateA, deactivateA, tickA, renderA);
    u8g2gl_screen screenB = makeScreen(activateB, deactivateB, nullptr, nullptr);
    U8G2GLScreenBoard boardA(screenA, "A");
    U8G2GLScreenBoard boardB(screenB, "B");

    boardA.onActivate();
    boardB.onActivate();                 // The runtime deactivates A and activates B
    EXPECT_EQ(g_a.deactivate, 1);
    EXPECT_EQ(g_b.activate, 1);

    boardA.onDeactivate();               // Late: B is already active
    EXPECT_EQ(u8g2gl_get_active_screen(), &screenB);
    EXPECT_EQ(g_a.deactivate, 1);        // Not run a second time
    EXPECT_EQ(g_b.deactivate, 0);        // B was not cancelled
}

/** onDeactivate() on a board that was never activated does nothing. */
TEST_F(ScreenBoardTest, OnDeactivateWithoutActivateIsHarmless) {
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "A");
    board.onDeactivate();
    EXPECT_EQ(g_a.deactivate, 0);
    EXPECT_EQ(u8g2gl_get_active_screen(), nullptr);
}

/** tick() reaches the screen's tick_fn with the carousel's time; a screen without one is fine. */
TEST_F(ScreenBoardTest, TickIsForwardedToTheScreen) {
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "A");
    board.tick(4242);
    EXPECT_EQ(g_a.tick, 1);
    EXPECT_EQ(g_a.lastTickMs, 4242u);

    u8g2gl_screen bare = makeScreen(nullptr, nullptr, nullptr, nullptr);
    U8G2GLScreenBoard bareBoard(bare, "bare");
    EXPECT_NO_FATAL_FAILURE(bareBoard.tick(1));
}

/** render() calls only the screen's render_fn; a screen without one draws nothing. */
TEST_F(ScreenBoardTest, RenderCallsTheScreen) {
    NativeU8G2Display display(U8G2_R0);
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "A");
    board.render(display);
    EXPECT_EQ(g_a.render, 1);

    u8g2gl_screen bare = makeScreen(nullptr, nullptr, nullptr, nullptr);
    U8G2GLScreenBoard bareBoard(bare, "bare");
    EXPECT_NO_FATAL_FAILURE(bareBoard.render(display));
}

/** The runtime's labels leave the font mode solid; the board puts the legacy transparent mode back. */
TEST_F(ScreenBoardTest, RenderRestoresTheFontMode) {
    NativeU8G2Display display(U8G2_R0);
    display.setFontMode(1);   // What the legacy module sets once at begin()
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "A");

    board.render(display);

    EXPECT_EQ(g_a.fontModeDuringRender, 0);                         // The screen did change it ...
    EXPECT_EQ(display.getU8g2()->font_decode.is_transparent, 1);    // ... and the board restored it
}

/** A mode that was solid before stays solid: the board restores, it does not impose. */
TEST_F(ScreenBoardTest, RenderRestoresWhateverModeWasSet) {
    NativeU8G2Display display(U8G2_R0);
    display.setFontMode(0);
    u8g2gl_screen screen = makeScreen(activateA, deactivateA, tickA, renderA);
    U8G2GLScreenBoard board(screen, "A");
    board.render(display);
    EXPECT_EQ(display.getU8g2()->font_decode.is_transparent, 0);
}
