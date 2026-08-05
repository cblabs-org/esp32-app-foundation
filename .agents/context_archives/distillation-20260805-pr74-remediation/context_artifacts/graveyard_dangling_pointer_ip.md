# Graveyard: Dangling Pointer in IP Change Detection

## The Bug

Added IP change detection in `Application::tick()` to re-publish `APP_CONFIGURING` when the device IP changes (e.g., WiFi connects and IP goes from 0.0.0.0 to real address). The initial code:

```cpp
const char* currentIP = WiFi.localIP().toString().c_str();
if (strcmp(currentIP, _deviceIPBuf) != 0) {
    changed = true;
}
```

## Why It Failed

`WiFi.localIP().toString()` returns a **temporary Arduino `String` object**. Calling `.c_str()` returns a pointer into its internal buffer. The temporary `String` is destroyed at the semicolon. `strcmp` then reads a **dangling pointer** — undefined behavior.

In practice, the memory was quickly overwritten, so `strcmp` always saw "different" content → `changed = true` on every tick → re-publish spam every 500ms. The monitor log showed `Config gate unsatisfied: Weather Setup (status=2)` repeating endlessly.

## The Fix

Store the `String` in a local variable to keep it alive through the `strcmp`:

```cpp
String currentIPStr = WiFi.localIP().toString();
if (strcmp(currentIPStr.c_str(), _deviceIPBuf) != 0) {
    changed = true;
}
```

## Key Lesson for Future Agents

**NEVER** do `const char* p = someArduinoObject.toString().c_str()` — the temporary `String` is destroyed immediately. Always store the `String` in a named variable first. This pattern appears frequently in Arduino/ESP32 code and is a common source of subtle, hard-to-reproduce bugs.

This same class of bug can occur with:
- `WiFi.localIP().toString().c_str()`
- `WiFi.macAddress().c_str()`
- `String(...).c_str()` (any temporary String)
