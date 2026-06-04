# ADR: ConfigManager Registry — Fixed Array → Locked Vector

## Decision

Replace `ConfigManager`'s fixed `IConfigurable*[8]` C-array + `uint8_t _moduleCount` with `std::vector<IConfigurable*> _modules` + `bool _locked`, matching `Application`'s two-tier locked-vector model.

## Rationale

**Consistency**: `Application` uses the locked-vector pattern for all its own registries (`_tickables`, `_webContributors`, etc.). `ConfigManager` was the only place in the framework using a fixed C-array + counter for the same conceptual pattern. Inconsistency makes the codebase harder to reason about.

**Safety**: The fixed-array had no post-`beginAll()` lock. A module registered after `beginAll()` would silently be accepted but never have its config loaded. The locked-vector makes this a loud, logged error.

**No embedded penalty**: The `std::vector` is already used extensively in this codebase. The pointer array itself is small (at most ~10 pointers × 4 bytes = 40 bytes). This is not a memory concern.

## Alternatives Considered

**Keep fixed array**: Marginally more memory-deterministic on embedded (no heap allocation for the pointer array). Rejected because vector is already pervasive in the codebase, and the consistency argument outweighs the marginal heap savings.

## Changes Made

**`ConfigManager.hpp`**:
- Removed `static constexpr uint8_t MAX_CONFIGURABLES = 8`
- Removed `IConfigurable* _modules[MAX_CONFIGURABLES] = {}`
- Removed `uint8_t _moduleCount = 0`
- Added `std::vector<IConfigurable*> _modules`
- Added `bool _locked = false`
- Added `#include <vector>`
- Updated class docstring

**`ConfigManager.cpp`**:
- `registerModule()`: checks `_locked` instead of `_moduleCount >= MAX_CONFIGURABLES`; uses `push_back()`
- `beginAll()`: adds `_locked = true` + `_modules.shrink_to_fit()` before iterating; range-for loop
- `exportAllConfig()`: range-for
- `importAllConfig()`: range-for in both inner loops
- `buildBackupMeta()`: `_modules.size()` instead of `_moduleCount`
- `/api/config` route handler: range-for, `_modules.size()` for count

## Build Verification

`libConfigManager.a` compiled and archived cleanly in the native build. The 5 errors reported in the same build run are pre-existing gaps in `ScreensaverController` introduced by the display refactor and are unrelated to this change.
