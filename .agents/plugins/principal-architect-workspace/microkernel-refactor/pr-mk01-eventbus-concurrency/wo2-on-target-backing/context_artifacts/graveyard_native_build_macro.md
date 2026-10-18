# Graveyard: NATIVE_BUILD Macro

**Failed Attempt:** Using `#ifndef NATIVE_BUILD` to isolate hardware code in `EventBus.cpp` during `env:native` builds.

**Symptom:**
`pio test -e native` failed with errors such as:
- `fatal error: 'esp_event.h' file not found`
- `error: unknown type name 'esp_err_t'`
- `error: use of undeclared identifier 'APP_EVENT_BASE'`

**Root Cause:**
A previous PR (PR-41) eliminated the `NATIVE_BUILD` macro across the repository to enforce a stub header architecture. Because `NATIVE_BUILD` is no longer defined in `platformio.ini` for `env:native`, `#ifndef NATIVE_BUILD` blocks evaluates to true, exposing hardware-specific types to the host compiler.

**Resolution:**
Use `#ifdef UNIT_TEST` to detect the GoogleTest host runner environment. `UNIT_TEST` remains defined by PlatformIO when running tests.
