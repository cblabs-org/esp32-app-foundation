# Graveyard: Native Build Quirks Encountered in WO1

## Issue 1: `pio build -e native` fails with "undefined _main"

**Symptom:** Running `mcp_platformio_build_project` with `environment: "native"` exits
with linker error:
```
Undefined symbols for architecture arm64:
  "_main", referenced from: <initial-undefines>
ld: symbol(s) not found for architecture arm64
*** [.pio/build/native/program] Error 1
```

**Root cause:** The `native` environment is a test-only environment. Its `src/main.cpp`
contains `setup()` and `loop()` (Arduino convention) but no `int main()`. The test
binary entry point is provided by GoogleTest (`googletest/src/gtest_main.cc`), which is
only linked when building test targets — not the main program.

**Correct command:** Use `mcp_platformio_run_tests` with `environment: "native"` (not
`build_project`). This links against gtest_main and produces the correct binary.

**Result with correct command:** 525/525 tests pass, including all OTA test suites.

**Important:** OTAUpdateManager itself compiled cleanly (`libOTAUpdateManager.a`
archived) even during the failing `build` run — the error is a test-harness linker issue
only, not a code compilation failure.

## Issue 2: `freertos/idf_additions.h` availability

**Concern from WORKORDER.md:** "If the symbol [xTaskCreateWithCaps] is unavailable,
check `#include "freertos/idf_additions.h"`."

**Resolution:** Adding `#include <freertos/idf_additions.h>` inside the
`#ifndef NATIVE_BUILD` guard was required and sufficient. The header is present in
ESP-IDF 5.5.4 (`framework-espidf @ 3.50504.0`) used by this project.
The esp32s3nano build compiled with zero errors, confirming the include path resolves.

## Issue 3: Device HTTP API unreachable from host

**Symptom:** `curl http://192.168.4.1/api/ota` times out. Host is on 192.168.86.x.

**Root cause:** The device had no WiFi station credentials configured (NVS wiped or
stale credentials). It fell back to captive-portal AP mode (`esp32-foundation` SSID,
192.168.4.1). The host machine was not connected to this AP.

**Mitigation:** API correctness was verified by code inspection (grep confirmed the two
new field assignments are inside the status handler before `serializeJson`). Serial log
confirmed the device boots healthy with full PSRAM access (8.6 MB free heap).

**How to verify in future:** Connect host machine to the `esp32-foundation` AP OR 
configure WiFi STA credentials via the portal first so the device gets a STA IP 
discoverable via `discovery.py`.
