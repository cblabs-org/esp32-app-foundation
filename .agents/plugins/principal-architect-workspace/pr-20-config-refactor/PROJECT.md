# PR-20 — FrameworkConfig Refactor + appconfig.js Clarity

**PR number:** 20
**Project name:** `pr-20-config-refactor`
**Base branch:** `main`
**Status:** ✅ **ALL WOs COMPLETE & VERIFIED** — `FrameworkConfig` now holds only the 3 OTA secrets; native 525/525, esp32 build + web tests green. Ready for stack submission.
**Principal Architect:** current session

---

## Architectural Intent

`FrameworkConfig` (`include/FrameworkConfig.hpp`) conflates two layers:

1. **Genuine compile-time app constants** the framework cannot know — **KEEP**:
   `otaPublicKey`, `otaGithubRepo`, `otaGithubToken`.
2. **Runtime module defaults masquerading as framework config** — **MOVE** into the
   owning module's `resetConfig()` factory default:
   `hostname`, `timezone`, `firmwareUpdatesEnabled`, `otaQuietHour`, `mcpReadOnly`.

The goal is to remove the false coupling whereby `main.cpp` configures module runtime
behaviour through a framework-level constant file (the wrong layer).

---

## Current-State Analysis (PA research, read-only)

### How the 5 moving fields flow today

`main.cpp` populates a `FrameworkConfig` and calls `app.begin(config)`. `Application::begin()`
(`modules/Application/Application.cpp`) stores it as `_config` and **distributes** it:

| Field | Current distribution path | Consumers |
|---|---|---|
| `hostname` | `Application.cpp:161` → `_wifiManager.begin(_config.hostname)`; also `Application.cpp:210` → `_diagnosticsBoard.configure(_config.hostname, …)`; also `MCPServer.cpp:436` reads `_frameworkConfig->hostname` for status | WiFiManager, DiagnosticsBoard, MCPServer |
| `timezone` | `Application.cpp:166` → `_timeManager.begin(_config.timezone)` | TimeManager |
| `firmwareUpdatesEnabled` | `OTAUpdateManager.cpp:408` reads `app_->getConfig().firmwareUpdatesEnabled` (live) | OTAUpdateManager |
| `otaQuietHour` | OTAUpdateManager reads via `app_->getConfig()` (live) | OTAUpdateManager |
| `mcpReadOnly` | `MCPServer.cpp:53` → `_mcpReadOnly = frameworkConfig->mcpReadOnly` at begin | MCPServer |

### Per-field persistence reality — **this is the key WO2 sizing insight**

Moving a field "into `resetConfig()`" only works if it is (or becomes) a **persisted**
module config field (`serializeConfig`/`applyConfig`/`resetConfig`). The modules are **not** uniform:

| Field → target | Already persisted in target module? | Work required |
|---|---|---|
| `hostname` → `WiFiManager::resetConfig()` | ✅ **Yes** — `serializeConfig` writes `"hostname"`, `applyConfig` reads it, has `currentHostname` member | Clean: add default to `resetConfig()`, drop `begin(hostname)` seeding, **re-source DiagnosticsBoard + MCPServer status hostname** |
| `timezone` → `TimeManager::resetConfig()` | ✅ **Yes** — `_timezone`/`_posixTz` members, serialize/apply present, `getTimezone()` exists | Clean: add default to `resetConfig()`, drop `begin(timezone)` seeding |
| `mcpReadOnly` → `MCPServer::resetConfig()` | ✅ **Yes** — `"readOnly"` serialized/applied, `_mcpReadOnly` member | Clean-ish: set **secure default (`true`)** in `resetConfig()` (header currently defaults `false` — must preserve secure-by-default), drop `frameworkConfig` security read |
| `firmwareUpdatesEnabled` → `OTAUpdateManager::resetConfig()` | ❌ **No** — not in serialize/apply; read live from `app_->getConfig()` | **Larger:** add as a *new* persisted config field (member + serialize + apply + reset), replace live `getConfig()` reads |
| `otaQuietHour` → `OTAUpdateManager::resetConfig()` | ❌ **No** — same as above | **Larger:** new persisted config field |

### Cross-module ripples that WILL break on field removal

- `modules/Application/Application.cpp:119-120` — log line reads `_config.hostname` / `_config.timezone`.
- `Application.cpp:161,166` — `begin()` calls pass removed fields.
- `Application.cpp:210` — `DiagnosticsBoard::configure(_config.hostname, …)` → must source hostname from WiFiManager.
- `MCPServer.cpp:436` — `_frameworkConfig->hostname` for the status endpoint → **breaks** (hostname leaves FrameworkConfig); must re-source.
- `MCPServer::begin()` signature carries `const FrameworkConfig* frameworkConfig` (`MCPServer.hpp:163`) → review/trim.
- `src/main.cpp:47` — `config.hostname = "esp32-foundation"` → remove.
- **Tests** referencing `resetConfig` / changed signatures: `test_wifiManager`, `test_mcpServer`, `test_application`, `test_configManager`, `test_u8g2Display/test_config`, `test_hardware/test_wifi`.

---

## Resolved Design Decisions (settled with user at approval gate)

1. **Hostname ownership & default (→ WO2a).** Hostname becomes a **WiFiManager-owned persisted
   network setting** (user-editable via the portal's network settings). The **app developer must
   still set the factory default** — but through a **WiFiManager-scoped API**, not the
   `FrameworkConfig` god-struct. `WiFiManager::resetConfig()` falls back to that app-provided
   default.
   - *WO2a design detail to finalise at authoring time:* the exact injection path for the app's
     default hostname (retain/repurpose `WiFiManager::begin(defaultHostname)` vs add a
     `setDefaultHostname()` seam, and how `main.cpp`/`Application` conveys it without
     reintroducing a catch-all config struct). The principle is fixed: WiFiManager owns it; the app names the default.
2. **OTA settings ownership (→ WO2b).** `firmwareUpdatesEnabled` + `otaQuietHour` (plus any
   related OTA toggles) become **OTAUpdateManager-owned persisted config**, written to the **OTA
   config JSON** (member + serialize + apply + reset), replacing the live `app_->getConfig()`
   reads. The OO-review's broader `app_` removal is **OUT OF SCOPE** — separate future PR.
3. **MCPServer hostname re-source (→ WO2a).** MCPServer's status endpoint sources hostname **from
   WiFiManager** (single source of truth), not a snapshot string.
4. **Secure-by-default (→ WO2a).** `mcpReadOnly` default remains **`true`** after the move —
   `MCPServer::resetConfig()` must set it `true`.

---

## Work Order Ledger

- **[x] [wo1-appconfig-clarity](wo1-appconfig-clarity/WORKORDER.md):** Investigation only — ✅ **COMPLETE**. See [FINDINGS.md](wo1-appconfig-clarity/FINDINGS.md). Key results: `appconfig.js` is the Display tab controller (misnomer), **complements** the refactor with zero overlap; `stash@{1}`/`580f2f5` was **redundant** (superseded by `ef7be1e`) → **dropped**; portal exposure → only `timezone` (time.js) + `mcpReadOnly` (mcp.js), both already persisted module keys (no portal change needed). **Baseline:** native suite was RED — pre-existing `OTAUpdateManager` build breakage from `505b9da` → **fixed on `main`** (`f437a92`, `9f2b3cd`, `a377718`); WO1 branch **rebased onto green `main`** → **519/519 native tests pass**. Precondition for WO2a satisfied.
- **[x] [wo2a-persisted-moves](wo2a-persisted-moves/WORKORDER.md):** ✅ **COMPLETE & verified.** Moved the three already-persisted defaults (`hostname`→WiFiManager via `setDefaultHostname`/`getHostname`, `timezone`→TimeManager `resetConfig`→`"GMT0"`, `mcpReadOnly`→MCPServer secure-`true`); DiagnosticsBoard + MCPServer re-source hostname from WiFiManager (MCPServer **shed its `FrameworkConfig` dependency**); `FrameworkConfig` slimmed to 2 OTA toggles + 3 secrets; `main.cpp`/`template` updated. **Verified: native 522/522 PASSED, 0 ERRORED + `esp32dev` device build SUCCESS + web-test hook passed.**
- **[x] [wo2b-ota-new-persistence](wo2b-ota-new-persistence/WORKORDER.md):** ✅ **COMPLETE & verified.** `firmwareUpdatesEnabled` → OTAUpdateManager persisted config as `updatesEnabled` (default `true`), with config-schema **v4 migration** + full OTA-tab toggle (handler + status + `ota.js`), replacing the live `getConfig()` read. **`otaQuietHour` was dead → deleted** (+ stale docs corrected across `OTADeveloperGuide.md` & `design_specification.md`). `FrameworkConfig` now = **the 3 OTA secrets only** (PR-20 goal reached). **Verified: native 525/525 PASSED + `esp32dev` build SUCCESS + web tests + portal asset regenerated.** *(Worker did core impl; PA closed out asset regen + device build + tests + docs after the worker's sandbox blocked `pio`/python.)*

  *Rationale: WO2-as-one-order is `architect`-tier with a wide blast radius; the approved 2-way split yields two tight, independently-verifiable `standard` orders, serialized (they share `FrameworkConfig.hpp` + `Application.cpp`).*

---

## Branch Topology (stacked model)

Per the bridge spec + STACKING_REFERENCE, this is a **true stack with no bare parent branch**
(a `pr-20-config-refactor` branch would collide with the slash-namespaced WO refs in git):

```
main
 └─ pr-20-config-refactor/wo1-appconfig-clarity   (base: main)
     └─ pr-20-config-refactor/wo2a-persisted-moves (base: wo1)
         └─ pr-20-config-refactor/wo2b-ota-new-persistence (base: wo2a)
```

Integration is by bottom-up stack submission/merge (`gt`/`gh stack`), not by merging into a bare
feature branch. The "PR-20 feature" is the stack itself.

## Sequencing, Concurrency & Risk

- **Strictly sequential:** WO1 → (design decisions) → WO2a → WO2b. Each WO branches off the one below it; none start until the prior is approved/landed.
- **No parallelism conflicts within WO1** (read-only). WO2a/WO2b touch overlapping files (`FrameworkConfig.hpp`, `Application.cpp`, tests) → **must be serialized**, not parallel.
- **⚠️ Branch-safety precondition (RESOLUTION APPROVED):** the working tree is currently **dirty** on `main` with this session's unrelated harness-adapter / skill-portability work (`SKILL.md`, `todo.md`, deleted `web-testing-strategy.md`, untracked `.claude` symlink, `.agents/commands`, `.agents/harness-adapters/`, `references/harnesses/`). **Agreed plan:** commit this harness work directly to `main` as its own commit first, return `main` to a clean state, then cut `pr-20-config-refactor` off clean `main`. (Awaiting final execute approval before running the commit.)
- **Pre-existing stashes:** `stash@{0}` `remaining-unstaged` (unrelated), `stash@{1}` `portal-tab-wip` / `580f2f5` (← WO1's target; the work order's `stash@{2}` reference is **stale** — index drifted).
- **Test baseline:** WO1 should confirm the native unit-test suite is green *before* WO2 starts, so WO2's "all tests pass" gate has a clean baseline (OO-review flagged a prior display build gap).

---

## Distillation

✅ Session distilled — see [./context_bridge.md](./context_bridge.md) for the strategic record (ADRs, graveyard, end-state) that future Architect sessions should page in first.
