/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/types/AppState.hpp
 * Description: Application lifecycle state machine enum. Defines a 3-state
 *              bidirectional model for the Application hub. Boot covers all
 *              hardware initialisation during setup(). Configuring and Running
 *              support bidirectional transitions: if a startup precondition becomes
 *              suppressed, the system regresses from Running back to Configuring.
 *
 * Valid transitions:
 *   Boot → Configuring   (startup preconditions unsatisfied after begin())
 *   Boot → Running       (all startup preconditions satisfied after begin())
 *   Configuring → Running (all startup preconditions satisfied during tick())
 *   Running → Configuring (startup precondition suppressed)
 *
 * Exported Enums:
 * - AppState: Lifecycle phase identifier
 *   - Boot:        Hardware init, module begin() — only valid during setup()
 *   - Configuring: Kernel up, awaiting user configuration / preconditions
 *   - Running:     Normal tick() loop; all prerequisites satisfied
 */

#pragma once
#include <stdint.h>

/**
 * @brief Bidirectional application lifecycle phases.
 *
 * The Application hub transitions through these states during startup and
 * runtime. Boot is the initial hardware-init phase. Configuring and Running
 * support bidirectional transitions: if a registered startup precondition becomes
 * suppressed, the system regresses from Running back to Configuring. Modules
 * may query the current state via Application::getState() to guard
 * phase-sensitive operations.
 */
enum class AppState : uint8_t {
    Boot         = 0, ///< Hardware init, module begin() — only valid during setup()
    Configuring  = 1, ///< Kernel up, awaiting user configuration / preconditions
    Running      = 2  ///< Normal tick() loop; all prerequisites satisfied
};
