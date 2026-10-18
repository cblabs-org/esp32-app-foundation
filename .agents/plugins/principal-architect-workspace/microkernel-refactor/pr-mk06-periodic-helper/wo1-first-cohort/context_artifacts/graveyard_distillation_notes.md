# Graveyard / Troubleshooting Notes — WO1 First Cohort

## Issues Encountered & Resolved

1. **`test_periodic.cpp` Missing Main Entry Point**:
   - **Symptom:** PlatformIO GoogleTest build error `undefined reference to main`.
   - **Cause:** Native test executables in PlatformIO require `int main(int argc, char** argv)` to initialize GoogleTest (`InitGoogleTest` + `RUN_ALL_TESTS`).
   - **Fix:** Added `int main()` block to `test/test_unit/test_periodic/test_periodic.cpp`.

2. **Undeclared Identifier `now` in `Application.cpp`**:
   - **Symptom:** Compiler error `use of undeclared identifier 'now'`.
   - **Cause:** When refactoring `(now - _lastGateCheckMs) >= GATE_CHECK_INTERVAL_MS` in `Application.cpp`, the variable declaration `unsigned long now = currentMillis;` was removed, but subsequent gate regression timer tracking statements on lines 305, 306, 308 still referenced `now`.
   - **Fix:** Updated lines to use `currentMillis`.

3. **`LVGLDisplay` Missing `lvgl.h` on Non-LVGL Targets**:
   - **Symptom:** `esp32dev` compilation failure due to missing `lvgl.h` header in `modules/LVGLDisplay`.
   - **Cause:** `platformio.ini` included `modules` in `lib_extra_dirs`, causing LDF to attempt compiling `LVGLDisplay` for `esp32dev` even though `lvgl` library was not in `esp32dev` `lib_deps`.
   - **Fix:** Added `lib_ignore = LVGLDisplay` to `[env:base_esp32]` in `platformio.ini`.
