# Architecture Decision Record: Application Hub

## Two-Tier Vector Model for Module Registry
To prevent runtime memory reallocations while avoiding hardcoded compile-time `MAX_*` limits, the `Application` class uses a two-tier vector model.
1. During `begin()` boot registration, modules are appended to `std::vector` lists.
2. At the end of the registration phase, `_compactAndLock()` is called, which invokes `shrink_to_fit()` on each vector and sets a `_locked` flag. All subsequent register attempts are rejected.

## Hollywood Principle Lifecycle
The `Application` class owns the lifecycle (`begin()`, `tick()`, and `softReset()`). The entry point (Arduino's `setup()` and `loop()`) delegates everything to the `Application` hub, which drives all registered modules. No application code inherits from a base app class.

## Native Build Compatibility with Stubbing
To allow unit tests to run on host environments, all hardware-specific APIs (such as `esp_restart()`, `LittleFS`, and free heap queries) are enclosed within `#ifndef NATIVE_BUILD ... #endif` preprocessor directives. This enables compiling and running tests on host CPU.
