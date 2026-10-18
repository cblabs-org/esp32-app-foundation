# ADR: otaUpdateManager Architecture Decisions

## 1. State Machine Design

**Decision:** Introduced `OtaState` enum with 6 states: `Idle`, `Checking`, `Downloading`, `Verifying`, `Complete`, `Error`.

**Why:** The previous manager had no state tracking — methods like `beginUpdate()` could be called in any order. The state machine enforces valid transitions and provides guard logic (e.g., `checkForUpdate()` rejects calls when state != Idle).

**Trade-off:** The state machine is simple (no formal transition table) — guards are inline in each method. This is sufficient for a single-threaded orchestrator but may need upgrading if concurrent OTA paths are introduced.

## 2. FreeRTOS Task Offloading

**Decision:** `performUpdate()` launches a FreeRTOS task via `xTaskCreate("ota_update", 8192, this, 1, &taskHandle_)` and returns immediately.

**Why:** The WO spec requires tick() ≤ 10ms. The OTA pipeline involves network I/O (GitHub API, firmware download) that can take 30+ seconds. Running in a FreeRTOS task with priority 1 (low) keeps the main loop responsive.

**Parameters chosen:**
- Stack: 8192 bytes — sufficient for WiFiClientSecure + HTTP parsing
- Priority: 1 — background, won't starve tick()
- `vTaskDelete(nullptr)` at end of wrapper

## 3. Application.getConfig() Addition

**Decision:** Added `const FrameworkConfig& getConfig() const` to `Application`.

**Why:** The WO spec requires `otaUpdateManager` to access `otaGithubRepo`, `otaPublicKey`, and `otaGithubToken` from the FrameworkConfig. Application already stores `_config` but had no public accessor. Adding a const-ref getter is the minimal, safe change — it doesn't expose mutability.

## 4. OtaProgressEvent Layout Compatibility

**Decision:** Created `OtaProgressEvent` struct with fields `{float progress, const char* status, bool complete, bool success}` — layout-compatible with `OtaProgressPayload` in `u8g2DisplayModule.hpp`.

**Why:** The display module's EventBus subscriber casts `void*` payloads to `OtaProgressPayload`. By making OtaProgressEvent layout-compatible, the display can render OTA progress without any code changes. This avoids a shared header dependency between modules.

## 5. WiFiClientSecure Heap Allocation

**Decision:** `std::unique_ptr<WiFiClientSecure> client(new (std::nothrow) WiFiClientSecure())` inside `performUpdateTask()`.

**Why:** WiFiClientSecure is ~16KB. Stack-allocating it in a FreeRTOS task would require a much larger stack. Heap allocation with `nothrow` + null-check is the safe pattern for memory-constrained ESP32.

## 6. Native Stub Architecture

**Decision:** The `.cpp` file contains a complete `Application` stub class behind `#ifdef NATIVE_BUILD`, with static `EventBus` and `FrameworkConfig` instances.

**Why:** The Hollywood Principle prohibits including `Application.hpp` in the header. On hardware, `Application.hpp` is included in the `.cpp`. On native, we need a minimal stub that provides `getEventBus()` and `getConfig()` — just enough for the manager's code paths to compile and execute guard logic in tests.
