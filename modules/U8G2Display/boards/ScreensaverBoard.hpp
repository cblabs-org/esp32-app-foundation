/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/boards/ScreensaverBoard.hpp
 * Description: Display board for inactivity screensaver mode. Renders a bouncing
 *              HH:MM clock that moves around the screen to prevent OLED burn-in.
 *              Activated by the display module on sleep and deactivated on wake.
 *              Self-expires when deactivated so the carousel can dismiss it.
 *
 * Exported Classes:
 * - ScreensaverBoard: Bouncing clock screensaver implementing IDisplayBoard
 *   - activate():     Start the screensaver (initialise position to centre)
 *   - deactivate():   Stop the screensaver (triggers board expiry)
 *   - isActive():     Query whether the screensaver is currently running
 *   - render():       Draw the bouncing clock to the U8G2 display buffer
 *   - isExpired():    True when the screensaver has been deactivated
 *   - getBoardName(): Returns "SYS: Screensaver"
 */

#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "widgets/LabelWidget.hpp"

/**
 * @brief Screensaver board with bouncing clock for OLED burn-in prevention.
 *
 * When activated, renders a large HH:MM clock via LabelWidget that bounces off
 * the edges of the display. The position updates on each render cycle based on
 * direction vectors (_xDir, _yDir) that flip on edge collisions.
 */
class ScreensaverBoard : public IDisplayBoard {
public:
    /**
     * @brief Construct with display dimensions for widget layout.
     * @param displayWidth  Display width in pixels (from U8G2 driver).
     * @param displayHeight Display height in pixels (from U8G2 driver).
     */
    ScreensaverBoard(uint16_t displayWidth, uint16_t displayHeight)
        : _clockLabel(0, 0, static_cast<int>(displayWidth), static_cast<int>(displayHeight))
    {}

    /**
     * @brief Start the screensaver and initialise the clock position to centre.
     */
    void activate();

    /**
     * @brief Stop the screensaver. isExpired() will return true.
     */
    void deactivate();

    /**
     * @brief Query whether the screensaver is currently active.
     * @return true if the screensaver is running.
     */
    bool isActive() const;

    /**
     * @brief Render the bouncing clock into the U8G2 display buffer.
     *
     * Gets the current time via time()/localtime() (guarded for native builds),
     * formats HH:MM, and renders via LabelWidget at the current bounce position.
     * Updates the position by _xDir/_yDir and bounces off display edges.
     *
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    void render(U8G2& u8g2) override;

    /**
     * @brief Query whether the screensaver has been deactivated.
     * @return true when deactivate() has been called (or never activated).
     */
    bool isExpired() const override;

    /**
     * @brief Return the human-readable board name for logging.
     * @return "SYS: Screensaver"
     */
    const char* getBoardName() const override;

    /**
     * @brief Set the font to be used by the bouncing clock.
     * @param font Pointer to the U8G2 font array.
     */
    void setClockFont(const uint8_t* font);

private:
    bool          _active       = false; ///< True when the screensaver is running
    unsigned long _animFrameMs  = 0;     ///< Timestamp of the last animation frame
    int           _xPos         = 0;     ///< Current horizontal position (pixels)
    int           _yPos         = 0;     ///< Current vertical position (pixels)
    int           _xDir         = 1;     ///< Horizontal direction (+1 right, -1 left)
    int           _yDir         = 1;     ///< Vertical direction (+1 down, -1 up)
    const uint8_t* _clockFont   = nullptr; ///< Configured clock font (nullptr = use default)

    // --- Widget composition (WO3 refactor) ---
    LabelWidget _clockLabel; ///< HH:MM clock text widget
};
