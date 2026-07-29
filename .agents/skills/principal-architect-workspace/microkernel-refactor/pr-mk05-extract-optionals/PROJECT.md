# PROJECT.md — pr-mk05: Extract Remaining Optionals (MCP, OTA, Time)

**WO branch stack base:** `pr-mk04-boot-seam-display`
**Depends on:** pr-mk03 (composition) · **Blocks:** pr-mk07 (stable module set)
**Design refs:** MK-2, MK-6 · **Gating decision:** **OQ-MK-1** (confirm Core stays owned)
**Status:** ✅ **APPROVED — human approved full autonomous execution (2026-07-29)**

---

## Architectural Overview

With the composition mechanism proven on HomeAssistant (mk03) and the display
extracted (mk04), this Project extracts the remaining Ring-2 optionals — **MCP**,
**OTA (default-off)**, **Time (default-on)** — from `Application`'s owned members,
each behind its enable flag. On completion the kernel owns **only** Ring-0
primitives + Ring-1 Core services; every optional enters through `registerModule`.

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| EO-01 | `MCPServer` → `registerModule` behind `WITH_MCP` (default **off**) | Remote agent-control surface; security + flash/heap weight (MK-6) |
| EO-02 | `OTAUpdateManager` → behind `WITH_OTA` (default **off**) | Heaviest optional (TLS + download engine + own task); shipping this fleet-update mechanism is a deliberate policy choice (user decision, MK-6) |
| EO-03 | `TimeManager` → behind `WITH_TIME` (default **on**) | Lightweight; NTP/TZ commonly wanted; gate already opt-in |
| EO-04 | `Application` stops owning MCP/OTA/Time as members and stops `#include`-ing them | Ring-2 kernel decoupling (MK-1) |
| EO-05 | **OQ-MK-1 RESOLVED → Ring-1 Core** (WebServer, Auth, WiFi, Config, Key, Crypto, DataManager, EventBus) remains owned + registered, no flag | Mandatory platform services are not extracted |
| EO-06 | Handle cross-service wiring the kernel did for these (e.g. OTA↔ConfigManager manifest snapshot, MCP built-in tools needing Time/WiFi) via injection at composition, not kernel ownership | Preserve behaviour without re-coupling |
| EO-07 | **Expose OTA's event-topic contract (`OTA_PROGRESS` + payload) on the facade, but keep it `WITH_OTA`-gated** (MK-6a) | power-monitor subscribes today via a **deep kernel include** — a coupling defect. Facade access fixes it; the flag-gate keeps an unguarded reference a **clean compile error** (make it legible: "requires `WITH_OTA`"), never a silent no-op. Soft consumers guard with `#ifdef WITH_OTA` |
| EO-08 | **Guarantee OTA-start→reboot headroom for async event callbacks, or expose a synchronous pre-reboot hook** | Under async dispatch (mk01), power-monitor's `OTA_PROGRESS → energy-flush` subscriber must complete before reboot. Prefer a synchronous pre-reboot hook on OTA |

## Strict Boundary Box

- Extract **only** MCP, OTA, Time. **Do not** touch Ring-1 Core ownership or the
  display/boot seam.
- Preserve every cross-service behaviour these modules had while kernel-owned
  (audit `Application.cpp` wiring: OTA `begin(*this)`, MCP `begin(..., &time, &wifi, &auth)`).
- Update `template-app` / `sample-app` `main.cpp` to reflect the new flags.

## Tooling Constraints

`platformio` MCP tools; verify `native` + `esp32dev` across the flag matrix
(all-on, all-off, default). Confirm binary shrinks with optionals off (flash/heap
payoff, MK-6/§8).

## Proposed Work Order Ledger _(PA to finalise)_

- **[ ] wo1 — extract MCP:** `WITH_MCP`; rewire built-in-tool dependencies at
  composition. `standard`
- **[x] wo2 — extract OTA:** `WITH_OTA` (default off); preserve ConfigManifest
  snapshot wiring + rollback bless; **move `OTA_PROGRESS` topic contract onto the
  facade, flag-gated with a legible absent-module error (EO-07); add a synchronous
  pre-reboot hook or documented headroom (EO-08).** `architect`
- **[ ] wo3 — extract Time:** `WITH_TIME` (default on). `standard`
- **[ ] wo4 — app templates + tests + size check:** update template/sample apps;
  flag-matrix tests; record flash deltas. `standard`

## Verification

Flag matrix builds green (native + esp32dev); OTA rollback + config snapshot
behaviour unchanged when `WITH_OTA` on; measurable flash reduction with optionals
off; template/sample apps boot.

## Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Drafted. Awaiting human approval. OTA confirmed default-off per user. |
| 2026-07-29 | Human approved full autonomous execution (provenance `7bdd6d3`). mk01 ✅, mk03 ✅, mk04 ✅. PA spawn authorised. |
