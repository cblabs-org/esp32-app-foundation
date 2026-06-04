/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/CarouselEngine.hpp
 * Description:
 *   Runtime-mutable collection of IDisplayBoard pointers driving a rotating
 *   carousel on a U8G2 display.
 *
 *   The carousel is the lowest-priority rendering layer — it shows only when
 *   no modal, base board, or screensaver is active.  Boards may be added,
 *   removed, or cleared at any time; the engine adjusts its active index to
 *   keep the visible board stable across mutations.
 *
 *   tick() is called each loop iteration; when the configured interval
 *   elapses the engine advances to the next non-expired board, evicting
 *   any boards whose isExpired() returns true.
 *
 *   Suspend / Resume: calling suspend() freezes the rotation timer so the
 *   current board remains visible indefinitely.  resume() re-enables
 *   automatic rotation from the current board.
 *
 *   show() override: callers may force-display a specific board at any
 *   time; the rotation timer is reset so the full interval elapses before
 *   the next automatic advance.
 *
 * Exported Functions/Classes:
 * - CarouselEngine: Runtime-mutable rotating board collection.
 *   - add(): Register a board into the pool
 *   - remove(): Remove a specific board from the pool
 *   - clear(): Remove all boards from the pool
 *   - contains(): Check if a board is in the pool
 *   - boardCount(): Return number of boards in the pool
 *   - getBoard(): Return a board by index
 *   - currentBoard(): Return the currently active board
 *   - activeIndex(): Return the index of the active board
 *   - show(): Force-show a specific pool board (reset rotation timer)
 *   - suspend(): Pause automatic rotation (timer frozen)
 *   - resume(): Resume automatic rotation from current board
 *   - setInterval(): Set the rotation interval in milliseconds
 *   - getInterval(): Get the rotation interval in milliseconds
 *   - tick(): Advance carousel if interval elapsed; call each tick
 */

#pragma once

#include <vector>
#include <cstdint>
#include "interfaces/IDisplayBoard.hpp"

class CarouselEngine {
public:
    // -------------------------------------------------------------------------
    // Collection mutators
    // -------------------------------------------------------------------------

    /**
     * @brief Add a board to the carousel pool.
     *
     * Safe to call at any time.  Null pointers and duplicates are ignored.
     *
     * @param board Non-null pointer to the board to add.
     */
    void add(IDisplayBoard* board);

    /**
     * @brief Remove a specific board from the pool.
     *
     * If the removed board is currently active, the carousel advances to the
     * board that slides into its slot.  If the pool becomes empty, activeIndex
     * resets to 0.
     *
     * @param board Pointer to the board to remove.
     * @return true if the board was found and removed; false otherwise.
     */
    bool remove(IDisplayBoard* board);

    /**
     * @brief Remove all boards from the pool.
     *
     * Resets the active index and rotation timer.
     */
    void clear();

    // -------------------------------------------------------------------------
    // Collection queries
    // -------------------------------------------------------------------------

    /**
     * @brief Check whether a board is registered in the pool.
     * @param board Pointer to the board to search for.
     * @return true if the board is in the pool.
     */
    bool contains(IDisplayBoard* board) const;

    /**
     * @brief Return number of boards in the pool.
     * @return Pool size.
     */
    int boardCount() const;

    /**
     * @brief Return a specific board by index.
     * @param index Zero-based index into the pool.
     * @return Pointer to the board, or nullptr if out of bounds.
     */
    IDisplayBoard* getBoard(int index) const;

    /**
     * @brief Return pointer to the active board, or nullptr if pool is empty.
     * @return Active IDisplayBoard pointer.
     */
    IDisplayBoard* currentBoard() const;

    /**
     * @brief Return the index of the currently active board.
     * @return Active index, or -1 if pool is empty.
     */
    int activeIndex() const;

    // -------------------------------------------------------------------------
    // Rotation control
    // -------------------------------------------------------------------------

    /**
     * @brief Force-show a specific pool board and reset the rotation timer.
     *
     * If the board is not in the pool, this call is a no-op.
     *
     * @param board Pointer to the board to show.
     * @param currentMs Current time in milliseconds.
     */
    void show(IDisplayBoard* board, unsigned long currentMs);

    /**
     * @brief Pause automatic rotation (timer frozen).
     */
    void suspend();

    /**
     * @brief Resume automatic rotation from current board.
     */
    void resume();

    /**
     * @brief Check whether automatic rotation is paused.
     * @return True if the carousel is suspended.
     */
    bool isSuspended() const { return _suspended; }

    /**
     * @brief Set the rotation interval in milliseconds.
     * @param ms Rotation interval.
     */
    void setInterval(unsigned long ms);

    /**
     * @brief Get the rotation interval in milliseconds.
     * @return Rotation interval.
     */
    unsigned long getInterval() const;

    /**
     * @brief Advance carousel if interval elapsed; call each tick.
     * @param currentMs Current time in milliseconds.
     */
    void tick(unsigned long currentMs);

private:
    std::vector<IDisplayBoard*> _pool;       ///< Carousel board pool
    int _activeIndex = 0;                    ///< Index of the currently rendering board
    unsigned long _lastRotationMs = 0;       ///< Timestamp of last carousel rotation
    unsigned long _intervalMs = 8000;        ///< Rotation interval (default 8 seconds)
    bool _suspended = false;                 ///< True when automatic rotation is paused

    /**
     * @brief Advance to next non-expired board; removes expired boards from pool.
     */
    void _advance();
};
