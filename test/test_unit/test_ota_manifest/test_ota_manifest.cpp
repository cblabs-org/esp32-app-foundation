/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Test: test/test_unit/test_ota_manifest/test_ota_manifest.cpp
 * Description: GoogleTest suite for the OTAManifest v2 parser. Validates
 *              multi-channel JSON parsing, per-channel lookup, per-board
 *              asset lookup, overflow capping, null termination, and edge
 *              cases (empty channels, malformed JSON).
 */

#include <gtest/gtest.h>
#include "OTAManifest.hpp"
#include <cstring>

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class OTAManifestTest : public ::testing::Test {
protected:
    // Standard two-channel, two-board v2 manifest JSON used by most tests
    static constexpr const char* kValidManifest = R"({
        "manifestVersion": 1,
        "manifestUrl": "https://github.com/owner/repo/releases/download/ota-manifest/manifest.json",
        "channels": {
            "stable": {
                "version": "v3.1.0",
                "securityVersion": 1,
                "assets": [
                    {
                        "board": "esp32dev",
                        "firmwareUrl": "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32dev.bin",
                        "signatureUrl": "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32dev.bin.sig",
                        "size": 843776,
                        "sha256": "abc123def456789012345678901234567890123456789012345678901234"
                    },
                    {
                        "board": "esp32s3nano",
                        "firmwareUrl": "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32s3nano.bin",
                        "signatureUrl": "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32s3nano.bin.sig",
                        "size": 912384,
                        "sha256": "def456abc789012345678901234567890123456789012345678901234567"
                    }
                ]
            },
            "dev": {
                "version": "v3.2.0-dev.5",
                "securityVersion": 1,
                "assets": [
                    {
                        "board": "esp32dev",
                        "firmwareUrl": "https://github.com/owner/repo/releases/download/v3.2.0-dev.5/firmware-esp32dev.bin",
                        "signatureUrl": "https://github.com/owner/repo/releases/download/v3.2.0-dev.5/firmware-esp32dev.bin.sig",
                        "size": 855040,
                        "sha256": "1234567890abcdef1234567890abcdef1234567890abcdef1234567890ab"
                    }
                ]
            }
        }
    })";

    OTAManifest manifest{};

    void SetUp() override {
        memset(&manifest, 0, sizeof(manifest));
    }
};

// ---------------------------------------------------------------------------
// Parse valid v2 manifest — top-level fields
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseValidManifestTopLevel) {
    bool ok = parseManifest(kValidManifest, strlen(kValidManifest), manifest);
    ASSERT_TRUE(ok);

    EXPECT_EQ(manifest.manifestVersion, 1);
    EXPECT_STREQ(manifest.manifestUrl,
                 "https://github.com/owner/repo/releases/download/ota-manifest/manifest.json");
    EXPECT_EQ(manifest.channelCount, 2);
}

// ---------------------------------------------------------------------------
// Parse valid v2 manifest — stable channel fields
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseValidManifestStableChannel) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAChannel* stable = findChannel(manifest, "stable");
    ASSERT_NE(stable, nullptr);

    EXPECT_STREQ(stable->name, "stable");
    EXPECT_STREQ(stable->version, "v3.1.0");
    EXPECT_EQ(stable->securityVersion, 1u);
    EXPECT_EQ(stable->assetCount, 2);
}

// ---------------------------------------------------------------------------
// Parse valid v2 manifest — dev channel fields
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseValidManifestDevChannel) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAChannel* dev = findChannel(manifest, "dev");
    ASSERT_NE(dev, nullptr);

    EXPECT_STREQ(dev->name, "dev");
    EXPECT_STREQ(dev->version, "v3.2.0-dev.5");
    EXPECT_EQ(dev->securityVersion, 1u);
    EXPECT_EQ(dev->assetCount, 1);
}

// ---------------------------------------------------------------------------
// findChannel — unknown channel returns nullptr
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindChannelUnknownReturnsNull) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAChannel* ch = findChannel(manifest, "nightly");
    EXPECT_EQ(ch, nullptr);
}

// ---------------------------------------------------------------------------
// findAssetForBoard — esp32dev via stable channel
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindAssetForBoardEsp32dev) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAChannel* stable = findChannel(manifest, "stable");
    ASSERT_NE(stable, nullptr);

    const OTAAsset* asset = findAssetForBoard(*stable, "esp32dev");
    ASSERT_NE(asset, nullptr);

    EXPECT_STREQ(asset->board, "esp32dev");
    EXPECT_STREQ(asset->firmwareUrl,
                 "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32dev.bin");
    EXPECT_STREQ(asset->signatureUrl,
                 "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32dev.bin.sig");
    EXPECT_EQ(asset->size, 843776u);
    EXPECT_STREQ(asset->sha256, "abc123def456789012345678901234567890123456789012345678901234");
}

// ---------------------------------------------------------------------------
// findAssetForBoard — esp32s3nano via stable channel
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindAssetForBoardEsp32s3nano) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAChannel* stable = findChannel(manifest, "stable");
    ASSERT_NE(stable, nullptr);

    const OTAAsset* asset = findAssetForBoard(*stable, "esp32s3nano");
    ASSERT_NE(asset, nullptr);

    EXPECT_STREQ(asset->board, "esp32s3nano");
    EXPECT_STREQ(asset->firmwareUrl,
                 "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32s3nano.bin");
    EXPECT_STREQ(asset->signatureUrl,
                 "https://github.com/owner/repo/releases/download/v3.1.0/firmware-esp32s3nano.bin.sig");
    EXPECT_EQ(asset->size, 912384u);
}

// ---------------------------------------------------------------------------
// findAssetForBoard — unknown board returns nullptr
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindAssetForBoardUnknownReturnsNull) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAChannel* stable = findChannel(manifest, "stable");
    ASSERT_NE(stable, nullptr);

    const OTAAsset* asset = findAssetForBoard(*stable, "unknown_board");
    EXPECT_EQ(asset, nullptr);
}

// ---------------------------------------------------------------------------
// findAssetForBoard — board in dev channel but not stable
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, FindAssetForBoardCrossChannel) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    // Dev channel only has esp32dev
    const OTAChannel* dev = findChannel(manifest, "dev");
    ASSERT_NE(dev, nullptr);

    const OTAAsset* devAsset = findAssetForBoard(*dev, "esp32dev");
    ASSERT_NE(devAsset, nullptr);
    EXPECT_EQ(devAsset->size, 855040u);

    // esp32s3nano is NOT in the dev channel
    const OTAAsset* missing = findAssetForBoard(*dev, "esp32s3nano");
    EXPECT_EQ(missing, nullptr);
}

// ---------------------------------------------------------------------------
// Parse manifest with empty channels object
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseEmptyChannels) {
    const char* json = R"({
        "manifestVersion": 1,
        "manifestUrl": "https://example.com/manifest.json",
        "channels": {}
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);
    EXPECT_EQ(manifest.channelCount, 0);
    EXPECT_EQ(manifest.manifestVersion, 1);
}

// ---------------------------------------------------------------------------
// Parse manifest with missing channels key
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseMissingChannelsKey) {
    const char* json = R"({
        "manifestVersion": 1,
        "manifestUrl": "https://example.com/manifest.json"
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);
    EXPECT_EQ(manifest.channelCount, 0);
}

// ---------------------------------------------------------------------------
// Parse channel with empty assets array
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseChannelEmptyAssets) {
    const char* json = R"({
        "manifestVersion": 1,
        "manifestUrl": "https://example.com/manifest.json",
        "channels": {
            "beta": {
                "version": "v1.0.0-beta.1",
                "securityVersion": 0,
                "assets": []
            }
        }
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);
    EXPECT_EQ(manifest.channelCount, 1);

    const OTAChannel* beta = findChannel(manifest, "beta");
    ASSERT_NE(beta, nullptr);
    EXPECT_EQ(beta->assetCount, 0);
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
// Parse manifest with more than kMaxChannels — only first kMaxChannels stored
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseOverflowCapsAtMaxChannels) {
    // Build a manifest with 5 channels (exceeds kMaxChannels = 4)
    const char* json = R"({
        "manifestVersion": 1,
        "manifestUrl": "https://example.com/manifest.json",
        "channels": {
            "stable": { "version": "v1.0.0", "securityVersion": 1, "assets": [] },
            "beta":   { "version": "v1.1.0-beta.1", "securityVersion": 1, "assets": [] },
            "rc":     { "version": "v1.1.0-rc.1", "securityVersion": 1, "assets": [] },
            "dev":    { "version": "v1.2.0-dev.1", "securityVersion": 1, "assets": [] },
            "nightly": { "version": "v1.2.0-dev.99", "securityVersion": 1, "assets": [] }
        }
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);

    // Only kMaxChannels (4) should be stored
    EXPECT_EQ(manifest.channelCount, kMaxChannels);
}

// ---------------------------------------------------------------------------
// Parse channel with more than kMaxBoards assets — only first kMaxBoards stored
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ParseOverflowCapsAtMaxBoards) {
    const char* json = R"({
        "manifestVersion": 1,
        "manifestUrl": "https://example.com/manifest.json",
        "channels": {
            "stable": {
                "version": "v2.0.0",
                "securityVersion": 2,
                "assets": [
                    { "board": "board1", "firmwareUrl": "https://x/fw1.bin", "signatureUrl": "https://x/fw1.bin.sig", "size": 100, "sha256": "aaa" },
                    { "board": "board2", "firmwareUrl": "https://x/fw2.bin", "signatureUrl": "https://x/fw2.bin.sig", "size": 200, "sha256": "bbb" },
                    { "board": "board3", "firmwareUrl": "https://x/fw3.bin", "signatureUrl": "https://x/fw3.bin.sig", "size": 300, "sha256": "ccc" },
                    { "board": "board4", "firmwareUrl": "https://x/fw4.bin", "signatureUrl": "https://x/fw4.bin.sig", "size": 400, "sha256": "ddd" },
                    { "board": "board5", "firmwareUrl": "https://x/fw5.bin", "signatureUrl": "https://x/fw5.bin.sig", "size": 500, "sha256": "eee" },
                    { "board": "board6", "firmwareUrl": "https://x/fw6.bin", "signatureUrl": "https://x/fw6.bin.sig", "size": 600, "sha256": "fff" }
                ]
            }
        }
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);

    const OTAChannel* stable = findChannel(manifest, "stable");
    ASSERT_NE(stable, nullptr);

    // Only kMaxBoards (4) should be stored
    EXPECT_EQ(stable->assetCount, kMaxBoards);

    // First 4 boards should be present
    EXPECT_STREQ(stable->assets[0].board, "board1");
    EXPECT_STREQ(stable->assets[1].board, "board2");
    EXPECT_STREQ(stable->assets[2].board, "board3");
    EXPECT_STREQ(stable->assets[3].board, "board4");

    // board5 and board6 should NOT be findable
    EXPECT_EQ(findAssetForBoard(*stable, "board5"), nullptr);
    EXPECT_EQ(findAssetForBoard(*stable, "board6"), nullptr);
}

// ---------------------------------------------------------------------------
// Asset URLs are correctly null-terminated
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, UrlsAreNullTerminated) {
    ASSERT_TRUE(parseManifest(kValidManifest, strlen(kValidManifest), manifest));

    const OTAChannel* stable = findChannel(manifest, "stable");
    ASSERT_NE(stable, nullptr);

    for (uint8_t i = 0; i < stable->assetCount; i++) {
        // Verify each string field is null-terminated within its buffer
        size_t boardLen     = strlen(stable->assets[i].board);
        size_t firmwareLen  = strlen(stable->assets[i].firmwareUrl);
        size_t signatureLen = strlen(stable->assets[i].signatureUrl);
        size_t sha256Len    = strlen(stable->assets[i].sha256);

        EXPECT_LT(boardLen,     kMaxBoardIdLen) << "Board ID not null-terminated for asset " << i;
        EXPECT_LT(firmwareLen,  kMaxUrlLen)     << "Firmware URL not null-terminated for asset " << i;
        EXPECT_LT(signatureLen, kMaxUrlLen)     << "Signature URL not null-terminated for asset " << i;
        EXPECT_LT(sha256Len,    kMaxSha256Len)  << "SHA-256 not null-terminated for asset " << i;
    }
}

// ---------------------------------------------------------------------------
// securityVersion correctly parsed per channel
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, SecurityVersionParsedPerChannel) {
    const char* json = R"({
        "manifestVersion": 1,
        "manifestUrl": "https://example.com/manifest.json",
        "channels": {
            "stable": {
                "version": "v5.0.0",
                "securityVersion": 42,
                "assets": []
            },
            "dev": {
                "version": "v5.1.0-dev.1",
                "securityVersion": 7,
                "assets": []
            }
        }
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);

    const OTAChannel* stable = findChannel(manifest, "stable");
    ASSERT_NE(stable, nullptr);
    EXPECT_EQ(stable->securityVersion, 42u);

    const OTAChannel* dev = findChannel(manifest, "dev");
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->securityVersion, 7u);
}

// ---------------------------------------------------------------------------
// manifestVersion defaults to 0 when missing
// ---------------------------------------------------------------------------

TEST_F(OTAManifestTest, ManifestVersionDefaultsToZero) {
    const char* json = R"({
        "manifestUrl": "https://example.com/manifest.json",
        "channels": {}
    })";

    bool ok = parseManifest(json, strlen(json), manifest);
    ASSERT_TRUE(ok);
    EXPECT_EQ(manifest.manifestVersion, 0);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
