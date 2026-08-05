/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/utils/ConfigGateResolver.hpp
 * Description: Pure-logic gate resolution utilities. Hardware-free functions
 *              extracted from Application for testability.
 *
 * Exported Functions:
 * - ConfigGateResolver::findActionableGate(): Find the first unsatisfied gate with met dependencies.
 * - ConfigGateResolver::shouldRepublish(): Determine whether gate/status change warrants republish.
 */

#pragma once

#include "interfaces/IConfigGate.hpp"
#include <vector>
#include <cstring>

/**
 * @brief Pure-logic gate resolution utilities.
 *
 * Hardware-free functions extracted from Application for testability.
 * Application delegates to these; the native test lane exercises them directly.
 *
 * @see Application::getActionableGate()
 * @see Application::tick() — gate-transition re-publish detector
 */
namespace ConfigGateResolver {

/**
 * @brief Find the first actionable unsatisfied gate (all dependencies satisfied).
 *
 * Iterates the registered config gates in registration order, skipping satisfied
 * gates. For each unsatisfied gate, checks whether all of its declared
 * dependencies are satisfied by comparing gate names via strcmp().
 *
 * @param gates  Vector of registered config gate pointers.
 * @return Pointer to the first actionable gate, or nullptr if all satisfied
 *         or no gate has its dependencies met.
 */
inline const IConfigGate* findActionableGate(const std::vector<IConfigGate*>& gates) {
    for (const auto* gate : gates) {
        if (gate->isSatisfied()) continue;

        uint8_t depCount = 0;
        const char* const* deps = gate->getDependencies(depCount);
        bool depsOk = true;
        for (uint8_t d = 0; d < depCount; d++) {
            bool found = false;
            for (const auto* other : gates) {
                if (other->getGateName() && strcmp(other->getGateName(), deps[d]) == 0) {
                    if (!other->isSatisfied()) depsOk = false;
                    found = true;
                    break;
                }
            }
            if (!found || !depsOk) { depsOk = false; break; }
        }
        if (depsOk) return gate;
    }
    return nullptr;
}

/**
 * @brief Determine whether a gate-transition re-publish is warranted.
 *
 * @param currentGate   Name of the currently actionable gate (or nullptr).
 * @param currentStatus Current gate's action status.
 * @param lastGate      Name from last publish (or nullptr).
 * @param lastStatus    Status from last publish.
 * @return true if the gate or status has changed since last publish.
 */
inline bool shouldRepublish(
    const char* currentGate, GateActionStatus currentStatus,
    const char* lastGate, GateActionStatus lastStatus)
{
    // Fast-path: same pointer implies same gate (static lifetimes).
    // Falls through to strcmp() when pointers differ but content may match.
    if (currentGate != lastGate) {
        if (lastGate == nullptr || currentGate == nullptr ||
            strcmp(currentGate, lastGate) != 0) {
            return true;
        }
    }
    // Status changed?
    return currentStatus != lastStatus;
}

} // namespace ConfigGateResolver
