/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ScrollingTextWidget.hpp
 * Description: Horizontal marquee scrolling text widget. Implements smooth
 *              pixel-based scrolling with initial delay, ~25 FPS pacing,
 *              and loop detection. Supports both partial-screen (Escape Hatch)
 *              and full-frame rendering modes. Zero dependency on framework services.
 *
 * Exported Classes:
 * - ScrollingTextWidget: [Class] Base widget for scrolling text rendering.
 *   - setText(): Assigns new text with change detection and layout invalidation.
 *   - setFont(): Updates the rendering font and triggers layout recalc.
 *   - isScrollFinished(): Returns true when text completes a full scroll loop.
 *   - resetScroll(): Rewinds scroll position and resets timers.
 *   - tick(): Marquee animation math (pixel shifts, delays, loop detection).
 *   - render(): Full-frame rendering with static/scroll detection.
 *   - renderAnimationUpdate(): Partial-screen refresh for smooth scrolling.
 */

#ifndef SCROLLING_TEXT_WIDGET_HPP
#define SCROLLING_TEXT_WIDGET_HPP                           ///< Scrolling Text Widget Hpp

#include "IGfxWidget.hpp"

/**
 * @brief Base widget for horizontal marquee scrolling of text within a
 *        physical bounding box. Handles all scroll math, delay logic,
 *        and loop detection independently.
 */
class ScrollingTextWidget : public IGfxWidget {
protected:
    char _currentText[512];       // Buffer for the active scrolling string
    int _scrollX;                 // Current pixel offset for the scroll
    int _messageWidth;            // Total rendered width of the string in pixels
    uint32_t _lastScrollMs;       // Timestamp of the last scroll tick
    bool _needsScroll;            // True if text is wider than bounding box
    bool _needsLayout;            // True when text/font changed (needs remeasure)
    const uint8_t* _font;         // Active font pointer
    bool _scrollFinished;         // True when text has fully exited the left bound

public:
    /**
     * @brief Construct a new scrolling text widget.
     * @param x X coordinate of top-left corner.
     * @param y Y coordinate of top-left corner.
     * @param w Width of bounding box in pixels.
     * @param h Height of bounding box in pixels.
     * @param font U8G2 font array to use for rendering (nullptr for default).
     */
    ScrollingTextWidget(int x, int y, int w, int h, const uint8_t* font = nullptr);
    /** @brief Destructor for ScrollingTextWidget. */
    virtual ~ScrollingTextWidget() = default;

    /**
     * @brief Set the text to be scrolled. Triggers layout recalculation.
     *        Deduplicates: identical text is ignored to prevent layout thrash.
     * @param newText Null-terminated string. nullptr clears the buffer.
     */
    void setText(const char* newText);

    /**
     * @brief Update the font used for rendering and trigger layout recalc.
     * @param font Pointer to U8G2 font array.
     */
    void setFont(const uint8_t* font) { _font = font; _needsLayout = true; }

    /**
     * @brief Check if the text has completed a full scroll loop off-screen.
     *        Useful for derived classes to trigger data loads or message cycling.
     * @return True when the scroll cycle is complete.
     */
    bool isScrollFinished() const { return _scrollFinished; }

    /**
     * @brief Manually reset the scroll position to the starting position.
     *        Also resets the initial delay timer.
     */
    void resetScroll();

    /**
     * @brief Marquee animation math: initial delay, pixel shift, loop detection.
     *        - 3-second initial delay before scrolling starts
     *        - Pixel shift at ~25 FPS (40ms interval)
     *        - Loop detection when text exits left edge (scrollX > messageWidth + 30)
     *        - Static text dwell: 5-second timer then set scrollFinished
     * @param currentMillis Current system uptime in milliseconds.
     */
    virtual void tick(uint32_t currentMillis) override;

    /**
     * @brief Full-frame render of the text inside the bounding box.
     *        Handles layout recalculation, static centering, and scrolling clip.
     * @param display Reference to the U8G2 display instance.
     */
    virtual void render(U8G2& display) override;

    /**
     * @brief Partial-screen refresh for smooth marquee scrolling updates.
     *        Uses display.updateDisplayArea() for minimal SPI bus traffic.
     * @param display Reference to the U8G2 display instance.
     * @param currentMillis Current system uptime in milliseconds.
     */
    virtual void renderAnimationUpdate(U8G2& display, uint32_t currentMillis) override;
};

#endif // SCROLLING_TEXT_WIDGET_HPP
