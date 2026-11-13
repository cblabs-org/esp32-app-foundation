/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: test/test_unit/test_configManager/test_configManager.cpp
 * Description: GoogleTest unit-test suite for the ConfigManager centralised
 *              configuration gateway. Validates module registration, beginAll()
 *              dispatch, saveModuleConfig, exportAllConfig, importAllConfig,
 *              schema version migration, and error handling.
 *
 *              All tests execute on the native GoogleTest runner (native builds).
 *              Config files are written as "test/tmp/mod/<ns>.json" in the working directory.
 *
 * Exported Functions:
 * - main(): GoogleTest entry point
 */

#include <gtest/gtest.h>
#include "ConfigManager.hpp"
#include "../test_helpers/MockTabRegistry.hpp"
#include <fstream>
#include <cstdio>
#include <string>
#include <filesystem>

// =============================================================================
// Mock IConfigurable Implementations
// =============================================================================

/** @brief Basic mock configurable with namespace "testmod" and schema v1. */
class MockModule : public IConfigurable {
public:
    bool applyConfigCalled = false;
    mutable bool serializeConfigCalled = false;
    bool resetConfigCalled = false;
    int lastFromVersion = -1;

    // Internal state that config should round-trip
    int setting1 = 42;
    bool setting2 = false;

    const char* getConfigNamespace() const override { return "testmod"; }
    int getSchemaVersion() const override { return 1; }

    void applyConfig(const JsonObject& section) override {
        applyConfigCalled = true;
        setting1 = section["setting1"] | 42;
        setting2 = section["setting2"] | false;
    }

    void serializeConfig(JsonObject& section) const override {
        serializeConfigCalled = true;
        section["setting1"] = setting1;
        section["setting2"] = setting2;
    }

    void resetConfig() override {
        resetConfigCalled = true;
        setting1 = 42;
        setting2 = false;
    }
};

/** @brief Mock module with a different namespace for multi-module tests. */
class MockModuleB : public IConfigurable {
public:
    bool applyConfigCalled = false;
    bool resetConfigCalled = false;
    std::string name = "default";

    const char* getConfigNamespace() const override { return "modB"; }
    int getSchemaVersion() const override { return 1; }

    void applyConfig(const JsonObject& section) override {
        applyConfigCalled = true;
        const char* n = section["name"] | "default";
        name = n;
    }

    void serializeConfig(JsonObject& section) const override {
        section["name"] = name.c_str();
    }

    void resetConfig() override {
        resetConfigCalled = true;
        name = "default";
    }
};

/** @brief Mock with schema version 2 that can migrate from v1. */
class MockModuleV2 : public IConfigurable {
public:
    bool migrateCalled = false;
    int lastFromVersion = -1;
    int value = 0;

    const char* getConfigNamespace() const override { return "v2mod"; }
    int getSchemaVersion() const override { return 2; }

    void applyConfig(const JsonObject& section) override {
        value = section["value"] | 0;
    }

    void serializeConfig(JsonObject& section) const override {
        section["value"] = value;
    }

    bool migrateConfig(JsonObject& section, int fromVersion) override {
        migrateCalled = true;
        lastFromVersion = fromVersion;
        // Migration: rename old_value → value
        if (section["old_value"].is<JsonVariant>()) {
            section["value"] = section["old_value"];
            section.remove("old_value");
        }
        return true;
    }

    void resetConfig() override {
        value = 0;
    }
};

// =============================================================================
// Test Fixture
// =============================================================================

class ConfigManagerTest : public ::testing::Test {
protected:
    ConfigManager configManager;

    void SetUp() override {
        std::error_code ec;
        std::filesystem::create_directories("test/tmp/mod", ec);
        // Clean up any leftover config files from previous test runs
        remove("test/tmp/mod/testmod.json");
        remove("test/tmp/mod/modB.json");
        remove("test/tmp/mod/v2mod.json");
        remove("test/tmp/config_manifest.json");
    }

    void TearDown() override {
        remove("test/tmp/mod/testmod.json");
        remove("test/tmp/mod/modB.json");
        remove("test/tmp/mod/v2mod.json");
        remove("test/tmp/config_manifest.json");
    }
};

// =============================================================================
// Test Group 1: Module Registration
// =============================================================================

TEST_F(ConfigManagerTest, RegisterModule_ValidPointer_ReturnsTrue) {
    MockModule mod;
    EXPECT_TRUE(configManager.registerModule(&mod));
}

TEST_F(ConfigManagerTest, RegisterModule_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(configManager.registerModule(nullptr));
}

TEST_F(ConfigManagerTest, RegisterModule_ManyModules_AllSucceed) {
    // Registry is now a dynamic vector — no hard cap. Verify multiple
    // modules register successfully.
    MockModule modA;
    MockModuleB modB;
    MockModuleV2 modC;
    EXPECT_TRUE(configManager.registerModule(&modA));
    EXPECT_TRUE(configManager.registerModule(&modB));
    EXPECT_TRUE(configManager.registerModule(&modC));
}

TEST_F(ConfigManagerTest, RegisterModule_AfterBeginAll_ReturnsFalse) {
    // After beginAll() locks the registry, further registrations must be
    // rejected — matching Application's two-tier locked-vector model.
    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    MockModule late;
    EXPECT_FALSE(configManager.registerModule(&late));
}

// =============================================================================
// Test Group 2: beginAll — Load dispatch
// =============================================================================

TEST_F(ConfigManagerTest, BeginAll_NoModules_DoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(configManager.beginAll());
}

TEST_F(ConfigManagerTest, BeginAll_NoFile_CallsResetConfig) {
    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    // No file on disk → module should get resetConfig() called
    EXPECT_TRUE(mod.resetConfigCalled);
}

TEST_F(ConfigManagerTest, BeginAll_WithFile_CallsApplyConfig) {
    // Pre-create a config file
    std::ofstream file("test/tmp/mod/testmod.json");
    file << R"({"setting1":99,"setting2":true,"_v":1})";
    file.close();

    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_TRUE(mod.applyConfigCalled);
    EXPECT_EQ(mod.setting1, 99);
    EXPECT_TRUE(mod.setting2);
}

TEST_F(ConfigManagerTest, BeginAll_CorruptedFile_CallsResetConfig) {
    std::ofstream file("test/tmp/mod/testmod.json");
    file << "{ invalid json : ";
    file.close();

    MockModule mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_TRUE(mod.resetConfigCalled);
}

TEST_F(ConfigManagerTest, BeginAll_MultipleModules_AllProcessed) {
    MockModule modA;
    MockModuleB modB;
    configManager.registerModule(&modA);
    configManager.registerModule(&modB);
    configManager.beginAll();

    // Both should have resetConfig called (no files exist)
    EXPECT_TRUE(modA.resetConfigCalled);
    EXPECT_TRUE(modB.resetConfigCalled);
}

// =============================================================================
// Test Group 3: saveModuleConfig
// =============================================================================

TEST_F(ConfigManagerTest, SaveModuleConfig_ValidModule_CreatesFile) {
    MockModule mod;
    mod.setting1 = 77;
    mod.setting2 = true;

    configManager.registerModule(&mod);
    EXPECT_TRUE(configManager.saveModuleConfig(&mod));

    // Verify file was created
    std::ifstream file("test/tmp/mod/testmod.json");
    EXPECT_TRUE(file.is_open());
}

TEST_F(ConfigManagerTest, SaveModuleConfig_Nullptr_ReturnsFalse) {
    EXPECT_FALSE(configManager.saveModuleConfig(nullptr));
}

TEST_F(ConfigManagerTest, SaveModuleConfig_Roundtrip_ValuesPreserved) {
    MockModule mod;
    mod.setting1 = 77;
    mod.setting2 = true;

    configManager.registerModule(&mod);
    EXPECT_TRUE(configManager.saveModuleConfig(&mod));

    // Read back via beginAll
    MockModule mod2;
    ConfigManager cm2;
    cm2.registerModule(&mod2);
    cm2.beginAll();

    EXPECT_TRUE(mod2.applyConfigCalled);
    EXPECT_EQ(mod2.setting1, 77);
    EXPECT_TRUE(mod2.setting2);
}

TEST_F(ConfigManagerTest, SaveModuleConfig_IncludesSchemaVersion) {
    MockModule mod;
    configManager.registerModule(&mod);
    configManager.saveModuleConfig(&mod);

    // Read the file directly
    std::ifstream file("test/tmp/mod/testmod.json");
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonDocument doc;
    deserializeJson(doc, jsonStr);
    EXPECT_EQ(doc["_v"].as<int>(), 1);
}

// =============================================================================
// Test Group 4: Schema Migration
// =============================================================================

TEST_F(ConfigManagerTest, BeginAll_OldVersion_CallsMigrateConfig) {
    // Create a v1 config file for a module that declares v2
    std::ofstream file("test/tmp/mod/v2mod.json");
    file << R"({"old_value":55,"_v":1})";
    file.close();

    MockModuleV2 mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_TRUE(mod.migrateCalled);
    EXPECT_EQ(mod.lastFromVersion, 1);
    EXPECT_EQ(mod.value, 55); // Should have migrated old_value → value
}

TEST_F(ConfigManagerTest, BeginAll_CurrentVersion_DoesNotMigrate) {
    // Create a v2 config file (current version)
    std::ofstream file("test/tmp/mod/v2mod.json");
    file << R"({"value":42,"_v":2})";
    file.close();

    MockModuleV2 mod;
    configManager.registerModule(&mod);
    configManager.beginAll();

    EXPECT_FALSE(mod.migrateCalled);
    EXPECT_EQ(mod.value, 42);
}

// =============================================================================
// Test Group 5: Bulk Export/Import
// =============================================================================

TEST_F(ConfigManagerTest, ExportAllConfig_ProducesNamespaceSections) {
    MockModule modA;
    modA.setting1 = 88;
    MockModuleB modB;
    modB.name = "exported";

    configManager.registerModule(&modA);
    configManager.registerModule(&modB);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    EXPECT_TRUE(outDoc["testmod"].is<JsonVariant>());
    EXPECT_TRUE(outDoc["modB"].is<JsonVariant>());
    EXPECT_EQ(outDoc["testmod"]["setting1"].as<int>(), 88);
    EXPECT_STREQ(outDoc["modB"]["name"].as<const char*>(), "exported");
}

TEST_F(ConfigManagerTest, ExportAllConfig_IncludesVersionPerSection) {
    MockModule mod;
    configManager.registerModule(&mod);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    EXPECT_EQ(outDoc["testmod"]["_v"].as<int>(), 1);
}

TEST_F(ConfigManagerTest, ImportAllConfig_AppliesAndPersists) {
    MockModule mod;
    configManager.registerModule(&mod);

    JsonDocument inDoc;
    JsonObject section = inDoc["testmod"].to<JsonObject>();
    section["setting1"] = 123;
    section["setting2"] = true;

    EXPECT_TRUE(configManager.importAllConfig(inDoc));
    EXPECT_EQ(mod.setting1, 123);
    EXPECT_TRUE(mod.setting2);

    // Verify persisted to disk
    std::ifstream file("test/tmp/mod/testmod.json");
    EXPECT_TRUE(file.is_open());
}

TEST_F(ConfigManagerTest, ImportAllConfig_MissingNamespace_Skipped) {
    MockModule mod;
    configManager.registerModule(&mod);

    JsonDocument inDoc;
    // Deliberately don't include "testmod" section
    inDoc["other"] = "data";

    EXPECT_TRUE(configManager.importAllConfig(inDoc));
    // Module should not have been touched
    EXPECT_FALSE(mod.applyConfigCalled);
}


// =============================================================================
// Test Group 6: _meta envelope — export/import WO1 behaviour
// =============================================================================

TEST_F(ConfigManagerTest, ExportAllConfig_ContainsMetaKey) {
    MockModule modA;
    MockModuleB modB;
    configManager.registerModule(&modA);
    configManager.registerModule(&modB);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    // _meta must be present and must be a JSON object
    ASSERT_TRUE(outDoc["_meta"].is<JsonVariant>());
    JsonObject meta = outDoc["_meta"].as<JsonObject>();
    EXPECT_FALSE(meta.isNull());

    // module_count must equal the number of registered modules (2)
    EXPECT_EQ(meta["module_count"].as<int>(), 2);
}

TEST_F(ConfigManagerTest, ImportAllConfig_SkipsMetaKey) {
    MockModule mod;
    configManager.registerModule(&mod);

    // Build a document that contains both a _meta envelope AND a valid namespace
    JsonDocument inDoc;
    JsonObject meta = inDoc["_meta"].to<JsonObject>();
    meta["backup_version"]   = 1;
    meta["firmware_version"] = "1.0.0-test";
    meta["module_count"]     = 1;

    JsonObject section = inDoc["testmod"].to<JsonObject>();
    section["setting1"] = 77;
    section["setting2"] = true;

    // Must not crash, must return true
    EXPECT_TRUE(configManager.importAllConfig(inDoc));

    // The valid namespace must have been applied
    EXPECT_TRUE(mod.applyConfigCalled);
    EXPECT_EQ(mod.setting1, 77);
    EXPECT_TRUE(mod.setting2);
}

TEST_F(ConfigManagerTest, ExportAllConfig_OtaNamespacePresent) {
    // Use a simple MockOtaConfigurable stub with namespace "ota"
    class MockOtaConfigurable : public IConfigurable {
    public:
        const char* getConfigNamespace() const override { return "ota"; }
        int getSchemaVersion() const override { return 1; }

        void applyConfig(const JsonObject& section) override {}

        void serializeConfig(JsonObject& section) const override {
            section["channel"]   = "stable";
            section["autoCheck"] = true;
        }

        void resetConfig() override {}
    };

    MockOtaConfigurable otaMod;
    configManager.registerModule(&otaMod);

    JsonDocument outDoc;
    configManager.exportAllConfig(outDoc);

    ASSERT_TRUE(outDoc["ota"].is<JsonVariant>());
    EXPECT_STREQ(outDoc["ota"]["channel"].as<const char*>(), "stable");
    EXPECT_TRUE(outDoc["ota"]["autoCheck"].as<bool>());
}

// =============================================================================
// Test Group 7: IWebContributor
// =============================================================================

TEST_F(ConfigManagerTest, RegisterTabs_BackupUnderSystem) {
    MockTabRegistry collector;
    configManager.registerTabs(collector);
    ASSERT_EQ(collector.entries.size(), 1u);
    EXPECT_EQ(collector.entries[0].name, "Backup");
    EXPECT_EQ(collector.entries[0].parent, "System");
    EXPECT_EQ(collector.entries[0].order, 40);
}

// =============================================================================
// Mock IConfigurable: PatchableModule — array-bearing config with clamping
// =============================================================================

/**
 * @brief Mock module with array config for JSON Patch integration tests.
 *
 * Serializes config with label, enabled, and channels array. The applyConfig()
 * method clamps `label` to 20 characters to test §7 (module validation stays
 * authoritative — the module's clamp must survive the patch roundtrip).
 */
class PatchableModule : public IConfigurable {
public:
    static constexpr int MAX_LABEL_LEN = 20;

    std::string label = "default";
    bool enabled = true;

    struct Channel {
        std::string name;
        std::string type;
    };
    std::vector<Channel> channels = {
        {"ch0", "voltage"},
        {"ch1", "current"},
        {"ch2", "voltage"}
    };

    const char* getConfigNamespace() const override { return "patchmod"; }
    int getSchemaVersion() const override { return 1; }

    void applyConfig(const JsonObject& section) override {
        const char* lbl = section["label"] | "default";
        label = std::string(lbl).substr(0, MAX_LABEL_LEN);  // clamp

        enabled = section["enabled"] | true;

        if (section["channels"].is<JsonArrayConst>()) {
            channels.clear();
            for (JsonObjectConst ch : section["channels"].as<JsonArrayConst>()) {
                Channel c;
                c.name = ch["name"] | "";
                c.type = ch["type"] | "";
                channels.push_back(c);
            }
        }
    }

    void serializeConfig(JsonObject& section) const override {
        section["label"] = label.c_str();
        section["enabled"] = enabled;
        JsonArray arr = section["channels"].to<JsonArray>();
        for (const auto& ch : channels) {
            JsonObject obj = arr.add<JsonObject>();
            obj["name"] = ch.name.c_str();
            obj["type"] = ch.type.c_str();
        }
    }

    void resetConfig() override {
        label = "default";
        enabled = true;
        channels = {{"ch0", "voltage"}, {"ch1", "current"}, {"ch2", "voltage"}};
    }
};

// =============================================================================
// Test Group 8: patchNamespaceConfig() Integration
// =============================================================================

class PatchIntegrationTest : public ::testing::Test {
protected:
    ConfigManager configManager;
    PatchableModule patchMod;

    void SetUp() override {
        std::error_code ec;
        std::filesystem::create_directories("test/tmp/mod", ec);
        remove("test/tmp/mod/patchmod.json");
        remove("test/tmp/config_manifest.json");

        configManager.registerModule(&patchMod);
        configManager.beginAll();
    }

    void TearDown() override {
        remove("test/tmp/mod/patchmod.json");
        remove("test/tmp/config_manifest.json");
    }

    /** @brief Serialize the module's current config to a JSON string for comparison. */
    std::string snapshotConfig() const {
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        patchMod.serializeConfig(root);
        std::string out;
        serializeJson(doc, out);
        return out;
    }
};

// 1. Mid-batch failure — ops 0-2 valid, op 3 invalid path → rollback
TEST_F(PatchIntegrationTest, MidBatchFailure_NamespaceBytesIdentical) {
    std::string before = snapshotConfig();

    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    // Op 0: valid replace
    JsonObject op0 = ops.add<JsonObject>();
    op0["op"] = "replace"; op0["path"] = "/label"; op0["value"] = "op0";
    // Op 1: valid replace
    JsonObject op1 = ops.add<JsonObject>();
    op1["op"] = "replace"; op1["path"] = "/enabled"; op1["value"] = false;
    // Op 2: valid replace on array element name
    JsonObject op2 = ops.add<JsonObject>();
    op2["op"] = "replace"; op2["path"] = "/channels/0/name"; op2["value"] = "ch0_patched";
    // Op 3: invalid path (non-existent)
    JsonObject op3 = ops.add<JsonObject>();
    op3["op"] = "replace"; op3["path"] = "/nonexistent/deep/path"; op3["value"] = "fail";

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);

    EXPECT_FALSE(ok);
    EXPECT_EQ(result.error, JsonPatch::Error::PATH_NOT_FOUND);
    EXPECT_EQ(result.opIndex, 3);

    // Config must be byte-identical to before (atomicity)
    std::string after = snapshotConfig();
    EXPECT_EQ(before, after);
}

// 2. Atomic test-and-replace — test matches, replace succeeds
TEST_F(PatchIntegrationTest, AtomicTestAndReplace_Succeeds) {
    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject testOp = ops.add<JsonObject>();
    testOp["op"] = "test"; testOp["path"] = "/label"; testOp["value"] = "default";
    JsonObject replOp = ops.add<JsonObject>();
    replOp["op"] = "replace"; replOp["path"] = "/label"; replOp["value"] = "new_label";

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(result.applied);
    EXPECT_EQ(patchMod.label, "new_label");
}

// 3. Test mismatch aborts batch — nothing changed
TEST_F(PatchIntegrationTest, TestMismatch_AbortsBatch) {
    std::string before = snapshotConfig();

    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject testOp = ops.add<JsonObject>();
    testOp["op"] = "test"; testOp["path"] = "/label"; testOp["value"] = "wrong";
    JsonObject replOp = ops.add<JsonObject>();
    replOp["op"] = "replace"; replOp["path"] = "/label"; replOp["value"] = "should_not_happen";

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);

    EXPECT_FALSE(ok);
    EXPECT_EQ(result.error, JsonPatch::Error::TEST_FAILED);
    EXPECT_EQ(result.opIndex, 0);

    std::string after = snapshotConfig();
    EXPECT_EQ(before, after);
}

// 4. Module clamping (§7) — over-length label is clamped by module
TEST_F(PatchIntegrationTest, ModuleClamping_OverLengthLabel_Clamped) {
    // Create a label longer than PatchableModule::MAX_LABEL_LEN (20)
    std::string longLabel(30, 'X');

    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject op = ops.add<JsonObject>();
    op["op"] = "replace"; op["path"] = "/label"; op["value"] = longLabel;

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(result.applied);
    // Module must have clamped to 20 characters
    EXPECT_EQ(patchMod.label.length(), static_cast<size_t>(PatchableModule::MAX_LABEL_LEN));
    EXPECT_EQ(patchMod.label, std::string(20, 'X'));
}

// 5. Single flash write — patch multiple fields → file has all changes
TEST_F(PatchIntegrationTest, SingleFlashWrite_MultipleFields_AllPersisted) {
    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();

    JsonObject op1 = ops.add<JsonObject>();
    op1["op"] = "replace"; op1["path"] = "/label"; op1["value"] = "patched";
    JsonObject op2 = ops.add<JsonObject>();
    op2["op"] = "replace"; op2["path"] = "/enabled"; op2["value"] = false;

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);
    EXPECT_TRUE(ok);

    // Read the persisted file and verify both fields
    std::ifstream file("test/tmp/mod/patchmod.json");
    ASSERT_TRUE(file.is_open());
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonDocument readDoc;
    DeserializationError err = deserializeJson(readDoc, jsonStr);
    EXPECT_EQ(err, DeserializationError::Ok);
    EXPECT_STREQ(readDoc["label"].as<const char*>(), "patched");
    EXPECT_FALSE(readDoc["enabled"].as<bool>());
}

// 6. Unknown namespace → error
TEST_F(PatchIntegrationTest, UnknownNamespace_ReturnsError) {
    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject op = ops.add<JsonObject>();
    op["op"] = "replace"; op["path"] = "/label"; op["value"] = "test";

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("nonexistent_ns", ops, result);

    EXPECT_FALSE(ok);
    EXPECT_EQ(result.error, JsonPatch::Error::PATH_NOT_FOUND);
    EXPECT_FALSE(result.applied);
}

// 7. Too many ops → TOO_MANY_OPS error
TEST_F(PatchIntegrationTest, TooManyOps_ReturnsTooManyOps) {
    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    // Add MAX_PATCH_OPS + 1 ops
    for (int i = 0; i <= JsonPatch::MAX_PATCH_OPS; ++i) {
        JsonObject op = ops.add<JsonObject>();
        op["op"] = "replace"; op["path"] = "/label"; op["value"] = "x";
    }

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);

    EXPECT_FALSE(ok);
    EXPECT_EQ(result.error, JsonPatch::Error::TOO_MANY_OPS);
    EXPECT_FALSE(result.applied);
}

// 8. Replace array element — /channels/1/name → value changes, others untouched
TEST_F(PatchIntegrationTest, ReplaceArrayElement_OthersUntouched) {
    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject op = ops.add<JsonObject>();
    op["op"] = "replace"; op["path"] = "/channels/1/name"; op["value"] = "ch1_patched";

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);

    EXPECT_TRUE(ok);
    EXPECT_EQ(patchMod.channels[1].name, "ch1_patched");
    // Other channels untouched
    EXPECT_EQ(patchMod.channels[0].name, "ch0");
    EXPECT_EQ(patchMod.channels[0].type, "voltage");
    EXPECT_EQ(patchMod.channels[2].name, "ch2");
    EXPECT_EQ(patchMod.channels[2].type, "voltage");
}

// 9. Remove on array index → ARRAY_REMOVE_BLOCKED
TEST_F(PatchIntegrationTest, RemoveArrayIndex_ArrayRemoveBlocked) {
    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject op = ops.add<JsonObject>();
    op["op"] = "remove"; op["path"] = "/channels/1";

    JsonPatch::PatchResult result;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result);

    EXPECT_FALSE(ok);
    EXPECT_EQ(result.error, JsonPatch::Error::ARRAY_REMOVE_BLOCKED);
}

// 10. Stored values returned — verify post-clamp values for patched paths
TEST_F(PatchIntegrationTest, StoredValues_ContainsPostClampValues) {
    // Patch with an over-length label so clamping is visible
    std::string longLabel(30, 'Z');

    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject op1 = ops.add<JsonObject>();
    op1["op"] = "replace"; op1["path"] = "/label"; op1["value"] = longLabel;
    JsonObject op2 = ops.add<JsonObject>();
    op2["op"] = "replace"; op2["path"] = "/enabled"; op2["value"] = false;

    JsonPatch::PatchResult result;
    JsonDocument storedDoc;
    bool ok = configManager.patchNamespaceConfig("patchmod", ops, result, &storedDoc);

    EXPECT_TRUE(ok);

    // Stored values should contain the post-clamp label (20 chars, not 30)
    const char* storedLabel = storedDoc["/label"].as<const char*>();
    ASSERT_NE(storedLabel, nullptr);
    EXPECT_EQ(strlen(storedLabel), static_cast<size_t>(PatchableModule::MAX_LABEL_LEN));

    // Enabled should be present too
    EXPECT_FALSE(storedDoc["/enabled"].as<bool>());
}

// =============================================================================
// Test Group 9: Write Ordering (Issue #32)
// =============================================================================

/**
 * @brief Second mock module for cross-namespace independence test.
 *
 * Must be a DIFFERENT namespace than PatchableModule ("patchmod") to verify
 * that version counters are per-namespace, not global.
 */
class SecondModule : public IConfigurable {
public:
    std::string value = "initial";

    const char* getConfigNamespace() const override { return "second"; }
    int getSchemaVersion() const override { return 1; }

    void applyConfig(const JsonObject& section) override {
        value = section["value"] | "initial";
    }

    void serializeConfig(JsonObject& section) const override {
        section["value"] = value.c_str();
    }

    void resetConfig() override {
        value = "initial";
    }
};

class WriteOrderingTest : public ::testing::Test {
protected:
    ConfigManager configManager;
    PatchableModule patchMod;
    SecondModule secondMod;

    void SetUp() override {
        std::error_code ec;
        std::filesystem::create_directories("test/tmp/mod", ec);
        remove("test/tmp/mod/patchmod.json");
        remove("test/tmp/mod/second.json");
        remove("test/tmp/config_manifest.json");

        configManager.registerModule(&patchMod);
        configManager.registerModule(&secondMod);
        configManager.beginAll();
    }

    void TearDown() override {
        remove("test/tmp/mod/patchmod.json");
        remove("test/tmp/mod/second.json");
        remove("test/tmp/config_manifest.json");
    }
};

// 1. Fresh write — normal sequential writes both persist and versions advance
TEST_F(WriteOrderingTest, FreshWrite_BothPersist) {
    // First save
    patchMod.label = "v1";
    EXPECT_TRUE(configManager.saveModuleConfig(&patchMod));

    // Second save
    patchMod.label = "v2";
    EXPECT_TRUE(configManager.saveModuleConfig(&patchMod));

    // Read back from file — should contain v2
    std::ifstream file("test/tmp/mod/patchmod.json");
    ASSERT_TRUE(file.is_open());
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonDocument readDoc;
    deserializeJson(readDoc, jsonStr);
    EXPECT_STREQ(readDoc["label"].as<const char*>(), "v2");
}

// 2. Monotonicity — N sequential writes yield N file writes and correct final state
TEST_F(WriteOrderingTest, Monotonicity_SequentialWritesAllPersist) {
    for (int i = 0; i < 5; ++i) {
        patchMod.label = "write_" + std::to_string(i);
        EXPECT_TRUE(configManager.saveModuleConfig(&patchMod));
    }

    // File should contain the last write
    std::ifstream file("test/tmp/mod/patchmod.json");
    ASSERT_TRUE(file.is_open());
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonDocument readDoc;
    deserializeJson(readDoc, jsonStr);
    EXPECT_STREQ(readDoc["label"].as<const char*>(), "write_4");
}

// 3. Cross-namespace independence — bumping version for one namespace
//    does NOT prevent writes to a different namespace.
//    This is THE guard for the per-namespace trap described in Issue #32.
TEST_F(WriteOrderingTest, CrossNamespace_IndependentVersions) {
    // Write patchmod several times to bump its version
    for (int i = 0; i < 5; ++i) {
        patchMod.label = "bump_" + std::to_string(i);
        EXPECT_TRUE(configManager.saveModuleConfig(&patchMod));
    }

    // Now write secondMod — this MUST succeed despite patchmod having
    // a high version counter. With a global counter, this would be skipped.
    secondMod.value = "independent";
    EXPECT_TRUE(configManager.saveModuleConfig(&secondMod));

    // Verify second.json has the correct value
    std::ifstream file("test/tmp/mod/second.json");
    ASSERT_TRUE(file.is_open());
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonDocument readDoc;
    deserializeJson(readDoc, jsonStr);
    EXPECT_STREQ(readDoc["value"].as<const char*>(), "independent");
}

// 4. Patch then save — version counters shared correctly between
//    patchNamespaceConfig() and saveModuleConfig()
TEST_F(WriteOrderingTest, PatchThenSave_VersionsShared) {
    // Patch first
    JsonDocument opsDoc;
    JsonArray ops = opsDoc.to<JsonArray>();
    JsonObject op = ops.add<JsonObject>();
    op["op"] = "replace"; op["path"] = "/label"; op["value"] = "patched";

    JsonPatch::PatchResult result;
    EXPECT_TRUE(configManager.patchNamespaceConfig("patchmod", ops, result));
    EXPECT_TRUE(result.applied);

    // Then save — should still work (higher version)
    patchMod.label = "saved_after_patch";
    EXPECT_TRUE(configManager.saveModuleConfig(&patchMod));

    // File should contain the save (latest)
    std::ifstream file("test/tmp/mod/patchmod.json");
    ASSERT_TRUE(file.is_open());
    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonDocument readDoc;
    deserializeJson(readDoc, jsonStr);
    EXPECT_STREQ(readDoc["label"].as<const char*>(), "saved_after_patch");
}

// 5. Stale-skip branch — artificially advance _lastPersistedVersion to prove
//    the skip logic actually works. Without this test, inverting the comparison
//    or deleting the _lastPersistedVersion assignment would go undetected.
//    Uses the write path to bump versions, then patches a DIFFERENT value but
//    checks that shouldSkipWrite() would reject a stale version.
TEST_F(WriteOrderingTest, StaleSkip_OlderVersionDoesNotOverwrite) {
    // beginAll() saves each module once (no file → reset → save), so
    // patchmod starts at version 1. Two explicit saves bring it to 3.
    patchMod.label = "first";
    EXPECT_TRUE(configManager.saveModuleConfig(&patchMod));  // version 2
    patchMod.label = "second";
    EXPECT_TRUE(configManager.saveModuleConfig(&patchMod));  // version 3, persisted = 3

    // Verify file has "second"
    {
        std::ifstream file("test/tmp/mod/patchmod.json");
        ASSERT_TRUE(file.is_open());
        std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        JsonDocument readDoc;
        deserializeJson(readDoc, jsonStr);
        EXPECT_STREQ(readDoc["label"].as<const char*>(), "second");
    }

    // Now test the stale-skip predicate directly.
    // This is the branch that carries the whole fix — without it, inverting
    // the comparison or deleting the _lastPersistedVersion assignment would
    // go undetected.
    int idx = 0;  // patchMod is registered first (slot 0)

    // Version 1 is stale (persisted is 3) → should skip
    EXPECT_TRUE(configManager.shouldSkipWrite(idx, 1));
    // Version 2 is also stale → should skip
    EXPECT_TRUE(configManager.shouldSkipWrite(idx, 2));

    // Same version as persisted → should NOT skip (not strictly less)
    EXPECT_FALSE(configManager.shouldSkipWrite(idx, 3));
    // Newer version → should NOT skip
    EXPECT_FALSE(configManager.shouldSkipWrite(idx, 4));

    // Invalid index → should NOT skip (fail open)
    EXPECT_FALSE(configManager.shouldSkipWrite(-1, 1));
    EXPECT_FALSE(configManager.shouldSkipWrite(99, 1));
}

// =============================================================================
// GoogleTest main entry point
// =============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
