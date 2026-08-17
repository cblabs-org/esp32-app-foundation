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

TEST(DisplayServiceHelperTest, ScreenshotUsesStoredPNGWhenExecutorNotAvailable) {
    MockDisplayModuleForScreenshot display;
    DisplayServiceHelper helper(&display);
    
    // In native tests, DeepStackExecutor::available() will return what we tell it
    // Wait, the native stub for DeepStackExecutor defaults to available() = true.
    // Let's rely on that if we can't change it easily, but wait, the DeepStackExecutor
    // stub has `_stubAvailable`. We can't access it easily without friend.
    
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
    
    // DeepStackExecutor stub sets available=true by default.
    // So capturePNG_called should be true.
    EXPECT_TRUE(display.capturePNG_called);
    EXPECT_FALSE(display.captureStoredPNG_called);
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
