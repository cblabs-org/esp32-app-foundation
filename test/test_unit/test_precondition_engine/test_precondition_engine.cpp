#include <gtest/gtest.h>
#include "PreconditionEngine.hpp"

class PreconditionEngineTest : public ::testing::Test {
protected:
    PreconditionEngine engine;
    uint32_t nowMs = 10000;
};

TEST_F(PreconditionEngineTest, RegistrationAndEdgeResolution) {
    engine.add("A", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; });
    engine.add("B", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; });
    
    Edge edges[] = {{"A", EdgeKind::Requires}};
    engine.add("C", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; }, nullptr, edges, 1);
    
    EXPECT_TRUE(engine.resolveEdges());
    EXPECT_EQ(engine.getPreconditionCount(), 3);
    
    PreconditionState cState;
    EXPECT_TRUE(engine.getStateByName("C", cState));
    EXPECT_EQ(cState.resolvedEdgeCount, 1);
    EXPECT_EQ(cState.resolvedEdges[0], 0); // Index of "A"
}

TEST_F(PreconditionEngineTest, EdgeResolutionUnknownRejection) {
    Edge edges[] = {{"Unknown", EdgeKind::Requires}};
    engine.add("A", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; }, nullptr, edges, 1);
    EXPECT_FALSE(engine.resolveEdges());
}

TEST_F(PreconditionEngineTest, EdgeResolutionCycleDetection) {
    Edge edgesA[] = {{"B", EdgeKind::Requires}};
    Edge edgesB[] = {{"A", EdgeKind::Requires}};
    
    engine.add("A", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; }, nullptr, edgesA, 1);
    engine.add("B", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; }, nullptr, edgesB, 1);
    
    EXPECT_FALSE(engine.resolveEdges());
}

TEST_F(PreconditionEngineTest, BasicEvaluation) {
    bool met = false;
    engine.add("A", PreconditionRole::Operational, [&]() { return PreconditionStatus{met, met ? Cause::Met : Cause::NotConfigured}; });
    EXPECT_TRUE(engine.resolveEdges());
    
    engine.tick(nowMs);
    PreconditionState aState;
    engine.getStateByName("A", aState);
    EXPECT_FALSE(aState.rawStatus.met);
    
    met = true;
    nowMs += 1000;
    engine.tick(nowMs);
    engine.getStateByName("A", aState);
    EXPECT_TRUE(aState.rawStatus.met);
}

TEST_F(PreconditionEngineTest, EdgePropagation) {
    bool aMet = false;
    engine.add("A", PreconditionRole::Operational, [&]() { return PreconditionStatus{aMet, aMet ? Cause::Met : Cause::NotConfigured}; });
    
    Edge edgesB[] = {{"A", EdgeKind::Requires}};
    engine.add("B", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; }, nullptr, edgesB, 1);

    EXPECT_TRUE(engine.resolveEdges());
    engine.tick(nowMs);

    PreconditionState bState;
    engine.getStateByName("B", bState);
    EXPECT_FALSE(bState.rawStatus.met);
    EXPECT_EQ(bState.rawStatus.cause, Cause::DependencyUnmet);

    aMet = true;
    nowMs += 1000;
    engine.tick(nowMs);
    engine.getStateByName("B", bState);
    EXPECT_TRUE(bState.rawStatus.met);
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
    engine.add("A", PreconditionRole::Operational, [&]() { return PreconditionStatus{met, met ? Cause::Met : Cause::NotConfigured}; });
    EXPECT_TRUE(engine.resolveEdges());
    
    engine.tick(nowMs);
    PreconditionState aState;
    engine.getStateByName("A", aState);
    EXPECT_FALSE(aState.everMet);
    
    met = true;
    engine.tick(nowMs += 1000);
    engine.getStateByName("A", aState);
    EXPECT_TRUE(aState.everMet);
    
    engine.bumpIdentityEpoch("A");
    engine.getStateByName("A", aState);
    EXPECT_FALSE(aState.everMet);
    EXPECT_EQ(aState.identityEpoch, 1);
}

TEST_F(PreconditionEngineTest, Damping) {
    bool met = false;
    engine.add("A", PreconditionRole::Operational, [&]() { return PreconditionStatus{met, met ? Cause::Met : Cause::NotConfigured}; }); // NeedsUser -> weight 5
    EXPECT_TRUE(engine.resolveEdges());
    engine.tick(nowMs); // init lastTickMs
    // Accumulate penalty (penaltyPerSecond=10 * 5 = 50 per sec)
    for(int i=0; i<41; i++) {
        nowMs += 1000;
        engine.tick(nowMs);
    }
    PreconditionState aState;
    engine.getStateByName("A", aState);
    // 41 secs * 50 = 2050, so it crosses 2000 threshold
    EXPECT_TRUE(aState.suppressed);
    EXPECT_FALSE(aState.dampedStatus.met);
    
    // Recovery
    met = true;
    for(int i=0; i<6; i++) {
        nowMs += 500000;
        engine.tick(nowMs);
    }
    engine.getStateByName("A", aState);
    EXPECT_FALSE(aState.suppressed);
}

TEST_F(PreconditionEngineTest, Breaker) {
    bool met = false;
    engine.add("A", PreconditionRole::Operational, [&]() { return PreconditionStatus{met, Cause::Unreachable}; });
    EXPECT_TRUE(engine.resolveEdges());
    
    engine.tick(nowMs);
    PreconditionState aState;
    engine.getStateByName("A", aState);
    EXPECT_EQ(aState.breaker, BreakerState::Closed);
    
    // Wait attemptsBeforeFallback * baseDelayMs (5 * 5000 = 25000)
    for(int i=0; i<5; i++) {
        nowMs += 5000;
        engine.tick(nowMs);
    }
    nowMs += 1000;
    engine.tick(nowMs);
    engine.getStateByName("A", aState);
    EXPECT_EQ(aState.breaker, BreakerState::Open);
    
    // HalfOpen after baseDelayMs
    nowMs += 5000;
    engine.tick(nowMs);
    engine.getStateByName("A", aState);
    EXPECT_EQ(aState.breaker, BreakerState::HalfOpen);
    
    // requestRetry overrides
    engine.requestRetry("A");
    engine.getStateByName("A", aState);
    EXPECT_EQ(aState.breaker, BreakerState::HalfOpen);
}

TEST_F(PreconditionEngineTest, Projection) {
    engine.add("A", PreconditionRole::Operational, []() { return PreconditionStatus{false, Cause::NotConfigured}; });
    engine.add("B", PreconditionRole::Startup, []() { return PreconditionStatus{true, Cause::Met}; });
    EXPECT_TRUE(engine.resolveEdges());
    engine.tick(nowMs);
    
    EXPECT_TRUE(engine.allStartupMet());
}

TEST_F(PreconditionEngineTest, CG8Invariant) {
    // Satisfaction propagates immediately (undamped); penalty never propagates across edges.
    bool aMet = false;
    engine.add("A", PreconditionRole::Operational, [&]() { return PreconditionStatus{aMet, aMet ? Cause::Met : Cause::NotConfigured}; });
    Edge edges[] = {{"A", EdgeKind::Requires}};
    engine.add("B", PreconditionRole::Operational, []() { return PreconditionStatus{true, Cause::Met}; }, nullptr, edges, 1);
    
    EXPECT_TRUE(engine.resolveEdges());
    
    // Tick while unmet to build up penalty in A
    for(int i=0; i<50; i++) {
        engine.tick(nowMs);
        nowMs += 1000;
    }
    
    PreconditionState bState;
    engine.getStateByName("B", bState);
    EXPECT_EQ(bState.penalty, 0.0f); // B should not accrue penalty due to DependencyUnmet
    
    // Satisfy A
    aMet = true;
    nowMs += 1000;
    engine.tick(nowMs);
    
    // B should immediately reflect satisfaction because satisfaction propagates undamped
    engine.getStateByName("B", bState);
    EXPECT_TRUE(bState.rawStatus.met);
    EXPECT_TRUE(bState.dampedStatus.met); // because it has no penalty
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
