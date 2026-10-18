# Architecture Decision Record: Native HomeAssistant

## Decision
Removed `#ifdef NATIVE_BUILD` guards from `HomeAssistantModule.cpp` and `HomeAssistantModule.hpp` and updated `test_native` tests and PlatformIO testing stubs for native builds to mirror standard ESP32 dependencies.

## Rationale
To ensure the production code compiles cleanly across both environments without conditional branching, we migrated native-specific behavior into standard PlatformIO stubs (`Arduino.h`, `mqtt_client.h`, `PsychicHttp.h`).
