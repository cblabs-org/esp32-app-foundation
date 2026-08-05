/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: test/test_unit/test_config_manifest/test_config_manifest.cpp
 * Description: GoogleTest suite for the ConfigManifest Three-Tier lifecycle.
 *              Validates slot creation, version tracking, snapshot operations,
 *              capacity limits, and multi-module independence.
 */

#include <gtest/gtest.h>
#include "ConfigManifest.hpp"
#include <cstring>

class ConfigManifestTest : public ::testing::Test {
protected:
    ConfigManifest manifest;

    void SetUp() override {
        // Each test starts with a fresh, empty manifest
    }
};

// ---------------------------------------------------------------------------
// Basic update and query
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, UpdateActiveAndRetrieve) {
    manifest.updateActive("ota", 2);
    EXPECT_EQ(manifest.getActiveVersion("ota"), 2);
}

TEST_F(ConfigManifestTest, UpdateGoldenAndRetrieve) {
    manifest.updateGolden("wifi", 1);
    EXPECT_EQ(manifest.getGoldenVersion("wifi"), 1);
}

TEST_F(ConfigManifestTest, UpdatePreviousAndRetrieve) {
    manifest.updatePrevious("time", 3);
    EXPECT_EQ(manifest.getPreviousVersion("time"), 3);
}

// ---------------------------------------------------------------------------
// Unknown namespace returns -1
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, UnknownNamespaceReturnsNegativeOne) {
    EXPECT_EQ(manifest.getActiveVersion("unknown"), -1);
    EXPECT_EQ(manifest.getPreviousVersion("unknown"), -1);
    EXPECT_EQ(manifest.getGoldenVersion("unknown"), -1);
}

TEST_F(ConfigManifestTest, GetSlotUnknownReturnsNull) {
    EXPECT_EQ(manifest.getSlot("nonexistent"), nullptr);
}

// ---------------------------------------------------------------------------
// Snapshot: Active → Previous
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, SnapshotActiveToPreviousCopiesAll) {
    manifest.updateActive("ota", 2);
    manifest.updateActive("wifi", 1);
    manifest.updateActive("time", 3);

    // Previous should all be 0 initially
    EXPECT_EQ(manifest.getPreviousVersion("ota"), 0);
    EXPECT_EQ(manifest.getPreviousVersion("wifi"), 0);
    EXPECT_EQ(manifest.getPreviousVersion("time"), 0);

    manifest.snapshotActiveToPrevious();

    EXPECT_EQ(manifest.getPreviousVersion("ota"), 2);
    EXPECT_EQ(manifest.getPreviousVersion("wifi"), 1);
    EXPECT_EQ(manifest.getPreviousVersion("time"), 3);

    // Active should remain unchanged
    EXPECT_EQ(manifest.getActiveVersion("ota"), 2);
    EXPECT_EQ(manifest.getActiveVersion("wifi"), 1);
    EXPECT_EQ(manifest.getActiveVersion("time"), 3);
}

// ---------------------------------------------------------------------------
// Snapshot: Active → Golden
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, SnapshotActiveToGoldenCopiesAll) {
    manifest.updateActive("ota", 2);
    manifest.updateActive("wifi", 1);

    // Golden should be 0 initially
    EXPECT_EQ(manifest.getGoldenVersion("ota"), 0);
    EXPECT_EQ(manifest.getGoldenVersion("wifi"), 0);

    manifest.snapshotActiveToGolden();

    EXPECT_EQ(manifest.getGoldenVersion("ota"), 2);
    EXPECT_EQ(manifest.getGoldenVersion("wifi"), 1);

    // Active should remain unchanged
    EXPECT_EQ(manifest.getActiveVersion("ota"), 2);
    EXPECT_EQ(manifest.getActiveVersion("wifi"), 1);
}

// ---------------------------------------------------------------------------
// Capacity limit: kMaxModules exceeded
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, ExceedMaxModulesReturnsGracefully) {
    // Fill all slots
    char ns[kMaxNamespaceLen];
    for (size_t i = 0; i < kMaxModules; i++) {
        snprintf(ns, sizeof(ns), "mod%zu", i);
        manifest.updateActive(ns, (int)(i + 1));
    }

    // Verify all slots are filled
    for (size_t i = 0; i < kMaxModules; i++) {
        snprintf(ns, sizeof(ns), "mod%zu", i);
        EXPECT_EQ(manifest.getActiveVersion(ns), (int)(i + 1));
    }

    // Attempt to add one more — should not crash
    manifest.updateActive("overflow", 99);
    EXPECT_EQ(manifest.getActiveVersion("overflow"), -1);
}

// ---------------------------------------------------------------------------
// Multiple modules tracked independently
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, MultipleModulesIndependent) {
    manifest.updateActive("ota", 2);
    manifest.updateActive("wifi", 1);
    manifest.updateGolden("ota", 1);
    manifest.updatePrevious("wifi", 0);

    EXPECT_EQ(manifest.getActiveVersion("ota"), 2);
    EXPECT_EQ(manifest.getActiveVersion("wifi"), 1);
    EXPECT_EQ(manifest.getGoldenVersion("ota"), 1);
    EXPECT_EQ(manifest.getGoldenVersion("wifi"), 0);  // default, not explicitly set
    EXPECT_EQ(manifest.getPreviousVersion("wifi"), 0);
    EXPECT_EQ(manifest.getPreviousVersion("ota"), 0);  // default, not explicitly set
}

// ---------------------------------------------------------------------------
// Update same namespace multiple times — latest value retained
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, UpdateSameNamespaceRetainsLatest) {
    manifest.updateActive("ota", 1);
    EXPECT_EQ(manifest.getActiveVersion("ota"), 1);

    manifest.updateActive("ota", 2);
    EXPECT_EQ(manifest.getActiveVersion("ota"), 2);

    manifest.updateActive("ota", 5);
    EXPECT_EQ(manifest.getActiveVersion("ota"), 5);
}

// ---------------------------------------------------------------------------
// getSlot returns correct data
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, GetSlotReturnsCorrectData) {
    manifest.updateActive("ota", 2);
    manifest.updatePrevious("ota", 1);
    manifest.updateGolden("ota", 1);

    const ModuleSlot* slot = manifest.getSlot("ota");
    ASSERT_NE(slot, nullptr);
    EXPECT_STREQ(slot->ns, "ota");
    EXPECT_EQ(slot->activeVersion, 2);
    EXPECT_EQ(slot->previousVersion, 1);
    EXPECT_EQ(slot->goldenVersion, 1);
}

// ---------------------------------------------------------------------------
// Snapshot does not affect other tiers
// ---------------------------------------------------------------------------

TEST_F(ConfigManifestTest, SnapshotToPreviousDoesNotAffectGolden) {
    manifest.updateActive("ota", 3);
    manifest.updateGolden("ota", 1);

    manifest.snapshotActiveToPrevious();

    EXPECT_EQ(manifest.getPreviousVersion("ota"), 3);
    EXPECT_EQ(manifest.getGoldenVersion("ota"), 1);  // Unchanged
}

TEST_F(ConfigManifestTest, SnapshotToGoldenDoesNotAffectPrevious) {
    manifest.updateActive("ota", 3);
    manifest.updatePrevious("ota", 1);

    manifest.snapshotActiveToGolden();

    EXPECT_EQ(manifest.getGoldenVersion("ota"), 3);
    EXPECT_EQ(manifest.getPreviousVersion("ota"), 1);  // Unchanged
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
