# modules

## Invariants & contracts

### [docseed-3e2cb6f7] **Clear dependency graph**: `adapters/` → depends on → `modules/` (via interface) + `hardware/` (via pin defines) + external libraries (LovyanGFX, ESP32Encoder). Neither `modules/` nor `hardware/` depend on `adapters/`.
`contract · kind: semantic · anchors: modules, hardware · source_doc: docs/developer/adr/adr-adapters-placement.md · evidence: doc:docs/developer/adr/adr-adapters-placement.md`

## Ruled out, failure signatures and gotchas

### [seed-258ab8e2] When building tests, the macOS sandbox blocked PlatformIO from finding `~/.platformio` toolchains, requiring the test to be run with `--gtest_filter` in the shell outside the sandbox to see compilation errors.
`ruled-out · kind: episodic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp, test/test_unit/test_precondition_engine/test_precondition_engine.cpp`

### [seed-93a32691] Testing the `Damping` mechanism with large `nowMs` jumps caused the penalty precision to fail; tests were updated to use more frequent `tick()` intervals to simulate the passage of time accurately.
`ruled-out · kind: episodic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp, test/test_unit/test_precondition_engine/test_precondition_engine.cpp`

### [seed-c26c9433] Initial edit of `OTAUpdateManager.cpp` resulted in a syntax error (include directive inside function body) due to multi-edit tool interaction; quickly repaired.
`ruled-out · kind: episodic · anchors: modules/OTAUpdateManager/OTAUpdateManager.cpp, web/portal/app.css`

### [seed-ed1d79bb] Initial include path iteration using `modules/ButtonModule/` prefix was revised to `ButtonModule/` per PA directive. Adding `-I modules` to `platformio.ini` was required to allow `ButtonModule/Header.h` resolution.
`ruled-out · kind: episodic · anchors: modules/ButtonModule/ButtonConfig.h, modules/EncoderModule/EncoderConfig.h, platformio.ini`

### [seed-ee8335b3] The WORKORDER specified `esp32dev_ota` as an environment to build, but it was not present in the configuration; I built `esp32dev` and `esp32s3nano` as proxies.
`ruled-out · kind: episodic · anchors: modules/OTAUpdateManager/OTAUpdateManager.cpp, web/portal/app.css`

## Decisions

### [seed-66067307] Set `MAX_PRECONDITIONS = 16` and `MAX_EDGES_PER_PRECONDITION = 8`.
`decision · kind: episodic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp, test/test_unit/test_precondition_engine/test_precondition_engine.cpp`

### [seed-845afbd3] In the test suite, directly `#include "modules/PreconditionEngine/PreconditionEngine.cpp"` to bypass PlatformIO's Library Dependency Finder not compiling the module by default for unit tests.
`decision · kind: episodic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp, test/test_unit/test_precondition_engine/test_precondition_engine.cpp`

### [seed-baf43b71] Used `std::pow(0.5f, elapsedSec / halfLifeSec)` in `PreconditionEngine.cpp` for accurate fractional exponential decay.
`decision · kind: episodic · anchors: modules/PreconditionEngine/PreconditionEngine.hpp, test/test_unit/test_precondition_engine/test_precondition_engine.cpp`

### [seed-e435855c] Replaced stack allocation with member variable storage on `DataManager` instance per Issue #175 resolution instructions.
`decision · kind: episodic · anchors: modules/DataManager/DataManager.hpp, modules/DataManager/DataManager.cpp`

## Procedures / Recipes

### [docseed-2306f035] **Lib extra dirs:** `modules` — framework kernel modules are treated as PlatformIO libraries
`recipe · kind: procedural · anchors: modules · source_doc: docs/developer/build-system.md · evidence: doc:docs/developer/build-system.md`
