---
name: pr-28-wo10
description: GithubClient WiFiClientSecure heap alloc, OTA stack hardening, embedded safety fixes
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Fix GithubClient `WiFiClientSecure` stack allocation (H-MEM-01), harden OTA
task stack budget (EMB-02/EMB-08), and apply embedded safety annotations
(EMB-01, EMB-03, EMB-04, EMB-05).

# Strict Boundary Box

- **ONLY** modify files listed in the checklist below.
- Do **NOT** change any module under `modules/` except `OTAUpdateManager`.
- Do **NOT** change any interface files under `include/interfaces/`.
- Do **NOT** modify test files.
- Do **NOT** create or push any beta or stable git tags. You MAY create dev
  tags per the versioning rules below.

# Versioning Rules

Read `.agents/rules/03-versioning.md` for full details. Summary for this WO:
- The current latest tag is `v0.0.10-dev.7`.
- If you do a flash test build with new functionality, tag it `v0.0.10-dev.8`
  **before** building: `git tag v0.0.10-dev.8 && git push origin v0.0.10-dev.8`
- `inject_version.py` reads the tag at build time — tag MUST be on HEAD before build.
- You MAY create dev tags autonomously.
- You MUST NOT create beta or stable tags.

# Device Info

- IP: `192.168.20.186`
- Board: ESP32-S3 NanoS3
- PlatformIO env: `esp32dev`
- The device MCP client skill is at `.agents/skills/device-mcp-client/SKILL.md`

# Checklist

## 1. GithubClient WiFiClientSecure → DRAM heap allocation (H-MEM-01)

**File:** `lib/GithubClient/GithubClient.cpp`

- [ ] **Line 55:** Replace `WiFiClientSecure client;` with heap allocation:
  ```cpp
  auto client = std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure());
  if (!client) {
      ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure for release query");
      return false;
  }
  ```
  Then update all `client.` references to `client->` in `getLatestRelease()`.

- [ ] **Line 230:** Same pattern for `WiFiClientSecure pageClient;` inside the
  paginated loop in `getLatestReleaseForChannel()`. Move the allocation
  **outside** the loop if possible (reuse the client across pages), or
  allocate/free inside each iteration with null check.

- [ ] Add `#include <memory>` to the includes if not already present.

- [ ] **IMPORTANT: DRAM-first allocation.** Because GithubClient runs on the
  OTA task and PSRAM cannot be used during flash writes, use standard `new
  (std::nothrow)` which defaults to DRAM on ESP-IDF. Do NOT use
  `heap_caps_malloc(MALLOC_CAP_SPIRAM)` for the client object.

## 2. OTA manifestBuf → static (EMB-08)

**File:** `modules/OTAUpdateManager/OTAUpdateManager.cpp`

- [ ] Find `char manifestBuf[2048]` (around line 575) and change to:
  ```cpp
  // Static buffer — only one OTA pipeline runs at a time (enforced by _state guard).
  // Saves 2 KB of stack headroom on the 16 KB OTA task.
  static char manifestBuf[2048];
  ```

## 3. Add stack high-water-mark logging (EMB-02)

**File:** `modules/OTAUpdateManager/OTAUpdateManager.cpp`

- [ ] At the end of `performUpdateTask()`, just before the `vTaskDelete(nullptr)`,
  add:
  ```cpp
  #ifndef NATIVE_BUILD
  ESP_LOGI(TAG, "OTA task stack high-water mark: %u bytes free",
           static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t)));
  #endif
  ```

## 4. PSRAM safety comment (EMB-01)

**File:** `lib/GithubClient/GithubClient.cpp`

- [ ] At lines ~89 and ~257 (the `heap_caps_malloc(MALLOC_CAP_SPIRAM)` calls),
  add a comment:
  ```cpp
  // SAFETY: PSRAM buffer is allocated and freed within this function scope.
  // It MUST be freed before any flash write operation begins (called from
  // OTA task where PSRAM and flash share the SPI bus).
  ```

## 5. Remove dead TWDT comment (EMB-03)

**File:** `lib/OTADownloadEngine/OTADownloadEngine.cpp`

- [ ] Find the commented-out `// esp_task_wdt_reset();` (around line 397) and
  remove the line entirely.

## 6. Add delay(2000) safety comment (EMB-04)

**File:** `modules/OTAUpdateManager/OTAUpdateManager.cpp`

- [ ] Find `delay(2000)` before `ESP.restart()` (around line 778) and add:
  ```cpp
  // SAFETY: TWDT was deregistered at line ~761. This delay is safe only
  // because we are outside TWDT scope. Do not move TWDT deregistration
  // below this point.
  delay(2000);
  ```

## 7. Add checkForUpdate() API safety comment (EMB-05)

**File:** `modules/OTAUpdateManager/OTAUpdateManager.hpp`

- [ ] Find the `checkForUpdate()` method declaration and add a docblock warning:
  ```cpp
  /**
   * ...existing doc...
   *
   * @warning This method makes blocking HTTPS calls. It MUST only be called
   *          from the OTA FreeRTOS task — never from the main loop tick().
   *          Calling from tick() will violate the 10ms tick budget and may
   *          trigger the Task Watchdog Timer.
   */
  ```

# Verification

1. **Native build:** Run PlatformIO native build — must compile cleanly.
2. **ESP32 build:** Run PlatformIO esp32dev build — must compile cleanly.
3. **Native tests:** Run PlatformIO native tests — all 525+ must pass.
4. **Flash test:** Flash to the device at 192.168.20.186 and verify via serial
   monitor that the heap report shows, and the stack high-water mark log line
   appears after OTA check.

**CRITICAL:** Before distilling or committing final work, send a message back
to the Principal Architect confirming implementation status. Do not wrap up
until explicitly approved.
