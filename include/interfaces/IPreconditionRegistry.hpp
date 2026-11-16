/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/IPreconditionRegistry.hpp
 * Description: Abstract registry interface for registering preconditions in the
 *              framework. Passed to IPreconditionContributor implementors to
 *              register evaluation callbacks, description formatters, and dependency
 *              edges.
 *
 * Exported Classes:
 * - IPreconditionRegistry: Abstract interface for precondition registration
 *   - add(): Register a new precondition with evaluation callback and metadata
 */

#pragma once

#include "interfaces/IPrecondition.hpp"
#include <cstddef>
#include <cstdint>

/**
 * @brief Abstract registry interface for registering preconditions.
 *
 * Passed to IPreconditionContributor::registerPreconditions() during module registration.
 * Concrete implementation is provided by PreconditionEngine.
 */
class IPreconditionRegistry {
public:
    /// Fast-path evaluation callback invoked every tick cycle.
    using EvalFn     = PreconditionStatus (*)(void* ctx);

    /// Cold-path description formatter writing into a caller-supplied buffer.
    using DescribeFn = void (*)(Cause cause, char* buf, size_t len, void* ctx);

    /** @brief Destructor for IPreconditionRegistry. */
    virtual ~IPreconditionRegistry() = default;

    /**
     * @brief Register a precondition with evaluation callback and dependency metadata.
     *
     * @param name       Human-readable unique identifier for the precondition (static lifetime).
     * @param role       Precondition role (Operational, Startup, or Both).
     * @param evaluate   Fast-path evaluation function returning PreconditionStatus.
     * @param describe   Optional cold-path description formatter writing into a buffer.
     * @param context    User context pointer passed to evaluate and describe callbacks.
     * @param edges      Optional pointer to array of dependency edges.
     * @param edgeCount  Number of dependency edges in the edges array.
     * @param damping    Optional per-precondition damping policy override (nullptr = engine default).
     *                   Must have static lifetime — pointer is stored, not copied.
     * @param retry      Optional per-precondition retry policy override (nullptr = engine default).
     *                   Must have static lifetime — pointer is stored, not copied.
     */
    virtual void add(const char* name,
                     PreconditionRole role,
                     EvalFn evaluate,
                     DescribeFn describe = nullptr,
                     void* context = nullptr,
                     const Edge* edges = nullptr,
                     uint8_t edgeCount = 0,
                     const DampingPolicy* damping = nullptr,
                     const RetryPolicy*  retry   = nullptr) = 0;

    /**
     * @brief Convenience overload for registering a precondition with context but no describe callback.
     *
     * @param name       Human-readable unique identifier for the precondition (static lifetime).
     * @param role       Precondition role (Operational, Startup, or Both).
     * @param evaluate   Fast-path evaluation function returning PreconditionStatus.
     * @param context    User context pointer passed to evaluate callback.
     * @param edges      Optional pointer to array of dependency edges.
     * @param edgeCount  Number of dependency edges in the edges array.
     * @param damping    Optional per-precondition damping policy override (nullptr = engine default).
     * @param retry      Optional per-precondition retry policy override (nullptr = engine default).
     */
    void add(const char* name,
             PreconditionRole role,
             EvalFn evaluate,
             void* context,
             const Edge* edges = nullptr,
             uint8_t edgeCount = 0,
             const DampingPolicy* damping = nullptr,
             const RetryPolicy* retry = nullptr) {
        add(name, role, evaluate, nullptr, context, edges, edgeCount, damping, retry);
    }
};
