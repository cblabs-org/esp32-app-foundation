# ADR: TimeManager Native Build Stubs

## Decision
We removed the `#ifdef NATIVE_BUILD` guards inside the TimeManager module to test the real implementation in the native testing environment.

## Context
Previously, TimeManager provided a mock implementation of its core methods (`begin`, `tick`, `getLocalTime`, etc.) when compiled under `NATIVE_BUILD`. This meant the core logic was completely skipped during unit tests.

## Consequences
By removing the mock implementation, the native build now compiles the actual logic. This surfaced several missing stubs:
- We added `esp_sntp.h` to `test/stubs/platform/` with an inline mock for `sntp_get_sync_status` and `configTzTime`.
- We had to fix `LittleFS` stubs by adding `readBytes` and `rename`.
- We added `write` methods to the `String` class stub in `Arduino.h` to satisfy ArduinoJson's `Writer` requirements.
- The unit tests for TimeManager were updated to accurately reflect the real implementation's state machine: calling `begin()` only transitions `_syncStatus` to `IN_PROGRESS`. We had to explicitly call `tick(0)` after `begin()` in tests to advance the status to `SYNCED`.
