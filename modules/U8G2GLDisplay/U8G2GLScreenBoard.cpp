/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2GLDisplay/U8G2GLScreenBoard.cpp
 * Description: Implementation of U8G2GLScreenBoard, the generic IDisplayBoard
 *              over a generated U8G2GL screen.
 *
 * Exported Functions:
 * - U8G2GLScreenBoard::U8G2GLScreenBoard(): Bind a screen and a name
 * - U8G2GLScreenBoard::render():            Draw the screen, restoring U8G2 state
 * - U8G2GLScreenBoard::tick():              Tick the screen
 * - U8G2GLScreenBoard::isExpired():         Always false
 * - U8G2GLScreenBoard::getBoardName():      The board name
 * - U8G2GLScreenBoard::onActivate():        Make the screen active
 * - U8G2GLScreenBoard::onDeactivate():      Release the screen
 */

#include "U8G2GLScreenBoard.hpp"

#include <Arduino.h>
#include <U8g2lib.h>
#include <u8g2gl/u8g2gl_core.h>  // u8g2gl_screen, u8g2gl_set_active_screen(), u8g2gl_get_active_screen()

#include "fonts/Fonts.hpp"  // AppFonts::defaultSmall(): a font already linked for the legacy boards

/**
 * @brief Bind a generated screen to a carousel board.
 * @param screen    The generated screen descriptor.
 * @param boardName Name for logging; must outlive the board.
 */
U8G2GLScreenBoard::U8G2GLScreenBoard(u8g2gl_screen& screen, const char* boardName)
    : _screen(screen), _boardName(boardName) {}

/**
 * @brief Draw the screen into the legacy module's display.
 *
 * What the screen inherits from the legacy module, and what it changes, were
 * read from the code and measured with the render harness (test/render; the
 * figures are in section 1 of
 * .agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md):
 *
 *  - INHERITED, left alone. setFontPosTop(), setFontRefHeightAll(),
 *    setDrawColor(1) and setFontMode(1) are set once at U8G2Display.cpp:143-146
 *    and no legacy board changes them. The screen depends on font position and
 *    reference height and the runtime sets neither: it calls font-position-top
 *    "the framework default" but nothing in its src/ sets it. Measured
 *    (test/render/after/diagnostics_board/state_probe.json): without
 *    setFontPosTop() the glyphs are drawn above their y and the labels' clip
 *    removes them: the frame has 15 lit pixels and none is where the 1392
 *    normal ones are; without setFontRefHeightAll()
 *    every row sits 1 pixel higher. The legacy module guarantees both, so the
 *    board does not set them again.
 *  - A FONT MUST BE SET. The runtime's state saver captures whatever font is
 *    current and puts it back after every object, and with none set that is
 *    nullptr, which U8G2 then reads: a null dereference (u8g2_font.c:114, U8G2 2.36.18),
 *    reproduced in the harness. The legacy module does not set one in begin();
 *    the legacy boards' own state saver skips a null font, so they never hit
 *    it. If no font is set yet, one that is already linked for the legacy
 *    boards is set first. It changes no pixel: every label sets its own font.
 *  - CHANGED BY THE RUNTIME, RESTORED HERE. Each label sets draw colour and font
 *    mode on every draw. The state saver puts the colour and font back but not
 *    the font mode, and a style's default is solid, not the legacy module's
 *    transparent. Left alone, every board drawn after this one would render
 *    in solid mode. So the mode is saved and restored.
 *  - The clip window is reset to the full display by the runtime
 *    (u8g2gl_clip_scope), which is the state legacy boards expect.
 *
 * @param u8g2 The legacy module's U8G2 instance.
 */
void U8G2GLScreenBoard::render(U8G2& u8g2) {
    // --- Guard: a screen without a render function draws nothing ---
    if (_screen.render_fn == nullptr) {
        return;
    }

    // --- Step 1: Save the one piece of state the runtime changes and does not restore ---
    // U8G2 has no getter for the font mode; it lives in the font decoder.
    u8g2_t* raw = u8g2.getU8g2();
    const uint8_t savedFontMode = raw->font_decode.is_transparent;

    // --- Step 2: Make sure a font is current, so the state saver has one to restore ---
    if (raw->font == nullptr) {
        u8g2.setFont(AppFonts::defaultSmall());
    }

    // --- Step 3: Draw. The carousel owns clearBuffer() and sendBuffer() ---
    _screen.render_fn(u8g2);

    // --- Step 4: Put the font mode back for the next board ---
    u8g2.setFontMode(savedFontMode);
}

/**
 * @brief Tick the screen so its animations can advance.
 * @param currentMillis Current system time in milliseconds.
 */
void U8G2GLScreenBoard::tick(uint32_t currentMillis) {
    if (_screen.tick_fn != nullptr) {
        _screen.tick_fn(currentMillis);  // Soonest deadline ignored: legacy redraws every frame
    }
}

/** @brief Always returns false: a screen board stays in the carousel. */
bool U8G2GLScreenBoard::isExpired() const {
    return false;
}

/** @brief Returns the board name given at construction. */
const char* U8G2GLScreenBoard::getBoardName() const {
    return _boardName;
}

/**
 * @brief The carousel switched to this board: make its screen active.
 *
 * u8g2gl_set_active_screen() deactivates the outgoing screen, activates this
 * one (calling its activate_fn) and marks it dirty (u8g2gl_core.cpp:71-86), so
 * calling activate_fn here as well would run it twice.
 */
void U8G2GLScreenBoard::onActivate() {
    u8g2gl_set_active_screen(&_screen);
}

/**
 * @brief The carousel switched away: release the active screen.
 *
 * Passing nullptr runs this screen's deactivate_fn and leaves no screen
 * active. Skipped when another screen has already taken over, so a board that
 * is deactivated late does not cancel its successor.
 */
void U8G2GLScreenBoard::onDeactivate() {
    if (u8g2gl_get_active_screen() == &_screen) {
        u8g2gl_set_active_screen(nullptr);
    }
}
