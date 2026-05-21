# State: Framework Design Specification — Current Status

This file captures the exact current state of the design specification and all
related outputs at the time of distillation (2026-05-20).

---

## Primary Deliverable

**File:** `docs/design_specification.md`  
**Path:** `.../esp32-app-foundation/docs/design_specification.md`  
**Status:** Draft v0.1 — substantively complete, embedded-systems review amendments
were pending application at time of distillation (the review was done, the spec
edits were started but the session ended before the final multi-replace was confirmed).

---

## Spec Sections Status

| Section | Status | Notes |
|---|---|---|
| §1 Purpose & Vision | ✅ Complete | |
| §2 Design Principles | ✅ Complete | `MessagePool` ref still in §2.3 — needs cleanup |
| §3 System Architecture | ✅ Complete | Diagram updated, `Application` hub named |
| §4 Framework Kernel | ✅ Complete | `KeyManager` added, `boardLED`/`buttonHandler` removed |
| §5 Module System | ✅ Complete | `scheduler`, `boardLED`, `buttonHandler` in optional catalog |
| §6 Extension Points | ✅ Complete | All `appContext` refs updated to `app.*` |
| §7 Display Architecture | ✅ Complete | Module variants decision documented |
| §8 Config Architecture | ⚠️ Needs amendment | Missing: LittleFS blocking note, never-from-ISR rule |
| §9 Application Lifecycle | ⚠️ Needs amendment | Missing: `tick()` 10ms budget rule, `yield()` mandate |
| §10 Web Portal | ⚠️ Needs amendment | Missing: hybrid asset model, POST safety rules, AP timeout |
| §11 Inter-Module Comms | ✅ Complete | EventBus documented with constraints |
| §12 Repo & Build | ✅ Complete | |
| §13 Security | ⚠️ Needs amendment | OTA section needs streaming verification + rollback timing |
| §14 Resource Budget | ⚠️ Needs amendment | Missing: registration overflow panic rule, DataManager task stack budget |
| §15 Module Authoring | ⚠️ Needs amendment | Checklist still refs `appContext.*` and `MessagePool`; module lifetime rule missing |
| §16 Decision Log | ⚠️ Needs amendment | Many new decisions not yet logged |

---

## Pending Spec Amendments (from embedded-systems review)

These were identified in `embedded_systems_review.md` but NOT yet written into
the spec when the session ended. The next agent should apply all of these.

### Critical (hardware safety)
- **§13.3 OTA Security**: Add streaming MD5/SHA256 verification rule (no heap buffering)
- **§13.3 OTA Security**: Define `esp_ota_mark_app_valid_cancel_rollback()` call point (after WiFi + LittleFS ready)
- **§13.3 / §12**: `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` must be in template `sdkconfig.defaults`
- **§14.4 Compile-Time Limits**: Registration overflow → panic (not silent corruption)
- **§11.2 EventBus**: `publish()` synchronous only; payload `const char*` valid for callback duration only

### Important (fragmentation / latency)
- **§11.2 EventBus**: Static function pointer pattern, not `std::function`; add code example
- **§4.1 dataManager**: Task stack ≥ 8KB; `WiFiClientSecure` must be constructed inside `executeFetch()`
- **§10 Web Portal**: `StaticJsonDocument` mandatory; `setMaxBodySize(4096)`; SPA serialises XHR
- **§10 Web Portal**: AP mode 10-minute inactivity timeout
- **§8.4 Config Consumer Notification**: Add LittleFS blocking note; never call `loadConfig()` from web handler or ISR
- **§5 / §15 Module Authoring**: Module instances MUST be `static` — stack-allocated modules are forbidden
- **§9.3 tick() Sequence**: Max 10ms per module; framework calls `yield()` between dispatches

### Enhancement (nice-to-have)
- **§10.3 Static Assets**: Document hybrid PROGMEM + LittleFS asset model
- **§11.2 EventBus**: Document enum topic constants as production alternative
- **§4.1 Logger**: Document `LOG_SECURE()` macro implementation
- **§3.3 Entry Point**: `Application` must be global/static — never stack-allocated

---

## Other Outputs Produced This Session

### Brain Artifacts (not in repo)
- `migration_analysis.md` — departures-board → framework migration cost breakdown
  (5 phases, 13–23 days estimate) + embedded-systems resource impact assessment
  **Location:** `/Users/mcneillm/.gemini/antigravity/brain/f6321fe3-7634-4640-bfaa-21f0c51c8633/migration_analysis.md`

- `embedded_systems_review.md` — Full embedded-systems architecture review with
  17 specific findings rated Critical / Important / Enhancement
  **Location:** `/Users/mcneillm/.gemini/antigravity/brain/f6321fe3-7634-4640-bfaa-21f0c51c8633/embedded_systems_review.md`

---

## Current Kernel Component List (Authoritative)

```
Application     — DI hub, orchestrator, AppState owner
configManager   — LittleFS JSON config persistence (/config.json)
keyManager      — Encrypted API key/credential registry (/apikeys.json)
dataManager     — Background HTTP fetch worker (FreeRTOS task, ≥8KB stack)
wifiManager     — WiFi connection + captive portal AP mode
webServer       — ESPAsyncWebServer, base portal routes (PROGMEM shell)
otaUpdateManager — GitHub-based OTA firmware updates (streaming verify)
timeManager     — NTP sync, timezone management
EventBus        — Generic publish/subscribe (static fn ptr pattern)
deviceCrypto    — Hardware-backed key derivation & AES-256 encryption
logger          — Thin ESP-IDF wrapper (adds LOG_SECURE redaction)
githubClient    — HTTPS GitHub API client (used by OTA)
md5Utils        — MD5 hashing utility
```

## Current Optional Module Catalog

```
u8g2-display    — Monochrome OLED/LCD (U8G2 library, page-buffer)
tft-display     — Colour TFT with touch (LovyanGFX, framebuffer)
mqtt-client     — MQTT broker connection
homeassistant   — Home Assistant API client
weather-client  — Weather data source (implements iDataSource)
rss-client      — RSS feed data source (implements iDataSource)
mcp-server      — MCP server module
scheduler       — Generic time-based callback scheduler (NOT display-aware)
boardLED        — Status LED abstraction (optional hardware)
buttonHandler   — Debounced hardware button input (optional hardware)
```

## Current `FrameworkConfig` Schema (Authoritative — slimmed from original)

```
hostname               — Network mDNS hostname
timezone               — POSIX timezone string
firmwareUpdatesEnabled — OTA update toggle
otaQuietHour           — Hour of quiet OTA check (default 3am)
```

**Removed vs. departures-board:** `carouselIntervalSecs`, `manualOverrideTimeoutSecs`,
`brightness` — these are display module / app concerns.

---

## Boot Sequence (Current — Authoritative)

```
1. DeviceCrypto init        (must precede any credential reads)
2. Display module init      (if registered — shows Boot Splash)
3. LittleFS mount
4. FrameworkConfig load → KeyManager load
5. App config load          (app-provided hook)
6. WiFi init                (non-blocking state machine)
7. DataManager task start   (FreeRTOS, ≥8KB stack)
8. All module config consumers registered
9. OTA manager init
10. → esp_ota_mark_app_valid_cancel_rollback() called HERE
    (after WiFi connects AND LittleFS mounted — proves device is healthy)
11. → Yield to tick() loop
```

## tick() Sequence (Current — Authoritative)

```
1. Check deferred config reload flag → reload + notify (if pending)
2. WiFiManager tick
3. OTA manager tick
4. AppState evaluation → drive state transitions
5. yield()
6. Registered ITickable dispatch — all modules (boardLED, buttonHandler,
   display module, MQTT, etc. — whatever the app registered)
   [Each module's tick() MUST complete in ≤ 10ms]
   [yield() called between each dispatch]
7. WiFi connection health monitor
```
