# ADR: String Stub Consolidation

## Problem

Four identical (but slightly divergent) `String` stub class definitions existed across the codebase, each guarded by `STRING_STUB_DEFINED`:
1. `lib/Logger/Logger.hpp` — most complete (had `operator=`, `length()`)
2. `modules/MCPServer/MCPServer.hpp` — had `operator=` from String
3. `modules/WifiManager/WifiManager.hpp` — minimal
4. `modules/OTAUpdateManager/OTAUpdateManager.hpp` — minimal

## Decision

Consolidate into a single `include/platform/NativeBuildStubs.hpp` header.

## Include Path Strategy

Framework **modules** use relative paths to reach `include/`:
```cpp
#include "../../include/platform/NativeBuildStubs.hpp"
```

Framework **libraries** use the PlatformIO include path (libraries get `include/` on their search path):
```cpp
#include "platform/NativeBuildStubs.hpp"
```

This asymmetry exists because libraries under `lib/` have different include resolution rules than modules under `modules/`. The relative path for modules is necessary because `modules/` is not on the default include search path — only `include/` and `lib/*/` are.

## Future Considerations

If other native-build stubs are needed (e.g., WiFiClient, esp_timer), they can be added to the same file under the `NATIVE_BUILD` guard, keeping all platform stubs centralized.
