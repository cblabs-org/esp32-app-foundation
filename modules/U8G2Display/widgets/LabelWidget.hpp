/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/LabelWidget.hpp
 * Description: Stateful text component with configurable alignment and
 *              truncation. Renders static text inside a bounding box using
 *              the drawText() primitive. Zero dependency on framework services.
 *
 * Exported Classes:
 * - LabelWidget: [Class] Graphics widget for static text display.
 *   - setText(): Assigns the display string buffer with bounds safety.
 *   - setFont(): Assigns the typography for rendering.
 *   - setAlignment(): Configures horizontal justification (Left/Center/Right).
 *   - setTruncated(): Enables/disables ellipsis truncation on overflow.
 *   - tick(): No-op (static text requires no timing).
 *   - render(): Paints text to the display buffer via drawText().
 */

#ifndef LABEL_WIDGET_HPP
#define LABEL_WIDGET_HPP                                    ///< Label Widget Hpp

#include "IGfxWidget.hpp"

/**
 * @brief A stateful text widget capable of rendering aligned and truncated text
 *        inside a specific bounding box. Uses drawText() from drawingPrimitives
 *        for all rendering operations.
 */
class LabelWidget : public IGfxWidget {
private:
    char textBuffer[128];         // Internal storage for the display string
    const uint8_t* font;          // Active font pointer (nullptr = use current)
    int alignment;                // Horizontal justification: 0=Left, 1=Center, 2=Right
    bool isTruncated;             // If true, append "..." when text exceeds width

public:
    /**
     * @brief Construct a new label widget.
     * @param _x X coordinate on the display.
     * @param _y Y coordinate (baseline) on the display.
     * @param _w Width of the bounding box (-1 for unconstrained).
     * @param _h Height of the bounding box (-1 for unconstrained).
     */
    LabelWidget(int _x, int _y, int _w = -1, int _h = -1);

    /**
     * @brief Set the text payload for the label.
     *        Safely bounds-copies to internal buffer (max 127 chars + null).
     * @param text The new string to display. nullptr clears the buffer.
     */
    void setText(const char* text);

    /**
     * @brief Apply a new font typography to the label.
     * @param newFont Pointer to the U8G2 font array.
     */
    void setFont(const uint8_t* newFont);

    /**
     * @brief Set the horizontal alignment strategy inside the widget width.
     * @param align 0=Left, 1=Center, 2=Right.
     */
    void setAlignment(int align);

    /**
     * @brief Enable or disable automated string truncation.
     *        When enabled, appends "..." if text exceeds layout boundaries.
     * @param truncated True to enable ellipsis truncation.
     */
    void setTruncated(bool truncated);

    /**
     * @brief No-op logic update. Static text requires no timing.
     * @param currentMillis Current system uptime in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Renders the text to the display buffer using drawText().
     * @param display Reference to the U8G2 display driver.
     */
    void render(U8G2& display) override;
};

#endif // LABEL_WIDGET_HPP
