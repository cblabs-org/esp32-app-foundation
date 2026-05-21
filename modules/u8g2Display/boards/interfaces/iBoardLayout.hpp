/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/boards/interfaces/iBoardLayout.hpp
 * Description: Base abstract interface for all visual board layouts (Views).
 *              Decouples the rendering and coordinate math from the Board
 *              (Controller). Each concrete layout receives an Application*
 *              for dependency injection and implements tick/render hooks.
 *
 * Exported Classes:
 * - iBoardLayout: [Interface] Visual layout template for board views.
 *   - iBoardLayout(): Constructor with Application* injection (WD-04).
 *   - tick(): View-specific timing logic (e.g. scrollers, animations).
 *   - render(): Full-frame rendering pass.
 *   - renderAnimationUpdate(): Targeted sub-frame animation updates (Escape Hatch).
 */

#ifndef I_BOARD_LAYOUT_HPP
#define I_BOARD_LAYOUT_HPP

#include <cstdint>

// --- Forward declarations (no header dependencies — preserves native testability) ---
class U8G2;
class Application;

/**
 * @brief Pure virtual interface representing a visual layout (View).
 *        Decouples the rendering and coordinate math from the Board (Controller).
 *        Concrete layouts receive an Application* for DI access to framework services.
 */
class iBoardLayout {
protected:
    Application* app;    // Injected application context pointer (WD-04: NOT departures-board's appContext*)

public:
    /**
     * @brief Construct the view with an application context for DI access.
     * @param _app Pointer to the Application composition root.
     */
    iBoardLayout(Application* _app) : app(_app) {}

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~iBoardLayout() = default;

    /**
     * @brief Periodic logic update for view-specific timing (e.g. scrolling).
     *        Called by the owning board's tick loop.
     * @param currentMillis Current system time in milliseconds.
     */
    virtual void tick(uint32_t currentMillis) = 0;

    /**
     * @brief Main rendering hook for the entire view.
     *        Called during the full-frame rendering pass.
     * @param display Reference to the U8G2 display instance.
     */
    virtual void render(U8G2& display) = 0;

    /**
     * @brief High-speed non-blocking animation update hook ("Escape Hatch").
     *        Default no-op. Override only if partial screen updates are needed
     *        during long blocking operations.
     * @param display Reference to the U8G2 display instance.
     * @param currentMillis Current system time in milliseconds.
     */
    virtual void renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {}
};

#endif // I_BOARD_LAYOUT_HPP
