#pragma once

#include "utils/PreconditionRegistry.hpp"
#include <cstdint>

constexpr size_t MAX_PRECONDITIONS = 16;
constexpr size_t MAX_EDGES_PER_PRECONDITION = 8;

struct PreconditionState {
    const char* name;
    PreconditionRole role;
    PreconditionRegistry::EvalFn evaluate;
    PreconditionRegistry::DescribeFn describe;
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
};

class PreconditionEngine : public PreconditionRegistry {
public:
    void add(const char* name,
             PreconditionRole role,
             EvalFn evaluate,
             DescribeFn describe = nullptr,
             const Edge* edges = nullptr,
             uint8_t edgeCount = 0) override;

    bool resolveEdges();
    void tick(uint32_t nowMs);
    bool allStartupMet() const;
    bool getAppRunning() const;
    size_t getPreconditionCount() const;
    const PreconditionState& getStateAt(size_t i) const;
    const PreconditionState* getStateByName(const char* name) const;
    void bumpIdentityEpoch(const char* name);
    void requestRetry(const char* name);

private:
    PreconditionState preconditions_[MAX_PRECONDITIONS];
    size_t count_ = 0;
    uint32_t lastTickMs_ = 0;
    
    // CG-12 Damping policy parameters. Using defaults from IPrecondition.hpp
    DampingPolicy dampingPolicy_;
    RetryPolicy retryPolicy_;
    
    void updateDamping(PreconditionState& state, float elapsedSec);
    void updateBreaker(PreconditionState& state, uint32_t nowMs);
};
