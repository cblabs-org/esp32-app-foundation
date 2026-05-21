# ADR: ESP32 App Foundation — Architecture Decisions

---

## ADR-01: Central Hub Named `Application` (not `appContext`)

**Status:** Resolved  
**Decision:** The central DI hub / composition root is named `Application`.  
**Rationale:** `appContext` is jargony and ambiguous. `Application` is idiomatic,
self-documenting, and clearly signals this is the root of the composition tree.  
**Impact:** All code using `appContext*` must use `Application*`. Module source
directory changes from `modules/appContext/` → `modules/application/`.

---

## ADR-02: No Unified `IDisplayDriver` Interface

**Status:** Resolved  
**Decision:** There is NO shared `IDisplayDriver` abstraction that both monochrome
(U8G2) and colour TFT (LovyanGFX) displays implement.  
**Rationale:**
- U8G2 is a page-buffer model; LovyanGFX is a framebuffer/sprite model
- Their drawing APIs are fundamentally incompatible (colour model, coordinate
  system, page-flushing semantics)
- A meaningful common interface would either be too thin to be useful or
  balloon into a full re-implementation of both libraries
- In practice, a project uses ONE display type; this is a compile-time decision
**Pattern used instead:** Display Module Variants — separate `u8g2-display-module`
and `tft-display-module` modules, each providing their own board interface and
carousel manager. App boards are written natively for their display technology.  
**Rejected alternative:** A `IDisplayDriver` wrapper that both U8G2 and TFT
implement — explicitly rejected for the above reasons.

---

## ADR-03: Three-Tier Model (Kernel / Module Catalog / Application)

**Status:** Resolved  
**Decision:** Strict three-tier separation:
1. **Kernel** — always present, no opt-out (WiFi, Config, OTA, WebServer, Crypto, Logger, EventBus)
2. **Module Catalog** — optional, pick-and-choose via `lib_deps` (display, MQTT, HA, scheduler, boardLED, buttonHandler)
3. **Application Tier** — per-project business logic, registers with the kernel via interfaces

**Rationale:** Prevents the "kitchen-sink" monolith problem. Modules self-register;
the kernel never `#include`s a module. This is enforced at the include-graph level.  
**Key rule:** The framework kernel MUST NOT `#include` any optional module header.

---

## ADR-04: Composition Over Inheritance (Hollywood Principle)

**Status:** Resolved  
**Decision:** Framework owns `setup()` and `loop()`. Apps do NOT subclass a base class.
Modules register via interfaces (`ITickable`, `iConfigurable`, `iDataSource`, etc.).  
**Rationale:** Single-inheritance is rigid; composition via registration bus allows
any combination of modules to be composed correctly. The Hollywood Principle
("don't call us, we'll call you") defines this as an application *framework*.  
**Entry point contract:**
```cpp
static Application app;   // MUST be global/static — never stack-allocated
void setup() { app.begin(); }
void loop()  { app.tick();  }
```
**Critical:** `Application` owns services by value → it is too large for the 8KB
Arduino task stack. It MUST be declared at global/static scope.

---

## ADR-05: Static Allocation Over Dynamic (Registration Arrays)

**Status:** Resolved  
**Decision:** Registration arrays are fixed-size, compile-time configurable:
```ini
-D MAX_TICKABLES=16
-D MAX_CONFIG_CONSUMERS=12
-D MAX_DATA_SOURCES=8
-D MAX_WEB_CONTRIBUTORS=8
-D MAX_BOARDS=8
```
Total RAM cost: ~208 bytes. Acceptable.  
**Rationale:** `std::vector` growth allocations on embedded cause heap fragmentation.
Static pools give predictable memory layout and eliminate fragmentation risk.  
**Critical safety rule:** Registration overflow MUST cause a compile-time assertion
or runtime panic — NOT silent memory corruption:
```cpp
void Application::registerTickable(ITickable* t) {
    ASSERT_MESSAGE(tickableCount < MAX_TICKABLES, "MAX_TICKABLES overflow");
    tickables[tickableCount++] = t;
}
```

---

## ADR-06: `keyManager` Separated from `configManager`

**Status:** Resolved  
**Decision:** A dedicated `KeyManager` kernel component handles the encrypted
API key/credential registry. `ConfigManager` only handles JSON config persistence.  
**Rationale:** These are different concerns — config persistence vs. credential
lifecycle management. Separation means `ConfigManager` never needs to know
about encryption, and `KeyManager` can enforce access patterns (never log key values).  
**Boot order dependency:** `DeviceCrypto` must initialise before `KeyManager`.
`KeyManager` loads before any module that calls `app.getKeyManager().getKeyById()`.

---

## ADR-07: `EventBus` Replaces `MessagePool` as the General IPC Mechanism

**Status:** Resolved  
**Decision:** A generic publish/subscribe `EventBus` is a kernel component.
The departures-board's `MessagePool` (a scrolling-text ring buffer for station
departure messages) is a domain-specific implementation, NOT a kernel component.  
**EventBus constraints (CRITICAL for embedded safety):**
1. Subscribers MUST use static function pointer + `void* context`, NOT `std::function`
   — `std::function` heap-allocates for captures > ~16 bytes, causing fragmentation
2. `publish()` dispatches SYNCHRONOUSLY and immediately — no queue, no defer
3. Payload `const char*` is only valid for the duration of the subscriber callback
   — subscribers MUST copy any data they need to retain
4. String topics for dev; typed enum `EventTopic::` constants for production  
**Display scrolling messages:** Display modules subscribe to `"display.message"` 
topic on the EventBus to achieve the scrolling-text effect generically.

---

## ADR-08: `schedulerManager` Moved to Optional Module Catalog

**Status:** Resolved  
**Decision:** No scheduler in the kernel. An optional `scheduler` module provides
a generic time-based callback mechanism (`schedule.at(hour, min, fn)`, `schedule.every(ms, fn)`).  
**Rationale:** The departures-board's `schedulerManager` encodes "which display board
shows between what hours" — a domain concept. A generic scheduler is useful but
NOT universal (headless sensor nodes don't need it).  
**App-specific logic:** "Show board X between 08:00 and 18:00" is implemented in
the application using the scheduler module. The module has no concept of boards.

---

## ADR-09: Logger is a Thin ESP-IDF Wrapper (Not a Parallel Log System)

**Status:** Resolved  
**Decision:** The `logger` kernel component is a thin macro wrapper over ESP-IDF's
`ESP_LOGX` macros. It does NOT replace them.  
**Rationale:** ESP-IDF logging is tag-based, level-filtered, tool-aware (Monitor, esptool),
and battle-tested. Replacing it with a parallel system is wasted effort.  
**What the wrapper adds:**
1. `LOG_SECURE(tag, key, value)` — logs key name and value length ONLY; never the value:
   ```cpp
   #define LOG_SECURE(tag, key, val) \
       ESP_LOGI(tag, "%s: [REDACTED] (len=%d)", key, strlen(val))
   ```
2. In production builds (`CONFIG_LOG_DEFAULT_LEVEL_WARN`), verbose/debug log
   calls compile to no-ops — zero flash, zero runtime cost.

---

## ADR-10: `boardLED` and `buttonHandler` Are Optional Modules

**Status:** Resolved  
**Decision:** Both moved from the kernel to the optional module catalog.  
**Rationale:** Not all devices have a status LED or a physical button. A headless
sensor node has neither. Making them optional is the correct embedded approach.  
**Registration:** Modules self-initialise when `registerTickable()` is called.
The kernel does not assume their presence.

---

## ADR-11: `carouselIntervalSecs` and `manualOverrideTimeoutSecs` Are App-Level Config

**Status:** Resolved  
**Decision:** These fields are removed from `FrameworkConfig`. They belong in the
display module's config (`/display.json`) or the application's own config schema.  
**Rationale:** The kernel has no concept of display carousel or button overrides.
These are display module / app domain concerns.  
**`FrameworkConfig` now contains only:**
```
hostname               — Network mDNS hostname
timezone               — POSIX timezone string
firmwareUpdatesEnabled — OTA update toggle
otaQuietHour           — Hour of quiet OTA check (default 3am)
```

---

## ADR-12: Hybrid Web Portal Asset Model (PROGMEM Shell + LittleFS App Assets)

**Status:** Resolved  
**Decision:** Framework SPA shell (WiFi tab, System tab, OTA tab) is served from
gzip-compressed `const uint8_t PROGMEM[]` arrays embedded in the kernel library.
App-specific tab fragments are served from LittleFS.  
**Rationale:**
- PROGMEM serve latency: ~0ms (direct Flash read, no filesystem overhead)
- LittleFS serve latency: 2–10ms per file read (blocking SPI Flash operation)
- Framework shell almost never changes; embedding it removes `portalBuilder.py`
  as a kernel dependency and makes it available in AP/captive-portal mode reliably
- App-specific assets still use LittleFS (updated via `pio run --target uploadfs`)  
**Impact:** `portalBuilder.py` is only needed for app asset packaging, not
for the base framework portal shell.

---

## ADR-13: OTA — Streaming Verification, Not Buffered

**Status:** Resolved  
**Decision:** OTA firmware downloads use streaming MD5/SHA256 verification — the
hash is computed over 512-byte chunks written directly to the inactive partition.
The full binary is NEVER buffered in heap.  
**Rationale:** A 1MB firmware binary cannot be heap-buffered on a device with
~120KB free heap. Attempting to do so causes immediate OOM / heap corruption.  
**Required sdkconfig:** Template repo `sdkconfig.defaults` MUST include:
```ini
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
```
**OTA blessing call point:** `esp_ota_mark_app_valid_cancel_rollback()` is called
by the framework AFTER WiFi connects AND LittleFS mounts successfully — not at
the start of `begin()`. Calling it too early defeats the rollback mechanism.

---

## ADR-14: DataManager FreeRTOS Task Stack Budget

**Status:** Resolved  
**Decision:** The `dataManager` FreeRTOS task MUST have a stack of at least 8KB
(`configSTACK_SIZE_DATA_TASK = 8192`).  
**Rationale:** TLS 1.2 handshake alone requires ~5–6KB of call stack depth.
Add ArduinoJson parsing + HTTP client overhead → 8KB is the minimum safe budget.  
**WiFiClientSecure allocation rule:** MUST be constructed inside `executeFetch()`
(on the data task's context) and destroyed before returning. NEVER allocated
on `begin()` from the main task.

---

## ADR-15: Web Portal POST Safety Rules

**Status:** Resolved  
**Decision:** All POST handlers in the framework and modules MUST:
1. Use `StaticJsonDocument<N>` with explicit size cap (NOT `DynamicJsonDocument`)
2. The `webServer` sets `AsyncWebServer::setMaxBodySize(4096)` to cap inbound POST
   bodies globally
3. SPA JavaScript MUST serialise API requests (no concurrent in-flight XHR)
   to avoid exhausting ESP32's limited socket pool (~5–7 sockets)
4. AP mode implements a 10-minute inactivity timeout (AP draws ~100–120mA continuously)  
**Rationale:** In AP mode (captive portal), any connected client can send
arbitrary oversized POST bodies. Without `StaticJsonDocument`, a malformed
payload can heap-exhaust the device.

---

## ADR-16: Module Instance Lifetime Rule

**Status:** Resolved  
**Decision:** Module instances MUST be declared `static` at file scope or as
static-duration class members. Stack-allocated module instances are forbidden.  
**Rationale:** C++ virtual destructors fire at scope exit. A stack-allocated module
whose destructor fires while still registered in the `Application` hub's tickable
array leaves a dangling pointer — catastrophic on embedded with no MMU.  
**Pattern:**
```cpp
// CORRECT — static storage duration
static MyModule myModule;

// WRONG — destructor fires on scope exit
void MyApp::begin() {
    MyModule myModule;  // BAD: dangling pointer after begin() returns
    app.registerTickable(&myModule);
}
```

---

## ADR-17: `tick()` Modules Must Not Block > 10ms

**Status:** Resolved  
**Decision:** Every module's `tick()` implementation MUST complete in ≤ 10ms.
Long operations (network I/O, file I/O) MUST be deferred to FreeRTOS tasks or
split into a state machine. The framework calls `yield()` between registered
tickable dispatches to feed the hardware watchdog (TWDT, default ~5s timeout).  
**Rationale:** If a module's `tick()` spins for > 5s, the TWDT resets the device.
Even at 1s, the display animation pipeline stalls visibly.
