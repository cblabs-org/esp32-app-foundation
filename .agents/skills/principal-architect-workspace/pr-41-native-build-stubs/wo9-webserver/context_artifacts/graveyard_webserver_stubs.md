# Graveyard: WebServer Native Build Stubs

## Failed Attempts and Dead Ends

1. **Scripting Header Guard Removal vs `#else` Blocks**:
   When aggressively removing `#ifndef NATIVE_BUILD` blocks, a naive python script removed everything between `#else` and `#endif`. For `WebServerManager.hpp`, this accidentally deleted the core API definitions (e.g., `PsychicHttpServer`) which had been mocked inside the `#else` block. The fix was to use a script that just dropped the `#else` but preserved the fallback, wait no, for `.hpp` we actually *wanted* to delete the mock classes since they were already extracted to `PsychicHttp.h`. However, the naive script kept the original code inside the `#ifndef` block but discarded the `#else` mock, which successfully eliminated the inline mocks.

2. **Accidental Removal of Transitive Types (`esp_system.h`)**:
   During the cleanup of `PsychicHttp.h` stub, the include for `"esp_system.h"` was accidentally removed. This broke `ConfigManager` and `WifiManager` which were implicitly relying on it for `ESP.restart()` and `esp_restart()`. **Lesson**: When refactoring stubs, ensure transitive includes that other modules rely upon are not silently dropped.

3. **Missing Stub Functions (`LittleFS.h`, `Arduino.h`)**:
   `WebServerManager.cpp` uses `File::isDirectory()`, `File::openNextFile()`, and `String::startsWith()`. Initially, the native build failed because these methods were missing from the stubs. Adding these directly to the stubs in `test/stubs/platform/` solved the issue without modifying the source `WebServerManager.cpp`.
