/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/AppState.hpp
 * Description: Application lifecycle state machine enum. Defines a 3-state
 *              bidirectional model for the Application hub. Boot covers all
 *              hardware initialisation during setup(). Configuring and Running
 *              support bidirectional transitions: if a config gate becomes
 *              unsatisfied beyond its regression tolerance, the system regresses
 *              from Running back to Configuring.
 *
 * Valid transitions:
 *   Boot → Configuring   (gates unsatisfied after begin())
 *   Boot → Running       (all gates satisfied after begin())
 *   Configuring → Running (all gates satisfied during tick())
 *   Running → Configuring (gate regression tolerance exceeded)
 *
 * Exported Enums:
 * - AppState: Lifecycle phase identifier
 *   - Boot:        Hardware init, module begin() — only valid during setup()
 *   - Configuring: Kernel up, awaiting user configuration via IConfigGate
 *   - Running:     Normal tick() loop; all prerequisites satisfied
 */

#pragma once
#include <stdint.h>

/**
 * @brief Bidirectional application lifecycle phases.
 *
 * The Application hub transitions through these states during startup and
 * runtime. Boot is the initial hardware-init phase. Configuring and Running
 * support bidirectional transitions: if a registered IConfigGate becomes
 * unsatisfied beyond its regression tolerance, the system regresses from
 * Running back to Configuring. Modules may query the current state via
 * Application::getState() to guard phase-sensitive operations.
 */
enum class AppState : uint8_t {
    Boot         = 0, ///< Hardware init, module begin() — only valid during setup()
    Configuring  = 1, ///< Kernel up, awaiting user configuration via IConfigGate
    Running      = 2  ///< Normal tick() loop; all prerequisites satisfied
};
