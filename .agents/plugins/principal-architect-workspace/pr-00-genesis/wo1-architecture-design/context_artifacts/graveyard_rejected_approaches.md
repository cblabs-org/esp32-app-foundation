# Graveyard: Rejected Approaches & Dead-Ends

This file records approaches that were explicitly considered and rejected during
the ESP32 App Foundation design session. Future agents MUST NOT re-propose these.

---

## GRAVE-01: Unified `IDisplayDriver` Abstraction

**Proposed:** A single `IDisplayDriver` pure-virtual interface that both U8G2
(monochrome) and TFT colour (LovyanGFX/TFT_eSPI) displays implement.

**Why it was rejected:**
- U8G2 uses a **page-buffer model** — the display is drawn in horizontal page
  strips, each requiring an SPI flush. There is no global framebuffer.
- LovyanGFX uses a **framebuffer/sprite model** — draws to RAM, then blits to
  display. Fundamentally different memory model.
- Their drawing APIs differ in: colour model (1-bit vs 16-bit), coordinate
  system expectations, font handling, and clipping primitives.
- A `drawStr(x, y, str)` common interface is too thin to be useful.
- A comprehensive common interface would require re-implementing both libraries.
- In practice, a project uses ONE display type — a runtime abstraction adds
  cost (vtable overhead per draw call) with zero runtime benefit.

**What to use instead:** Separate display module variants (`u8g2-display-module`,
`tft-display-module`). App boards are written natively for their display tech.
Cross-module status is communicated via `IStatusReporter` (not display primitives).

---

## GRAVE-02: `MessagePool` as a General-Purpose Message Bus

**Proposed:** Promoting the departures-board's `MessagePool` to a kernel component
and using it as the general inter-module message bus.

**Why it was rejected:**
- The departures-board `MessagePool` is a **scrolling-text ring buffer** designed
  specifically for station departure announcement strings.
- It has no concept of typed events, topics, structured payloads, or subscriptions.
- It is coupled to the concept of a display that scrolls text — meaningless for
  headless/MQTT-only apps.

**What to use instead:** A generic `EventBus` with topic-based publish/subscribe.
Display modules that want scrolling text subscribe to `"display.message"` topic.

---

## GRAVE-03: `schedulerManager` as a Kernel Component

**Proposed:** Keeping `schedulerManager` in the framework kernel (as it was in
departures-board).

**Why it was rejected:**
- The departures-board `schedulerManager` encodes "which `IDisplayBoard` shows
  between `startHour` and `endHour`" in its `ScheduleRule` struct.
- This is entirely domain-specific — it assumes the existence of a display carousel.
- A headless sensor node or MQTT bridge has no use for a display-routing scheduler.
- Putting domain logic in the kernel violates the "Kernel Knows Nothing About Modules" principle.

**What to use instead:** An optional `scheduler` module providing generic
`schedule.at(hour, min, fn)` / `schedule.every(ms, fn)` callbacks.
App-specific board routing logic is implemented by the application using this module.

---

## GRAVE-04: Monolithic `Config` Struct (departures-board pattern)

**Proposed:** Keeping a single monolithic `Config` struct in `configManager`
containing all settings for all modules.

**Why it was rejected:**
- The departures-board `Config` struct contains `wsdlHost`, `wsdlAPI`,
  `tflLineFilter`, `BoardConfig boards[MAX_BOARDS]`, etc. — all UK rail/transit domain fields.
- Any derived app would have to include meaningless fields from other domains.
- Adds schema coupling between the kernel and every domain this framework might target.

**What to use instead:** Split config architecture:
- `FrameworkConfig` — kernel-only: hostname, timezone, OTA settings
- Each module owns its own JSON file (`/display.json`, `/mqtt.json`, `/ha.json`)
- Each app defines its own config struct + JSON file (`/<appname>.json`)

---

## GRAVE-05: `std::function` for EventBus Subscribers

**Proposed:** Using `std::function<void(const char*)>` for EventBus subscriber callbacks.

**Why it was rejected:**
- `std::function` on GCC Xtensa performs a **heap allocation** for any lambda
  that captures more data than fits in the Small Buffer Optimisation (~16 bytes).
- A lambda capturing `this` + one other pointer + a string literal can exceed SBO.
- With 8–16 subscribers registered at boot, this creates 8–16 permanent small
  heap allocations that live forever — a classic fragmentation seed on embedded.
- Heap fragmentation on ESP32 can cause `new WiFiClientSecure()` to fail hours
  into runtime as free heap becomes non-contiguous.

**What to use instead:**
```cpp
struct EventSubscriber {
    void (*fn)(const char* payload, void* ctx);
    void* ctx;
};
```
Plain static function pointer + `void* context`. Zero heap allocation.
Subscriber registration uses a static array (not `std::vector`).

---

## GRAVE-06: Dynamic Board Factory (Heap-Allocated Board Instances)

**Proposed:** Registering boards via factory lambdas that `new` board instances
on demand:
```cpp
displayMgr.registerBoardFactory("nr_board", []() -> IDisplayBoard* {
    return new NationalRailBoard();
});
```

**Why it was rejected:**
- Board instances allocated on the heap have non-deterministic lifetime.
- `delete` must be called explicitly; missing it leaks. Calling it too early
  creates dangling pointers in the carousel slot array.
- The departures-board's static slot pool pattern (`IDisplayBoard* slots[MAX_BOARDS]`)
  is already correct and avoids this entirely.

**What to use instead:** App-owned static board instances registered by pointer:
```cpp
static NationalRailBoard nrBoard;
displayModule.registerBoard(&nrBoard);
```
The display module's static slot pool holds the pointer; lifetime is managed by
the app at global/static scope.

---

## GRAVE-07: Buffered OTA Binary Download

**Proposed:** Downloading the full firmware binary into a heap buffer, verifying
MD5, then writing to flash.

**Why it was rejected:**
- A typical firmware binary is 800KB–1.5MB.
- The ESP32 has ~120–160KB of free heap during normal operation (WiFi stack and
  web server consume the majority of the 320KB DRAM).
- Allocating a 1MB buffer causes immediate OOM and heap corruption.

**What to use instead:** Streaming verification — compute MD5/SHA256 over 512-byte
chunks, writing each chunk directly to the inactive OTA partition as it arrives.
The ESP-IDF `esp_ota_ops.h` API (`esp_ota_begin` / `esp_ota_write` / `esp_ota_end`)
is designed for exactly this streaming pattern.

---

## GRAVE-08: ESPHome / Framework Comparison (Resolved Concern)

**Concern raised:** Is this framework just reinventing ESPHome or ESP-IDF?

**Resolution:**
- **ESPHome** is a YAML/config-driven system for declarative sensor reporting.
  It does not support complex state machines, custom display pipelines, or
  arbitrary application logic. Wrong tool for bespoke apps.
- **ESP-IDF** is a hardware abstraction layer + RTOS API. This framework sits
  ABOVE it, providing the application lifecycle, DI hub, and service composition
  that ESP-IDF deliberately leaves to the application developer.
- **Arduino libraries** are stateless utility libraries with no lifecycle management.
  This framework provides the lifecycle glue.

**Verdict:** The niche (code-first application framework for complex ESP32 apps)
is genuinely unoccupied. The framework is NOT reinventing existing tools.

---

## GRAVE-09: Calling `esp_ota_mark_app_valid_cancel_rollback()` at Start of `begin()`

**Proposed:** Blessing the new OTA firmware at the very start of `begin()` before
any services initialise.

**Why it was rejected:**
- Blessing the firmware before verifying it works defeats the entire purpose of
  the rollback mechanism.
- If the firmware crashes during WiFi connect or LittleFS mount (e.g., due to a
  newly introduced bug), the bootloader CANNOT roll back — the blessing already fired.
- The device would be permanently bricked with the faulty firmware.

**Correct pattern:** Bless AFTER WiFi connects AND LittleFS mounts successfully.
This proves the device is functional before committing to the new firmware slot.

---

## GRAVE-10: LittleFS-Only Portal Assets (All Framework Assets via portalBuilder.py)

**Proposed:** Serving all web portal assets (including framework base shell)
from LittleFS, packaged by `portalBuilder.py`.

**Why it was rejected:**
- LittleFS file reads are blocking SPI Flash operations taking 2–10ms per file.
- In captive portal/AP mode, there is no guarantee the `data/` LittleFS partition
  is flashed (the user may have only flashed the firmware, not the filesystem image).
- `portalBuilder.py` as a build-time dependency adds friction for library users.

**What to use instead:** Hybrid model:
- Framework SPA shell → gzip-compressed `const uint8_t PROGMEM[]` in kernel library
  (serves in ~0ms directly from Flash; always present with firmware)
- App-specific tab fragments → LittleFS (updated via `uploadfs`)
