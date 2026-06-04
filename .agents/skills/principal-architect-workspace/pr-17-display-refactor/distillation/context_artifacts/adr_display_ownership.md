# ADR: Display Ownership — Application as Composition Root

## Decision
`U8G2DisplayModule` and `DiagnosticsBoard` are owned as member variables of `Application`, not as file-scope variables in `main.cpp`.

## Context
Prior to this change, `main.cpp` acted as a mini-orchestrator: it declared `display` and `diagnosticsBoard` at file scope, called `display.bootstrapHardware()`, manually wired `display` as status reporter, registered it with ConfigManager, EventBus, web contributors, tickables, and MCP server. This violated the Hollywood Principle — `Application` is the composition root but wasn't actually composing the display.

## Approach
- Two-phase bootstrap: Phase 1 (`bootstrapHardware()`) runs before `begin()` for immediate boot splash; Phase 2 (`display.begin(&_eventBus)`) runs after all services are initialized
- `#ifndef NATIVE_BUILD` guards wrap all display members in Application.hpp since U8G2/SPI headers are unavailable in native test builds
- `_statusReporter = &_display` is set in Phase 1, routing all boot progress to the display immediately
- WO5 hooks (`_onEnterConfiguring`, `_onEnterRunning`) drive the display from Application state transitions

## Rejected
- Making U8G2DisplayModule header-safe for native builds (option B) — too invasive for this WO, would require mocking SPI/U8G2 in native
- Making display "optional" via ifdefs — user explicitly said "we will always customise/extend the application code"
