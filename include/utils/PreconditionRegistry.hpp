/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/utils/PreconditionRegistry.hpp
 * Description: Abstract registry interface for registering preconditions in the
 *              framework. Passed to IPreconditionContributor implementors to
 *              register evaluation callbacks, description formatters, and dependency
 *              edges.
 *
 * Exported Classes:
 * - PreconditionRegistry: Abstract interface for precondition registration
 *   - add(): Register a new precondition with evaluation callback and metadata
 */

#pragma once

#include "interfaces/IPrecondition.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>

/**
 * @brief Abstract registry interface for registering preconditions.
 *
 * Passed to IPreconditionContributor::registerPreconditions() during module registration.
 * Concrete implementation is provided by PreconditionEngine in WO2.
 */
class PreconditionRegistry {
public:
    /// Fast-path evaluation callback invoked every tick cycle.
    using EvalFn     = std::function<PreconditionStatus()>;

    /// Cold-path description formatter writing into a caller-supplied buffer.
    using DescribeFn = std::function<void(Cause, char*, size_t)>;

    virtual ~PreconditionRegistry() = default;

    /**
     * @brief Register a precondition with evaluation callback and dependency metadata.
     *
     * @param name       Human-readable unique identifier for the precondition (static lifetime).
     * @param role       Precondition role (Operational, Startup, or Both).
     * @param evaluate   Fast-path evaluation function returning PreconditionStatus.
     * @param describe   Optional cold-path description formatter writing into a buffer.
     * @param edges      Optional pointer to array of dependency edges.
     * @param edgeCount  Number of dependency edges in the edges array.
     */
    virtual void add(const char* name,
                     PreconditionRole role,
                     EvalFn evaluate,
                     DescribeFn describe = nullptr,
                     const Edge* edges = nullptr,
                     uint8_t edgeCount = 0) = 0;
};
