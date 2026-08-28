/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IPreconditionContributor.hpp
 * Description: Pure interface for kernel and application modules that contribute
 *              preconditions to the IPreconditionRegistry. Implements the
 *              registry/visitor pattern (CG-2b).
 *
 * Exported Classes:
 * - IPreconditionContributor: Abstract base for modules contributing preconditions
 *   - registerPreconditions(): Register preconditions with the provided IPreconditionRegistry
 *
 * ## Extensibility Guide: Implementing Preconditions
 *
 * Modules that represent readiness conditions (such as WiFi connectivity,
 * API key configuration, sensor hardware initialization, or cloud sync) implement
 * `IPreconditionContributor` and register their preconditions via the visitor pattern:
 *
 * ```cpp
 * #include "interfaces/IPreconditionContributor.hpp"
 * #include "interfaces/IPreconditionRegistry.hpp"
 *
 * class SensorModule : public IPreconditionContributor {
 * public:
 *     void registerPreconditions(IPreconditionRegistry& reg) override {
 *         // 1. Declare any prerequisite dependencies (e.g. WiFi connection)
 *         static const Edge edges[] = {
 *             {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
 *         };
 *
 *         // 2. Register precondition
 *         reg.add("Sensor Config", PreconditionRole::Operational,
 *             [this]() -> PreconditionStatus {
 *                 if (!_sensorConfigured)
 *                     return {false, Cause::NotConfigured};
 *                 if (!_sensorConnected)
 *                     return {false, Cause::HardwareFault};
 *                 return {true, Cause::Met};
 *             },
 *             [](Cause cause, char* buf, size_t len) {
 *                 switch (cause) {
 *                     case Cause::NotConfigured:
 *                         snprintf(buf, len, "Configure sensor in Web Portal");
 *                         break;
 *                     case Cause::HardwareFault:
 *                         snprintf(buf, len, "Check sensor I2C wiring");
 *                         break;
 *                     default:
 *                         snprintf(buf, len, "Sensor not ready");
 *                         break;
 *                 }
 *             },
 *             edges, 1);
 *     }
 * private:
 *     bool _sensorConfigured = false;
 *     bool _sensorConnected = true;
 * };
 * ```
 *
 * ## Registration Model
 *
 * When registering a module via `app.registerModule(&sensorModule)`, the framework's
 * compile-time introspection auto-detects `IPreconditionContributor` and registers
 * its preconditions into the central `PreconditionEngine`.
 *
 * ## Roles
 *
 * - `PreconditionRole::Startup`: Precondition blocks entry into `RUNNING` state until met.
 * - `PreconditionRole::Operational`: Runtime prerequisite. Does not block boot transition.
 * - `PreconditionRole::Both`: Blocks boot AND triggers regression to `CONFIGURING` if suppressed at runtime.
 *
 * ## Precondition Name Stability
 *
 * Precondition names are part of the **public API**. Display modules use string
 * matching against well-known precondition names (e.g., `PreconditionNames::WIFI_STATION`)
 * for built-in modal dispatch. Always use `PreconditionNames::` constants when available.
 */

#pragma once

// Forward declaration to avoid circular dependencies.
// Concrete implementations include "interfaces/IPreconditionRegistry.hpp".
class IPreconditionRegistry;

/**
 * @brief Contract for modules that contribute preconditions to the system.
 *
 * Implementing modules define registerPreconditions() to install one or more
 * preconditions into the IPreconditionRegistry via the visitor pattern (CG-2b).
 * This eliminates the need for inner stub objects and prevents silent omission
 * of preconditions.
 */
class IPreconditionContributor {
public:
    /** @brief Destructor for IPreconditionContributor. */
    virtual ~IPreconditionContributor() = default;

    /**
     * @brief Register all preconditions this module provides.
     *
     * @param registry The IPreconditionRegistry instance to add preconditions to.
     */
    virtual void registerPreconditions(IPreconditionRegistry& registry) = 0;
};
