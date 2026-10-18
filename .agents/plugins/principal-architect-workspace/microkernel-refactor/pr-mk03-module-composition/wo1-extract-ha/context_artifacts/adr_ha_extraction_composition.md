# ADR: HomeAssistant Extraction & Honest main.cpp Composition

## Status
Decided & Implemented (WO1)

## Context
In the original framework design, `HomeAssistantModule` was an owned member of `Application` (`_haModule`), unconditionally instantiated and wired in `Application::begin()`. This violated microkernel separation principles (Ring-2 optional service directly owned by Ring-1 kernel).

## Decision
1. **Extract Ownership**: Remove `HomeAssistantModule _haModule`, `#include "../HomeAssistant/HomeAssistantModule.hpp"`, and `getHomeAssistant()` from `Application`.
2. **Availability Flag**: Guard `HomeAssistantModule` compilation and composition behind `#ifdef WITH_HOMEASSISTANT`.
3. **Honest `main.cpp` Composition**: Instantiate `HomeAssistantModule` at file scope in `main.cpp` (and app templates/samples) behind `#ifdef WITH_HOMEASSISTANT`. Wire dependencies (`KeyManager`, `EventBus`, `ConfigManager`, AppName, AppVersion) in `setup()` and register via `app.registerModule(&haModule)`.
4. **Conditional Module Implementation**: Wrap `HomeAssistantModule.hpp` and `HomeAssistantModule.cpp` contents in `#ifdef WITH_HOMEASSISTANT` so non-HA builds exclude all MQTT/HA overhead.

## Consequences
- `Application` no longer has compile-time or runtime dependencies on `HomeAssistantModule`.
- HomeAssistant is purely opt-in via `-D WITH_HOMEASSISTANT` in `platformio.ini`.
- `main.cpp` is the composition root where optional modules are explicitly wired and registered.
