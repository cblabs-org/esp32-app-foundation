/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/IDisplayBoard.hpp
 * Description: Pure interface for U8G2 display carousel boards. Each
 *              "board" is a full-screen rendering unit that the u8g2Display
 *              module rotates through. Both system boards (BootSplash, Error)
 *              and application boards (app-defined layouts) implement this.
 *
 * Exported Classes:
 * - IDisplayBoard: Abstract base for a U8G2 carousel rendering unit
 *   - render(): Draw a full-screen frame to the U8G2 display buffer
 *   - isExpired(): Return true if this board should be evicted from the carousel
 *   - getBoardName(): Human-readable name for logging and diagnostics
 *   - onActivate(): Called when this board becomes the active rendering target
 *   - onDeactivate(): Called when this board is no longer the active rendering target
 *   - tick(): Per-frame timing hook for animations, scrolling, etc.
 */

#pragma once

#include <cstdint>

// Forward-declare to avoid pulling in U8G2 header in interface files.
// Concrete board classes include the U8G2 header directly.
class U8G2;

/**
 * @brief Contract for a U8G2 carousel display board (system or application).
 *
 * The u8g2Display module maintains a std::vector<IDisplayBoard*> slot pool.
 * On each carousel rotation, the current board's render() is called within
 * the U8G2 draw loop. Boards signal their own expiry via isExpired().
 *
 * Lifecycle hooks onActivate() / onDeactivate() are called by the carousel
 * engine when the active board changes. tick() is called every frame to
 * allow boards to run animation timers and scrolling logic.
 */
class IDisplayBoard {
public:
    virtual ~IDisplayBoard() = default;

    /**
     * @brief Render a full-screen frame into the U8G2 display buffer.
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    virtual void render(U8G2& u8g2) = 0;

    /**
     * @brief Check whether this board should be evicted from the carousel.
     *
     * The CarouselEngine calls this method after each render cycle to decide
     * whether to keep this board in rotation or remove it.
     *
     * Contract:
     * - **Permanent boards** (e.g., status dashboards, sensor readouts):
     *   MUST return `false` — they are never evicted.
     * - **Transient boards** (e.g., notification overlays, progress bars):
     *   MUST return `true` once their display duration has elapsed.
     * - **Conditional boards** (e.g., error boards that auto-dismiss):
     *   MUST return `true` when their dismissal condition is met.
     *
     * Implementations MUST NOT perform side effects in this method — it is
     * a pure query. The actual removal is handled by the CarouselEngine.
     *
     * @return true if this board should be removed from the carousel.
     */
    virtual bool isExpired() const = 0;

    /**
     * @brief Return a human-readable name for logging and diagnostics.
     * @return Null-terminated string board identifier.
     */
    virtual const char* getBoardName() const = 0;

    /**
     * @brief Called when this board becomes the active rendering target.
     *
     * Override to initialise animation state, reset scroll positions, or
     * trigger a data refresh when the board becomes visible.
     * Default implementation is a no-op.
     */
    virtual void onActivate() {}

    /**
     * @brief Called when this board is no longer the active rendering target.
     *
     * Override to release resources, pause animations, or snapshot state
     * before the board goes off-screen. Default implementation is a no-op.
     */
    virtual void onDeactivate() {}

    /**
     * @brief Per-frame timing hook for animations, scrolling, etc.
     *
     * Called by the carousel engine on every tick() before render().
     * Override to advance scroll positions, update animation timers,
     * or run board-specific periodic logic. Default implementation is a no-op.
     *
     * @param currentMillis Current system time in milliseconds.
     */
    virtual void tick(uint32_t currentMillis) { (void)currentMillis; }
};
