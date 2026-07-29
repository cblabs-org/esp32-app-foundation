# Architecture Decision Record: EventBus Concurrency

## Decision
1. **Target Concurrency:** Implemented `esp_event` backing for the `EventBus` on ESP32 targets (esp32dev, esp32s3nano) to ensure thread-safety and robust event loop concurrency. 
2. **Native Mocking:** Preserved the original synchronous, `std::vector`-based subscription iteration exclusively for the GoogleTest native environment.
3. **Macro Pivot:** Changed the preprocessor guard from `#ifndef NATIVE_BUILD` to `#ifdef UNIT_TEST`. 

## Context and Rationale
The `EventBus` required thread-safety on-target to prevent corruption during multi-threaded `publish()` calls. The ESP-IDF `esp_event` API was selected as the engine.
During implementation, the initial design proposed using `#ifndef NATIVE_BUILD` to isolate the `esp_event` implementation. However, a prior architectural refactor (PR-41) eliminated the `NATIVE_BUILD` macro globally in favor of a stub-header architecture. As a result, compiling `EventBus.cpp` for native testing with `#ifndef NATIVE_BUILD` mistakenly exposed the `esp_event` target code to the native test compiler, leading to dependency failures (missing `esp_event.h` and undeclared types like `esp_err_t`). 
Switching to `#ifdef UNIT_TEST` correctly isolated the native stub path because `UNIT_TEST` is reliably defined by PlatformIO when compiling `env:native`.
