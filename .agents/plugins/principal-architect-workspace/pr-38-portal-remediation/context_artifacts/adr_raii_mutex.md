# ADR: RAII FreeRTOS Mutex for Entity Thread Safety

## Context

The `WeatherModule` implements `IDataSource`, meaning its `fetchUpdate()` runs on a background FreeRTOS task managed by `DataManager`. It writes to `HaSensor::setState()` from this background task while the main loop and MQTT event handler read from `HaSensor::getState()`. Since `std::string` is not thread-safe and can trigger heap reallocation during concurrent access, this constitutes a data race (undefined behavior).

## Decision

Created `include/platform/FreeRTOSMutex.hpp` — an RAII wrapper around `xSemaphoreCreateMutex()` that satisfies C++ `Lockable`, enabling direct use with `std::lock_guard<FreeRTOSMutex>`.

### Why RAII over virtual lock()/unlock() on IHaEntity

| Approach | Verdict |
|----------|---------|
| Virtual `lock()`/`unlock()` on `IHaEntity` | Rejected — leaks on early returns, pollutes interface with platform detail, callers must remember to unlock |
| RAII `std::lock_guard` | Chosen — leak-proof, zero caller burden, idiomatic C++, invisible to consumers |

### Why not std::mutex

`std::mutex` is not available on ESP-IDF's newlib without `CONFIG_COMPILER_CXX_EXCEPTIONS=y`. FreeRTOS semaphores are the native primitive.

### Platform shim pattern

Follows `LogShim.h` convention: real mutex on hardware (`#ifndef NATIVE_BUILD`), no-op stubs for GoogleTest native builds.

## Consequences

- `HaSensor` gains a `mutable FreeRTOSMutex _mutex` member (+8 bytes DRAM per entity)
- `setState()` and `getState()` are internally locked — zero changes to call sites
- Other entities (`HaSwitch`, `HaCover`, `HaSelect`) have `std::string _state` but are currently single-threaded; add mutex when they gain cross-thread writers
