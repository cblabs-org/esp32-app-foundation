# Graveyard: Kernel Services

## Missing Hardware during Verification
During WO6 execution, the worker agent attempted to automatically acquire a physical ESP32 device to test the OTA Update Manager using the PlatformIO MCP tools. The attempt resulted in a timeout during the upload phase because the physical device was unavailable or busy.
**Lesson Learned**: Automated integration tests requiring physical hardware must be clearly flagged and queued for manual testing, or require a guaranteed lock on a simulated/remote test bench.

## Monolithic Service Anti-Pattern
Early considerations to bundle all OS-level features into a single `systemManager` were discarded in favor of granular services (`timeManager`, `otaUpdateManager`, etc.) to adhere to the House Style and strict OOP guidelines.
