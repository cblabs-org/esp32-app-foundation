# Project Todo List

> Last updated: 2026-06-06

---

## 🔧 Build System

- [x] **Refactor sdkconfig to base+delta layered architecture** — Completed in PR-19. `sdkconfig.defaults` is now the shared base; `sdkconfig.defaults.esp32s3nano` reduced to a 12-line delta. `scripts/sync_sdkconfig.py` pre-build guard added. Fixes PSRAM regression from PR-14.

---

## 🏗️ Architecture

- [ ] **Slim down `FrameworkConfig`** — Currently conflates two different things. Should only hold genuinely app-specific compile-time constants that the framework can't know (`otaPublicKey`, `otaGithubRepo`, `otaGithubToken`). Everything else is a module default masquerading as framework config and should move into each module's own `resetConfig()`:
  - `hostname` → `WiFiManager::resetConfig()`
  - `timezone` → `TimeManager::resetConfig()`
  - `firmwareUpdatesEnabled` → `OTAUpdateManager::resetConfig()`
  - `otaQuietHour` → `OTAUpdateManager::resetConfig()`
  - `mcpReadOnly` → `MCPServer` default
  This removes false coupling between `main.cpp` and module internals.

---

## 🔄 OTA

- [x] **Quiet hour / scheduled daily check** — Implemented: `tick()` added to `OTAUpdateManager`, daily check fires at `_checkHour` (0–23, default 03:00). Config schema bumped to v3 with migration. Portal Settings card updated with "Daily Check Time" selector.
- [x] **`_autoCheck` boot trigger** — Implemented: first `tick()` after entering Running state fires `performUpdate()` once (`_hasBootChecked` flag). `_otaManager` registered in `_tickables` in `Application.cpp`.
- [x] **Password-protect private key** — `generate_keys.sh` updated to use `-aes256`. User still needs to run: `openssl pkey -in private_key.pem -aes256 -out private_key_enc.pem && mv private_key_enc.pem private_key.pem`).
- [ ] **Harden OTA task creation against low-memory conditions** — The OTA FreeRTOS task requires a 32 KB stack (`16384 * 2`, `OTAUpdateManager.cpp` line 344). On a build with missing PSRAM (e.g. due to sdkconfig drift), only ~35 KB DRAM is free — `xTaskCreate` returns `pdFAIL` and the device shows "Failed to create OTA task" with no useful diagnostic. This creates a deadlock: the firmware bug that caused memory loss cannot be healed via OTA because OTA itself cannot run. Investigate: (1) pre-flight `heap_caps_get_free_size()` check before `xTaskCreate` with an explicit log/error including actual free heap, (2) `xTaskCreateWithCaps(MALLOC_CAP_SPIRAM)` to allocate the stack from PSRAM when available, (3) a portal warning when free DRAM < OTA task stack threshold.
  > 📄 Context: conversation [`34de947b`](file:///Users/mcneillm/.gemini/antigravity/brain/34de947b-cd38-4c49-8e6b-3c79452f4912/implementation_plan.md) — discovered during PR-19 OTA live test; sdkconfig drift left S3 Nano without PSRAM, making the OTA self-heal path impossible. Required direct USB flash to recover.

---

## 🌐 Web Portal — UI Consistency

- [ ] **Standardise "Advanced" sections as collapsible yellow drawers** — Establish a consistent drawer component pattern for all Advanced/danger-zone functionality across the portal. The OTA Advanced card is the reference implementation: yellow (`--warning`) border/header. All other Advanced sections (currently blue and/or expanded) must be migrated to match. The drawer should be collapsed by default so advanced settings are hidden under a fold — users explicitly open them. Audit every tab for Advanced sections and apply the pattern consistently. The goal is a single reusable visual language: yellow = advanced/caution, fold = opt-in, so users aren't confronted with danger-zone controls on first glance.
- [ ] **Screensaver toggle switches always grey** — The on/off toggle switches for screensaver settings appear permanently grey and do not reflect state correctly. Investigate toggle styling and state binding in `display.js` / relevant CSS.

---

## 🗂️ Web Portal — Navigation

- [ ] **Build a module-owned 'Display' bottom-nav tab** — Create a new tier-one bottom bar tab for all display-related settings: screensaver, flip 180°, brightness, and screenshot tool. The tab must be declared and injected by the display module itself via `IWebContributor` — not hardcoded into the portal shell. This makes display settings a first-class concern of the display module: if the U8G2 module is swapped out for a different display driver in future, its successor registers its own tab with its own settings in its place. Remove the display controls from the System tab once the Display tab is live.

---

## 🚀 Release Process

- [x] **Publish v0.0.6-beta.1 (beta channel)** — Released via `deploy_release.py --local`. PR-19 branch; contains sdkconfig base+delta fix and `sync_sdkconfig.py` build guard. Direct USB flash required to bootstrap due to low-memory OTA deadlock (see OTA section).
- [/] **OTA end-to-end update test** — v0.0.6-beta.1 published on GitHub. Device bootstrapped via USB flash. OTA path from v0.0.6-beta.1 → next release should work once PSRAM is restored on the running firmware.

---

## 📱 Sample App

- [ ] **Extend sample app: weather widget + diagnostics carousel** — The current sample app (clock + font-size setting) only demonstrates basic display and config patterns. Extend it to showcase data-fetching and richer UI by adding:
  1. **Weather module** — A new data-fetching module that retrieves current conditions from a weather API. Reuse networking/JSON patterns from the departures-board project as reference. Module should own its config (location, units, refresh interval) via `resetConfig()`.
  2. **Weather widget** — Rendered on the clock face alongside the time (e.g. current temp + condition icon).
  3. **2-panel carousel** — The display rotates automatically every 10 seconds between two screens: Panel A: Clock + weather widget. Panel B: Diagnostics board (heap, PSRAM, WiFi RSSI, uptime, temp).
  This gives the foundation a much stronger illustration: an app that fetches external data, drives a multi-panel display loop, and exposes module config through the portal — not just a clock with one slider.

---

## 📚 Documentation & Developer Experience

- [ ] **Build developer documentation + self-contained progressive discovery skill** — Two-phase effort to make the framework approachable for developers building their own app on top of it.

  **Phase 1 — Developer documentation**
  Comprehensive written docs covering:
  - Framework architecture overview (modules, lifecycle, `ITickable`, `IWebContributor`)
  - How to create a new module from scratch
  - How to use existing optional modules (OTA, MCP, display, WiFi)
  - How to add a new display driver (replacing U8G2)
  - How to schedule recurring data fetches
  - How to extend the web portal (registering tabs, cards, API actions)
  - Config system: `resetConfig()`, schema versioning, NVS persistence
  - Build system: PlatformIO envs, sdkconfig layering, LittleFS assets

  **Phase 2 — Self-contained progressive discovery skill**
  A skill file (`.agents/skills/framework-expert/`) that:
  - Acts as an expert advisor to an agent helping a developer build on the foundation
  - Uses **progressive disclosure**: surfaces high-level guidance first, goes deeper on demand rather than dumping everything upfront
  - **Self-updating:** includes its own update instructions within the skill itself (distilled from skill-creator patterns) — no external dependency on the `skill-creator` skill being installed. Any developer who forks the foundation gets the full update mechanism without needing anything beyond this repo.
  - Knows to prompt for a refresh when the Phase 1 docs have changed
  - Usable both within this project and in any downstream fork
