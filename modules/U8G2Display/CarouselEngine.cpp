/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/CarouselEngine.cpp
 * Description:
 *   Runtime-mutable collection of IDisplayBoard pointers driving a rotating
 *   carousel on a U8G2 display.  See CarouselEngine.hpp for the full
 *   description of pool lifecycle, suspend/resume, eviction, and show().
 *
 * Exported Functions:
 *   - add(), remove(), clear(), contains()
 *   - boardCount(), getBoard(), currentBoard(), activeIndex()
 *   - show(), nextBoard(), previousBoard()
 *   - suspend(), resume(), setInterval(), getInterval(), tick()
 */

#include "CarouselEngine.hpp"
#include "platform/LogShim.h"

static constexpr const char* TAG = "CarouselEngine";

// =============================================================================
// Collection mutators
// =============================================================================

/** @brief Add a board to the pool.  Null pointers and duplicates are ignored. */
void CarouselEngine::add(IDisplayBoard* board) {
    if (board == nullptr) return;
    if (contains(board)) {
        ESP_LOGW(TAG, "Board '%s' already in pool — ignoring duplicate", board->getBoardName());
        return;
    }
    _pool.push_back(board);
    ESP_LOGI(TAG, "Added board '%s' (pool size: %d)", board->getBoardName(), (int)_pool.size());
}

/**
 * @brief Remove a specific board from the pool.
 *
 * Adjusts _activeIndex to maintain stable rendering:
 *   - If the removed board is before the active board, decrement index.
 *   - If the removed board IS the active board, the next board slides into
 *     its slot (no advance needed).
 *   - If the pool becomes empty, reset to 0.
 *
 * @return true if found and removed.
 */
bool CarouselEngine::remove(IDisplayBoard* board) {
    if (board == nullptr) return false;

    for (auto it = _pool.begin(); it != _pool.end(); ++it) {
        if (*it == board) {
            int removedIndex = static_cast<int>(it - _pool.begin());
            ESP_LOGI(TAG, "Removed board '%s' (pool size: %d)", board->getBoardName(), (int)_pool.size() - 1);
            _pool.erase(it);

            if (_pool.empty()) {
                _activeIndex = 0;
            } else if (removedIndex < _activeIndex) {
                _activeIndex--;
            } else if (removedIndex == _activeIndex && _activeIndex >= static_cast<int>(_pool.size())) {
                _activeIndex = 0;  // Wrap around
            }
            return true;
        }
    }
    return false;
}

/** @brief Remove all boards from the pool and reset state. */
void CarouselEngine::clear() {
    _pool.clear();
    _activeIndex = 0;
    _lastRotationMs = 0;
    ESP_LOGI(TAG, "Pool cleared");
}

// =============================================================================
// Collection queries
// =============================================================================

/** @brief Check whether a board is registered in the pool. */
bool CarouselEngine::contains(IDisplayBoard* board) const {
    if (board == nullptr) return false;
    for (const auto* b : _pool) {
        if (b == board) return true;
    }
    return false;
}

/** @brief Return the number of boards currently in the pool. */
int CarouselEngine::boardCount() const {
    return (int)_pool.size();
}

/** @brief Return a specific board by index, or nullptr if out of bounds. */
IDisplayBoard* CarouselEngine::getBoard(int index) const {
    if (index < 0 || index >= (int)_pool.size()) {
        return nullptr;
    }
    return _pool[index];
}

/** @brief Return pointer to the active board, or nullptr if pool is empty. */
IDisplayBoard* CarouselEngine::currentBoard() const {
    if (_pool.empty() || _activeIndex < 0 || _activeIndex >= static_cast<int>(_pool.size())) {
        return nullptr;
    }
    return _pool[_activeIndex];
}

/** @brief Return the index of the currently active board, or -1 if pool is empty. */
int CarouselEngine::activeIndex() const {
    if (_pool.empty()) {
        return -1;
    }
    return _activeIndex;
}

// =============================================================================
// Rotation control
// =============================================================================

/** @brief Force-show a specific pool board and reset the rotation timer.
 *         Fires lifecycle hooks when the visible board actually changes. */
void CarouselEngine::show(IDisplayBoard* board, unsigned long currentMs) {
    if (board == nullptr) return;

    for (size_t i = 0; i < _pool.size(); ++i) {
        if (_pool[i] == board) {
            IDisplayBoard* outgoing = currentBoard();
            _activeIndex = static_cast<int>(i);
            _lastRotationMs = currentMs;
            if (outgoing != board) {
                if (outgoing != nullptr) outgoing->onDeactivate();
                board->onActivate();
            }
            break;
        }
    }
}

/**
 * @brief Advance to the next board (user-initiated, bypasses canAdvance()).
 *
 * Fires onDeactivate() on the outgoing board and onActivate() on the incoming
 * board when the visible board actually changes.  Resets the rotation timer.
 */
void CarouselEngine::nextBoard(unsigned long currentMs) {
    if (_pool.empty()) return;
    IDisplayBoard* outgoing = currentBoard();
    _activeIndex = (_activeIndex + 1) % static_cast<int>(_pool.size());
    _lastRotationMs = currentMs;
    IDisplayBoard* incoming = currentBoard();
    if (outgoing != incoming) {
        if (outgoing != nullptr) outgoing->onDeactivate();
        if (incoming != nullptr) incoming->onActivate();
    }
}

/**
 * @brief Return to the previous board (user-initiated, bypasses canAdvance()).
 *
 * Fires onDeactivate() on the outgoing board and onActivate() on the incoming
 * board when the visible board actually changes.  Resets the rotation timer.
 */
void CarouselEngine::previousBoard(unsigned long currentMs) {
    if (_pool.empty()) return;
    IDisplayBoard* outgoing = currentBoard();
    _activeIndex = (_activeIndex - 1 + static_cast<int>(_pool.size()))
                   % static_cast<int>(_pool.size());
    _lastRotationMs = currentMs;
    IDisplayBoard* incoming = currentBoard();
    if (outgoing != incoming) {
        if (outgoing != nullptr) outgoing->onDeactivate();
        if (incoming != nullptr) incoming->onActivate();
    }
}

/** @brief Pause automatic rotation (timer frozen). */
void CarouselEngine::suspend() {
    _suspended = true;
}

/** @brief Resume automatic rotation from the current board. */
void CarouselEngine::resume() {
    _suspended = false;
}

/** @brief Set the rotation interval in milliseconds. */
void CarouselEngine::setInterval(unsigned long ms) {
    _intervalMs = ms;
}

/** @brief Get the rotation interval in milliseconds. */
unsigned long CarouselEngine::getInterval() const {
    return _intervalMs;
}

/** @brief Set the bounded max-hold ceiling for canAdvance() defers. */
void CarouselEngine::setMaxHoldMs(unsigned long ms) {
    _maxHoldMs = ms;
}

/** @brief Get the bounded max-hold ceiling for canAdvance() defers. */
unsigned long CarouselEngine::getMaxHoldMs() const {
    return _maxHoldMs;
}

/**
 * @brief Advance carousel if interval elapsed; call from the main loop each tick.
 *
 * Once the interval elapses, the active board is asked whether it may be
 * advanced away from via canAdvance(). While it returns false, rotation is
 * deferred (re-checked every tick) — but expired boards are still evicted,
 * since a hold must not block eviction. The hold is bounded: once it exceeds
 * getInterval() + getMaxHoldMs(), rotation is forced anyway and a warning is
 * logged, so a board that never releases cannot wedge the display.
 */
void CarouselEngine::tick(unsigned long currentMs) {
    // --- Step 1: Check suspension and interval elapsed state ---
    if (_suspended) return;

    unsigned long heldMs = currentMs - _lastRotationMs;
    if (heldMs < _intervalMs) return;

    // --- Step 2: Query active board permission to advance via canAdvance() with bounded hold timeout ---
    IDisplayBoard* active = currentBoard();
    if (active != nullptr && !active->canAdvance()) {
        if (heldMs <= _intervalMs + _maxHoldMs) {
            // Bounded hold: defer rotation, but eviction still runs.
            _evictExpired();
            return;
        }
        ESP_LOGW(TAG, "Board '%s' held past max hold (%lu ms) — forcing rotation",
                 active->getBoardName(), _maxHoldMs);
    }

    // --- Step 3: Advance carousel to next valid board and update rotation timestamp ---
    _advance();
    _lastRotationMs = currentMs;
}

// =============================================================================
// Private
// =============================================================================

/**
 * @brief Evict expired boards from the pool; keeps _activeIndex stable.
 *
 * Walks the pool and erases every board whose isExpired() returns true,
 * adjusting _activeIndex so it continues to reference the same board.
 *
 * @return true if the board at _activeIndex was itself evicted.
 */
bool CarouselEngine::_evictExpired() {
    bool activeRemoved = false;

    // --- Step 1: Iterate pool and erase boards reporting expired status ---
    for (auto it = _pool.begin(); it != _pool.end(); ) {
        if ((*it)->isExpired()) {
            int removedIndex = static_cast<int>(it - _pool.begin());
            ESP_LOGI(TAG, "Removing expired board: %s", (*it)->getBoardName());
            it = _pool.erase(it);

            // --- Step 2: Adjust active board index to maintain reference stability across removals ---
            if (removedIndex < _activeIndex) {
                _activeIndex--;
            } else if (removedIndex == _activeIndex) {
                activeRemoved = true;
                // Clamp so _activeIndex stays within bounds after removal
                if (_activeIndex >= static_cast<int>(_pool.size()) && !_pool.empty()) {
                    _activeIndex = 0;
                }
            }
        } else {
            ++it;
        }
    }

    // --- Step 3: Handle empty pool boundary condition by resetting index ---
    if (_pool.empty()) {
        _activeIndex = 0;
    }

    return activeRemoved;
}

/**
 * @brief Advance to the next non-expired board; evicts expired boards first.
 *
 * If the active board itself was removed, the index now points to the
 * board that slid into its slot and we do NOT advance further — this
 * prevents skipping a board.
 */
void CarouselEngine::_advance() {
    if (_pool.empty()) return;

    IDisplayBoard* outgoing = currentBoard();

    bool activeRemoved = _evictExpired();

    if (_pool.empty()) return;

    // Only advance if the active board was NOT the one removed.
    // When the active board is evicted the next board has already slid
    // into its slot, so advancing would skip it.
    if (!activeRemoved) {
        _activeIndex = (_activeIndex + 1) % _pool.size();
    }

    // Fire lifecycle hooks if the visible board actually changed
    IDisplayBoard* incoming = currentBoard();
    if (outgoing != incoming) {
        if (outgoing != nullptr) outgoing->onDeactivate();
        if (incoming != nullptr) incoming->onActivate();
    }
}
