# ADR: EventBus Boot-Order Race Condition

## Context

During hardware validation of the CONFIGURING orchestration (PR #68), the display module's configuring screen never appeared. The carousel kept rotating as if the `APP_CONFIGURING` event was never received.

## Root Cause

`EventBus::subscribe()` on ESP32 calls `esp_event_handler_instance_register()` to register handlers on the IDF default event loop. However, this loop is created by `esp_event_loop_create_default()`, which was only called inside `EventBus::begin()`.

The display module subscribes to `APP_CONFIGURING` in its `begin()` method, which runs **before** `EventBus::begin()`. At that point, `esp_event_loop_create_default()` hasn't been called. The IDF function `esp_event_handler_instance_register()` **silently returns ESP_OK** even when no event loop exists — it does not fail or log a warning. The subscription is simply lost.

## Decision

Added `_ensureEventLoop()` — a private method that lazily calls `esp_event_loop_create_default()` on first use. Both `subscribe()` and `begin()` call it, making boot order irrelevant. A static `s_eventLoopCreated` flag prevents double-creation.

## Alternatives Rejected

| Alternative | Why Rejected |
|---|---|
| Reorder `begin()` calls so EventBus comes first | Fragile — any future module could break the ordering. The framework should be resilient to init order. |
| Clear and re-register all subscribers in `begin()` | Was attempted first. `_subscribers.clear()` in `begin()` wiped pre-registered subscriptions but didn't fix the underlying issue — the IDF registration was the problem, not the vector. |
| Add error checking on `esp_event_handler_instance_register` return | IDF returns `ESP_OK` even when the loop doesn't exist (at least on ESP-IDF 5.x). Can't detect the failure this way. |

## Files Changed

- `modules/EventBus/EventBus.cpp` — Added `_ensureEventLoop()`, called from `subscribe()` and `begin()`
- `modules/EventBus/EventBus.hpp` — Added `_ensureEventLoop()` declaration

## Key Lesson

ESP-IDF's default event loop APIs silently succeed when the loop doesn't exist. Always create the loop before first use, and defensive lazy-init is the safest pattern.
