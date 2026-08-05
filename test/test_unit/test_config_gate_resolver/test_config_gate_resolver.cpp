/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_config_gate_resolver/test_config_gate_resolver.cpp
 * Description: GoogleTest suite for the ConfigGateResolver pure-logic utility.
 *              Exercises findActionableGate() and shouldRepublish() without hardware.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point — initialises and runs all test suites
 */

#include <gtest/gtest.h>
#include "utils/ConfigGateResolver.hpp"
#include "interfaces/IConfigGate.hpp"
#include <vector>

class MockGate : public IConfigGate {
public:
    MockGate(const char* name, const char* help, bool satisfied = false)
        : _name(name), _help(help), _satisfied(satisfied) {}

    bool isSatisfied() const override { return _satisfied; }
    const char* getGateName() const override { return _name; }
    const char* getHelpText() const override { return _help; }
    void setSatisfied(bool val) { _satisfied = val; }

    void setDependencies(const char* const* deps, uint8_t count) {
        _deps = deps;
        _depCount = count;
    }
    const char* const* getDependencies(uint8_t& count) const override {
        count = _depCount;
        return _deps;
    }

    void setActionStatus(GateActionStatus status) { _status = status; _override = true; }
    GateActionStatus getActionStatus() const override {
        return _override ? _status : IConfigGate::getActionStatus();
    }

private:
    const char* _name;
    const char* _help;
    bool _satisfied;
    const char* const* _deps = nullptr;
    uint8_t _depCount = 0;
    GateActionStatus _status = GateActionStatus::NeedsUserAction;
    bool _override = false;
};

// ---------------------------------------------------------------------------
// findActionableGate Tests
// ---------------------------------------------------------------------------

TEST(ConfigGateResolverTest, FindActionableGate_AllSatisfied_ReturnsNullptr) {
    MockGate g1("wifi", "WiFi setup", true);
    MockGate g2("tz", "Timezone setup", true);
    std::vector<IConfigGate*> gates = { &g1, &g2 };

    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), nullptr);
}

TEST(ConfigGateResolverTest, FindActionableGate_FirstUnsatisfied_NoDeps_ReturnsIt) {
    MockGate g1("wifi", "WiFi setup", false);
    MockGate g2("tz", "Timezone setup", false);
    std::vector<IConfigGate*> gates = { &g1, &g2 };

    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), &g1);
}

TEST(ConfigGateResolverTest, FindActionableGate_SkipsSatisfied_ReturnsFirstUnsatisfied) {
    MockGate g1("wifi", "WiFi setup", true);
    MockGate g2("tz", "Timezone setup", false);
    std::vector<IConfigGate*> gates = { &g1, &g2 };

    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), &g2);
}

TEST(ConfigGateResolverTest, FindActionableGate_DepsUnsatisfied_SkipsGate) {
    MockGate g1("wifi", "WiFi setup", false);
    MockGate g2("tz", "Timezone setup", false);

    const char* deps[] = { "wifi" };
    g2.setDependencies(deps, 1);

    // Order: g2 first, then g1
    // g2 depends on g1 ("wifi") which is unsatisfied -> g2 skipped -> g1 returned
    std::vector<IConfigGate*> gates = { &g2, &g1 };
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), &g1);

    // If g1 is not in the list at all, g2 is skipped -> nullptr returned
    std::vector<IConfigGate*> gatesOnlyG2 = { &g2 };
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gatesOnlyG2), nullptr);
}

TEST(ConfigGateResolverTest, FindActionableGate_DepsSatisfied_ReturnsGate) {
    MockGate g1("wifi", "WiFi setup", true);
    MockGate g2("tz", "Timezone setup", false);

    const char* deps[] = { "wifi" };
    g2.setDependencies(deps, 1);

    std::vector<IConfigGate*> gates = { &g1, &g2 };
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), &g2);
}

TEST(ConfigGateResolverTest, FindActionableGate_ChainedDeps_ReturnsCorrectGate) {
    MockGate gA("wifi", "WiFi", false);
    MockGate gB("net", "Network", false);
    MockGate gC("app", "App Config", false);

    const char* depsB[] = { "wifi" };
    gB.setDependencies(depsB, 1);

    const char* depsC[] = { "net" };
    gC.setDependencies(depsC, 1);

    std::vector<IConfigGate*> gates = { &gA, &gB, &gC };

    // Stage 1: Only gA is actionable (gB needs gA, gC needs gB)
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), &gA);

    // Stage 2: gA satisfied -> gB becomes actionable
    gA.setSatisfied(true);
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), &gB);

    // Stage 3: gB satisfied -> gC becomes actionable
    gB.setSatisfied(true);
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), &gC);

    // Stage 4: gC satisfied -> all satisfied
    gC.setSatisfied(true);
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), nullptr);
}

TEST(ConfigGateResolverTest, FindActionableGate_UnknownDep_SkipsGate) {
    MockGate g1("tz", "Timezone setup", false);

    const char* deps[] = { "nonexistent_gate" };
    g1.setDependencies(deps, 1);

    std::vector<IConfigGate*> gates = { &g1 };
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), nullptr);
}

TEST(ConfigGateResolverTest, FindActionableGate_EmptyVector_ReturnsNullptr) {
    std::vector<IConfigGate*> gates;
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), nullptr);
}

TEST(ConfigGateResolverTest, FindActionableGate_CyclicDeps_ReturnsNullptr) {
    MockGate gA("gateA", "Gate A", false);
    MockGate gB("gateB", "Gate B", false);

    const char* depsA[] = { "gateB" };
    gA.setDependencies(depsA, 1);

    const char* depsB[] = { "gateA" };
    gB.setDependencies(depsB, 1);

    std::vector<IConfigGate*> gates = { &gA, &gB };
    EXPECT_EQ(ConfigGateResolver::findActionableGate(gates), nullptr);
}

// ---------------------------------------------------------------------------
// shouldRepublish Tests
// ---------------------------------------------------------------------------

TEST(ConfigGateResolverTest, ShouldRepublish_SameGateSameStatus_ReturnsFalse) {
    EXPECT_FALSE(ConfigGateResolver::shouldRepublish(
        "wifi", GateActionStatus::NeedsUserAction,
        "wifi", GateActionStatus::NeedsUserAction));
}

TEST(ConfigGateResolverTest, ShouldRepublish_DifferentGate_ReturnsTrue) {
    EXPECT_TRUE(ConfigGateResolver::shouldRepublish(
        "wifi", GateActionStatus::NeedsUserAction,
        "tz", GateActionStatus::NeedsUserAction));
}

TEST(ConfigGateResolverTest, ShouldRepublish_SameGateDifferentStatus_ReturnsTrue) {
    EXPECT_TRUE(ConfigGateResolver::shouldRepublish(
        "wifi", GateActionStatus::NeedsUserAction,
        "wifi", GateActionStatus::InProgress));
}

TEST(ConfigGateResolverTest, ShouldRepublish_NullToNonNull_ReturnsTrue) {
    EXPECT_TRUE(ConfigGateResolver::shouldRepublish(
        "wifi", GateActionStatus::NeedsUserAction,
        nullptr, GateActionStatus::Satisfied));
}

TEST(ConfigGateResolverTest, ShouldRepublish_NonNullToNull_ReturnsTrue) {
    EXPECT_TRUE(ConfigGateResolver::shouldRepublish(
        nullptr, GateActionStatus::Satisfied,
        "wifi", GateActionStatus::NeedsUserAction));
}

TEST(ConfigGateResolverTest, ShouldRepublish_BothNull_SameStatus_ReturnsFalse) {
    EXPECT_FALSE(ConfigGateResolver::shouldRepublish(
        nullptr, GateActionStatus::Satisfied,
        nullptr, GateActionStatus::Satisfied));
}

TEST(ConfigGateResolverTest, ShouldRepublish_BothNull_DifferentStatus_ReturnsTrue) {
    EXPECT_TRUE(ConfigGateResolver::shouldRepublish(
        nullptr, GateActionStatus::NeedsUserAction,
        nullptr, GateActionStatus::Satisfied));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
