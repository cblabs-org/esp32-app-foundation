/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ProgressBarWidget.hpp
 * Description: Graphical progress indicator with centered message text,
 *              animated linear interpolation, and optional numeric percentage
 *              display. Uses a checkered stipple pattern for the filled bar
 *              to achieve a classic LCD/DMD "shaded" look.
 *              Zero dependency on framework services.
 *
 * Exported Classes:
 * - ProgressBarWidget: [Class] Graphics widget for showing task progression.
 *   - setMessage(): Updates the status label (centered above bar).
 *   - setShowPercentText(): Toggles numeric percentage rendering.
 *   - setPercent(): Sets target percentage with optional animation.
 *   - tick(): Smooth frame-interpolated animation logic.
 *   - render(): Full-frame drawing (message + bar).
 *   - renderAnimationUpdate(): Partial-screen redraw on percent change.
 */

#ifndef PROGRESS_BAR_WIDGET_HPP
#define PROGRESS_BAR_WIDGET_HPP

#include "IGfxWidget.hpp"

/**
 * @brief Animated progress bar widget with status message and percentage display.
 *        Supports smooth linear interpolation from current to target value.
 */
class ProgressBarWidget : public IGfxWidget {
private:
    char message[256];            // Status text displayed above the bar
    int currentPercent;           // Current visual percentage (0-100)
    int targetPercent;            // Target percentage for animation
    int startPercent;             // Starting percentage for current animation
    uint32_t animStartTime;       // Timestamp when current animation began
    uint32_t animDurationMs;      // Duration of current animation in milliseconds

    // State tracking for deduplication
    int oldPercent;               // Previous percent (for animation update dedup)
    bool textChanged;             // True if message changed since last render
    bool showPercentText;         // If true, render numeric percentage (e.g. "45%")

    const uint8_t* font;          // Active font pointer

public:
    /**
     * @brief Construct a new progress bar widget.
     * @param _x X position on the display.
     * @param _y Y position on the display.
     * @param _w Width (-1 for full display width).
     * @param _h Height (-1 for auto height).
     * @param _font Pointer to the U8G2 font to use (nullptr for default).
     */
    ProgressBarWidget(int _x, int _y, int _w = -1, int _h = -1,
                      const uint8_t* _font = nullptr);

    /**
     * @brief Set the status text displayed above the progress bar.
     *        Deduplicates: identical text is silently ignored.
     * @param newMessage Null-terminated string. nullptr is ignored.
     */
    void setMessage(const char* newMessage);

    /**
     * @brief Toggle rendering of the numeric percentage text (e.g. "45%").
     * @param show True to render percentage alongside the message.
     */
    void setShowPercentText(bool show);

    /**
     * @brief Update the font used for rendering the message and percentage text.
     * @param _font Pointer to U8G2 font array. nullptr defers to the current
     *              display font.
     */
    void setFont(const uint8_t* _font) { font = _font; }

    /**
     * @brief Set the progress value (0-100) with optional animation.
     *        Clamped to [0, 100]. durationMs=0 snaps immediately.
     * @param percent Target progress amount (0-100).
     * @param durationMs Animation duration in milliseconds (0 for instant).
     */
    void setPercent(int percent, uint32_t durationMs = 0);

    /**
     * @brief Smooth interpolation logic for animated progress transitions.
     *        Linear interpolation from startPercent to targetPercent over animDurationMs.
     * @param currentMillis Current system uptime in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Full-frame render of the progress bar and status label.
     *        Message is centered above the bar; bar uses checkered stipple fill.
     * @param display Reference to the U8G2 display instance.
     */
    void render(U8G2& display) override;

    /**
     * @brief Partial-screen redraw when percent or text has changed.
     *        Uses display.updateDisplayArea() for minimal refresh.
     * @param display Reference to the U8G2 display instance.
     * @param currentMillis Current system uptime in milliseconds.
     */
    void renderAnimationUpdate(U8G2& display, uint32_t currentMillis) override;
};

#endif // PROGRESS_BAR_WIDGET_HPP
