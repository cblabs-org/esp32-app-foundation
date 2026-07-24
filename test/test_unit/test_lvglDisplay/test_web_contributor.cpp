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

class LVGLWebContributorTest : public ::testing::Test {
protected:
    MockLVGLHardwareAdapter adapter;
    LVGLDisplayModule display{adapter};

    void SetUp() override {
    }
};

TEST_F(LVGLWebContributorTest, TabMetadata) {
    EXPECT_STREQ(display.getTabName(), "Display");
    EXPECT_EQ(display.getTabOrder(), 50);
    EXPECT_STREQ(display.getApiPrefix(), "/api/display");
    EXPECT_NE(display.getTabIcon(), nullptr);

    IWebContributor* settingsStub = display.settingsTabStub();
    ASSERT_NE(settingsStub, nullptr);
    EXPECT_STREQ(settingsStub->getTabName(), "Settings");
    EXPECT_STREQ(settingsStub->getParentTab(), "Display");
    EXPECT_EQ(settingsStub->getTabOrder(), 10);

    IWebContributor* screenshotStub = display.screenshotTabStub();
    ASSERT_NE(screenshotStub, nullptr);
    EXPECT_STREQ(screenshotStub->getTabName(), "Screenshot");
    EXPECT_STREQ(screenshotStub->getParentTab(), "Display");
    EXPECT_EQ(screenshotStub->getTabOrder(), 20);
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

    EXPECT_GE(server.routes.size(), 6);

    bool statusFound = false;
    bool brightnessFound = false;
    bool rotationFound = false;
    bool flipFound = false;
    bool sleepFound = false;
    bool screenshotFound = false;

    for (const auto& r : server.routes) {
        if (r.uri == "/api/display/status") statusFound = true;
        if (r.uri == "/api/display/brightness") brightnessFound = true;
        if (r.uri == "/api/display/rotation") rotationFound = true;
        if (r.uri == "/api/display/flip") flipFound = true;
        if (r.uri == "/api/display/sleep") sleepFound = true;
        if (r.uri == "/api/display/screenshot") screenshotFound = true;
    }

    EXPECT_TRUE(statusFound);
    EXPECT_TRUE(brightnessFound);
    EXPECT_TRUE(rotationFound);
    EXPECT_TRUE(flipFound);
    EXPECT_TRUE(sleepFound);
    EXPECT_TRUE(screenshotFound);
}
