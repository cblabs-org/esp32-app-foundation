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
 */

#pragma once

// Forward-declare to avoid pulling in U8G2 header in interface files.
// Concrete board classes include the U8G2 header directly.
class U8G2;

/**
 * @brief Contract for a U8G2 carousel display board (system or application).
 *
 * The u8g2Display module maintains a std::vector<IDisplayBoard*> slot pool.
 * On each carousel rotation, the current board's render() is called within
 * the U8G2 draw loop. Boards signal their own expiry via isExpired().
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
     * @brief Query whether this board has expired and should be removed.
     * @return true if the u8g2Display module should evict this board from the pool.
     */
    virtual bool isExpired() const = 0;

    /**
     * @brief Return a human-readable name for logging and diagnostics.
     * @return Null-terminated string board identifier.
     */
    virtual const char* getBoardName() const = 0;
};
