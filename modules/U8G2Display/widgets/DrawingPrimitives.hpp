/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/widgets/DrawingPrimitives.hpp
 * Description: Stateless U8G2 graphics utility wrappers for common drawing tasks.
 *              Provides text alignment, area clearing, string measurement, and
 *              geometric primitives. Includes the U8G2StateSaver RAII scope guard
 *              for safe display state management.
 *
 *              The TextAlign enum is always available (including NATIVE_BUILD).
 *              All other declarations require the U8G2 hardware driver and are
 *              guarded behind #ifndef NATIVE_BUILD.
 *
 * Exported Functions/Classes:
 * - TextAlign: [Enum] Text justification options (Left, Center, Right).
 * - U8G2StateSaver: [Class] RAII scope guard to preserve U8G2 display properties.
 *   - U8G2StateSaver(): Caches current font, draw color, and clip window.
 *   - ~U8G2StateSaver(): Restores cached state on scope exit.
 * - blankArea(): Clears a rectangular region by drawing a black box.
 * - getStringWidth(): Measures string width in pixels under the current font.
 * - drawText(): Aligned text rendering with optional truncation and clipping.
 * - drawTriangle(): Draws filled or outlined triangles.
 *
 * NOTE: __FlashStringHelper overloads are intentionally omitted (WD-07).
 *       This is a foundation framework; PROGMEM wrappers are app-specific.
 */

#ifndef DRAWING_PRIMITIVES_HPP
#define DRAWING_PRIMITIVES_HPP

#include <cstdint>

/**
 * @brief Text alignment options for consolidated drawing primitives.
 *        Always available, including in NATIVE_BUILD for test validation.
 */
enum class TextAlign : uint8_t {
    Left   = 0,
    Center = 1,
    Right  = 2
};

// =============================================================================
// Hardware-only declarations — requires U8G2 driver
// =============================================================================

#include <U8g2lib.h>

/**
 * @brief RAII scope guard to preserve and restore U8G2 display properties.
 *        Tracks clipping windows, draw colors, and fonts natively without
 *        heap allocation. Instantiate at the top of any function that modifies
 *        display state to guarantee clean restoration on scope exit.
 */
class U8G2StateSaver {
private:
    U8G2& display;               // Reference to the display context
    const uint8_t* oldFont;      // Cached font pointer
    uint8_t oldDrawColor;        // Cached draw color
    u8g2_uint_t oldClipX0;       // Cached top-left X clip bound
    u8g2_uint_t oldClipY0;       // Cached top-left Y clip bound
    u8g2_uint_t oldClipX1;       // Cached bottom-right X clip bound
    u8g2_uint_t oldClipY1;       // Cached bottom-right Y clip bound

public:
    /**
     * @brief Construct the saver, caching current display state.
     *        Reads font, draw_color, and clip bounds from the u8g2_t struct.
     * @param _display Active U8G2 instance to guard.
     */
    U8G2StateSaver(U8G2& _display);

    /**
     * @brief Destructor automatically reverts display state.
     *        Restores font, draw color, and clip window to cached values.
     */
    ~U8G2StateSaver();
};

/**
 * @brief Clear a rectangular area on the display by drawing a black box.
 * @param display Reference to U8G2 context.
 * @param x Top-left X coordinate.
 * @param y Top-left Y coordinate.
 * @param w Width in pixels.
 * @param h Height in pixels.
 */
void blankArea(U8G2& display, int x, int y, int w, int h);

/**
 * @brief Calculate the width in pixels of a null-terminated string.
 * @param display Reference to U8G2 context.
 * @param message Text to measure.
 * @return Width in pixels under the currently active font.
 */
int getStringWidth(U8G2& display, const char* message);

/**
 * @brief Draw text with alignment and optional truncation within a bounding box.
 *
 *        Provides a fast-path for standard left-aligned text (no measurement or
 *        clipping) and a full-path for alignment, truncation, or explicit bounding.
 *
 * @param display Reference to U8G2 context.
 * @param message Null-terminated text to display.
 * @param x Top-left X coordinate of the text area.
 * @param y Top-left Y coordinate of the text area.
 * @param w Width of the area (-1 for screen width minus x).
 * @param h Height of the area (-1 for font height).
 * @param align Alignment within the width (Left, Center, Right).
 * @param truncate If true, appends '...' if text exceeds width.
 * @param font Optional font override. If nullptr, current font is used.
 * @param textOffsetY Optional vertical push offset from the clipping box top.
 */
void drawText(U8G2& display, const char* message, int x, int y,
              int w = -1, int h = -1,
              TextAlign align = TextAlign::Left,
              bool truncate = false,
              const uint8_t* font = nullptr,
              int textOffsetY = 0);

/**
 * @brief Draw a triangle geometric primitive.
 * @param display Reference to U8G2 context.
 * @param x0 Vertex 0 X coordinate.
 * @param y0 Vertex 0 Y coordinate.
 * @param x1 Vertex 1 X coordinate.
 * @param y1 Vertex 1 Y coordinate.
 * @param x2 Vertex 2 X coordinate.
 * @param y2 Vertex 2 Y coordinate.
 * @param isFilled True for solid fill, false for wireframe outline.
 */
void drawTriangle(U8G2& display, int x0, int y0, int x1, int y1,
                  int x2, int y2, bool isFilled = true);


#endif // DRAWING_PRIMITIVES_HPP
