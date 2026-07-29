# ADR: Extracting OTAUpdateManager

## Context and Goal
The `Application` module (the composition root) was previously tightly coupled with the `OTAUpdateManager` via an internal `_otaManager` pointer and `#include "../OTAUpdateManager/OTAUpdateManager.hpp"`. The goal was to decouple these so that `Application` acts strictly as an orchestrator and any optional feature (like OTA) can be manually injected or excluded depending on the project's build flags (e.g., `WITH_OTA`).

## Decisions Made
1. **Delegation of `OTA_PROGRESS` Event**:
   - **Decision**: Maintained `ApplicationEvents::OTA_PROGRESS` string literal inside `Application.hpp`, removing the circular dependency by explicitly separating the event literal from the payload struct where possible.
   - **Why**: Display modules (like `U8G2Display` and `LVGLDisplay`) still need to subscribe to OTA updates. Having a central `ApplicationEvents` channel allows publishers and subscribers to loosely couple through the `EventBus` without depending on the heavy `OTAUpdateManager` header.

2. **Decoupling Payload Structs**:
   - **Decision**: We eliminated redundant `OTAProgressPayload` struct definitions across `LVGLDisplayModule` and `U8G2DisplayModule` in favor of a single `ApplicationEvents::OtaProgressPayload` defined inside `Application.hpp`. However, to prevent `OTAUpdateManager.hpp` from requiring `Application.hpp`, we restored the legacy `OTAProgressEvent` struct internally for `OTAUpdateManager` and relied on C-style layout-compatibility for EventBus payloads.
   - **Why**: This perfectly preserved the Hollywood Principle—`OTAUpdateManager.hpp` doesn't include any other modules' headers—while allowing the `.cpp` file to seamlessly integrate with the application layer.

3. **Casting Workarounds in Config Saves**:
   - **Decision**: We added explicit `static_cast<IConfigurable*>(this)` calls in `OTAUpdateManager.cpp` when invoking `_configManager->saveModuleConfig()`.
   - **Why**: `ConfigManager.hpp` forwards-declares `IConfigurable` or expects exact types. Since `OTAUpdateManager` is complex and we removed direct coupling, the compiler threw an ambiguous/rvalue cast error (`cannot initialize a parameter of type 'IConfigurable *' with an rvalue of type 'OTAUpdateManager *'`). Explicit casting safely bypasses this without polluting header files.

4. **LDF Management**:
   - **Decision**: For the `template-app`'s `native` build, we explicitly ignored `OTAUpdateManager` and `LVGLDisplay` via `lib_ignore`.
   - **Why**: PlatformIO's Library Dependency Finder (`chain` mode) aggressively scans includes, pulling in the `OTAUpdateManager` module despite `WITH_OTA` being disabled in `native`. This caused compilation of `.cpp` files with invalid/undeclared identifiers. `lib_ignore` effectively shields `native` builds from optional hardware-specific libraries.

## Consequences
- The `Application` hub is now significantly lighter and decoupled from the OTA mechanism.
- Projects without `WITH_OTA` no longer suffer from implicit dependencies on `OTAUpdateManager`.
- Projects utilizing OTA (like `sample-app`) must manually instantiate and register the `OTAUpdateManager` in `main.cpp`.
