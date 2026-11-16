/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/PreconditionEngine/PreconditionEngine.hpp
 * Description: Implementation of PreconditionEngine class.
 *
 * Exported Classes:
 * - PreconditionEngine
 * - PreconditionState
 */

#pragma once

#include "interfaces/IPreconditionRegistry.hpp"
#include <cstdint>

constexpr size_t MAX_PRECONDITIONS = 16;                    ///< Maximum preconditions
constexpr size_t MAX_EDGES_PER_PRECONDITION = 8;            ///< Maximum edges per precondition

struct PreconditionState {
    const char* name;
    PreconditionRole role;
    IPreconditionRegistry::EvalFn evaluate;
    IPreconditionRegistry::DescribeFn describe;
    void* context = nullptr;
    const Edge* edges;
    uint8_t edgeCount;
    // Resolved edge indices (CG-7)
    uint8_t resolvedEdges[MAX_EDGES_PER_PRECONDITION];
    EdgeKind resolvedEdgeKinds[MAX_EDGES_PER_PRECONDITION];
    bool prevUpstreamMet[MAX_EDGES_PER_PRECONDITION];
    uint8_t resolvedEdgeCount = 0;
    // Runtime state
    PreconditionStatus rawStatus;         // undamped
    PreconditionStatus dampedStatus;      // damped
    Severity severity = Severity::Transient;
    bool everMet = false;                 // CG-4a trust history
    uint32_t identityEpoch = 0;           // CG-4a: bumped when target changes
    // Damping state (CG-12)
    float penalty = 0.0f;
    bool suppressed = false;
    // Breaker state (CG-11)
    BreakerState breaker = BreakerState::Closed;
    uint8_t attemptCount = 0;
    uint32_t lastAttemptMs = 0;
    uint32_t cooldownMs = 0;
    // Per-precondition policy overrides (#154). nullptr = use engine defaults.
    const DampingPolicy* dampingOverride = nullptr;
    const RetryPolicy*   retryOverride   = nullptr;
};

class PreconditionEngine : public IPreconditionRegistry {
public:
    /** @brief Register a precondition with evaluation function, description, and options. */
    void add(const char* name,
             PreconditionRole role,
             EvalFn evaluate,
             DescribeFn describe = nullptr,
             void* context = nullptr,
             const Edge* edges = nullptr,
             uint8_t edgeCount = 0,
             const DampingPolicy* damping = nullptr,
             const RetryPolicy*  retry   = nullptr) override;

    /** @brief Resolve Edges. */
    bool resolveEdges();
    /** @brief Periodic tick handler. */
    void tick(uint32_t nowMs);
    /** @brief All Startup Met. */
    bool allStartupMet() const;
    /** @brief Get the app running. */
    bool getAppRunning() const;
    /** @brief Get the precondition count. */
    size_t getPreconditionCount() const;
    /** @brief Get the state at. */
    const PreconditionState& getStateAt(size_t i) const;
    /** @brief Get the state by name. */
    const PreconditionState* getStateByName(const char* name) const;
    /** @brief Bump Identity Epoch. */
    void bumpIdentityEpoch(const char* name);
    /** @brief Request Retry. */
    void requestRetry(const char* name);

private:
    PreconditionState _preconditions[MAX_PRECONDITIONS];
    size_t _count = 0;
    uint32_t _lastTickMs = 0;
    
    // CG-12 Damping policy parameters. Using defaults from IPrecondition.hpp
    DampingPolicy _dampingPolicy;
    RetryPolicy _retryPolicy;
    
    void updateDamping(PreconditionState& state, float elapsedSec);
    void updateBreaker(PreconditionState& state, uint32_t nowMs);
};
