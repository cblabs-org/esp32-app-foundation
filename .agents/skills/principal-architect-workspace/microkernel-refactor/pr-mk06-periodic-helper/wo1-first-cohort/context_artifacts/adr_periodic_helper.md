# ADR: Periodic Helper Class Design and Application Core Throttling

## Context
Across the framework (~87 sites), non-blocking time throttling logic was hand-rolled using raw `millis()` comparisons (`now - lastMs >= intervalMs`). This caused duplication and boilerplate.

## Decision
1. Introduced a lightweight, header-only `Periodic` class in `include/utils/Periodic.hpp`.
2. Designed `due(uint32_t now)` to update the internal `_lastFireMs` timestamp to `now` upon returning `true`, allowing single-expression usage in loop ticks: `if (_periodic.due(currentMillis)) { ... }`.
3. Handled 32-bit `uint32_t` overflow seamlessly via unsigned arithmetic `now - _lastFireMs >= _intervalMs`.
4. Migrated the first cohort (`Application::tick()` gate regression check and 30-second heartbeat log) to use `Periodic`.

## Rationale
- Zero runtime overhead / zero virtual method dispatch.
- Purely in-loop context, avoiding cross-task FreeRTOS software timers (which run in the timer service task and require mutex locking).
