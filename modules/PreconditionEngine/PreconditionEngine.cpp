/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/PreconditionEngine/PreconditionEngine.cpp
 * Description: PreconditionEngine implementation.
 *
 */

#include "PreconditionEngine.hpp"
#include "platform/LogShim.h"
#include <cstring>
#include <cmath>
#include <cassert>

static const char* TAG = "PreconditionEngine";

void PreconditionEngine::add(const char* name,
                             PreconditionRole role,
                             EvalFn evaluate,
                             DescribeFn describe,
                             void* context,
                             const Edge* edges,
                             uint8_t edgeCount,
                             const DampingPolicy* damping,
                             const RetryPolicy*  retry) {
    if (_count >= MAX_PRECONDITIONS) {
        ESP_LOGE(TAG, "Cannot add %s: MAX_PRECONDITIONS reached", name);
        return;
    }
    auto& p = _preconditions[_count++];
    p.name = name;
    p.role = role;
    p.evaluate = evaluate;
    p.describe = describe;
    p.context = context;
    p.edges = edges;
    p.edgeCount = edgeCount;
    p.resolvedEdgeCount = 0;
    for (uint8_t i = 0; i < MAX_EDGES_PER_PRECONDITION; ++i) {
        p.prevUpstreamMet[i] = false;
    }
    
    p.rawStatus = {false, Cause::NotConfigured};
    p.dampedStatus = {false, Cause::NotConfigured};
    p.severity = Severity::Transient;
    p.everMet = false;
    p.identityEpoch = 0;
    
    p.penalty = 0.0f;
    p.suppressed = false;
    
    p.breaker = BreakerState::Closed;
    p.attemptCount = 0;
    p.lastAttemptMs = 0;
    p.dampingOverride = damping;
    p.retryOverride = retry;
    const RetryPolicy& rp = retry ? *retry : _retryPolicy;
    p.cooldownMs = rp.baseDelayMs;
}

bool PreconditionEngine::resolveEdges() {
    bool ok = true;
    for (size_t i = 0; i < _count; ++i) {
        auto& p = _preconditions[i];
        p.resolvedEdgeCount = 0;
        for (uint8_t e = 0; e < p.edgeCount; ++e) {
            bool found = false;
            for (size_t j = 0; j < _count; ++j) {
                if (strcmp(_preconditions[j].name, p.edges[e].name) == 0) {
                    if (p.resolvedEdgeCount < MAX_EDGES_PER_PRECONDITION) {
                        p.resolvedEdges[p.resolvedEdgeCount] = static_cast<uint8_t>(j);
                        p.resolvedEdgeKinds[p.resolvedEdgeCount] = p.edges[e].kind;
                        p.resolvedEdgeCount++;
                        found = true;
                    }
                    break;
                }
            }
            if (!found) {
                ESP_LOGE(TAG, "Unknown edge name '%s' requested by '%s'", p.edges[e].name, p.name);
                p.resolvedEdgeCount = 0;
                ok = false;
                break;
            }
        }
    }

    if (!ok) return false;

    uint8_t visited[MAX_PRECONDITIONS] = {0};
    
    auto dfs = [&](auto& self, size_t node) -> bool {
        if (visited[node] == 1) return true;
        if (visited[node] == 2) return false;
        
        visited[node] = 1;
        const auto& p = _preconditions[node];
        for (uint8_t e = 0; e < p.resolvedEdgeCount; ++e) {
            if (self(self, p.resolvedEdges[e])) {
                ESP_LOGE(TAG, "Cycle detected involving '%s'", p.name);
                return true;
            }
        }
        visited[node] = 2;
        return false;
    };

    for (size_t i = 0; i < _count; ++i) {
        if (visited[i] == 0) {
            if (dfs(dfs, i)) {
                ok = false;
            }
        }
    }

    if (ok) {
        for (size_t i = 0; i < _count; ++i) {
            const auto& p = _preconditions[i];
            if (p.edgeCount == 0) {
                ESP_LOGI(TAG, "Graph: [%s]", p.name);
            } else {
                for (uint8_t e = 0; e < p.resolvedEdgeCount; ++e) {
                    ESP_LOGI(TAG, "Graph: [%s] -> [%s]", p.name, _preconditions[p.resolvedEdges[e]].name);
                }
            }
        }
    }

    return ok;
}

void PreconditionEngine::tick(uint32_t nowMs) {
    float elapsedSec = 0.0f;
    if (_lastTickMs > 0 && nowMs > _lastTickMs) {
        elapsedSec = (nowMs - _lastTickMs) / 1000.0f;
    } else if (nowMs < _lastTickMs) {
        elapsedSec = 0.0f;
    }
    _lastTickMs = nowMs;

    for (size_t i = 0; i < _count; ++i) {
        auto& p = _preconditions[i];
        
        // Step 1: Evaluate the precondition's own function first.
        if (p.evaluate) {
            p.rawStatus = p.evaluate(p.context);
        } else {
            p.rawStatus = {false, Cause::InternalError};
        }

        // Step 2: Walk edges and overlay dependency constraints.
        bool dependencyUnmet = false;
        bool bindsToBecameUnmet = false;

        for (uint8_t e = 0; e < p.resolvedEdgeCount; ++e) {
            uint8_t idx = p.resolvedEdges[e];
            EdgeKind kind = p.resolvedEdgeKinds[e];
            const auto& upstream = _preconditions[idx];
            
            bool upstreamMet = upstream.rawStatus.met;
            bool wasMet = p.prevUpstreamMet[e];
            p.prevUpstreamMet[e] = upstreamMet;
            
            if (kind == EdgeKind::Requires || kind == EdgeKind::BindsTo) {
                if (!upstreamMet) {
                    dependencyUnmet = true;
                }
                // BindsTo teardown: upstream WAS met last tick but is unmet now
                if (kind == EdgeKind::BindsTo && wasMet && !upstreamMet) {
                    bindsToBecameUnmet = true;
                }
            }
        }

        // Step 3: Override rawStatus based on dependency evaluation.
        if (dependencyUnmet) {
            p.rawStatus.met = false;
            p.rawStatus.cause = Cause::DependencyUnmet;
        }
        if (bindsToBecameUnmet) {
            p.rawStatus.met = false;
            p.rawStatus.cause = Cause::DependencyUnmet;
        }

        if (p.rawStatus.met) {
            p.everMet = true;
        }

        p.severity = deriveSeverity(p.rawStatus.cause, p.everMet);

        updateDamping(p, elapsedSec);
        updateBreaker(p, nowMs);

        if (p.suppressed) {
            p.dampedStatus.met = false;
            p.dampedStatus.cause = p.rawStatus.cause;
        } else {
            p.dampedStatus = p.rawStatus;
        }
    }
}

void PreconditionEngine::updateDamping(PreconditionState& state, float elapsedSec) {
    if (!state.everMet) return;

    // Per-precondition override (#154) or engine-wide default
    const DampingPolicy& dp = state.dampingOverride ? *state.dampingOverride : _dampingPolicy;

    float weight = 1.0f;
    switch (state.severity) {
        case Severity::NeedsUser: weight = 5.0f; break;
        case Severity::Persistent: weight = 5.0f; break;
        case Severity::Transient: weight = 1.0f; break;
        case Severity::Blocked: weight = 0.0f; break;
        case Severity::Degraded: weight = 1.0f; break;
        case Severity::Inactive: weight = 0.0f; break;
    }

    if (state.rawStatus.met) {
        if (state.penalty > 0.0f && elapsedSec > 0.0f) {
            float halfLifeSec = dp.halfLifeMs / 1000.0f;
            if (halfLifeSec > 0.0f) {
                state.penalty = state.penalty * std::pow(0.5f, elapsedSec / halfLifeSec);
            }
        }
    } else {
        state.penalty += dp.penaltyPerSecond * weight * elapsedSec;
    }
    
    float maxPenalty = dp.suppressThreshold * 2.0f;
    if (state.penalty > maxPenalty) {
        state.penalty = maxPenalty;
    }
    
    if (!state.suppressed && state.penalty > dp.suppressThreshold) {
        state.suppressed = true;
    } else if (state.suppressed && state.penalty < dp.reuseThreshold) {
        state.suppressed = false;
    }
}

void PreconditionEngine::updateBreaker(PreconditionState& state, uint32_t nowMs) {
    // Per-precondition override (#154) or engine-wide default
    const RetryPolicy& rp = state.retryOverride ? *state.retryOverride : _retryPolicy;

    if (state.rawStatus.met) {
        state.breaker = BreakerState::Closed;
        state.attemptCount = 0;
        state.cooldownMs = rp.baseDelayMs;
        state.lastAttemptMs = nowMs;
        return;
    }

    if (state.rawStatus.cause == Cause::DependencyUnmet) {
        state.lastAttemptMs = nowMs; // prevent timeouts while blocked
        return;
    }

    if (state.lastAttemptMs == 0) {
        state.lastAttemptMs = nowMs;
    }

    if (state.breaker == BreakerState::Closed) {
        if (nowMs - state.lastAttemptMs >= state.cooldownMs) {
            state.attemptCount++;
            state.lastAttemptMs = nowMs;
            
            if (state.attemptCount >= rp.attemptsBeforeFallback) {
                state.breaker = BreakerState::Open;
                state.cooldownMs = rp.baseDelayMs;
            }
        }
    } else if (state.breaker == BreakerState::Open) {
        if (nowMs - state.lastAttemptMs >= state.cooldownMs) {
            state.breaker = BreakerState::HalfOpen;
            state.lastAttemptMs = nowMs;
        }
    } else if (state.breaker == BreakerState::HalfOpen) {
        if (nowMs - state.lastAttemptMs >= rp.baseDelayMs) {
            // Failure in HalfOpen
            state.breaker = BreakerState::Open;
            state.lastAttemptMs = nowMs;
            
            uint32_t nextCooldown = state.cooldownMs * 2;
            if (nextCooldown > rp.ceilingMs) {
                nextCooldown = rp.ceilingMs;
            }
            state.cooldownMs = nextCooldown;
        }
    }
}

bool PreconditionEngine::allStartupMet() const {
    for (size_t i = 0; i < _count; ++i) {
        if (_preconditions[i].role == PreconditionRole::Startup || _preconditions[i].role == PreconditionRole::Both) {
            if (!_preconditions[i].dampedStatus.met) return false;
        }
    }
    return true;
}

bool PreconditionEngine::getAppRunning() const {
    return allStartupMet();
}

size_t PreconditionEngine::getPreconditionCount() const {
    return _count;
}

const PreconditionState& PreconditionEngine::getStateAt(size_t i) const {
    assert(i < _count);
    return _preconditions[i];
}

const PreconditionState* PreconditionEngine::getStateByName(const char* name) const {
    for (size_t i = 0; i < _count; ++i) {
        if (strcmp(_preconditions[i].name, name) == 0) {
            return &_preconditions[i];
        }
    }
    return nullptr;
}

void PreconditionEngine::bumpIdentityEpoch(const char* name) {
    for (size_t i = 0; i < _count; ++i) {
        if (strcmp(_preconditions[i].name, name) == 0) {
            _preconditions[i].identityEpoch++;
            _preconditions[i].everMet = false;
            break;
        }
    }
}

void PreconditionEngine::requestRetry(const char* name) {
    for (size_t i = 0; i < _count; ++i) {
        if (strcmp(_preconditions[i].name, name) == 0) {
            _preconditions[i].breaker = BreakerState::HalfOpen;
            _preconditions[i].lastAttemptMs = 0; // force immediate retry
            _preconditions[i].penalty = 0.0f;
            _preconditions[i].suppressed = false;
            break;
        }
    }
}
