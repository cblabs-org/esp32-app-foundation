/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/CarouselEngine.hpp
 * Description: Manages the rotating pool of display boards.
 *
 * Exported Functions/Classes:
 * - CarouselEngine: Manages the rotating pool of display boards.
 *   - add(): Register a board into the pool (pre-lock only)
 *   - lock(): Lock the pool — no further add() calls accepted
 *   - show(): Force-show a specific pool board (reset rotation timer)
 *   - suspend(): Pause automatic rotation (timer frozen)
 *   - resume(): Resume automatic rotation from current board
 *   - setInterval(): Set the rotation interval in milliseconds
 *   - tick(): Advance carousel if interval elapsed; call each tick
 *   - currentBoard(): Return pointer to the active board, or nullptr
 *   - boardCount(): Return number of boards in the pool
 */

#pragma once

#include <vector>
#include <cstdint>
#include "interfaces/IDisplayBoard.hpp"

class CarouselEngine {
public:
    /**
     * @brief Register a board into the pool (pre-lock only).
     * @param board Pointer to the board to add.
     */
    void add(IDisplayBoard* board);

    /**
     * @brief Lock the pool — no further add() calls accepted.
     */
    void lock();

    /**
     * @brief Force-show a specific pool board (reset rotation timer).
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

    /**
     * @brief Return pointer to the active board, or nullptr.
     * @return Active IDisplayBoard pointer.
     */
    IDisplayBoard* currentBoard() const;

    /**
     * @brief Return number of boards in the pool.
     * @return Number of boards.
     */
    int boardCount() const;

    /**
     * @brief Return a specific board by index.
     * @param index Index of the board.
     * @return Pointer to the board, or nullptr if out of bounds.
     */
    IDisplayBoard* getBoard(int index) const;

    /**
     * @brief Return the index of the currently active board.
     * @return Active index, or -1 if pool is empty.
     */
    int activeIndex() const;

private:
    std::vector<IDisplayBoard*> _pool;       ///< Carousel board pool — populated before lock()
    int _activeIndex = 0;                    ///< Index of the currently rendering board
    unsigned long _lastRotationMs = 0;       ///< Timestamp of last carousel rotation
    unsigned long _intervalMs = 8000;        ///< Rotation interval (default 8 seconds)
    bool _locked = false;                    ///< True after lock() — blocks add()
    bool _suspended = false;                 ///< True when automatic rotation is paused

    /**
     * @brief Advance to next non-expired board; removes expired boards from pool.
     */
    void _advance();
};
