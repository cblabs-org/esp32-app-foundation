/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Test: test/test_hardware/test_wifi/test_main.cpp
 * Description: Unit tests for test_main.
 */

#include "AppFoundation.h"
#include <Arduino.h>
#include <unity.h>
#include "WifiManager.hpp"
#include <LittleFS.h>

WifiManager* wifiMgr;

void setUp(void) {
    LittleFS.begin(true);
    wifiMgr = new WifiManager();
}

void tearDown(void) {
    delete wifiMgr;
    LittleFS.end();
}

void test_wifi_manager_initialization() {
    TEST_ASSERT_FALSE(wifiMgr->isReady());
    TEST_ASSERT_FALSE(wifiMgr->getAPMode());
    TEST_ASSERT_FALSE(wifiMgr->isWifiPersistentError());
}

void test_wifi_manager_save_and_load_config() {
    wifiMgr->updateWiFi("TestSSID", "TestPass123");
    
    WifiManager freshMgr;
    bool loaded = freshMgr.loadConfig();
    
    TEST_ASSERT_TRUE(loaded);
    TEST_ASSERT_EQUAL_STRING("TestSSID", freshMgr.getSSID());
    TEST_ASSERT_TRUE(freshMgr.hasPassword());
}

void test_wifi_manager_reset_config() {
    wifiMgr->updateWiFi("TestSSID", "TestPass123");
    wifiMgr->resetConfig();
    
    WifiManager freshMgr;
    bool loaded = freshMgr.loadConfig();
    
    TEST_ASSERT_FALSE(loaded);
    TEST_ASSERT_EQUAL_STRING("", freshMgr.getSSID());
    TEST_ASSERT_FALSE(freshMgr.hasPassword());
}

void setup() {
    delay(2000); // Give time for monitor to connect
    UNITY_BEGIN();
    RUN_TEST(test_wifi_manager_initialization);
    RUN_TEST(test_wifi_manager_save_and_load_config);
    RUN_TEST(test_wifi_manager_reset_config);
    UNITY_END();
}

void loop() {
    // Tests are complete, do nothing.
}
