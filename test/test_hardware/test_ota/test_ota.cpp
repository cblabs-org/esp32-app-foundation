#include <Arduino.h>
#include <unity.h>
#include "otaUpdateManager.hpp"
#include <esp_ota_ops.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_ota_update_manager_begin(void) {
    // Start an update with a dummy size and a dummy MD5
    bool started = otaManager.beginUpdate(1024, "d41d8cd98f00b204e9800998ecf8427e");
    TEST_ASSERT_TRUE(started);
}

void setup() {
    delay(2000); // Wait for serial
    UNITY_BEGIN();
    RUN_TEST(test_ota_update_manager_begin);
    UNITY_END();
}

void loop() {
}
