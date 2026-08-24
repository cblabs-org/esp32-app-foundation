/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/test_unit/test_lvglDisplay/test_web_contributor.cpp
 * Description: Unit tests for LVGLDisplayModule IWebContributor implementation,
 *              REST API route handlers, tab metadata, and snapshot capture primitive.
 */

#include <gtest/gtest.h>
#include "LVGLDisplayModule.hpp"
#include "MockLVGLHardwareAdapter.hpp"
#include <PsychicHttp.h>
#include <vector>

class LVGLWebContributorTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    LVGLDisplayModule display{adapter};

    void SetUp() override {
    }
};

#include "../test_helpers/MockTabRegistry.hpp"

TEST_F(LVGLWebContributorTest, TabMetadata) {
    EXPECT_STREQ(display.getApiPrefix(), "/api/display");
    EXPECT_NE(display.getTabIcon(), nullptr);

    // Verify registerTabs() registers the expected parent + child tabs
    MockTabRegistry registry;
    display.registerTabs(registry);
    ASSERT_EQ(registry.entries.size(), 4u);

    // Parent: Display
    EXPECT_EQ(registry.entries[0].name, "Display");
    EXPECT_EQ(registry.entries[0].parent, "");
    EXPECT_EQ(registry.entries[0].order, 50);

    // Child: Settings
    EXPECT_EQ(registry.entries[1].name, "Settings");
    EXPECT_EQ(registry.entries[1].parent, "Display");
    EXPECT_EQ(registry.entries[1].order, 10);

    // Child: Screenshot
    EXPECT_EQ(registry.entries[2].name, "Screenshot");
    EXPECT_EQ(registry.entries[2].parent, "Display");
    EXPECT_EQ(registry.entries[2].order, 20);

    // Child: Hardware
    EXPECT_EQ(registry.entries[3].name, "Hardware");
    EXPECT_EQ(registry.entries[3].parent, "Display");
    EXPECT_EQ(registry.entries[3].order, 30);
}

TEST_F(LVGLWebContributorTest, CaptureSnapshotPrimitive) {
    uint8_t* buf = nullptr;
    size_t size = 0;
    uint16_t width = 0;
    uint16_t height = 0;

    bool success = display.captureSnapshot(buf, size, width, height);
    EXPECT_TRUE(success);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(width, 240);
    EXPECT_EQ(height, 240);
    EXPECT_EQ(size, 240 * 240 * 2);

    display.freeSnapshot(buf);
}

TEST_F(LVGLWebContributorTest, RouteRegistrationAndHandlers) {
    PsychicHttpServer server;
    display.registerRoutes(server);

    // Post-#88: module registers status/rotation/flip/sleep inline;
    // brightness + screenshot routes are registered by DisplayServiceHelper (hardware-only)
    EXPECT_GE(server.routes.size(), 4);

    bool statusFound = false;
    bool rotationFound = false;
    bool flipFound = false;
    bool sleepFound = false;

    for (const auto& r : server.routes) {
        if (r.uri == "/api/display/status") statusFound = true;
        if (r.uri == "/api/display/rotation") rotationFound = true;
        if (r.uri == "/api/display/flip") flipFound = true;
        if (r.uri == "/api/display/sleep") sleepFound = true;
    }

    EXPECT_TRUE(statusFound);
    EXPECT_TRUE(rotationFound);
    EXPECT_TRUE(flipFound);
    EXPECT_TRUE(sleepFound);
}
