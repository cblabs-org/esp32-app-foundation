# PROJECT.md — pr-mk03: Module Composition & Optionality Framework

**WO branch stack base:** `pr-mk01-eventbus-concurrency` (sequencing; may rebase to `main` if mk01 delayed)
**Depends on:** pr-mk01 (stack) · **Blocks:** pr-mk04, pr-mk05
**Design refs:** MK-2, MK-4, MK-5, MK-6, MK-7 · **Gating decisions:** **OQ-MK-2** (composition home), **OQ-MK-1** (Core vs Optional tiering)
**Status:** ✅ **APPROVED — human approved decomposition + full autonomous execution**

---

## Architectural Overview

This is the keystone Project: it introduces the **two-axis optionality model**
(availability flag + explicit composition) and the composition mechanism, then
**proves it** by extracting the clearest Ring-2 optional — **HomeAssistant** —
out of `Application`'s owned members and routing it through `registerModule`
behind `WITH_HOMEASSISTANT`. After this, adding/removing an optional module no
longer edits the kernel.

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| MC-01 | **Resolve OQ-MK-2:** composition lives in a thin framework `ModuleComposition` unit (preferred) or honest `main.cpp` | Keep `main.cpp` tiny while preserving the Hollywood principle |
| MC-02 | One enable flag per optional module, owned by that module (`WITH_HOMEASSISTANT` first) | Availability axis (MK-4); two sources — hardware `board.h` vs app policy — feed one path (MK-5) |
| MC-03 | Composition order is **explicit and deterministic** | Init/tick order is load-bearing (WiFi before Time, sensor before its board) |
| MC-04 | **Static self-registration is forbidden** (no global-ctor / `.init_array` auto-register) | Avoids the C++ static-init-order fiasco (MK-7) |
| MC-05 | HomeAssistant removed as an `Application` owned member; kernel stops `#include`-ing it | Ring-2 kernel decoupling (MK-1); dogfooding proof (MK-2) |
| MC-06 | **Confirm OQ-MK-1:** Ring-1 Core (WebServer, Auth, WiFi, Config, Key, Crypto, DataManager, EventBus) stays owned | Do not extract mandatory core services |

## Strict Boundary Box

- Build the composition mechanism + enable-flag convention; extract **only**
  HomeAssistant as the proof. **Do not** extract MCP/OTA/Time/display here
  (that is mk04/mk05).
- **Do not** touch Ring-1 Core service ownership.
- Preserve exact existing init/tick ordering for all non-extracted modules.

## Tooling Constraints

`platformio` MCP tools; verify `native` + `esp32dev` **with and without**
`WITH_HOMEASSISTANT` (both must compile and boot-test in native).

## Proposed Work Order Ledger

- **[ ] [wo1-extract-ha](wo1-extract-ha/WORKORDER.md):** extract HomeAssistant: remove owned member from `Application`; implement the "honest main.cpp" pattern by instantiating and registering it directly in `main.cpp` (and templates) behind `WITH_HOMEASSISTANT`. `standard`
- **[ ] [wo2-tests-docs](wo2-tests-docs/WORKORDER.md):** tests + docs: flag-on/flag-off builds; update `using-optional-modules.md` / `creating-a-module.md`. `standard`

## Verification

Native suite green in both flag states; `esp32dev` builds both states; HA absent
from the binary when `WITH_HOMEASSISTANT` is off (size check); no reordering of
surviving modules.

## Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Drafted. Awaiting human approval + OQ-MK-1/OQ-MK-2 resolution before PA spawn. |
| 2026-07-29 | Human approved full autonomous execution (provenance 7bdd6d3). PA decomposition finalised (honest main.cpp). Spawning workers. |
