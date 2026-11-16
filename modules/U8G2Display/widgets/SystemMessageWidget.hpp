/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/SystemMessageWidget.hpp
 * Description: Full-screen modal widget for system alerts and diagnostics.
 *              Renders a centred title line followed by up to three detail lines.
 *              Uses foundation AppFonts for font selection. All rendering is
 *              using native test stubs for native test compatibility.
 *
 * Exported Classes:
 * - SystemMessageWidget: [Class] Multi-line system notification widget.
 *   - setMessage(): Batch update the title and up to 3 detail lines.
 *   - tick(): No-op — system messages are static content.
 *   - render(): Draw the centred multi-line message to the display buffer.
 */

#ifndef SYSTEM_MESSAGE_WIDGET_HPP
#define SYSTEM_MESSAGE_WIDGET_HPP                           ///< System Message Widget Hpp

#include "IGfxWidget.hpp"

/**
 * @brief Full-screen modal widget for displaying system alerts.
 *        Renders a title line (large font) followed by up to three
 *        detail lines (small font), all horizontally centred within
 *        the widget bounding box.
 */
class SystemMessageWidget : public IGfxWidget {
private:
    char _title[64];        // Primary heading text buffer
    char _lines[3][128];    // Up to 3 detail lines
    int _numLines;          // Number of active detail lines (0-3)

public:
    /**
     * @brief Construct a system message widget with the given bounding box.
     *        Defaults to full screen (128x64) for typical OLED displays.
     * @param x X coordinate (default 0).
     * @param y Y coordinate (default 0).
     * @param w Width in pixels (default 128).
     * @param h Height in pixels (default 64).
     */
    SystemMessageWidget(int x = 0, int y = 0, int w = 128, int h = 64);

    /**
     * @brief Batch update the widget content.
     *        Null pointers are allowed; null title clears it, null lines are skipped.
     * @param newTitle Primary heading text (or nullptr to clear).
     * @param l1 Optional detail line 1 (or nullptr).
     * @param l2 Optional detail line 2 (or nullptr).
     * @param l3 Optional detail line 3 (or nullptr).
     */
    void setMessage(const char* newTitle,
                    const char* l1 = nullptr,
                    const char* l2 = nullptr,
                    const char* l3 = nullptr);

    /**
     * @brief No-op tick — system messages are static content.
     * @param currentMillis Milliseconds since boot (unused).
     */
    void tick(uint32_t currentMillis) override { (void)currentMillis; }

    /**
     * @brief Draw the multi-line message centred in the bounding box.
     *        Title uses AppFonts::defaultLarge(), detail lines use defaultSmall().
     * @param display Reference to the U8G2 display instance.
     */
    void render(U8G2& display) override;
};

#endif // SYSTEM_MESSAGE_WIDGET_HPP
