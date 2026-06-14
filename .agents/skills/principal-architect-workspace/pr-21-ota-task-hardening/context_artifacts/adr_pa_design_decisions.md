# ADR — PR-21 Principal Architect Design Decisions

## ADR-1: Expose readiness via existing `POST /api/ota {"action":"status"}` rather than a new endpoint

**Decision:** Add `dram_free_bytes` and `ota_stack_bytes` to the existing status action
response. Rejected `/api/ota/readiness` as a dedicated endpoint.

**Rationale:**
- `ota.js` already calls `POST /api/ota status` on every page load — no extra round trip.
- Keeping all OTA state in one payload reduces WebServer route count and response surface.
- The portal only needs the two fields at page-load time, not on a separate trigger.
- A dedicated readiness endpoint would require a new `server.on()` handler, new JS fetch
  call, and new error handling — complexity with no benefit given the data is already fresh.

**Consequence:** The status handler now exposes two live heap metrics. These are safe to
read on the AsyncWebServer request thread (no mutex required — `heap_caps_get_free_size`
is ISR-safe and thread-safe per ESP-IDF docs).

---

## ADR-2: Use `xTaskCreateWithCaps(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)` over `xTaskCreate`

**Decision:** Replace the bare `xTaskCreate` call with `xTaskCreateWithCaps` targeting PSRAM.

**Rationale:**
- On S3 Nano, PSRAM is abundant (~8 MB free at boot). The 32 KB OTA task stack is a
  fixed, known allocation — an ideal PSRAM candidate.
- `xTaskCreateWithCaps` is the idiomatic ESP-IDF PSRAM-aware FreeRTOS API. It is already
  declared in `freertos/idf_additions.h` (included transitively on Arduino-ESP32).
- `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT` is the correct bitmask: SPIRAM targets PSRAM;
  8BIT ensures byte-addressable alignment (required for stack memory).
- On PSRAM-absent boards, `pvPortMalloc` falls back to DRAM silently — no `#ifdef` needed.

**Rejected alternatives:**
- `pvPortMallocCaps` + manual task + TCB allocation: more code, same effect.
- `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY` sdkconfig flag: board-wide, not per-task.
- Keeping `xTaskCreate` + reducing stack size: TLS handshake requires 12-16 KB minimum;
  32 KB is already the floor. Any reduction risks stack overflow during OTA.

**Consequence:** `#include <freertos/idf_additions.h>` was required inside `#ifndef NATIVE_BUILD`
to resolve the symbol on the esp32s3nano target. The native build path is unchanged.

---

## ADR-3: Extract `OTA_TASK_STACK_BYTES` and `OTA_TASK_DRAM_HEADROOM_BYTES` as named constants

**Decision:** Replace the magic number `16384 * 2` with named `constexpr` constants in
`OTAUpdateManager.hpp`.

**Rationale:**
- The stack size now appears in three places: `xTaskCreateWithCaps`, the pre-flight log,
  and the status API response. A single constant prevents drift.
- The portal threshold (`ota_stack_bytes`) must match the firmware value exactly. Sharing
  the same constant via the API response (rather than hardcoding in JS) makes this
  self-synchronising — no separate config needed.
- `OTA_TASK_DRAM_HEADROOM_BYTES = 8192` (8 KB) is a pragmatic safety margin for the
  pre-flight warning log. It does NOT block task creation — it is a log-only advisory.
  Value chosen as ~25% of the stack size, covering typical DRAM fragmentation overhead.

---

## ADR-4: Warning banner is advisory-only — `btn-ota-check` never disabled

**Decision:** The portal low-memory warning does not block or disable the Check for Updates
button.

**Rationale:**
- WO1 already provides PSRAM fallback — OTA will likely succeed even in low-DRAM states.
- Blocking the button on a soft warning would create a worse UX than the pre-PR-21 silent
  failure, since at least then the user could still attempt OTA.
- The warning text explicitly mentions PSRAM fallback: *"Device has PSRAM fallback enabled
  but low DRAM may cause task delays."* — gives the user enough information to decide.
- A hard block would only be appropriate if PSRAM was also absent AND DRAM was below
  threshold — a multi-condition that is not currently checked.
