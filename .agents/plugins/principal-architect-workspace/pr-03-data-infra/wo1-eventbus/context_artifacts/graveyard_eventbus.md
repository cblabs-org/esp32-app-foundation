# Graveyard: EventBus

No major failed attempts occurred during the implementation of WO1. 

**Noteworthy Observations**:
- **Include Paths**: When writing the native tests (`test/test_unit/test_eventbus/test_main.cpp`), it was initially ambiguous how `EventBus.hpp` should be included. Due to the `platformio.ini` containing `lib_extra_dirs = modules` inside the `[env:base_esp32]` and `[env:native]` definitions, PlatformIO treats each directory inside `modules/` as an independent library. Thus, `#include "EventBus.hpp"` correctly resolves without needing the `eventBus/` prefix or relative path hacking. This works consistently across native host execution and hardware builds.
- **Background Processes**: Build processes (both `native` testing and hardware compiling) run smoothly via `pio run` background tasks and correctly resolve in parallel if required.
