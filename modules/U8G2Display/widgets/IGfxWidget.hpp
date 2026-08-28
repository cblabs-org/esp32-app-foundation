/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/IGfxWidget.hpp
 * Description: Base abstract interface for all graphical UI components (Widgets).
 *              Provides standard controls for positioning, sizing, visibility,
 *              and hardware-accelerated rendering updates. All concrete widgets
 *              in the toolkit extend this class.
 *
 * Exported Classes:
 * - IGfxWidget: Pure virtual base class for all display widgets.
 *   - setVisible() / getVisible(): Rendering visibility state control.
 *   - setCoords(): Positioning and bounding box configuration.
 *   - getX() / getY() / getW() / getH(): Geometry accessors.
 *   - tick(): Periodic logic and timing update hook.
 *   - render(): Primary full-frame drawing entry point.
 *   - renderAnimationUpdate(): Targeted high-speed partial updates (Escape Hatch).
 */

#ifndef I_GFX_WIDGET_HPP
#define I_GFX_WIDGET_HPP                                    ///< I Gfx Widget Hpp

#include <cstdint>

// --- Forward declaration (preserves native testability — no U8g2lib.h in headers) ---
class U8G2;

/**
 * @brief Abstract base class for all UI widgets.
 *        Manages its own position, optional size, and visibility.
 *        Concrete widgets must implement tick() and render().
 */
class IGfxWidget {
protected:
    int x, y;                  // Top-left origin of the widget's bounding box
    int width, height;         // Size in pixels (-1 = dynamic/auto)
    bool isVisible = true;     // Rendering visibility flag

public:
    /**
     * @brief Construct a widget with position and optional size.
     * @param _x X coordinate on the display.
     * @param _y Y coordinate on the display.
     * @param _w Width in pixels (-1 for dynamic or full width).
     * @param _h Height in pixels (-1 for dynamic or full height).
     */
    IGfxWidget(int _x, int _y, int _w = -1, int _h = -1)
        : x(_x), y(_y), width(_w), height(_h) {}

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~IGfxWidget() = default;

    // -------------------------------------------------------------------------
    // Visibility API
    // -------------------------------------------------------------------------

    /**
     * @brief Set the rendering visibility of the widget.
     * @param visible True to render, false to skip during render passes.
     */
    void setVisible(bool visible) { isVisible = visible; }

    /**
     * @brief Check if the widget is currently visible.
     * @return True if the widget should be rendered.
     */
    bool getVisible() const { return isVisible; }

    // -------------------------------------------------------------------------
    // Geometry API
    // -------------------------------------------------------------------------

    /**
     * @brief Update the widget's position and size.
     * @param _x New X coordinate.
     * @param _y New Y coordinate.
     * @param _w New width (-1 for dynamic).
     * @param _h New height (-1 for dynamic).
     */
    void setCoords(int _x, int _y, int _w = -1, int _h = -1) {
        x = _x; y = _y; width = _w; height = _h;
    }

    /** @brief Get the X coordinate of the widget. */
    int getX() const { return x; }
    /** @brief Get the Y coordinate of the widget. */
    int getY() const { return y; }
    /** @brief Get the width of the widget (-1 if dynamic). */
    int getW() const { return width; }
    /** @brief Get the height of the widget (-1 if dynamic). */
    int getH() const { return height; }

    // -------------------------------------------------------------------------
    // Core virtual interface
    // -------------------------------------------------------------------------

    /**
     * @brief Periodic logic update for state math and timing.
     *        USE THIS 99% of the time for standard animations (e.g. scrollX++).
     *        Called by the board layout's tick loop and safely precedes a
     *        synchronized full-screen render.
     * @param currentMillis Current system time in milliseconds.
     */
    virtual void tick(uint32_t currentMillis) = 0;

    /**
     * @brief Render the widget to the display buffer.
     *        Called during the full-frame rendering pass. Implementations should
     *        check isVisible before drawing if conditional rendering is desired.
     * @param display Reference to the U8G2 display instance.
     */
    virtual void render(U8G2& display) = 0;

    /**
     * @brief Emergency non-blocking hardware-accelerated update ("Escape Hatch").
     *        ONLY use this if the physical screen must move during a long blocking
     *        operation (e.g. Wi-Fi download).
     *
     *        WARNING: Implementations MUST constrain their visual update using
     *        `display.updateDisplayArea` to a tiny local bounding box to avoid
     *        corrupting the frozen screen buffer or saturating the SPI bus.
     *
     *        DEDUPLICATION PATTERN: Implementations should unconditionally call
     *        `tick(currentMillis)` first to calculate state changes, and only
     *        trigger `updateDisplayArea()` if `tick` actually modified the
     *        widget's mathematical state.
     *
     * @param display Reference to the U8G2 display instance.
     * @param currentMillis Current system time in milliseconds.
     */
    virtual void renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {}
};

#endif // I_GFX_WIDGET_HPP
