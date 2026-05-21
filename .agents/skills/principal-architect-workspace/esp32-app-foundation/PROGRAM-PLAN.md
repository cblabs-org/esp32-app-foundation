# PROGRAM-PLAN.md
# ESP32 App Foundation — Chief Architect Program Plan

**Status:** PLANNING — Awaiting Approval  
**Program Manager:** Chief Architect (Tier 1)  
**Source of Truth:** `docs/design_specification.md`  
**Reference Codebase:** `../departures-board` (mine for patterns; do NOT copy wholesale)  
**Last Updated:** 2026-05-21 (Rev 4 — git setup + branching strategy added)

---

## Confirmed Architecture Decisions (pre-plan)

| # | Decision | Rationale |
|---|---|---|
| CA-01 | **Monorepo** — kernel + all optional modules live in `esp32-app-foundation` | No premature repo fragmentation; modules extracted to submodules only when mature |
| CA-02 | **No `esp32-modules` separate repo** — optional modules under `modules/` | Avoids a two-library version compatibility matrix in v1 |
| CA-03 | **No `lib_deps` registry model** — apps consume via git submodule | Deterministic SHA pinning; no absolute path hacks; CI reproducible |
| CA-04 | **`esp32-app-template` folds into this repo** as `template/` skeleton | One repo to clone; no separate template repo overhead |
| CA-05 | **`std::vector` two-tier model** — no `MAX_*` compile-time ceilings | Boot-phase allocation is functionally static; spec §2.5, §14.3, §14.4 are authoritative. ADR-05 (fixed arrays) is **SUPERSEDED** — see graveyard |
| CA-06 | **Dual framework** `arduino + espidf` | OTA rollback, IDF logging, DeviceCrypto require IDF APIs; display/web libs require Arduino |

---

## Program Summary

Build the **ESP32 App Foundation** from scratch — a clean, reusable C++ application
framework for ESP32 devices on PlatformIO (Arduino + ESP-IDF dual framework).

### Repository Topology

```
esp32-app-foundation/           ← THIS repo (monorepo — everything lives here)
  include/                      ← All framework interface headers + FrameworkConfig
  lib/                          ← Low-level utilities (deviceCrypto, logger, md5Utils, githubClient)
  modules/
    application/                ← Kernel: Application hub + AppState
    configManager/              ← Kernel: LittleFS JSON config
    keyManager/                 ← Kernel: encrypted credential store
    wifiManager/                ← Kernel: WiFi + captive portal
    timeManager/                ← Kernel: NTP + POSIX timezone
    otaUpdateManager/           ← Kernel: GitHub OTA + rollback
    webServer/                  ← Kernel: ESPAsyncWebServer + REST API
    eventBus/                   ← Kernel: typed pub/sub
    dataManager/                ← Kernel: FreeRTOS background fetch worker
    u8g2Display/                ← Optional: U8G2 monochrome display module
    scheduler/                  ← Optional: generic time-based callback scheduler
    mqttClient/                 ← Optional: (future)
    homeassistant/              ← Optional: (future)
    weatherClient/              ← Optional: (future)
    rssClient/                  ← Optional: (future)
    boardLED/                   ← Optional: status LED
    buttonHandler/              ← Optional: physical button
  scripts/                      ← Build scripts (portalBuilder.py, build_timestamp.py, etc.)
  web/portal/                   ← Web portal SPA source (compiled to PROGMEM)
  include/portal_assets/        ← Generated PROGMEM gzip headers (build output)
  data/                         ← LittleFS image root (app-specific assets)
  test/                         ← ALL tests (established by PR-01 WO1)
    test_unit/                  ← GoogleTest native — no hardware required
      test_application/
      test_eventbus/
      test_configManager/
      test_keyManager/
      test_dataManager/
      test_webServer/
      test_u8g2Display/
    test_hardware/              ← Hardware integration tests
      test_wifi/
      test_ota/
      test_littlefs/
      test_crypto/
      test_display/
      test_webserver/
    test_e2e/                   ← Full boot-to-RUNNING integration test (PR-06 gate)
  template/                     ← Starter skeleton for new apps using this framework
    src/main.cpp                ← 3-line entry point
    src/myApp.hpp/.cpp          ← Skeleton app class
    platformio.ini              ← lib_deps = framework/; esp32dev + esp32s3nano + native
    test/test_unit/test_myapp/  ← Stub unit test for new app developers
    README.md                   ← "How to start a new app" guide
  docs/                         ← Design specification + ADRs
  partitions.esp32dev.csv
  partitions.esp32s3nano.csv
  sdkconfig.defaults
  sdkconfig.defaults.esp32s3nano
  library.json                  ← PlatformIO manifest
```

### How Apps Consume the Framework

```bash
# Create your app repo
git init my-esp32-app
cd my-esp32-app

# Add the framework as a submodule (pinned to a commit SHA)
git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git framework
git submodule update --init

# Copy the starter template into your app
cp -r framework/template/src .
cp framework/template/platformio.ini .
```

App `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
framework = arduino, espidf
lib_deps =
    framework/                  ; local submodule path — no registry, no absolute paths
    olikraus/U8g2@2.36.18
    bblanchon/ArduinoJson@7.4.3
    ESP32Async/ESPAsyncWebServer
```

**Future extraction:** When an optional module (e.g., `u8g2Display`) matures into a
standalone concern, it can be extracted to its own repo and added back as a nested
git submodule under `modules/u8g2Display/` — transparent to app consumers.

---

## Testing Mandate

> [!IMPORTANT]
> Testing is **first-class in every Work Order**. Workers do not close a WO until
> both implementation and tests pass. **"It compiles" is not done.**

### Three Testing Tiers

| Tier | Runs Where | Required |
|---|---|---|
| **Tier 1 — Native Unit Tests** | Host CPU via `[env:native]` + GoogleTest | Every WO that adds logic |
| **Tier 2 — Hardware Integration** | Physical board | Every WO touching hardware/network/filesystem |
| **Tier 3 — End-to-End Boot** | Physical board, full stack | Gate before PR-06 merges |

### Per-PR Test Requirements

| PR | Native Unit Tests | Hardware Tests | E2E |
|---|---|---|---|
| PR-01 | `test_application`: registration, vector lock, `softReset`, `AppState` | — | — |
| PR-02 | `test_configManager`, `test_keyManager` (mocked crypto) | WiFi, LittleFS, OTA, crypto | — |
| PR-03 | `test_eventbus` (dispatch, lifetime, overflow), `test_dataManager` (scheduling) | FreeRTOS stack ≥ 8KB | — |
| PR-04 | `test_webServer` (routing, dispatch, POST cap, tabs) | Config round-trip, AP timeout | — |
| PR-05 | `test_u8g2Display` (carousel, slot pool, IStatusReporter, EventBus) + LDF isolation | Board cycle on hardware | — |
| PR-06 | — | — | Boot-to-`RUNNING` all services |
| PR-07 | Template stub compiles | Flash template, confirm `RUNNING` | — |

### Worker Agent Testing Rules
1. Every WO that adds a class or function adds a corresponding test — no exceptions
2. Native tests pass before hardware tests run
3. Hardware results verified via `mcp_platformio_query_logs` with `searchPattern` — not eyeballed
4. Failed hardware tests block WO closure
5. `test/` hierarchy established by PR-01 WO1 — subsequent PAs add their `test_*` subdirs

---

## Mandatory Skill Boot Sequence

> [!IMPORTANT]
> **Every Principal Architect AND every Worker Agent MUST load all four skills below
> as the very first step before reading any code or writing any file.**
> These are non-negotiable. Failure to load them results in code that violates the
> project's embedded-safety, OOP, documentation, and build constraints.

All four skills live in `.agents/skills/` within this repo:

| # | Skill | Path | Load When |
|---|---|---|---|
| 1 | **pio-manager** | `.agents/skills/pio-manager/SKILL.md` | Always — before any build, flash, or test operation |
| 2 | **embedded-systems** | `.agents/skills/embedded-systems/SKILL.md` | Always — governs all memory, stack, heap, and power decisions |
| 3 | **house-style-docs** | `.agents/skills/house-style-docs/SKILL.md` | Always — every C++ file authored must meet house style |
| 4 | **oo-expert** | `.agents/skills/oo-expert/SKILL.md` | Always — all architectural and interface decisions |

### Boot Sequence (exact order)

Every agent session MUST execute this exact sequence as its first four tool calls:

```
1. view_file .agents/skills/pio-manager/SKILL.md
2. view_file .agents/skills/embedded-systems/SKILL.md
3. view_file .agents/skills/house-style-docs/SKILL.md
4. view_file .agents/skills/oo-expert/SKILL.md
```

The PlatformIO MCP reference is also required before any build operation:
```
5. view_file .agents/skills/pio-manager/references/mcp-agent-reference.md
```

### How This Is Enforced

- The **PA Bridge Prompt** (issued by the Chief Architect) includes these steps verbatim
- The **WORKORDER template** includes these steps verbatim so Workers receive them
- The PA must verify Worker WORKORDERs include the boot sequence before issuing them
- The `CONTRIBUTING.md` (authored in PR-06) documents this as the required agent onboarding

---

## Git Repository Setup & Branching Strategy

> [!CAUTION]
> **`esp32-app-foundation` is NOT yet a git repository.** It contains only `docs/`
> and `.agents/`. PA-01 WO1 MUST bootstrap the repo as its absolute first task.

### Phase 0 — Repo Bootstrap (PA-01 WO1, step 1)

```bash
git init
git checkout -b main
# PA-01 writes .gitignore first, then:
git add .gitignore docs/ .agents/
git commit -m "chore: initial repo scaffold — docs, agents, gitignore"
```

`.gitignore` must include:
```
.pio/
.vscode/
build/
.DS_Store
*.o *.a *.elf *.bin *.map
include/portal_assets/
```

### Branch Naming Convention

```
main                                       ← protected, production
  └─ pr-01-kernel-interfaces                ← PA feature branch (base: main)
       └─ pr-01-kernel-interfaces/wo1-scaffold
       └─ pr-01-kernel-interfaces/wo2-interfaces
       └─ pr-01-kernel-interfaces/wo3-app-hub
       └─ pr-01-kernel-interfaces/wo4-entry-point
  └─ pr-02-kernel-services                  ← branched from main after PR-01 merges
  └─ pr-03-data-infra
  └─ pr-04-web-portal                        ← parallel with PR-05
  └─ pr-05-display-module                   ← parallel with PR-04
  └─ pr-06-build-infra
  └─ pr-07-template-scaffold
```

**Format:** `pr-[NN]-[name]` for feature branch — `pr-[NN]-[name]/wo[Y]-[topic]` for WO branches.

### PA Git Lifecycle (each PA repeats this)

```bash
# Start: branch off main (after previous PR merges)
git checkout main && git pull
git checkout -b pr-NN-name

# Each WO: branch off feature branch
git checkout -b pr-NN-name/woY-topic pr-NN-name

# WO complete: merge back to feature branch
git checkout pr-NN-name && git merge pr-NN-name/woY-topic

# Sync before each new WO:
git checkout main && git pull
git checkout pr-NN-name && git rebase main
```

### Squash-Merge Rules
- **WO → Feature branch:** regular merge — no squash
- **Feature branch → main:** squash permitted — one clean commit per PR
- **After any squash-merge to main:** all downstream PAs must `git rebase main` immediately

### Chief Architect Sequencing Gates

| Gate | Chief Architect Action |
|---|---|
| Repo bootstrap confirmed (`git log`) | PA-01 proceeds to WO2 |
| PR-01 merged to `main` | Issue PA-02 bridge prompt |
| PR-02 merged to `main` | Issue PA-03 bridge prompt |
| PR-03 merged to `main` | Issue PA-04 **and** PA-05 simultaneously |
| PR-04 **and** PR-05 both merged | Issue PA-06 bridge prompt |
| PR-06 merged to `main` | Issue PA-07 bridge prompt |

---

## Non-Negotiable Architectural Rules (all PAs must enforce)

**Architecture:**
1. **Hollywood Principle** — Framework owns `setup()` and `loop()`. No app writes these.
2. **Composition over Inheritance** — Modules register via interfaces; no `BaseApp`.
3. **No Cross-Module Includes** — Optional modules never `#include` another optional module.
4. **Kernel Knows Nothing About Modules** — Kernel headers never import module headers.
5. **Two-Tier Vector Model** — No `MAX_*` ceilings; structural arrays locked post-`begin()`.
6. **LittleFS Directory Convention** — `etc/`, `mod/`, `app/`, `www/` ownership rules.
7. **House Style Documentation** — Every C++ file follows the project's header/doc standard.
8. **`tick()` ≤ 10ms** — Long I/O deferred to FreeRTOS tasks.
9. **Static module instances** — Never stack-allocated.
10. **No `DynamicJsonDocument`** — Always `StaticJsonDocument<N>` in handlers.
11. **EventBus subscribers use static fn-ptr + `void* ctx`** — Never `std::function`.
12. **`WiFiClientSecure` heap-allocated inside `executeFetch()` only** — Never on stack.

**Testing:**
13. Every WO that adds a class or function must add a corresponding test — no exceptions.
14. Test directories must be prefixed `test_` (PlatformIO v6 rule).
15. Native tests must pass before hardware tests are run.
16. Hardware test results verified via `mcp_platformio_query_logs` with `searchPattern`.
17. All MCP build/flash/test calls use explicit `environment` and `background: true`.
18. `mcp_platformio_release_lock` called after every `acquire_lock` — even on error paths.
19. E2E boot test in `test/test_e2e/` is the gate before PR-06 merges.

---

## Dependency Graph

```
pr-01-kernel-interfaces  ──────────────────────────────────────────┐
    │                                                               │
    ▼                                                               │
pr-02-kernel-services  ──────────────────────────────────────────┐ │
    │                                                             │ │
    ▼                                                             │ │
pr-03-data-infra  ──────────────────────────────────────────┐    │ │
    │                                                        │    │ │
    ▼                                                        ▼    ▼ ▼
pr-04-web-portal  ◄── (depends on 01, 02, 03)          pr-05-display-module
    │                                                        │
    └──────────────────┐                                     │
                       ▼                                     ▼
                pr-06-build-infra  ◄── (depends on 01–05)
                       │
                       ▼
                pr-07-template-scaffold  ◄── (depends on all)
```

**Critical Path:** `01 → 02 → 03 → 04 → 06 → 07`  
**Parallel Opportunity:** `pr-05-display-module` runs alongside `pr-04` once `pr-03` merges.

---

## Project Definitions

---

### PR-01: Framework Kernel & Core Interfaces {#pr-01}

**Branch:** `pr-01-kernel-interfaces`  
**Depends on:** nothing (foundation)  
**Blocks:** ALL other projects  
**Estimated WOs:** 4

**Scope:**
Establish the repository structure, PlatformIO baseline, all pure interface headers,
the `Application` hub class (skeleton with two-tier vector model), and the `AppState`
state machine. Zero optional-module code. Everything here is the skeleton every
downstream project builds on.

**Deliverables:**
- `library.json` — PlatformIO library manifest
- `platformio.ini` — dual-env `esp32dev` + `esp32s3nano`, Arduino+ESP-IDF
- `include/AppFoundation.h` — single public include façade
- `include/interfaces/ITickable.hpp`
- `include/interfaces/iConfigurable.hpp`
- `include/interfaces/iDataSource.hpp` + `UpdateStatus` enum + `PriorityTier` enum
- `include/interfaces/IStatusReporter.hpp`
- `include/interfaces/IWebContributor.hpp`
- `include/interfaces/IDisplayBoard.hpp` (U8G2 variant)
- `include/AppState.hpp` — `BOOTING`, `WIFI_SETUP`, `APP_SETUP`, `RUNNING`
- `include/FrameworkConfig.hpp` — `hostname`, `timezone`, `firmwareUpdatesEnabled`, `otaQuietHour`
- `modules/application/Application.hpp/.cpp` — hub with two-tier vector, `begin()`, `tick()`, `softReset()`, `_compactAndLock()`, `_reportHeap()`
- `partitions.esp32dev.csv`, `partitions.esp32s3nano.csv`
- `sdkconfig.defaults`, `sdkconfig.defaults.esp32s3nano` (includes `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`)

**Reference mining:** `departures-board/modules/appContext/appContext.hpp`

**Sketch WOs:**
| WO | Topic |
|---|---|
| WO1 | Repo scaffold, `platformio.ini`, partition tables, `sdkconfig.defaults*` |
| WO2 | Interface headers, `AppState.hpp`, `FrameworkConfig.hpp` |
| WO3 | `Application` hub skeleton (two-tier vector, lifecycle, `softReset`) |
| WO4 | Entry point (`src/main.cpp`), façade header (`include/AppFoundation.h`), `library.json` |

---

### PR-02: Kernel Services {#pr-02}

**Branch:** `pr-02-kernel-services`  
**Depends on:** PR-01  
**Blocks:** PR-03, PR-04, PR-05  
**Estimated WOs:** 6

**Scope:**
All "always-present" kernel services. Each is a self-contained unit wired into
the `Application` hub via accessor methods.

| Service | Path | Key Notes |
|---|---|---|
| `deviceCrypto` | `lib/deviceCrypto/` | Hardware MAC key derivation, AES-256 |
| `logger` | `lib/logger/` | ESP-IDF wrapper + `LOG_SECURE()` macro |
| `md5Utils` | `lib/md5Utils/` | Streaming MD5 for OTA verification |
| `githubClient` | `lib/githubClient/` | HTTPS + cert validation |
| `configManager` | `modules/configManager/` | LittleFS `etc/config.json`, versioning, `requestReload()` |
| `keyManager` | `modules/keyManager/` | Encrypted registry, `addKey/getKeyById/deleteKey` |
| `wifiManager` | `modules/wifiManager/` | AP captive portal, `AppState` transitions |
| `timeManager` | `modules/timeManager/` | NTP, POSIX timezone, non-blocking |
| `otaUpdateManager` | `modules/otaUpdateManager/` | Streaming MD5, rollback, bless AFTER WiFi+LittleFS |

**Sketch WOs:**
| WO | Topic |
|---|---|
| WO1 | Low-level libs: `deviceCrypto`, `logger`, `md5Utils`, `githubClient` |
| WO2 | `configManager` |
| WO3 | `keyManager` |
| WO4 | `wifiManager` |
| WO5 | `timeManager` |
| WO6 | `otaUpdateManager` |

---

### PR-03: Data Infrastructure {#pr-03}

**Branch:** `pr-03-data-infra`  
**Depends on:** PR-01, PR-02  
**Blocks:** PR-04, PR-05  
**Estimated WOs:** 3

**Scope:**
`EventBus`, `dataManager`, `NullStatusReporter`, and full Application hub wiring.

| Deliverable | Key Notes |
|---|---|
| `modules/eventBus/EventBus.hpp/.cpp` | Static fn-ptr subscriber table (NOT `std::function`). `publish()` is synchronous. Payload valid only for callback duration. |
| `modules/dataManager/dataManager.hpp/.cpp` | FreeRTOS task, ≥ 8KB stack, `iDataSource` scheduling, `PriorityTier` |
| `NullStatusReporter` | Inline in `Application` — silent no-op when no display registered |
| Application hub wiring | Wire EventBus + DataManager into `begin()`/`tick()`; publish `"system.heap"` after `_compactAndLock()` |

**Sketch WOs:**
| WO | Topic |
|---|---|
| WO1 | `EventBus` — static fn-ptr table, `publish()`, `subscribe()`, topic validation |
| WO2 | `dataManager` — FreeRTOS task, `iDataSource` full interface, scheduling loop |
| WO3 | `NullStatusReporter` + Application hub full wiring |

---

### PR-04: Web Portal {#pr-04}

**Branch:** `pr-04-web-portal`  
**Depends on:** PR-01, PR-02, PR-03  
**Blocks:** PR-06  
**Estimated WOs:** 4

**Scope:**
`WebServerManager` (ESPAsyncWebServer), full REST API, SPA frontend (hybrid PROGMEM
shell + LittleFS app fragments), and `portalBuilder.py` gzip pipeline.

**Backend:**
- `modules/webServer/webServer.hpp/.cpp`
- `AsyncWebServer::setMaxBodySize(4096)` — global POST body cap
- `GET /api/tabs`, `GET/POST /api/wifi`, `GET /api/system`, `GET/POST /api/ota`, `GET/POST /api/config`
- AP mode 10-minute inactivity timeout

**Frontend:**
- `web/portal/index.html`, `app.css`, `app.js`, `wifi.js`, `system.js`, `ota.js`
- SPA serialises API requests (no concurrent XHR — protects ESP32 socket pool)

**Build pipeline:**
- `scripts/portalBuilder.py` → `include/portal_assets/*.h` PROGMEM gzip arrays

**Sketch WOs:**
| WO | Topic |
|---|---|
| WO1 | `WebServerManager` skeleton + `IWebContributor` dispatch |
| WO2 | REST API handlers (wifi, system, ota, config, tabs) |
| WO3 | SPA frontend (HTML, CSS, JS — all tabs) |
| WO4 | `portalBuilder.py` + PROGMEM header generation |

---

### PR-05: U8G2 Display Module *(parallel with PR-04)* {#pr-05}

**Branch:** `pr-05-display-module`  
**Depends on:** PR-01, PR-02, PR-03  
**Blocks:** PR-06  
**Estimated WOs:** 4

**Scope:**
First optional module. Lives under `modules/u8g2Display/`. Designed for future
extraction to its own repo/submodule when appropriate.

**No `MessagePool`** — uses `EventBus.publish("display.message", ...)` for scrolling text.

| Deliverable | Notes |
|---|---|
| `modules/u8g2Display/u8g2DisplayModule.hpp/.cpp` | `ITickable`, `IStatusReporter`, `iConfigurable` |
| Carousel engine | Board slot pool (`std::vector`, locked post-begin), rotation timer |
| System boards | BootSplash, Loading, WiFiWizard, OtaProgress, Error, Screensaver |
| Config | `/mod/display.json` — brightness, flip, sleep timeout |

**Sketch WOs:**
| WO | Topic |
|---|---|
| WO1 | Module skeleton + carousel engine + board slot pool |
| WO2 | BootSplash, Loading, WiFiWizard system boards |
| WO3 | OtaProgress, Error, Screensaver system boards |
| WO4 | `IStatusReporter` impl + EventBus `"display.message"` subscription + config |

---

### PR-06: Build Infrastructure {#pr-06}

**Branch:** `pr-06-build-infra`  
**Depends on:** PR-01 through PR-05  
**Blocks:** PR-07  
**Estimated WOs:** 3

**Scope:**
Build scripts, GitHub Actions CI (dual-env + include-graph lint), `README.md`,
`CONTRIBUTING.md`, and formal `library.json`.

| Deliverable | Notes |
|---|---|
| `scripts/build_timestamp.py` | `BUILD_TIME` macro injection |
| `scripts/build_fonts.py` | U8G2 font compilation |
| `scripts/generate_layouts.py` | Display layout constants |
| `.github/workflows/ci.yml` | Build `esp32dev` + `esp32s3nano`; kernel-includes-module lint |
| `README.md` | Framework overview, submodule quick-start |
| `CONTRIBUTING.md` | Module authoring guide (spec §15 condensed) |
| `library.json` | Formally verified PlatformIO manifest |

**Sketch WOs:**
| WO | Topic |
|---|---|
| WO1 | Build scripts: `build_timestamp.py`, `build_fonts.py`, `generate_layouts.py` |
| WO2 | CI workflow: dual-env build + include-graph lint |
| WO3 | `README.md`, `CONTRIBUTING.md`, formal `library.json` |

---

### PR-07: App Template Scaffold {#pr-07}

**Branch:** `pr-07-template-scaffold`  
**Depends on:** PR-01 through PR-06  
**Blocks:** nothing (final deliverable)  
**Estimated WOs:** 2

**Scope:**
Create `template/` within this repo — a minimal starter skeleton showing how to
wire a new application into the framework via git submodule. NOT a separate repo.

| Deliverable | Notes |
|---|---|
| `template/src/main.cpp` | 3-line entry point |
| `template/src/myApp.hpp/.cpp` | Skeleton `begin()` with `// TODO:` markers |
| `template/platformio.ini` | `lib_deps = framework/` (submodule local path) |
| `template/README.md` | "How to use this template" — git submodule add instructions |

**Sketch WOs:**
| WO | Topic |
|---|---|
| WO1 | Template skeleton: `src/`, `platformio.ini`, submodule setup instructions |
| WO2 | Template README + integration test (verify template compiles against framework) |

---

## Source Mining Guide

| What to Mine | Location in departures-board | Extract |
|---|---|---|
| DI hub pattern | `modules/appContext/appContext.hpp` | Composition root, service accessor pattern |
| Config persistence | `modules/configManager/` | LittleFS JSON, versioning |
| WiFi management | `modules/wifiManager/` | AP mode, reconnect state machine |
| OTA pipeline | `modules/otaUpdateManager/` | GitHub HTTPS, streaming MD5, rollback |
| Background fetches | `modules/dataManager/` | FreeRTOS task, queue pattern |
| Display carousel | `modules/displayManager/` | Board slot pool, rotation timer |
| Crypto engine | `lib/deviceCrypto/` | Hardware key derivation, AES-256 |
| Logger | `lib/logger/` | ESP-IDF wrapper, secret redaction |
| GitHub HTTPS client | `lib/githubClient/` | Certificate validation |
| Web portal UI | `web/index.html` | Portal UX patterns |
| Web server routes | `modules/webServer/` | ESPAsyncWebServer handler patterns |

**Do NOT carry over:**
- `MessagePool` → use `EventBus.publish("display.message", ...)`
- Monolithic `appContext` header including all modules → use `Application` with `register*()` 
- `weatherClient`/`rssClient` in the kernel → optional modules only
- `schedulerManager` in the kernel → optional module
- Fixed `MAX_BOARDS` arrays → `std::vector` two-tier model

---

## Execution Sequencing

| Phase | Projects | Gate |
|---|---|---|
| 1 — Foundation | PR-01 only | Must complete before anything else |
| 2 — Core Services | PR-02 | Begin immediately after Phase 1 merges |
| 3 — Infrastructure | PR-03 | Blocked by PR-02 |
| 4 — Features | PR-04 + PR-05 in parallel | Blocked by PR-03; no file overlap |
| 5 — Finalization | PR-06 then PR-07 | PR-06 validates CI; PR-07 creates template |

---

*Chief Architect Program Plan — ESP32 App Foundation — 2026-05-21 (rev 2)*
