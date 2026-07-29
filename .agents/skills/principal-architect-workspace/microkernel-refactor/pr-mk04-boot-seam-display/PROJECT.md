# PROJECT.md — pr-mk04: Boot Seam (`IBootReporter`) & Display Extraction

**WO branch stack base:** `pr-mk03-module-composition`
**Depends on:** pr-mk01 (safe EventBus), pr-mk03 (composition) · **Blocks:** pr-mk05 (sequencing)
**Design refs:** MK-8, MK-9 · **Coordinate with:** hardware-config program (`BOARD_DISPLAY_KIND`)
**Status:** ✅ **APPROVED — human approved full autonomous execution (2026-07-29)**

---

## Architectural Overview

The display is the last thing giving the kernel concrete module knowledge. It is
special for exactly one reason — the **boot splash must run before the EventBus
and module lifecycle exist**. This Project adds the narrow pre-bus seam
(`IBootReporter`), extracts the display to a registered Ring-2 module, and moves
all *post-boot* display behaviour onto the EventBus — after which
`Application` mentions neither U8G2 nor "display" nor `#ifdef HAS_DISPLAY`.

### Key Design Decisions

| # | Decision | Rationale |
|---|---|---|
| BS-01 | Add `IBootReporter : public IStatusReporter` with one `beginEarly()` hook | Capability-named (not `IDisplayStartup`); formalises the existing two-phase bootstrap (MK-8) |
| BS-02 | Kernel `begin()` step 0 scans the registry for an `IBootReporter`, calls `beginEarly()`, adopts it as reporter | Zero concrete display knowledge in the kernel; `NullStatusReporter` fallback for headless |
| BS-03 | Boot reporter runs **inside** the rollback barrier; never blesses/persists-as-good early | A bad-OTA `beginEarly()` bug must bootloop → auto-rollback (MK-8) |
| BS-04 | Move CONFIGURING/RUNNING display behaviour to `APP_CONFIGURING`/`APP_RUNNING` handlers; **delete** kernel `_display.carousel.*` / `showBase` calls | Post-boot behaviour rides the bus (MK-9); removes `#ifdef HAS_DISPLAY` scatter |
| BS-05 | Display becomes a `registerModule` module (both U8G2 and LVGL implement `IBootReporter`) | Ring-2 decoupling; both display kinds get the boot splash the LVGL path currently skips |
| BS-06 | Display enable-flag surface coordinated with `BOARD_DISPLAY_KIND` (hardware-config OQ-2) | Avoid duplicating the display-kind selector; may land the flag bridge here and finalise in hardware-config |
| BS-07 | **Preserve `IDisplayBoard::canAdvance()` (FW-1 / #47) through the display rework** | FW-1 (board→engine query) and MK-9 (removes kernel→carousel calls) are orthogonal; **FW-1 should merge first and mk04 must carry `canAdvance()` forward, not drop it** (departures-board consumer review, #48) |

## Strict Boundary Box

- Touch the boot-reporter seam, the display module(s), and the kernel's display
  coupling **only**. **Do not** extract MCP/OTA/Time (mk05).
- **Do not** introduce a display *drawing* abstraction (ADR-02 stands — seam is
  lifecycle-only).
- Preserve boot-splash timing/behaviour as observed today.
- **Sequencing:** FW-1 (#47, `canAdvance()`) should merge **before** mk04; mk04
  carries it forward. Add the async transition-ordering caveat (MK-9) to the
  display's event-handler docs — the bounded post-CONFIGURING carousel lag is
  accepted; do not reintroduce a kernel→display synchronous call to hide it.

## Tooling Constraints

`platformio` MCP tools; verify `native`, `esp32dev` (U8G2), and
`crowpanel-128-rotary` (LVGL) all build; hardware boot-splash smoke test on at
least one display target via the `device-mcp-client` / pio-manager flow.

## Finalised Work Order Ledger

- **[x] [wo1 — `IBootReporter` seam](wo1-iboot-reporter/WORKORDER.md):** interface + kernel registry-scan boot flow +
  `NullStatusReporter` fallback. `architect`
- **[ ] [wo2 — display extraction](wo2-display-extraction/WORKORDER.md):** register display as a module; implement
  `IBootReporter` on U8G2 + LVGL; delete `#ifdef HAS_DISPLAY`. `architect`
- **[ ] [wo3 — display-on-EventBus](wo3-display-on-eventbus/WORKORDER.md):** move CONFIGURING/RUNNING behaviour to event
  handlers; delete kernel `_display.*` direct calls. `standard`
- **[ ] [wo4 — flag coordination + tests](wo4-flag-coordination-tests/WORKORDER.md):** `BOARD_DISPLAY_KIND` bridge; boot-splash
  smoke tests. `standard`

## Verification

All three envs build; boot splash + CONFIGURING board + wizard behave identically
on U8G2; LVGL now shows a boot splash; `grep` confirms zero `HAS_DISPLAY` /
`_display.` references in `Application`.

## Status Log

| Date | Entry |
|---|---|
| 2026-07-28 | Drafted. Awaiting human approval. Coordinate display-flag surface with hardware-config PA. |
| 2026-07-29 | Human approved full autonomous execution (provenance `7bdd6d3`). mk01 ✅, mk03 ✅. PA spawn authorised. |
