# ADR: WO3 Test Design Decisions

## Decision 1: Inline `MockOtaConfigurable` for `ExportAllConfig_OtaNamespacePresent`

**Context:** The WORKORDER said "if OTAUpdateManager is not usable in the native test fixture, use a simple `MockOtaConfigurable` stub". OTAUpdateManager has WiFi/hardware dependencies that make it unsuitable for native builds.

**Decision:** Used a locally-defined inline class within the test body itself (C++11 local class). This keeps the mock close to the test, avoids polluting the top-level mock namespace, and mirrors what the WORKORDER explicitly recommended.

**Why not a file-level mock class?** The mock is single-purpose and only used once. A file-level class would add cognitive overhead for future readers. The inline pattern is standard in GoogleTest for small, single-use stubs.

## Decision 2: `ImportAllConfig_SkipsMetaKey` asserts `applyConfigCalled`

**Context:** The WORKORDER said "assert no crash, assert return value is true, assert the valid module's `applyConfig` was called".

**Decision:** The test registers `MockModule` with namespace `testmod`, injects both `_meta` and `testmod` sections, then asserts `mod.applyConfigCalled == true`. This directly validates the spy pattern already established by earlier tests.

**Why this matters:** Without asserting `applyConfigCalled`, the test only proves "no crash" — it doesn't prove `_meta` was skipped AND the valid module was processed correctly.

## Decision 3: `ExportAllConfig_ContainsMetaKey` checks `module_count` dynamically

**Context:** The `buildBackupMeta()` implementation sets `module_count = _moduleCount` at export time.

**Decision:** Register exactly 2 modules, then assert `meta["module_count"] == 2`. This tests the live count rather than a hard-coded sentinel, ensuring the test would catch regressions where `_moduleCount` is not propagated.

## Decision 4: Server.js used `_v` field per-namespace (not just top-level)

**Context:** WO1 firmware `exportAllConfig()` sets `section["_v"] = mod->getSchemaVersion()` for each namespace section.

**Decision:** `MOCK_CONFIG_BACKUP` mirrors this exactly: each namespace object has `_v: 1`. This makes the mock a faithful representation of real firmware output for portal integration testing.
