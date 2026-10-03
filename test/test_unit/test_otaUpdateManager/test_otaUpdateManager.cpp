/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * Module: test/test_unit/test_otaUpdateManager/test_otaUpdateManager.cpp
 * Description: Native unit tests for the OTAUpdateManager OTA orchestrator.
 *              Tests the new state machine, guard logic, and legacy API on
 *              the native (non-FreeRTOS) platform.
 */

#include <gtest/gtest.h>

#ifdef WITH_OTA

// Include the real Application header (Application now owns OTAUpdateManager)
#include "Application.hpp"

#include "OTAUpdateManager.hpp"
#include "../test_helpers/MockTabRegistry.hpp"
#include <ArduinoJson.h>

#include "WebServer.hpp"

class OtaUpdateManagerTest : public ::testing::Test {
protected:
    OTAUpdateManager manager;
};

// ---------------------------------------------------------------------------
// New Orchestrator API Tests
// ---------------------------------------------------------------------------

/// Verify the initial state is Idle.
TEST_F(OtaUpdateManagerTest, DefaultState_IsIdle) {
    EXPECT_EQ(OTAState::Idle, manager.getState());
}

/// checkForUpdate() must return false if begin() was never called.
TEST_F(OtaUpdateManagerTest, CheckForUpdate_WithoutBegin_ReturnsFalse) {
    EXPECT_FALSE(manager.checkForUpdate());
}

/// performUpdate() without begin() should set state to Error on native.
TEST_F(OtaUpdateManagerTest, PerformUpdate_WithoutBegin_SetsError) {
    manager.performUpdate();
    EXPECT_EQ(OTAState::Error, manager.getState());
}

/// getLastOtaError() should return nullptr initially.
TEST_F(OtaUpdateManagerTest, GetLastOtaError_Default_ReturnsNull) {
    EXPECT_EQ(nullptr, manager.getLastOtaError());
}

/// performUpdate() after begin() transitions state to Checking and notifies
/// the persistent worker task. On native the worker runs synchronously via the
/// xTaskCreateWithCaps stub but exits at ulTaskNotifyTake before running the
/// pipeline — so state stays at Checking (not Error) after performUpdate().
TEST_F(OtaUpdateManagerTest, PerformUpdate_AfterBegin_TransitionsToChecking) {
    Application app;
    manager.begin(app);
    manager.performUpdate();
    EXPECT_EQ(OTAState::Checking, manager.getState());
    EXPECT_EQ(nullptr, manager.getLastOtaError());
}

/// checkForUpdate() after begin() with no repo should return false.
TEST_F(OtaUpdateManagerTest, CheckForUpdate_NoRepo_ReturnsFalse) {
    Application app;
    manager.begin(app);
    EXPECT_FALSE(manager.checkForUpdate());
    // State should return to Idle (not stuck in Checking)
    EXPECT_EQ(OTAState::Idle, manager.getState());
}

// ---------------------------------------------------------------------------
// Legacy API Tests
// ---------------------------------------------------------------------------

// Legacy beginUpdate() tests were removed because the #ifndef native test stubs
// were removed in PR-41, so they now execute the real stubbed update logic.

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

/// registerTabs() should register OTA under System at order 50.
TEST_F(OtaUpdateManagerTest, RegisterTabs_OTAUnderSystem) {
    MockTabRegistry collector;
    manager.registerTabs(collector);
    ASSERT_EQ(collector.entries.size(), 1u);
    EXPECT_EQ(collector.entries[0].name, "OTA");
    EXPECT_EQ(collector.entries[0].parent, "System");
    EXPECT_EQ(collector.entries[0].order, 50);
}

/// writeStream() should return 0 on native.
TEST_F(OtaUpdateManagerTest, WriteStream_OnNative_ReturnsZero) {
    Stream stream;
    EXPECT_EQ(0u, manager.writeStream(stream));
}

/// registerRoutes() should not crash on native.
TEST_F(OtaUpdateManagerTest, RegisterRoutes_NoCrash) {
    PsychicHttpServer server(80);
    manager.registerRoutes(server);
    SUCCEED();
}

/// A locally-constructed OTAUpdateManager instance starts in Idle state.
TEST_F(OtaUpdateManagerTest, LocalInstance_StartsIdle) {
    OTAUpdateManager localManager;
    EXPECT_EQ(OTAState::Idle, localManager.getState());
}

// ---------------------------------------------------------------------------
// updatesEnabled config persistence (WO2b — moved from FrameworkConfig)
// ---------------------------------------------------------------------------

/// resetConfig() must restore the secure-by-design default: updates enabled.
TEST_F(OtaUpdateManagerTest, ResetConfig_UpdatesEnabled_DefaultsTrue) {
    manager.resetConfig();
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    manager.serializeConfig(section);
    ASSERT_TRUE(section["updatesEnabled"].is<bool>());
    EXPECT_TRUE(section["updatesEnabled"].as<bool>());
}

/// applyConfig()/serializeConfig() must round-trip updatesEnabled in both states.
TEST_F(OtaUpdateManagerTest, Config_UpdatesEnabled_RoundTrips) {
    // false → persisted as false
    JsonDocument inDoc;
    inDoc["updatesEnabled"] = false;
    manager.applyConfig(inDoc.as<JsonObject>());

    JsonDocument outDoc;
    JsonObject out = outDoc.to<JsonObject>();
    manager.serializeConfig(out);
    EXPECT_FALSE(out["updatesEnabled"].as<bool>());

    // true → persisted as true
    JsonDocument inDoc2;
    inDoc2["updatesEnabled"] = true;
    manager.applyConfig(inDoc2.as<JsonObject>());

    JsonDocument outDoc2;
    JsonObject out2 = outDoc2.to<JsonObject>();
    manager.serializeConfig(out2);
    EXPECT_TRUE(out2["updatesEnabled"].as<bool>());
}

/// A missing updatesEnabled key must default to true (secure-by-design fallback).
TEST_F(OtaUpdateManagerTest, ApplyConfig_MissingUpdatesEnabled_DefaultsTrue) {
    JsonDocument inDoc;
    inDoc["channel"] = "stable";  // present but updatesEnabled absent
    manager.applyConfig(inDoc.as<JsonObject>());

    JsonDocument outDoc;
    JsonObject out = outDoc.to<JsonObject>();
    manager.serializeConfig(out);
    EXPECT_TRUE(out["updatesEnabled"].as<bool>());
}

// ---------------------------------------------------------------------------
// checkOnBoot config tests
// ---------------------------------------------------------------------------

TEST_F(OtaUpdateManagerTest, ResetConfig_CheckOnBoot_DefaultsTrue) {
    manager.resetConfig();
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    manager.serializeConfig(section);
    ASSERT_TRUE(section["checkOnBoot"].is<bool>());
    EXPECT_TRUE(section["checkOnBoot"].as<bool>());
}

TEST_F(OtaUpdateManagerTest, Config_CheckOnBoot_RoundTrips) {
    JsonDocument inDoc;
    inDoc["checkOnBoot"] = false;
    manager.applyConfig(inDoc.as<JsonObject>());
    JsonDocument outDoc;
    JsonObject out = outDoc.to<JsonObject>();
    manager.serializeConfig(out);
    EXPECT_FALSE(out["checkOnBoot"].as<bool>());
}

TEST_F(OtaUpdateManagerTest, ApplyConfig_MissingCheckOnBoot_DefaultsTrue) {
    JsonDocument inDoc;
    manager.applyConfig(inDoc.as<JsonObject>());
    JsonDocument outDoc;
    JsonObject out = outDoc.to<JsonObject>();
    manager.serializeConfig(out);
    EXPECT_TRUE(out["checkOnBoot"].as<bool>());
}

TEST_F(OtaUpdateManagerTest, Tick_SkipsBootCheck_WhenCheckOnBootFalse) {
    Application app;
    manager.begin(app);
    JsonDocument inDoc;
    inDoc["checkOnBoot"] = false;
    manager.applyConfig(inDoc.as<JsonObject>());
    manager.tick(0); // Should skip performUpdate()
    EXPECT_EQ(OTAState::Idle, manager.getState());
}

TEST_F(OtaUpdateManagerTest, Tick_PerformsBootCheck_WhenCheckOnBootTrue) {
    Application app;
    manager.begin(app);
    JsonDocument inDoc;
    inDoc["checkOnBoot"] = true;
    manager.applyConfig(inDoc.as<JsonObject>());
    manager.tick(0); // Should trigger performUpdate()
    EXPECT_EQ(OTAState::Checking, manager.getState());
}

TEST_F(OtaUpdateManagerTest, MigrateConfig_V5_AddsCheckOnBoot) {
    JsonDocument doc;
    JsonObject section = doc.to<JsonObject>();
    manager.migrateConfig(section, 5);
    EXPECT_TRUE(section["checkOnBoot"].as<bool>());
}

#endif // WITH_OTA

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
