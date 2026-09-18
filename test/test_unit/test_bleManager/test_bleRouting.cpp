/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_bleManager/test_bleRouting.cpp
 * Description: Unit tests for BLERoutingTable pure lookup mapping characteristic
 *              handles to service providers. Authoritative: RFC #225 rev 2 §6.2.
 */

#include <gtest/gtest.h>
#include "BLERoutingTable.hpp"
#include "IBLEServiceProvider.hpp"

namespace {
    class MockProvider : public IBLEServiceProvider {
    public:
        explicit MockProvider(const char* id) : _id(id) {}
        const char* getProviderId() const override { return _id; }
        bool registerServices(NimBLEServer&) override { return true; }

    private:
        const char* _id;
    };
}

TEST(BLERoutingTableTest, EmptyTableBehavior) {
    BLERoutingTable table;
    EXPECT_EQ(table.size(), 0);
    EXPECT_FALSE(table.hasRoute(1));
    EXPECT_EQ(table.findProvider(1), nullptr);
}

TEST(BLERoutingTableTest, AddRouteValidations) {
    BLERoutingTable table;
    MockProvider provider("ftms");

    // Invalid handle 0
    EXPECT_FALSE(table.addRoute(0, &provider));
    EXPECT_EQ(table.size(), 0);

    // Null provider
    EXPECT_FALSE(table.addRoute(10, nullptr));
    EXPECT_EQ(table.size(), 0);

    // Valid route
    EXPECT_TRUE(table.addRoute(10, &provider));
    EXPECT_EQ(table.size(), 1);
    EXPECT_TRUE(table.hasRoute(10));
    EXPECT_EQ(table.findProvider(10), &provider);
}

TEST(BLERoutingTableTest, MultipleProvidersAndHandles) {
    BLERoutingTable table;
    MockProvider ftms("ftms");
    MockProvider hr("hr");

    // Provider 1 owns handles 10 and 11
    EXPECT_TRUE(table.addRoute(10, &ftms));
    EXPECT_TRUE(table.addRoute(11, &ftms));

    // Provider 2 owns handle 20
    EXPECT_TRUE(table.addRoute(20, &hr));

    EXPECT_EQ(table.size(), 3);
    EXPECT_EQ(table.findProvider(10), &ftms);
    EXPECT_EQ(table.findProvider(11), &ftms);
    EXPECT_EQ(table.findProvider(20), &hr);
    EXPECT_EQ(table.findProvider(30), nullptr);
}

TEST(BLERoutingTableTest, OverwriteRoute) {
    BLERoutingTable table;
    MockProvider provA("prov_a");
    MockProvider provB("prov_b");

    EXPECT_TRUE(table.addRoute(15, &provA));
    EXPECT_EQ(table.findProvider(15), &provA);

    // Overwriting same handle with provB
    EXPECT_TRUE(table.addRoute(15, &provB));
    EXPECT_EQ(table.size(), 1);
    EXPECT_EQ(table.findProvider(15), &provB);
}

TEST(BLERoutingTableTest, ClearTable) {
    BLERoutingTable table;
    MockProvider prov("prov");

    table.addRoute(1, &prov);
    table.addRoute(2, &prov);
    EXPECT_EQ(table.size(), 2);

    table.clear();
    EXPECT_EQ(table.size(), 0);
    EXPECT_EQ(table.findProvider(1), nullptr);
    EXPECT_EQ(table.findProvider(2), nullptr);
}
