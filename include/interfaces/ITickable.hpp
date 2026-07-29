/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/ITickable.hpp
 * Description: Pure interface for components that participate in the Application
 *              main-loop tick cycle. Every registered module that requires
 *              periodic execution must implement this interface.
 *              tick() MUST complete in ≤ 10ms; long I/O must be deferred to
 *              a FreeRTOS task.
 *
 * Exported Classes:
 * - ITickable: Abstract base defining the tick() contract
 *   - tick(): Called once per Application loop iteration (mandatory ≤ 10ms budget)
 */

#pragma once
#include <cstdint>

/**
 * @brief Contract for any component participating in the Application tick loop.
 *
 * Implementors receive a tick() call on every Application::loop() iteration.
 * Implementations MUST complete within 10ms; any long I/O or blocking call
 * MUST be deferred to a dedicated FreeRTOS task.
 */
class ITickable {
public:
    virtual ~ITickable() = default;

    /**
     * @brief Periodic update hook called by the Application hub on every loop iteration.
     *
     * Implementations must be non-blocking and MUST complete within 10ms.
     *
     * @param currentMillis  Milliseconds since boot, captured once per Application::tick() loop.
     */
    virtual void tick(uint32_t currentMillis) = 0;
};
