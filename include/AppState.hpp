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
 *              bidirectional model for the Application hub. BOOT covers all
 *              hardware initialisation during setup(). CONFIGURING and RUNNING
 *              support bidirectional transitions: if a config gate becomes
 *              unsatisfied beyond its regression tolerance, the system regresses
 *              from RUNNING back to CONFIGURING.
 *
 * Valid transitions:
 *   BOOT → CONFIGURING   (gates unsatisfied after begin())
 *   BOOT → RUNNING       (all gates satisfied after begin())
 *   CONFIGURING → RUNNING (all gates satisfied during tick())
 *   RUNNING → CONFIGURING (gate regression tolerance exceeded)
 *
 * Exported Enums:
 * - AppState: Lifecycle phase identifier
 *   - BOOT:        Hardware init, module begin() — only valid during setup()
 *   - CONFIGURING: Kernel up, awaiting user configuration via IConfigGate
 *   - RUNNING:     Normal tick() loop; all prerequisites satisfied
 */

#pragma once
#include <stdint.h>

/**
 * @brief Bidirectional application lifecycle phases.
 *
 * The Application hub transitions through these states during startup and
 * runtime. BOOT is the initial hardware-init phase. CONFIGURING and RUNNING
 * support bidirectional transitions: if a registered IConfigGate becomes
 * unsatisfied beyond its regression tolerance, the system regresses from
 * RUNNING back to CONFIGURING. Modules may query the current state via
 * Application::getState() to guard phase-sensitive operations.
 */
enum class AppState : uint8_t {
    BOOT         = 0, ///< Hardware init, module begin() — only valid during setup()
    CONFIGURING  = 1, ///< Kernel up, awaiting user configuration via IConfigGate
    RUNNING      = 2  ///< Normal tick() loop; all prerequisites satisfied
};
