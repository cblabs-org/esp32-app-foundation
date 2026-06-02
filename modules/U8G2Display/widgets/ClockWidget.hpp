/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/widgets/ClockWidget.hpp
 * Description: Real-time clock display widget with configurable format (HH:MM
 *              or HH:MM:SS), 1Hz colon blink animation, and optional secondary
 *              font for seconds. Receives TimeManager via dependency injection.
 *              Zero dependency on global state or framework services.
 *
 * Exported Classes:
 * - ClockWidget: [Class] Graphical widget for real-time display.
 *   - ClockFormat: Enum for HH_MM and HH_MM_SS display modes.
 *   - setFont(): Update the primary typography.
 *   - setSecondaryFont(): Set optional font for seconds digits.
 *   - setFormat(): Switch between HH_MM and HH_MM_SS modes.
 *   - setBlink(): Toggle the 1Hz colon blink animation.
 *   - setAlignment(): Configure horizontal justification.
 *   - tick(): Logic update for blink timing (500ms toggle).
 *   - render(): Primary full-frame drawing.
 *   - renderAnimationUpdate(): Partial-screen redraw on blink/minute change.
 */

#ifndef CLOCK_WIDGET_HPP
#define CLOCK_WIDGET_HPP

#include "IGfxWidget.hpp"

// --- Forward declaration: TimeManager is injected, not included ---
class TimeManager;

/**
 * @brief Clock display widget with configurable format and blink animation.
 *        TimeManager is injected via constructor — no global state dependency.
 */
class ClockWidget : public IGfxWidget {
public:
    /**
     * @brief Display format options for the clock widget.
     */
    enum class ClockFormat {
        HH_MM,     ///< Standard hour:minute display (e.g., "14:36")
        HH_MM_SS   ///< Full display including seconds (e.g., "14:36:50")
    };

private:
    bool showColon;               // Current colon visibility state
    bool oldColon;                // Previous colon state (for dedup in animation)
    bool blinkEnabled;            // If true, colon blinks at 1Hz
    uint32_t lastBlinkMs;         // Timestamp of the last blink toggle
    int lastMinute;               // Cached minute for change detection
    int lastSecond;               // Cached second for change detection
    ClockFormat format;           // Active display format
    const uint8_t* font;          // Primary font pointer
    const uint8_t* secondaryFont; // Optional secondary font for seconds
    int alignment;                // Horizontal justification: 0=Left, 1=Center, 2=Right
    TimeManager* timeMgr;         // Injected TimeManager dependency

public:
    /**
     * @brief Construct the clock widget with a TimeManager dependency.
     * @param _timeMgr Pointer to the TimeManager instance (may be nullptr for tests).
     * @param _x X position on the display.
     * @param _y Y position on the display.
     * @param _w Width (-1 for auto).
     * @param _h Height (-1 for auto).
     * @param _font Pointer to the primary U8G2 font (nullptr for default).
     */
    ClockWidget(TimeManager* _timeMgr, int _x, int _y, int _w = -1, int _h = -1,
                const uint8_t* _font = nullptr);

    /**
     * @brief Update the primary font used by the clock.
     * @param newFont Pointer to the U8G2 font array.
     */
    void setFont(const uint8_t* newFont);

    /**
     * @brief Update the secondary font used for the seconds component.
     * @param newFont Pointer to the U8G2 font array (nullptr = use primary font).
     */
    void setSecondaryFont(const uint8_t* newFont);

    /**
     * @brief Set the display format of the clock.
     * @param newFormat The ClockFormat enum value (HH_MM or HH_MM_SS).
     */
    void setFormat(ClockFormat newFormat);

    /**
     * @brief Toggle the blinking colon animation.
     * @param enable True to enable blinking, false for solid colon.
     */
    void setBlink(bool enable) { blinkEnabled = enable; showColon = true; }

    /**
     * @brief Set the horizontal alignment strategy inside the widget width.
     * @param align 0=Left, 1=Center, 2=Right.
     */
    void setAlignment(int align) { alignment = align; }

    /**
     * @brief Update the clock's internal blink state.
     *        Toggles showColon at 1Hz (500ms interval) when blinkEnabled.
     * @param currentMillis Current system uptime in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Render the clock to the display with the configured format.
     * @param display Reference to the U8G2 display context.
     */
    void render(U8G2& display) override;

    /**
     * @brief Partial-screen update for the blink animation.
     *        Only redraws if the blink state or time digits have changed.
     * @param display Reference to the U8G2 display context.
     * @param currentMillis Current system uptime in milliseconds.
     */
    void renderAnimationUpdate(U8G2& display, uint32_t currentMillis) override;
};

#endif // CLOCK_WIDGET_HPP
