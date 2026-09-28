/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_unit/test_precondition_engine/test_precondition_engine.cpp
 * Description: Implementation of PreconditionEngineTest class.
 */

#include <gtest/gtest.h>
#include "PreconditionEngine.hpp"

class PreconditionEngineTest : public ::testing::Test {
protected:
    PreconditionEngine engine;
    uint32_t nowMs = 10000;
};

TEST_F(PreconditionEngineTest, RegistrationAndEdgeResolution) {
    engine.add("A", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; });
    engine.add("B", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; });
    
    Edge edges[] = {{"A", EdgeKind::Requires}};
    engine.add("C", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; }, nullptr, nullptr, edges, 1);
    
    EXPECT_TRUE(engine.resolveEdges());
    EXPECT_EQ(engine.getPreconditionCount(), 3);
    
    const PreconditionState* cState = engine.getStateByName("C");
    EXPECT_NE(cState, nullptr);
    EXPECT_EQ(cState->resolvedEdgeCount, 1);
    EXPECT_EQ(cState->resolvedEdges[0], 0); // Index of "A"
}

TEST_F(PreconditionEngineTest, EdgeResolutionUnknownRejection) {
    Edge edges[] = {{"Unknown", EdgeKind::Requires}};
    engine.add("A", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; }, nullptr, nullptr, edges, 1);
    EXPECT_FALSE(engine.resolveEdges());
}

TEST_F(PreconditionEngineTest, EdgeResolutionCycleDetection) {
    Edge edgesA[] = {{"B", EdgeKind::Requires}};
    Edge edgesB[] = {{"A", EdgeKind::Requires}};
    
    engine.add("A", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; }, nullptr, nullptr, edgesA, 1);
    engine.add("B", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; }, nullptr, nullptr, edgesB, 1);
    
    EXPECT_FALSE(engine.resolveEdges());
}

TEST_F(PreconditionEngineTest, BasicEvaluation) {
    bool met = false;
    engine.add("A", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::NotConfigured};
    }, nullptr, &met);
    EXPECT_TRUE(engine.resolveEdges());
    
    engine.tick(nowMs);
    const PreconditionState* aState = engine.getStateByName("A");
    EXPECT_FALSE(aState->rawStatus.met);
    
    met = true;
    nowMs += 1000;
    engine.tick(nowMs);
    aState = engine.getStateByName("A");
    EXPECT_TRUE(aState->rawStatus.met);
}

TEST_F(PreconditionEngineTest, EdgePropagation) {
    bool aMet = false;
    engine.add("A", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::NotConfigured};
    }, nullptr, &aMet);
    
    Edge edgesB[] = {{"A", EdgeKind::Requires}};
    engine.add("B", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; }, nullptr, nullptr, edgesB, 1);

    EXPECT_TRUE(engine.resolveEdges());
    engine.tick(nowMs);

    const PreconditionState* bState = engine.getStateByName("B");
    EXPECT_FALSE(bState->rawStatus.met);
    EXPECT_EQ(bState->rawStatus.cause, Cause::DependencyUnmet);

    aMet = true;
    nowMs += 1000;
    engine.tick(nowMs);
    bState = engine.getStateByName("B");
    EXPECT_TRUE(bState->rawStatus.met);
}

TEST_F(PreconditionEngineTest, SeverityDerivation) {
    EXPECT_EQ(deriveSeverity(Cause::Met, false), Severity::Transient);
    EXPECT_EQ(deriveSeverity(Cause::Unreachable, false), Severity::NeedsUser);
    EXPECT_EQ(deriveSeverity(Cause::Unreachable, true), Severity::Transient);
    EXPECT_EQ(deriveSeverity(Cause::Implausible, false), Severity::NeedsUser);
    EXPECT_EQ(deriveSeverity(Cause::Implausible, true), Severity::Transient);
    EXPECT_EQ(deriveSeverity(Cause::NotConfigured, false), Severity::NeedsUser);
    EXPECT_EQ(deriveSeverity(Cause::DependencyUnmet, false), Severity::Blocked);
    EXPECT_EQ(deriveSeverity(Cause::Indeterminate, false), Severity::Persistent);
}

TEST_F(PreconditionEngineTest, TrustHistory) {
    bool met = false;
    engine.add("A", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::NotConfigured};
    }, nullptr, &met);
    EXPECT_TRUE(engine.resolveEdges());
    
    engine.tick(nowMs);
    const PreconditionState* aState = engine.getStateByName("A");
    EXPECT_FALSE(aState->everMet);
    
    met = true;
    engine.tick(nowMs += 1000);
    aState = engine.getStateByName("A");
    EXPECT_TRUE(aState->everMet);
    
    engine.bumpIdentityEpoch("A");
    aState = engine.getStateByName("A");
    EXPECT_FALSE(aState->everMet);
    EXPECT_EQ(aState->identityEpoch, 1);
}

TEST_F(PreconditionEngineTest, Damping) {
    bool met = false;
    engine.add("A", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::NotConfigured};
    }, nullptr, &met); // weight 5
    EXPECT_TRUE(engine.resolveEdges());
    engine.tick(nowMs); // init lastTickMs
    
    // 1. Never met -> zero penalty
    nowMs += 1000;
    engine.tick(nowMs);
    const PreconditionState* aState = engine.getStateByName("A");
    EXPECT_EQ(aState->penalty, 0.0f);
    EXPECT_FALSE(aState->everMet);
    
    // Meet it
    met = true;
    nowMs += 1000;
    engine.tick(nowMs);
    aState = engine.getStateByName("A");
    EXPECT_TRUE(aState->everMet);
    EXPECT_TRUE(aState->rawStatus.met);
    
    // 2. Regresses -> accrues penalty
    met = false;
    nowMs += 1000;
    engine.tick(nowMs);
    aState = engine.getStateByName("A");
    EXPECT_GT(aState->penalty, 0.0f);
    
    // Accumulate to cap
    for(int i=0; i<41; i++) {
        nowMs += 1000;
        engine.tick(nowMs);
    }
    aState = engine.getStateByName("A");
    EXPECT_TRUE(aState->suppressed);
    
    // 3. Cap
    EXPECT_LE(aState->penalty, 4000.0f); // suppressThreshold (2000) * 2 = 4000
    
    // 4. Recovery takes bounded time (half life 500ms)
    met = true;
    nowMs += 1000;
    engine.tick(nowMs);
    aState = engine.getStateByName("A");
    EXPECT_LT(aState->penalty, 4000.0f);
    
    for(int i=0; i<6; i++) {
        nowMs += 500000;
        engine.tick(nowMs);
    }
    aState = engine.getStateByName("A");
    EXPECT_FALSE(aState->suppressed);
    
    // 5. requestRetry clears penalty and suppressed
    met = false;
    nowMs += 1000;
    engine.tick(nowMs);
    for(int i=0; i<41; i++) {
        nowMs += 1000;
        engine.tick(nowMs);
    }
    aState = engine.getStateByName("A");
    EXPECT_TRUE(aState->suppressed);
    EXPECT_GT(aState->penalty, 0.0f);
    
    engine.requestRetry("A");
    aState = engine.getStateByName("A");
    EXPECT_FALSE(aState->suppressed);
    EXPECT_EQ(aState->penalty, 0.0f);
}

TEST_F(PreconditionEngineTest, Breaker) {
    bool met = false;
    engine.add("A", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, Cause::Unreachable};
    }, nullptr, &met);
    EXPECT_TRUE(engine.resolveEdges());
    
    engine.tick(nowMs);
    const PreconditionState* aState = engine.getStateByName("A");
    EXPECT_EQ(aState->breaker, BreakerState::Closed);
    
    // Wait attemptsBeforeFallback * baseDelayMs (5 * 5000 = 25000)
    for(int i=0; i<5; i++) {
        nowMs += 5000;
        engine.tick(nowMs);
    }
    nowMs += 1000;
    engine.tick(nowMs);
    aState = engine.getStateByName("A");
    EXPECT_EQ(aState->breaker, BreakerState::Open);
    
    // HalfOpen after baseDelayMs
    nowMs += 5000;
    engine.tick(nowMs);
    aState = engine.getStateByName("A");
    EXPECT_EQ(aState->breaker, BreakerState::HalfOpen);
    
    // requestRetry overrides
    engine.requestRetry("A");
    aState = engine.getStateByName("A");
    EXPECT_EQ(aState->breaker, BreakerState::HalfOpen);
}

TEST_F(PreconditionEngineTest, Projection) {
    engine.add("A", PreconditionRole::Operational, [](void*) { return PreconditionStatus{false, Cause::NotConfigured}; });
    engine.add("B", PreconditionRole::Startup, [](void*) { return PreconditionStatus{true, Cause::Met}; });
    EXPECT_TRUE(engine.resolveEdges());
    engine.tick(nowMs);
    
    EXPECT_TRUE(engine.allStartupMet());
}

TEST_F(PreconditionEngineTest, CG8Invariant) {
    // Satisfaction propagates immediately (undamped); penalty never propagates across edges.
    bool aMet = false;
    engine.add("A", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::NotConfigured};
    }, nullptr, &aMet);
    Edge edges[] = {{"A", EdgeKind::Requires}};
    engine.add("B", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; }, nullptr, nullptr, edges, 1);
    
    EXPECT_TRUE(engine.resolveEdges());
    
    // Tick while unmet to build up penalty in A
    for(int i=0; i<50; i++) {
        engine.tick(nowMs);
        nowMs += 1000;
    }
    
    const PreconditionState* bState = engine.getStateByName("B");
    EXPECT_EQ(bState->penalty, 0.0f); // B should not accrue penalty due to DependencyUnmet
    
    // Satisfy A
    aMet = true;
    nowMs += 1000;
    engine.tick(nowMs);
    
    // B should immediately reflect satisfaction because satisfaction propagates undamped
    bState = engine.getStateByName("B");
    EXPECT_TRUE(bState->rawStatus.met);
    EXPECT_TRUE(bState->dampedStatus.met); // because it has no penalty
}

TEST_F(PreconditionEngineTest, BindsTo_TearsDown_WhenUpstreamRegresses) {
    bool aMet = true;
    bool bMet = true;
    bool cMet = true;
    engine.add("A", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::Unreachable};
    }, nullptr, &aMet);
    
    Edge edgesB[] = {{"A", EdgeKind::BindsTo}};
    engine.add("B", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::NotConfigured};
    }, nullptr, &bMet, edgesB, 1);
    
    Edge edgesC[] = {{"A", EdgeKind::Requires}};
    engine.add("C", PreconditionRole::Operational, [](void* ctx) {
        bool m = *static_cast<bool*>(ctx);
        return PreconditionStatus{m, m ? Cause::Met : Cause::NotConfigured};
    }, nullptr, &cMet, edgesC, 1);
    
    EXPECT_TRUE(engine.resolveEdges());
    
    engine.tick(nowMs);
    const PreconditionState* bState = engine.getStateByName("B");
    const PreconditionState* cState = engine.getStateByName("C");
    EXPECT_TRUE(bState->rawStatus.met);
    EXPECT_TRUE(cState->rawStatus.met);
    
    // Regress upstream A
    aMet = false;
    nowMs += 1000;
    engine.tick(nowMs);
    bState = engine.getStateByName("B");
    cState = engine.getStateByName("C");
    
    // B should be torn down because of BindsTo
    EXPECT_FALSE(bState->rawStatus.met);
    EXPECT_EQ(bState->rawStatus.cause, Cause::DependencyUnmet);
    
    // C should also be unmet because A is unmet and it Requires A
    EXPECT_FALSE(cState->rawStatus.met);
    EXPECT_EQ(cState->rawStatus.cause, Cause::DependencyUnmet);

    // Recover upstream A
    aMet = true;
    nowMs += 1000;
    engine.tick(nowMs);
    bState = engine.getStateByName("B");
    cState = engine.getStateByName("C");
    
    EXPECT_TRUE(cState->rawStatus.met);
    EXPECT_TRUE(bState->rawStatus.met);
}


TEST_F(PreconditionEngineTest, EdgeMisalignment) {
    engine.add("A", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; });
    
    Edge edgesB[] = {{"TYPO", EdgeKind::Requires}, {"A", EdgeKind::Requires}};
    engine.add("B", PreconditionRole::Operational, [](void*) { return PreconditionStatus{true, Cause::Met}; }, nullptr, nullptr, edgesB, 2);
    
    EXPECT_FALSE(engine.resolveEdges());
    
    const PreconditionState* bState = engine.getStateByName("B");
    EXPECT_EQ(bState->resolvedEdgeCount, 0); // Safe degradation
    
    engine.tick(nowMs);
    bState = engine.getStateByName("B");
    EXPECT_NE(bState->rawStatus.cause, Cause::DependencyUnmet);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
