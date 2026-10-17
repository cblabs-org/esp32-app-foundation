/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2GLDisplay/U8G2GLScreenBoard.hpp
 * Description: A generic IDisplayBoard that runs one generated U8G2GL screen
 *              inside the legacy carousel. This is the "board host" of
 *              ruling R1 of U8G2GL PR-05 (see
 *              .agents/skills/principal-architect-workspace/pr-u8g2gl-05/PROJECT.md):
 *              the legacy module keeps the display, and this class adapts the
 *              carousel's calls to the screen's own lifecycle functions and
 *              draws into that display.
 *
 *              It never calls ui_handler(). ui_handler() clears the buffer and
 *              sends it, and the legacy module does both itself around every
 *              board (U8G2Display.cpp:1413-1415); a second clear would wipe
 *              what the carousel has drawn and a second send would double the
 *              SPI traffic. The board calls the screen's tick_fn and render_fn
 *              directly instead.
 *
 *              Each concrete board is a few lines: a screen, a name, and
 *              whatever feeds the screen's subjects (see
 *              boards/DiagnosticsScreenBoard.hpp).
 *
 * Exported Classes:
 * - U8G2GLScreenBoard: IDisplayBoard over a u8g2gl_screen
 *
 *   Own API:
 *   - U8G2GLScreenBoard(): Bind a generated screen and a board name
 *
 *   IDisplayBoard:
 *   - render():       Draw the screen into the legacy display, restoring the font mode
 *   - tick():         Tick the screen (animation deadlines)
 *   - isExpired():    Always false (a permanent carousel board)
 *   - getBoardName(): The name given at construction
 *   - onActivate():   Make the screen the runtime's active screen
 *   - onDeactivate(): Release the active screen if it is this one
 *
 *   Protected:
 *   - screen():       The bound screen, for a derived board to reach its tick or render
 */

#pragma once

#include <cstdint>

#include "IDisplayBoard.hpp"

// Forward-declared, not included: the generated config header (capacities counted
// from the XML) must be seen before any runtime header in a translation unit
// that includes both, and this header is included by code that has not yet
// included the generated ui.hpp. The full definition is in <u8g2gl/u8g2gl_core.h>.
struct u8g2gl_screen;

/**
 * @brief An IDisplayBoard that renders one generated U8G2GL screen.
 *
 * The screen is the generated `ui_screen_<name>` descriptor. Generated screens
 * are statically allocated, so the reference stays valid for the life of the
 * program.
 */
class U8G2GLScreenBoard : public IDisplayBoard {
public:
    /**
     * @brief Bind a generated screen to a carousel board.
     * @param screen    The generated screen descriptor, e.g. ui_screen_diagnostics.
     * @param boardName Name for logging and the carousel's board list. Must
     *                  outlive the board; pass a string literal.
     */
    U8G2GLScreenBoard(u8g2gl_screen& screen, const char* boardName);

    /**
     * @brief Draw the screen into the legacy module's display.
     *
     * Calls the screen's render_fn only. The carousel has already cleared the
     * buffer and will send it afterwards. The font mode is restored: the
     * runtime's labels set it and do not put it back, and the legacy module
     * sets it once and relies on it for every other board. The one other
     * change is that a font is set if none was (see the .cpp), which is
     * harmless because every label sets its own.
     *
     * @param u8g2 The legacy module's U8G2 instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Tick the screen so its animations can advance.
     *
     * Calls the screen's tick_fn. Its return value, the soonest deadline, is
     * ignored: the legacy carousel redraws every frame regardless.
     *
     * @param currentMillis Current system time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /** @brief Always returns false: a screen board stays in the carousel. */
    bool isExpired() const override;

    /** @brief Returns the board name given at construction. */
    const char* getBoardName() const override;

    /**
     * @brief The carousel switched to this board: make its screen active.
     *
     * u8g2gl_set_active_screen() runs the screen's activate_fn itself, so this
     * does not call it a second time. Being active is what lets a subject
     * write mark this screen dirty.
     */
    void onActivate() override;

    /**
     * @brief The carousel switched away: release the active screen.
     *
     * Only if it is still this board's screen, so a board that was already
     * replaced does not cancel its successor. u8g2gl_set_active_screen() runs
     * the screen's deactivate_fn.
     */
    void onDeactivate() override;

protected:
    /** @brief The bound screen, for a derived board to reach its tick or render. */
    u8g2gl_screen& screen() { return _screen; }

private:
    u8g2gl_screen& _screen;   ///< The generated screen this board runs
    const char* _boardName;   ///< Name for logging and the carousel's board list
};
