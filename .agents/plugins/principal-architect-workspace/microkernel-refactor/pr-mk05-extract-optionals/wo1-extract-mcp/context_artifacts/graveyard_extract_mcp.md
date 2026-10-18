# Graveyard: MCPServer Extraction Lessons & Pitfalls (`pr-mk05-wo1`)

## 1. Include Path Resolution under PlatformIO `lib_extra_dirs`
- **Issue**: Specifying `#include "MCPServer/MCPServer.hpp"` in `main.cpp` caused a `MissingHeader` error during `esp32dev` build.
- **Cause**: PlatformIO's `lib_extra_dirs = modules` setting adds each module folder (e.g. `modules/MCPServer/`) directly to the compiler include path.
- **Fix**: Use `#include "MCPServer.hpp"` when including from `main.cpp` or external consumers.

## 2. Unit Test Transitive Header Dependency
- **Issue**: `test/test_unit/test_application/test_application.cpp` failed to compile after `Application.hpp` stopped including `MCPServer.hpp`.
- **Cause**: `test_application.cpp` relied on `Application.hpp` transitively including `MCPServer.hpp`.
- **Fix**: Explicitly `#include "MCPServer.hpp"` in `test_application.cpp`.

## 3. `mcpContributors` Vector Size Assumption in Tests
- **Issue**: A test assertion `ASSERT_EQ(mcpContribs.size(), 1u)` in `test_application.cpp` failed (actual size was 4).
- **Cause**: Built-in core domain services (`WifiManager`, `ConfigManager`, `OTAUpdateManager`) also implement `IMCPContributor` and register themselves into `_mcpContributors` during `Application::begin()`.
- **Fix**: Check `EXPECT_GE(mcpContribs.size(), 1u)` and verify the mock contributor's presence using `std::find`.
