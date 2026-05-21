#include <gtest/gtest.h>
#include "../../../modules/otaUpdateManager/otaUpdateManager.hpp"

#ifdef NATIVE_BUILD
#include "../../../modules/webServer/webServer.hpp"
#endif

class OtaUpdateManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset state if needed
    }
};

// Construction
TEST_F(OtaUpdateManagerTest, Construction) {
    otaUpdateManager manager;
    SUCCEED();
}

// beginUpdate() returns false
TEST_F(OtaUpdateManagerTest, BeginUpdateReturnsFalse) {
    EXPECT_FALSE(otaManager.beginUpdate(1024, "abc123"));
}

// beginUpdate() with nullptr MD5
TEST_F(OtaUpdateManagerTest, BeginUpdateNullptrMD5) {
    EXPECT_FALSE(otaManager.beginUpdate(1024, nullptr));
}

// beginUpdate() with zero size
TEST_F(OtaUpdateManagerTest, BeginUpdateZeroSize) {
    EXPECT_FALSE(otaManager.beginUpdate(0, "abc"));
}

// endUpdate() returns false
TEST_F(OtaUpdateManagerTest, EndUpdateReturnsFalse) {
    EXPECT_FALSE(otaManager.endUpdate());
}

// markAppValid()
TEST_F(OtaUpdateManagerTest, MarkAppValid) {
    otaManager.markAppValid(); // Should not crash
    SUCCEED();
}

// rollbackFirmware()
TEST_F(OtaUpdateManagerTest, RollbackFirmware) {
    otaManager.rollbackFirmware(); // Should not crash
    SUCCEED();
}

// getTabName()
TEST_F(OtaUpdateManagerTest, GetTabName) {
    EXPECT_STREQ("OTA", otaManager.getTabName());
}

// Global instance
TEST_F(OtaUpdateManagerTest, GlobalInstance) {
    // Access global instance
    EXPECT_FALSE(otaManager.beginUpdate(1024, "abc"));
}

// Lifecycle sequence
TEST_F(OtaUpdateManagerTest, LifecycleSequence) {
    otaManager.beginUpdate(1024, "abc");
    otaManager.endUpdate();
    SUCCEED();
}

// markAppValid() multiple calls
TEST_F(OtaUpdateManagerTest, MarkAppValidMultiple) {
    otaManager.markAppValid();
    otaManager.markAppValid();
    SUCCEED();
}

// rollbackFirmware() multiple calls
TEST_F(OtaUpdateManagerTest, RollbackFirmwareMultiple) {
    otaManager.rollbackFirmware();
    otaManager.rollbackFirmware();
    SUCCEED();
}

// beginUpdate() after endUpdate()
TEST_F(OtaUpdateManagerTest, BeginAfterEnd) {
    otaManager.beginUpdate(1024, "abc");
    otaManager.endUpdate();
    EXPECT_FALSE(otaManager.beginUpdate(2048, "def"));
}

// writeStream() returns 0
TEST_F(OtaUpdateManagerTest, WriteStreamReturns0) {
    Stream stream;
    EXPECT_EQ(0, otaManager.writeStream(stream));
}

// registerRoutes()
TEST_F(OtaUpdateManagerTest, RegisterRoutes) {
    AsyncWebServer server(80);
    otaManager.registerRoutes(server);
    SUCCEED();
}

#if defined(NATIVE_BUILD)
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
