/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/ITickable.hpp
 * Description: Pure interface for components that participate in the Application
 *              main-loop tick cycle. Every registered module that requires
 *              periodic execution must implement this interface.
 *              tick() SHOULD complete within its tick budget (default 10ms;
 *              overrideable via getTickBudgetMs()); long I/O must be deferred to
 *              a FreeRTOS task.
 *
 * Exported Classes:
 * - ITickable: Abstract base defining the tick() contract
 *   - tick(): Called once per Application loop iteration (default ≤ 10ms budget)
 *   - getTickBudgetMs(): Returns maximum expected tick duration in milliseconds (default 10ms)
 */

#pragma once
#include <cstdint>

/**
 * @brief Contract for any component participating in the Application tick loop.
 *
 * Implementors receive a tick() call on every Application::loop() iteration.
 * Implementations SHOULD complete within their tick budget (default 10ms,
 * overrideable via getTickBudgetMs()); any long I/O or blocking call
 * MUST be deferred to a dedicated FreeRTOS task.
 */
class ITickable {
public:
    /** @brief Destructor for ITickable. */
    virtual ~ITickable() = default;

    /**
     * @brief Periodic update hook called by the Application hub on every loop iteration.
     *
     * Implementations must be non-blocking and MUST complete within their tick budget.
     *
     * @param currentMillis  Milliseconds since boot, captured once per Application::tick() loop.
     */
    virtual void tick(uint32_t currentMillis) = 0;

    /**
     * @brief Per-module tick budget in milliseconds.
     *
     * The Application instrumentation loop warns if a module's tick() exceeds
     * this value. The default is 10 ms (ADR-17). Modules with inherently
     * expensive tick work (e.g. display rendering) may override this to
     * declare a higher budget without triggering false-positive warnings.
     *
     * @return Maximum expected tick duration in milliseconds.
     */
    virtual uint32_t getTickBudgetMs() const { return 10; }
};
