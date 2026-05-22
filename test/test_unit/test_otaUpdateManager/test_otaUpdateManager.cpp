/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * Module: test/test_unit/test_otaUpdateManager/test_otaUpdateManager.cpp
 * Description: Native unit tests for the otaUpdateManager OTA orchestrator.
 *              Tests the new state machine, guard logic, and legacy API on
 *              the native (non-FreeRTOS) platform.
 */

#include <gtest/gtest.h>

// Include FrameworkConfig and EventBus before the otaUpdateManager header
// so Application can be fully defined for the test (the cpp stub defines it).
#ifdef NATIVE_BUILD
#include "../../../include/FrameworkConfig.hpp"
#include "../../../modules/eventBus/EventBus.hpp"
// Match the Application stub in otaUpdateManager.cpp exactly.
// The linker will merge these with the same-named class from the .cpp TU.
// Since both TUs include the same inline definition, this is safe.
class Application {
public:
    EventBus& getEventBus();
    const FrameworkConfig& getConfig() const;
};
#endif

#include "../../../modules/otaUpdateManager/otaUpdateManager.hpp"

#ifdef NATIVE_BUILD
#include "../../../modules/webServer/webServer.hpp"
#endif

class OtaUpdateManagerTest : public ::testing::Test {
protected:
    otaUpdateManager manager;
};

// ---------------------------------------------------------------------------
// New Orchestrator API Tests
// ---------------------------------------------------------------------------

/// Verify the initial state is Idle.
TEST_F(OtaUpdateManagerTest, DefaultState_IsIdle) {
    EXPECT_EQ(OtaState::Idle, manager.getState());
}

/// checkForUpdate() must return false if begin() was never called.
TEST_F(OtaUpdateManagerTest, CheckForUpdate_WithoutBegin_ReturnsFalse) {
    EXPECT_FALSE(manager.checkForUpdate());
}

/// performUpdate() without begin() should set state to Error on native.
TEST_F(OtaUpdateManagerTest, PerformUpdate_WithoutBegin_SetsError) {
    manager.performUpdate();
    EXPECT_EQ(OtaState::Error, manager.getState());
}

/// getLastOtaError() should return nullptr initially.
TEST_F(OtaUpdateManagerTest, GetLastOtaError_Default_ReturnsNull) {
    EXPECT_EQ(nullptr, manager.getLastOtaError());
}

/// performUpdate() after begin() but on native should set Error (no FreeRTOS).
TEST_F(OtaUpdateManagerTest, PerformUpdate_OnNative_SetsError) {
    Application app;
    manager.begin(app);
    manager.performUpdate();
    EXPECT_EQ(OtaState::Error, manager.getState());
    EXPECT_NE(nullptr, manager.getLastOtaError());
}

/// checkForUpdate() after begin() with no repo should return false.
TEST_F(OtaUpdateManagerTest, CheckForUpdate_NoRepo_ReturnsFalse) {
    Application app;
    manager.begin(app);
    EXPECT_FALSE(manager.checkForUpdate());
    // State should return to Idle (not stuck in Checking)
    EXPECT_EQ(OtaState::Idle, manager.getState());
}

// ---------------------------------------------------------------------------
// Legacy API Tests
// ---------------------------------------------------------------------------

/// Legacy beginUpdate() should return false on native builds.
TEST_F(OtaUpdateManagerTest, BeginUpdate_OnNative_ReturnsFalse) {
    EXPECT_FALSE(manager.beginUpdate(1024, "abc123"));
}

/// Legacy endUpdate() should return false on native builds.
TEST_F(OtaUpdateManagerTest, EndUpdate_OnNative_ReturnsFalse) {
    EXPECT_FALSE(manager.endUpdate());
}

/// beginUpdate with nullptr MD5 should return false on native.
TEST_F(OtaUpdateManagerTest, BeginUpdate_NullptrMD5_ReturnsFalse) {
    EXPECT_FALSE(manager.beginUpdate(1024, nullptr));
}

/// beginUpdate with zero size should return false on native.
TEST_F(OtaUpdateManagerTest, BeginUpdate_ZeroSize_ReturnsFalse) {
    EXPECT_FALSE(manager.beginUpdate(0, "abc"));
}

/// markAppValid() should not crash.
TEST_F(OtaUpdateManagerTest, MarkAppValid_NoCrash) {
    manager.markAppValid();
    SUCCEED();
}

/// rollbackFirmware() should not crash.
TEST_F(OtaUpdateManagerTest, RollbackFirmware_NoCrash) {
    manager.rollbackFirmware();
    SUCCEED();
}

/// getTabName() should return "OTA".
TEST_F(OtaUpdateManagerTest, GetTabName_ReturnsOTA) {
    EXPECT_STREQ("OTA", manager.getTabName());
}

/// writeStream() should return 0 on native.
TEST_F(OtaUpdateManagerTest, WriteStream_OnNative_ReturnsZero) {
    Stream stream;
    EXPECT_EQ(0u, manager.writeStream(stream));
}

/// registerRoutes() should not crash on native.
TEST_F(OtaUpdateManagerTest, RegisterRoutes_NoCrash) {
    AsyncWebServer server(80);
    manager.registerRoutes(server);
    SUCCEED();
}

/// Global instance should be accessible.
TEST_F(OtaUpdateManagerTest, GlobalInstance_Accessible) {
    EXPECT_EQ(OtaState::Idle, otaManager.getState());
}

#if defined(NATIVE_BUILD)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
