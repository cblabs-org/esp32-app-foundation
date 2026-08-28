/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/IDisplayBoard.hpp
 * Description: Pure interface for U8G2 display carousel boards. Each
 *              "board" is a full-screen rendering unit that the u8g2Display
 *              module rotates through. Both system boards (BootSplash, Error)
 *              and application boards (app-defined layouts) implement this.
 *
 * Exported Classes:
 * - IDisplayBoard: Abstract base for a U8G2 carousel rendering unit
 *   - render(): Draw a full-screen frame to the U8G2 display buffer
 *   - isExpired(): Return true if this board should be evicted from the carousel
 *   - canAdvance(): Return false to ask the carousel to defer rotation
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
    /** @brief Destructor for IDisplayBoard. */
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
     * @brief Ask whether the carousel may rotate away from this board right now.
     *
     * The CarouselEngine consults this once its rotation interval has elapsed,
     * before advancing to the next board. Returning `false` defers rotation so
     * an in-flight scroll, page transition, or multi-frame animation can finish
     * instead of being truncated mid-frame; the engine re-queries on every
     * subsequent tick and rotates as soon as this returns `true`.
     *
     * Contract:
     * - This is a **pure query** — implementations MUST NOT perform side
     *   effects (no mutating board state, no reaching into the carousel).
     * - The hold is **bounded**: the engine will force rotation once the hold
     *   exceeds `CarouselEngine::getMaxHoldMs()`, regardless of this return
     *   value, so a board that never releases cannot wedge the display.
     * - Eviction via `isExpired()` still runs on every tick even while this
     *   returns `false` — a hold defers rotation, not eviction.
     *
     * Default implementation returns `true` (rotates freely), preserving
     * existing behaviour for boards that don't override it.
     *
     * @return false to defer rotation; true to allow it.
     */
    virtual bool canAdvance() const { return true; }

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
