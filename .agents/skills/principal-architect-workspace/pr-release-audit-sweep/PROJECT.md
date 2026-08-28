# PROJECT — Release Audit Sweep

> **Project**: `pr-release-audit-sweep`
> **Base branch**: `main` (commit `f53cc1d7`)
> **Created**: 2026-08-28
> **Scope**: Blocks 1, 2, 3, 4, 7, 8, 9, 10 from the release audit triage — 27 issues across 8 blocks
> **Excluded**: Block 0 (#175 P1 blocker — handled separately), Block 5 (OTA pipeline), Block 6 (CI)

---

## Architectural Overview

This project sweeps the codebase to resolve all findings from the v0.0.12-dev.54 release audit plus the pre-existing open issues from the triage. The work spans:

- **Build fixes** (compile failures on specific configs)
- **DataManager correctness** (stack headroom, MCP missing fields, job options)
- **Display UX** (misleading precondition checkmark, SLS naming)
- **Portal CSS/JS** (dead classes, visibility, branding)
- **Tick-path performance** (heap allocations, polling, vector copies)
- **PreconditionEngine polish** (stale docs, per-precondition policies)
- **Documentation sweep** (file headers, Doxygen, constants)
- **Audit script improvements** (false positive fixes)

## File Overlap Analysis & Ordering Rationale

| File | WO1 | WO2 | WO3 | WO4 | WO5 | WO6 | WO7 | WO8 |
|------|-----|-----|-----|-----|-----|-----|-----|-----|
| `U8G2DisplayModule.cpp` | ✗ | ✗ | | | | | | ✗ |
| `LVGLDisplayModule.cpp` | ✗ | ✗ | | ✗* | | | | ✗ |
| `Application.cpp/.hpp` | | ✗ | ✗* | | | | | ✗ |
| `DataManager.cpp/.hpp` | | | ✗ | | | | | ✗ |

The linear WO chain is ordered to place overlapping file modifications adjacent:
**WO1 → WO2 → WO3 → WO4 → WO5 → WO6 → WO7 → WO8**

---

## Work Order Ledger

| WO | Block | Topic | Issues | Complexity | Effort | Status |
|----|-------|-------|--------|------------|--------|--------|
| **[ ] [wo1-build-breakers](wo1-build-breakers/WORKORDER.md):** | 🔴 1 | Fix compile failures across build configs | #155, #153, #138 | standard | ~1h | Not started |
| **[ ] [wo2-tick-optimisations](wo2-tick-optimisations/WORKORDER.md):** | ⚡ 9 | Eliminate heap allocations and polling in tick() | #180, #181, #182 | standard | ~1h | Not started |
| **[ ] [wo3-datamanager-residuals](wo3-datamanager-residuals/WORKORDER.md):** | 🟠 2 | DataManager stack, MCP fields, job options, façade | #173, #170, #171, #172 | architect | ~3h | Not started |
| **[ ] [wo4-sls-precondition](wo4-sls-precondition/WORKORDER.md):** | 🟡 3 | Fix misleading precondition screen + SLS naming | #158, #97 | standard | ~2h | Not started |
| **[ ] [wo5-portal-css](wo5-portal-css/WORKORDER.md):** | 🟢 4 | Portal CSS/JS cleanup + hex colour triage | #139, #142, #141, #140, #80, #185 | standard | ~3h | Not started |
| **[ ] [wo6-precondition-polish](wo6-precondition-polish/WORKORDER.md):** | ⚪ 7 | Sweep stale docs + per-precondition policies | #152, #154 | standard | ~2h | Not started |
| **[ ] [wo7-audit-script](wo7-audit-script/WORKORDER.md):** | 🔧 10 | Fix audit script false positives | #184, #186, #187 | mechanical | ~1h | Not started |
| **[ ] [wo8-doc-sweep](wo8-doc-sweep/WORKORDER.md):** | 📝 8 | File headers, Doxygen, constant comments | #176, #177, #178, #179 | mechanical | ~6h | Not started |

**Total: 8 WOs · 27 issues · ~19h estimated**

---

## Stacked Branch Plan

```
main
 └─ release-audit-sweep/wo1-build-breakers          ← #155, #153, #138
     └─ release-audit-sweep/wo2-tick-optimisations   ← #180, #181, #182
         └─ release-audit-sweep/wo3-datamanager-residuals  ← #173, #170, #171, #172
             └─ release-audit-sweep/wo4-sls-precondition   ← #158, #97
                 └─ release-audit-sweep/wo5-portal-css     ← #139, #142, #141, #140, #80, #185
                     └─ release-audit-sweep/wo6-precondition-polish ← #152, #154
                         └─ release-audit-sweep/wo7-audit-script   ← #184, #186, #187
                             └─ release-audit-sweep/wo8-doc-sweep  ← #176, #177, #178, #179
```

---

## WO Details

### WO1 — Build Breakers (Block 1)

**Issues**: #155, #153, #138
**Files**:
- `modules/HomeAssistant/HomeAssistantModule.cpp` — move `#include "utils/serveGzipAsset.hpp"` inside `#ifdef WITH_HOMEASSISTANT` guard
- `modules/U8G2Display/U8G2DisplayModule.cpp` — fix static call in OTA predicate lambda (L821–823)
- `modules/LVGLDisplay/LVGLDisplayModule.cpp` — align OTA predicate to match U8G2 fix (L1008–1010)
- `lib/DisplayServiceHelper/DisplayServiceHelper.hpp` — may need predicate signature update

**Verification**: Build all `platformio.ini` environments green. Specifically `esp32dev_all_on` and `WITHOUT_HOMEASSISTANT` configs.

---

### WO2 — Tick-Path Optimisations (Block 9)

**Issues**: #180, #181, #182
**Files**:
- `modules/WebServer/WebServerManager.cpp/.hpp` — replace `_wifiManager->isReady()` polling with EventBus subscription
- `modules/U8G2Display/U8G2DisplayModule.cpp` — `std::swap` instead of vector copy (L297–304)
- `modules/LVGLDisplay/LVGLDisplayModule.cpp` — `std::swap` instead of vector copy (L343–350)
- `modules/Application/Application.cpp` — fixed `char[16]` buffer for `WiFi.localIP()` (L327)

**Verification**: Build green. Existing tests pass. No functional change — pure optimisation.

---

### WO3 — DataManager Residuals (Block 2)

**Issues**: #173, #170, #171, #172
**Files**:
- `modules/DataManager/DataManager.hpp` — configurable stack size constant, `requiresTls` in `DataJobOptions`
- `modules/DataManager/DataManager.cpp` — use configurable stack, honour `requiresTls` in job drain
- `modules/MCPServer/MCPBuiltinTools.cpp` — add `lastStatus`, `everSucceeded` to `get_data_sources`
- `include/AppFoundation.h` — decouple direct hub includes for native testability

**Verification**: Build green. Existing DataManager and MCP tests pass. New test cases for `requiresTls` and expanded MCP fields.

> [!WARNING]
> **#172 (façade decoupling)** is architecturally sensitive — `AppFoundation.h` is the single entry point for all consumers. The fix must not break any downstream `#include "AppFoundation.h"` consumer. Marked `architect` complexity.

---

### WO4 — SLS Precondition Screen (Block 3)

**Issues**: #158, #97
**Files**:
- `modules/LVGLDisplay/screens/SLSSystemScreens.cpp` — fix checkmark icon for unmet preconditions, call `describe()`
- `modules/LVGLDisplay/screens/SLSSystemScreens.hpp` — supporting declarations
- `squareline/crowpanel-128/SquareLine_Project.spj` — naming convention cleanup (if safe to edit)

**Verification**: Build green. Visual verification on hardware (or screenshot via MCP) that unmet preconditions show warning icon + description text, not a checkmark.

---

### WO5 — Portal CSS/JS Cleanup (Block 4)

**Issues**: #139, #142, #141, #140, #80, #185
**Files**:
- `web/portal/app.css` — add `.hidden` class, fix `button.secondary` contrast, clean up unused classes
- `web/portal/auth.js` — relies on `.hidden` fix
- `web/portal/wifi.js` — consolidate `.password-input-wrapper` / `.eye-btn`
- `web/portal/utils.js` — document `getTabIcon()` stroke behaviour
- `web/portal/system.js`, `web/portal/backup.js`, etc. — hex colour audit (from #185)
- Module JS files: `hardware.js`, `mcp.js`, `ota.js`, `time.js`, `weather.js` — hex colour audit

**Verification**: Portal loads cleanly in browser. All tabs render. Dark mode contrast acceptable. No visual regressions.

---

### WO6 — PreconditionEngine Polish (Block 7)

**Issues**: #152, #154
**Files**:
- `docs/developer/*.md` — sweep stale `IConfigGate` references
- `modules/PreconditionEngine/PreconditionEngine.hpp` — per-precondition `DampingPolicy`/`RetryPolicy` override
- `modules/PreconditionEngine/PreconditionEngine.cpp` — honour per-precondition policies
- Tests — update/add coverage for per-precondition policies

**Verification**: Build green. PreconditionEngine tests pass. Docs grep shows zero `IConfigGate` references.

---

### WO7 — Audit Script Improvements (Block 10)

**Issues**: #184, #186, #187
**Files**:
- `.agents/skills/framework-expert/scripts/release-audit-checks.sh` — add `.gitignore` filtering, suppress known-valid driver registrations, enumerate config overrides with intentionality markers
- `platformio.ini`, `sample-app/platformio.ini`, `template-app/platformio.ini` — add intentionality comments to overrides

**Verification**: Re-run audit script. Confirm false-positive count drops. No regressions in true-positive detection.

---

### WO8 — Documentation Sweep (Block 8)

**Issues**: #176, #177, #178, #179
**Files**: ~50+ framework `.hpp`/`.cpp` files across `modules/`, `lib/`, `include/`, `adapters/`

**Scope**:
- Add structured file headers (Module:/Description:) to 25 files
- Add Doxygen `@brief` to public/protected methods (prioritise framework core, deprioritise tests)
- Add inline comments to 78 undocumented constants
- Document multi-file module directory convention (for #179 — script/convention, not code)

**Verification**: Re-run audit script dimension 1 checks. Hit count significantly reduced.

> [!TIP]
> This WO is ideal for a `/goal` session — systematic, large, and mechanical. Can be split into sub-passes (headers first, then Doxygen, then constants).

---

## Concurrency & Conflict Advisory

All WOs are serialised in the stacked branch. No parallel execution planned. The ordering minimises merge conflicts:

- WO1 → WO2: Both touch `U8G2DisplayModule.cpp` and `LVGLDisplayModule.cpp` but at different line ranges (OTA predicate vs vector copy). Stacking ensures clean rebasing.
- WO2 → WO3: Potential `Application.hpp` overlap. Serialised to avoid.
- WO4 uses `LVGLDisplayModule.cpp` (SLS screens, not main module file) — no conflict with WO1/WO2.
- WO5–WO8 have zero file overlap with WO1–WO4.

---

## Approval Record

| Date | Approver | Decision |
|------|----------|----------|
| 2026-08-28 | User (matthewmcneill) | Approved with two amendments: (1) WO4 must use squareline-expert skill for .spj edits, (2) Verification plan must include full release audit re-run |

---

## Session Log

### 2026-08-28 — Project Creation

**Context**: Release audit completed (v0.0.12-dev.54, commit f53cc1d7). 13 audit issues filed (#175–#187). User requested PA decomposition of blocks 1, 2, 3, 4, 7, 8, 9, 10 (excluding Block 0 P1, Block 5 OTA, Block 6 CI).

**Decisions**:
- Single stacked PR with 8 WOs, not multi-PR programme (blocks are cohesive enough, all from the same audit)
- WO ordering driven by file overlap analysis (display modules touched by blocks 1, 3, 9)
- WO3 (DataManager) marked `architect` complexity due to façade decoupling (#172)
- WO8 (doc sweep) placed last — largest, most mechanical, least risk
- Block 0 (#175 P1 use-after-free) excluded per user — handled separately before this project starts
