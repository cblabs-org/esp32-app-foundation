# PROGRAM-PLAN.md — Microkernel Architecture Refactor

Detailed sequencing, stack topology, conflict analysis, and per-Project mandate
for the `microkernel-refactor` program. Companion to [`PROGRAM.md`](./PROGRAM.md).

---

## 1. Stack Topology

Linear main-line stack, plus two independent side PRs that base directly on
`main` and can merge in any order.

```
main
 │
 ├── pr-mk02-kernel-hardening        (independent · tick budget)      ──▶ merges to main anytime
 ├── pr-mk06-periodic-helper         (independent · Periodic helper)  ──▶ merges to main anytime
 │
 └── pr-mk01-eventbus-concurrency    (MK-11, MK-12)   ◀── PREREQUISITE
      │
      └── pr-mk03-module-composition (MK-2, MK-4–7 · extract HA)
           │
           └── pr-mk04-boot-seam-display (MK-8, MK-9 · needs mk01 + mk03)
                │
                └── pr-mk05-extract-optionals (MK-2, MK-6 · MCP/OTA/Time)
                     │
                     └── pr-mk07-persistence-partitions (MK-14–18 · + hardware-config)
```

**Why this order:**
- **mk01 first** — the EventBus race + per-publish allocation is a prerequisite:
  mk04 moves display state onto the bus, and the whole "bus as IPC" premise rests
  on it. Landing it first also gets a latent bug fixed early.
- **mk03 before mk04/mk05** — the composition mechanism must exist before any
  module (display included) can be extracted from the kernel's owned members.
- **mk04 needs both mk01 and mk03** — it is stacked on mk03, with mk01 already
  in the stack below, so both ancestors are present. (mk03's dependency on mk01
  is *sequencing, not code* — mk03 can rebase onto `main` if mk01 is delayed.)
- **mk07 last** — it has the widest blast radius and an external dependency on
  the hardware-config program; it should land on a stable, fully-extracted
  module set.
- **mk02 and mk06 are independent** — small, low-risk, no shared files with the
  main line; keep them off the critical path so they never block it.

## 2. Conflict / Parallelism Analysis (Core Directive 4)

| Pair | Shared files? | Guidance |
|---|---|---|
| mk01 ↔ mk03 | `EventBus.*` (mk01) vs `Application.*` (mk03) — minimal overlap; both touch `Application` wiring | **Serialize** (stack mk03 on mk01). |
| mk04 ↔ mk05 | Both touch `Application` composition + `main.cpp`/composition unit | **Serialize** (mk05 stacks on mk04). |
| mk02 (side) ↔ main line | `Application::tick()` dispatch loop (mk02) vs mk03/mk04 module wiring | Low overlap but both edit `Application`. **Land mk02 early** to avoid rebase churn, or accept a small manual rebase. |
| mk06 (side) ↔ main line | ~87 module `tick()` bodies (mk06) vs targeted module extraction (mk03–05) | Broad-but-shallow edits. **Land mk06 early** or expect mechanical rebase conflicts in extracted modules. |
| mk07 ↔ hardware-config program | `partitions.*.csv`, `board.<id>.h`, `platformio.ini` | **Cross-program serialize** — coordinate with hardware-config PA; do not run concurrently. |

**Recommendation:** run the main line strictly serialized (stacked). Land the two
side PRs (mk02, mk06) **first / early** so their broad-but-shallow edits don't
collide with the main-line module surgery.

## 3. Per-Project Mandate Summary

> Full detail in each `PROJECT.md`. WO decomposition is the assigned PA's job.

### pr-mk01 — eventbus-concurrency  ·  _prerequisite_
Give `EventBus` a defined concurrency contract and remove the per-publish
allocation. **Resolve OQ-MK-3 first.** Preferred: keep the `EventBus` interface,
back it with ESP-IDF `esp_event` on-target (thread-safe, ISR-safe, already linked
and used by HomeAssistant), synchronous stub on native; intern topics away from
`std::string`. Blast radius = every publisher/subscriber; semantics shift sync→
async on-target. Gate: **OQ-MK-3**.

### pr-mk02 — kernel-hardening  ·  _independent_
Promote the ≤10 ms `tick()` budget into the `ITickable` contract; add debug-build
per-module tick-duration instrumentation (`millis()` wrap + `ESP_LOGW` over
budget). No scheduler. Small, low risk. Land early.

### pr-mk03 — module-composition
Introduce the two-axis optionality model (availability flag + explicit
composition) and the composition unit (**resolve OQ-MK-2**, leaning the thin
`ModuleComposition` unit). Prove the pattern by extracting **HomeAssistant**
(default-off) from `Application`'s owned members and routing it through
`registerModule` behind `WITH_HOMEASSISTANT`. Establishes deterministic ordering;
forbids static self-registration. Gates: **OQ-MK-2, OQ-MK-1** (Core vs Optional
tiering shapes what is/ isn't extracted).

### pr-mk04 — boot-seam-display
Add `IBootReporter` (capability-named, refines `IStatusReporter` with
`beginEarly()`) for the pre-EventBus splash; extract the display to a registered
module; move CONFIGURING/RUNNING display behaviour onto the existing
`APP_CONFIGURING`/`APP_RUNNING` events and delete the kernel's direct
`_display.carousel.*` calls and `#ifdef HAS_DISPLAY` scatter. Coordinate the
display enable-flag surface with the hardware-config program (`BOARD_DISPLAY_KIND`
vs `HAS_DISPLAY`/`HAS_LVGL_DISPLAY`). Depends: mk01 + mk03.

### pr-mk05 — extract-optionals
Extract the remaining optionals — **MCP**, **OTA (now default-off)**, **Time** —
through the composition mechanism, behind `WITH_MCP` / `WITH_OTA` / `WITH_TIME`.
`Application` stops owning these as members. Confirm Core tier (WebServer,
AuthManager, WiFi, Config, Key, Crypto, DataManager, EventBus) stays owned per
**OQ-MK-1**. Depends: mk03 (stacked on mk04).

### pr-mk06 — periodic-helper  ·  _independent_
Factor the ~87 hand-rolled `millis()`-throttle sites into a tiny loop-context
`Periodic{interval}` helper. **Do not** use FreeRTOS software timers (cross-task
context reintroduces locking). Mechanical, low risk. Land early.

### pr-mk07 — persistence-partitions  ·  _cross-program_
Implement the four-tier persistence model (NVS secrets / `config` LittleFS
partition / A/B app slots with compiled-in assets / disposable cache), the
consumer-selectable partition presets (A compiled-in vs B FS-assets-over-OTA with
verify-before-commit + compiled-in fallback), the layout-agnostic runtime mount
(probe + config-root path abstraction), and a **1 MB `config` partition** sized
for the worst-case ~5×. **Depends on the hardware-config program** (`board.<id>.h`,
partition selection) and **OQ-MK-4/OQ-MK-5**. Sequence last; may spin its own
sub-plan.

## 4. Execution Protocol

1. Human approves this decomposition → Chief records approval in each
   `PROJECT.md` status log.
2. Commit design docs + workspace to the correct base on `main` (human-approved).
3. Land side PRs mk02, mk06 early.
4. For each main-line Project in order: spawn a Tier-2 Principal Architect with a
   Bridge Prompt pointing at that `PROJECT.md`; the PA decomposes into WOs and
   spawns Tier-3 workers; on completion the PA distills and the Chief re-ingests
   before advancing.
5. Resolve each Project's gating decision (§4 of `PROGRAM.md`) **before** spawning
   its PA.
6. Submit the stack via the stacking tooling (Torvalds Standard PR descriptions);
   cascade `gt sync` as bottom-of-stack PRs merge.

## 5. Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Plan drafted. Topology, conflict analysis, and per-project mandates recorded. Awaiting human approval. |
