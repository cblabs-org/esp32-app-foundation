# test/stubs/platform

## Invariants & contracts

### [docseed-18b38291] Identify the header that provides the API (e.g., `esp_timer.h`).
`invariant · kind: semantic · anchors: test/stubs/platform, test/stubs/platform/esp_timer.h, modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/testing.md · evidence: doc:docs/developer/testing.md`

### [docseed-77ba0fa1] Provide a minimal implementation of the required functions, macros, or types. If the API is complex, you can define the mock class/function behavior or simply stub a no-op if the test does not require the return value.
`invariant · kind: semantic · anchors: test/stubs/platform, test/stubs/platform/esp_timer.h, modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/testing.md · evidence: doc:docs/developer/testing.md`

### [docseed-844d8bbc] **API Mirroring**: Stub headers (`test/stubs/platform/`) mirror the exact API surface of ESP-IDF and Arduino functions. During native test builds (`env:native`), the include path priority ensures the compiler resolves `#include <esp_system.h>` to the stub implementation.
`invariant · kind: semantic · anchors: test/stubs/platform · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-b4cf47d8] Create a mock header in `test/stubs/platform/` matching that exact name (e.g., `test/stubs/platform/esp_timer.h`).
`invariant · kind: semantic · anchors: test/stubs/platform, test/stubs/platform/esp_timer.h · source_doc: docs/developer/testing.md · evidence: doc:docs/developer/testing.md`

## Ruled out, failure signatures and gotchas

### [seed-0980777d] In D41, rephrased "ESPAsyncWebServer" to "legacy asynchronous web server library" so that zero occurrences of `AsyncWebServer` are found in §12–§17.
`ruled-out · kind: episodic · anchors: platformio.ini, library.json, test/stubs/platform, test/test_unit, docs/developer/adr`

### [seed-74d70437] Avoided substituting fresh static numbers in §14 and §16 per [PA-9]/[PA-10] — only pointers to dynamic measurement procedures were documented.
`ruled-out · kind: episodic · anchors: platformio.ini, library.json, test/stubs/platform, test/test_unit, docs/developer/adr`

### [seed-95626753] Calling `WiFi.begin(ssid, pass)` directly in Arduino-ESP32 resets `threshold.rssi` to `-127` and clears `btm_enabled`/`rm_enabled`. Using the two-stage configuration (`WiFi.begin(..., false)` + `esp_wifi_set_config`) cleanly sets these parameters before initiating association.
`ruled-out · kind: episodic · anchors: test/stubs/platform/WiFi.h, test/stubs/platform/esp_wifi.h, test/test_unit/test_wifiManager/test_wifiManager.cpp`

## Decisions

### [seed-03c4aa31] **Do NOT compile the generated `ui/*.c` on native.** It's SquareLine-generated widget construction (regenerated each export) and is verified by the hardware compile + on-device visual check — not a unit-test target. Native tests cover *our* logic (`SLSSystemScreens`, `LVGLDisplayModule`) against mock `lv_obj_t` widgets. (Confirmed acceptable with the human mid-WO.)
`decision · kind: episodic · anchors: test/stubs/platform/lvgl.h, modules/LVGLDisplay/library.json`

### [seed-10be90e2] Defined the two-tier architectural decision documentation model: §17 is the summary ledger/index in the design specification; `docs/developer/adr/` contains deep ADR documents for multi-module architectural trade-offs.
`decision · kind: episodic · anchors: platformio.ini, library.json, test/stubs/platform, test/test_unit, docs/developer/adr`

### [seed-648d0893] Keeps `exclude_on_native.py` unchanged (respects the WO boundary box — no module `.cpp` modifications).
`decision · kind: episodic · anchors: modules/Application/Application.cpp, test/stubs/platform`

### [seed-81385e64] Chose `-75 dBm` as the default RSSI threshold: this provides a robust balance between connection resilience in fringe locations and filtering out weak beacons when stronger mesh nodes are present on the same SSID.
`decision · kind: episodic · anchors: test/stubs/platform/WiFi.h, test/stubs/platform/esp_wifi.h, test/test_unit/test_wifiManager/test_wifiManager.cpp`

### [seed-9ed13ddb] The shim is self-documenting and only affects the test_application build target.
`decision · kind: episodic · anchors: modules/Application/Application.cpp, test/stubs/platform`

### [seed-9fdf9b81] Used ESP-IDF `wifi_sta_config_t` directly during connection setup (`WiFi.begin(..., false)` followed by `esp_wifi_get_config`/`esp_wifi_set_config` and `esp_wifi_connect`) to avoid Arduino core's default override of `threshold.rssi = -127` and disabled BTM flags.
`decision · kind: episodic · anchors: test/stubs/platform/WiFi.h, test/stubs/platform/esp_wifi.h, test/test_unit/test_wifiManager/test_wifiManager.cpp`

### [seed-a5840402] Formatted Architectural Decision Log with an explicit Status column (`Implemented`, `Superseded`, `Deferred`) for every entry D1–D54.
`decision · kind: episodic · anchors: platformio.ini, library.json, test/stubs/platform, test/test_unit, docs/developer/adr`

### [seed-bf946bdd] Adopted strict Stub-Header Architecture documentation in §16, fully obsoleting `#ifndef NATIVE_BUILD` in production code and documenting `-Itest/stubs/platform`.
`decision · kind: episodic · anchors: platformio.ini, library.json, test/stubs/platform, test/test_unit, docs/developer/adr`

### [seed-dc7fd0b8] The shim compiles Application.cpp as part of the test translation unit, giving the linker all needed symbols.
`decision · kind: episodic · anchors: modules/Application/Application.cpp, test/stubs/platform`

### [seed-dee360e5] Made `lv_label` stubs stateful rather than keeping tests structural — unlocks value assertions (SSID/URL/message population) for a few lines.
`decision · kind: episodic · anchors: test/stubs/platform/lvgl.h, modules/LVGLDisplay/library.json`

## Procedures / Recipes

### [docseed-277373db] **Native Unit Test Builds:** Following the WO1 cleanup, native builds resolve `esp_log.h` through clean test stubs located at `test/stubs/platform/esp_log.h` via PlatformIO native include path flags (`-Itest/stubs/platform`), eliminating `#ifndef NATIVE_BUILD` preprocessor clutter from production headers.
`recipe · kind: procedural · anchors: test/stubs/platform/esp_log.h · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-ea0c5caa] **Stub-Header Architecture**: Native host tests resolve hardware dependencies via stub headers in `test/stubs/platform/`, completely eliminating the need for invasive preprocessor guards in production source files.
`recipe · kind: procedural · anchors: test/stubs/platform · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

## Open issues

### [seed-ca1b7fe1] Same recipe (a stateful stub + a suite-local `ui`/board stub + drop the ignore) would revive `test_u8g2Display` and `test_application`, still dormant. Left for a separate item.
`open-issue · kind: semantic · anchors: test/stubs/platform/lvgl.h, modules/LVGLDisplay/library.json`

### [seed-fdf166e6] `docs/design_specification.md` still contains isolated mentions of `AsyncWebServer` (line 1623 in §9) and `NATIVE_BUILD` (lines 227, 1589, 1889 in §2, §8, §11A), which belong to sections outside WO5's boundary box (§12–§17). These should be cleaned up during their respective work orders or in a final cleanup pass.
`open-issue · kind: semantic · anchors: platformio.ini, library.json, test/stubs/platform, test/test_unit, docs/developer/adr`
