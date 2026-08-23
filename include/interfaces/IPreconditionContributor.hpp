/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IPreconditionContributor.hpp
 * Description: Pure interface for kernel and application modules that contribute
 *              preconditions to the PreconditionRegistry. Implements the
 *              registry/visitor pattern (CG-2b).
 *
 * Exported Classes:
 * - IPreconditionContributor: Abstract base for modules contributing preconditions
 *   - registerPreconditions(): Register preconditions with the provided PreconditionRegistry
 */

#pragma once

// Forward declaration to avoid circular dependencies.
// Concrete implementations include "utils/PreconditionRegistry.hpp".
class PreconditionRegistry;

/**
 * @brief Contract for modules that contribute preconditions to the system.
 *
 * Implementing modules define registerPreconditions() to install one or more
 * preconditions into the PreconditionRegistry via the visitor pattern (CG-2b).
 * This eliminates the need for inner stub objects and prevents silent omission
 * of preconditions.
 */
class IPreconditionContributor {
public:
    virtual ~IPreconditionContributor() = default;

    /**
     * @brief Register all preconditions this module provides.
     *
     * @param registry The PreconditionRegistry instance to add preconditions to.
     */
    virtual void registerPreconditions(PreconditionRegistry& registry) = 0;
};
