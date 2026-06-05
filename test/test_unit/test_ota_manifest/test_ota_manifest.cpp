/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Test: test/test_unit/test_ota_manifest/test_ota_manifest.cpp
 * Description: GoogleTest suite for the OTAManifest parser. Validates JSON
 *              parsing, per-board asset lookup, overflow capping, null
 *              termination, and edge cases (empty assets, malformed JSON).
 */

#include <gtest/gtest.h>
#include "OTAManifest.hpp"
#include <cstring>

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class OTAManifestTest : public ::testing::Test {
protected:
    // Standard two-board manifest JSON used by most tests
    static constexpr const char* kValidManifest = R"({
        "version": "v3.1.0",
        "channel": "stable",
        "securityVersion": 1,
        "assets": [
            {
                "board": "esp32dev",
                "firmware": "firmware-esp32dev.bin",
                "signature": "firmware-esp32dev.bin.sig",
                "size": 843776,
                "sha256": "abc123def456789012345678901234567890123456789012345678901234"
            },
            {
                "board": "esp32s3nano",
                "firmware": "firmware-esp32s3nano.bin",
                "signature": "firmware-esp32s3nano.bin.sig",
                "size": 912384,
                "sha256": "def456abc789012345678901234567890123456789012345678901234567"
            }
        ]
    })";

    OTAManifest manifest{};

    void SetUp() override {
        memset(&manifest, 0, sizeof(manifest));
    }
};

// ---------------------------------------------------------------------------
// Parse valid manifest
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseValidManifest) {
    bool ok = parseManifest(kValidManifest, strlen(kValidManifest), manifest);
    ASSERT_TRUE(ok);

    EXPECT_STREQ(manifest.version, "v3.1.0");
    EXPECT_STREQ(manifest.channel, "stable");
    EXPECT_EQ(manifest.securityVersion, 1u);
    EXPECT_EQ(manifest.assetCount, 2);
}

// ---------------------------------------------------------------------------
// findAssetForBoard — esp32dev
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindAssetForBoardEsp32dev) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAAsset* asset = findAssetForBoard(manifest, "esp32dev");
    ASSERT_NE(asset, nullptr);

    EXPECT_STREQ(asset->board, "esp32dev");
    EXPECT_STREQ(asset->firmware, "firmware-esp32dev.bin");
    EXPECT_STREQ(asset->signature, "firmware-esp32dev.bin.sig");
    EXPECT_EQ(asset->size, 843776u);
    EXPECT_STREQ(asset->sha256, "abc123def456789012345678901234567890123456789012345678901234");
}

// ---------------------------------------------------------------------------
// findAssetForBoard — esp32s3nano
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindAssetForBoardEsp32s3nano) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAAsset* asset = findAssetForBoard(manifest, "esp32s3nano");
    ASSERT_NE(asset, nullptr);

    EXPECT_STREQ(asset->board, "esp32s3nano");
    EXPECT_STREQ(asset->firmware, "firmware-esp32s3nano.bin");
    EXPECT_STREQ(asset->signature, "firmware-esp32s3nano.bin.sig");
    EXPECT_EQ(asset->size, 912384u);
}

// ---------------------------------------------------------------------------
// findAssetForBoard — unknown board returns nullptr
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindAssetForBoardUnknownReturnsNull) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAAsset* asset = findAssetForBoard(manifest, "unknown_board");
    EXPECT_EQ(asset, nullptr);
}

// ---------------------------------------------------------------------------
// Parse empty assets array
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseEmptyAssetsArray) {
    const char* json = R"({
        "version": "v1.0.0",
        "channel": "beta",
        "securityVersion": 0,
        "assets": []
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);
    EXPECT_EQ(manifest.assetCount, 0);
}

// ---------------------------------------------------------------------------
// Parse malformed JSON returns false
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseMalformedJsonReturnsFalse) {
    const char* json = "{ this is not valid json!!!";

    bool ok = parseManifest(json, strlen(json), manifest);
    EXPECT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// Parse manifest with more than kMaxBoards assets — only first kMaxBoards stored
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseOverflowCapsAtMaxBoards) {
    // Build a manifest with 6 boards (exceeds kMaxBoards = 4)
    const char* json = R"({
        "version": "v2.0.0",
        "channel": "stable",
        "securityVersion": 2,
        "assets": [
            { "board": "board1", "firmware": "fw1.bin", "signature": "fw1.bin.sig", "size": 100, "sha256": "aaa" },
            { "board": "board2", "firmware": "fw2.bin", "signature": "fw2.bin.sig", "size": 200, "sha256": "bbb" },
            { "board": "board3", "firmware": "fw3.bin", "signature": "fw3.bin.sig", "size": 300, "sha256": "ccc" },
            { "board": "board4", "firmware": "fw4.bin", "signature": "fw4.bin.sig", "size": 400, "sha256": "ddd" },
            { "board": "board5", "firmware": "fw5.bin", "signature": "fw5.bin.sig", "size": 500, "sha256": "eee" },
            { "board": "board6", "firmware": "fw6.bin", "signature": "fw6.bin.sig", "size": 600, "sha256": "fff" }
        ]
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);

    // Only kMaxBoards (4) should be stored
    EXPECT_EQ(manifest.assetCount, kMaxBoards);

    // First 4 boards should be present
    EXPECT_STREQ(manifest.assets[0].board, "board1");
    EXPECT_STREQ(manifest.assets[1].board, "board2");
    EXPECT_STREQ(manifest.assets[2].board, "board3");
    EXPECT_STREQ(manifest.assets[3].board, "board4");

    // board5 and board6 should NOT be findable
    EXPECT_EQ(findAssetForBoard(manifest, "board5"), nullptr);
    EXPECT_EQ(findAssetForBoard(manifest, "board6"), nullptr);
}

// ---------------------------------------------------------------------------
// OTAAsset filenames are correctly null-terminated
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FilenamesAreNullTerminated) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    for (uint8_t i = 0; i < manifest.assetCount; i++) {
        // Verify each string field is null-terminated within its buffer
        size_t boardLen     = strlen(manifest.assets[i].board);
        size_t firmwareLen  = strlen(manifest.assets[i].firmware);
        size_t signatureLen = strlen(manifest.assets[i].signature);
        size_t sha256Len    = strlen(manifest.assets[i].sha256);

        EXPECT_LT(boardLen,     kMaxBoardIdLen)   << "Board ID not null-terminated for asset " << i;
        EXPECT_LT(firmwareLen,  kMaxFilenameLen)   << "Firmware name not null-terminated for asset " << i;
        EXPECT_LT(signatureLen, kMaxFilenameLen)   << "Signature name not null-terminated for asset " << i;
        EXPECT_LT(sha256Len,    kMaxSha256Len)     << "SHA-256 not null-terminated for asset " << i;
    }
}

// ---------------------------------------------------------------------------
// securityVersion correctly parsed as integer
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, SecurityVersionParsedAsInteger) {
    const char* json = R"({
        "version": "v5.0.0",
        "channel": "stable",
        "securityVersion": 42,
        "assets": []
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);
    EXPECT_EQ(manifest.securityVersion, 42u);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
