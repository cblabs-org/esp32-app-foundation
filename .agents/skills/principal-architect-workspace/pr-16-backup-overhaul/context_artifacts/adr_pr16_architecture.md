# ADR — PR-16 Backup System Architecture Decisions

## ADR-1: Root Cause — Route Registration Order

**Decision:** Fix by reordering `server.on()` calls in `ConfigManager::registerRoutes()` — specific paths first, generic parent last.

**Context:** `GET /api/config/backup` was returning `{"namespaces":[...],"count":4}` (the namespace list) instead of the actual config export. ESPAsyncWebServer matches routes in registration order using prefix matching. Since `/api/config` was registered first, it captured all requests whose URL started with `/api/config`, including `/api/config/backup` and `/api/config/restore`. Both sub-path handlers were dead code.

**Alternatives considered:**
- Using a wildcard handler for `/api/config/*` and dispatching internally — rejected as over-engineering for a simple ordering fix
- Consolidating all three into one handler — rejected as it would muddle the clean separation of concerns

**Why specific-before-generic:** This is the standard ESPAsyncWebServer pattern and is the least invasive, most readable fix.

---

## ADR-2: `_meta` Envelope Design

**Decision:** Add `_meta` as the first key in the exported JSON document, populated by `buildBackupMeta()` private helper on `ConfigManager`.

**Fields chosen:**
- `backup_version: 1` — for future format migration
- `firmware_version` — from `APP_FOUNDATION_VERSION` (see ADR-5)
- `device_id` — chip MAC address for device identification
- `device_type` — chip model string (e.g. `"ESP32-S3"`)
- `datetime` — ISO-8601 from `time()` if clock is set
- `timezone` — defaults `"UTC"` with `setMetaTimezone()` hook (see ADR-4)
- `module_count` — count of exported module sections

**Why not include it in `importAllConfig()`:** The existing loop in `importAllConfig()` already skips any key that doesn't match a registered module namespace. `_meta` simply falls through harmlessly. A debug log was added for clarity.

---

## ADR-3: AsyncResponseStream vs String Buffer

**Decision:** Replace `String output; serializeJson(doc, output); request->send(...)` with `AsyncResponseStream`.

**Why:** The intermediate `String` double-allocates the full JSON payload on the heap. On an ESP32 with a constrained DRAM heap (~360KB total), allocating a full config export (~2-3KB) twice in sequence is wasteful and contributes to fragmentation. `AsyncResponseStream` serializes directly into the network buffer.

---

## ADR-4: Timezone in `_meta` — Deferred Coupling

**Decision:** `timezone` in `_meta` defaults to `"UTC"` via a TODO comment. `setMetaTimezone(const char*)` is available on `ConfigManager` for `TimeManager` to call during `applyConfig()`, but the wire-up was deferred.

**Why deferred:** Directly coupling `ConfigManager` to `TimeManager` would violate the Hollywood Principle that governs this codebase's module separation. The correct approach is for `TimeManager` to call `configManager.setMetaTimezone(tz)` after applying its own config. This is a small follow-up item, not a blocker for the backup fix itself.

---

## ADR-5: `APP_FOUNDATION_VERSION` vs `FIRMWARE_VERSION`

**Decision:** Use `APP_FOUNDATION_VERSION` (defined in `platformio.ini` line 29) — NOT `FIRMWARE_VERSION` (which is never defined anywhere in the project).

**Why it matters:** The WO1 worker used `FIRMWARE_VERSION` (undefined), causing the `#ifdef` to always fall to the `"unknown"` branch. The correct project-wide convention — used by `MCPBuiltinTools.cpp`, `WebServerManager.cpp`, `Application.cpp`, `U8G2DisplayModule.cpp`, and `MCPServer.cpp` — is `APP_FOUNDATION_VERSION`. Discovered during live device validation by comparing the MCP `system_info` response (`0.1.0`) against the backup `_meta.firmware_version` (`"unknown"`).

---

## ADR-6: OTAUpdateManager as IConfigurable

**Decision:** Extend `OTAUpdateManager` to implement `IConfigurable` with namespace `"ota"`, schema version 1, fields `channel` (string, default `"stable"`) and `autoCheck` (bool, default `true`).

**Why now:** OTA channel configuration was previously volatile — any setting made via the portal was lost on reboot. With `IConfigurable`, settings persist to `/mod/ota.json` and are included in config backups.

**What is NOT included:** The OTA download URL, GitHub token, and release metadata — these are runtime ephemera, not user configuration. They come from `FrameworkConfig` (compile-time) and the GitHub API (runtime).

**Allocation timing:** `OTAUpdateManager` must be registered via `registerModule()` before `beginAll()` is called, but `begin(*this)` (which wires the Application reference for EventBus access) is still called later in the boot sequence after WiFi and Time init. This order is intentional and correct.
