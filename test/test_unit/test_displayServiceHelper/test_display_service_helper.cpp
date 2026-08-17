#include <gtest/gtest.h>
#include "DisplayServiceHelper.h"
#include "interfaces/IDisplayModule.hpp"
#include <PsychicHttp.h>

class MockDisplayModuleForScreenshot : public IDisplayModule {
public:
    bool capturePNG_called = false;
    bool captureStoredPNG_called = false;

    bool capturePNG(Print& sink) override {
        capturePNG_called = true;
        sink.write('1');
        return true;
    }
    bool captureStoredPNG(Print& sink) override {
        captureStoredPNG_called = true;
        sink.write('1');
        return true;
    }
    
    uint16_t getDisplayWidth() const override { return 100; }
    uint16_t getDisplayHeight() const override { return 100; }
    void setBrightness(uint8_t level) override {}
    uint8_t getBrightness() const override { return 255; }
    void resetActivity() override {}
    bool isDimmed() const override { return false; }
    bool isSleeping() const override { return false; }
    void showSystemState(const SystemDisplayParams& params) override {}
    bool beginEarly() override { return true; }
    void reportStatus(const char* message, float progress = -1.0f) override {}
    void reportError(const char* errorMessage) override {}
    void clearStatus() override {}
};

TEST(DisplayServiceHelperTest, ScreenshotUsesExecutorWhenAvailable) {
    MockDisplayModuleForScreenshot display;
    DisplayServiceHelper helper(&display);
    
    // Default: executor stub is available
    EXPECT_TRUE(helper.getExecutor()._stubAvailable);
    
    PsychicHttpServer server;
    helper.registerScreenshotRoute(server);
    
    PsychicHttpRequestCallback handler = nullptr;
    for (const auto& route : server.routes) {
        if (route.uri == "/api/display/screenshot") {
            handler = route.handler;
            break;
        }
    }
    ASSERT_NE(handler, nullptr);
    
    PsychicRequest req;
    PsychicResponse res;
    handler(&req, &res);
    
    // DeepStackExecutor stub is available → capturePNG (primary path)
    EXPECT_TRUE(display.capturePNG_called);
    EXPECT_FALSE(display.captureStoredPNG_called);
}

TEST(DisplayServiceHelperTest, ScreenshotUsesStoredPNGWhenExecutorNotAvailable) {
    MockDisplayModuleForScreenshot display;
    DisplayServiceHelper helper(&display);
    
    // Force executor to report unavailable via test accessor
    helper.getExecutor()._stubAvailable = false;
    
    PsychicHttpServer server;
    helper.registerScreenshotRoute(server);
    
    PsychicHttpRequestCallback handler = nullptr;
    for (const auto& route : server.routes) {
        if (route.uri == "/api/display/screenshot") {
            handler = route.handler;
            break;
        }
    }
    ASSERT_NE(handler, nullptr);
    
    PsychicRequest req;
    PsychicResponse res;
    handler(&req, &res);
    
    // Executor not available → stored-PNG fallback
    EXPECT_FALSE(display.capturePNG_called);
    EXPECT_TRUE(display.captureStoredPNG_called);
}

TEST(DisplayServiceHelperTest, ScreenshotOTAGate) {
    MockDisplayModuleForScreenshot display;
    DisplayServiceHelper helper(&display);
    
    helper.setOtaPredicate([]() { return true; }); // OTA active
    
    PsychicHttpServer server;
    helper.registerScreenshotRoute(server);
    
    PsychicHttpRequestCallback handler = nullptr;
    for (const auto& route : server.routes) {
        if (route.uri == "/api/display/screenshot") {
            handler = route.handler;
            break;
        }
    }
    ASSERT_NE(handler, nullptr);
    
    PsychicRequest req;
    PsychicResponse res;
    handler(&req, &res);
    
    // Should short-circuit and not call capture
    EXPECT_FALSE(display.capturePNG_called);
    EXPECT_FALSE(display.captureStoredPNG_called);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
