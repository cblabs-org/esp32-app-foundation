---
name: lvgl-display-wo11
description: Handoff instructions for Work Order 11 — documentation, skill updates, design-doc amendments
complexity: mechanical
---

# lvgl-display / WO11 — Documentation, Skill Updates & Design-Doc Amendments

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo10-hardware-bringup`
- **Your branch:** `lvgl-display/wo11-docs` (created for you — **do not branch**)
- **Stack position:** WO11 of 11 (top of stack — the PR head).
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Complete the documentation surface for the LVGL module: module README, `framework-expert` skill updates, design-spec cross-reference, using-optional-modules entry, and the agreed amendments to the design doc.

## Context You Need
- Read `house-style-docs` (documentation standards) and the `framework-expert` skill display-system docs structure (`systems/display/`). Mirror how `U8G2Display` is documented.
- The three **design-doc amendments** agreed during PR-47 planning (apply to `docs/lvgl-display-design.md`):
  1. **§7.5 encoder** → primary path is `ESP32Encoder`/hardware PCNT; the software-`IRAM_ATTR`-ISR writeup is retained as the documented fallback (OQ-4).
  2. **§12.1 `lv_conf.h`** → module-local, wired via `-DLV_CONF_PATH` (not repo root / `LV_CONF_INCLUDE_SIMPLE`).
  3. **§12.1 partitions** → set `board_build.partitions = partitions.esp32s3nano.csv` (reuse existing 16 MB scheme); add the ≥16 MB-only note (D9) and the assets-compiled-into-binary rationale (§9.5), cross-referencing framework issue #40.

## Strict Boundary Box — do NOT:
- Change any module/adapter source or tests (docs only). If you find a code/doc mismatch, report it — do not fix code here.
- Touch the U8G2 branding retrofit (tracked in issue #39) beyond a forward-reference.
- Modify `PROJECT.md` history (that is the PA's).

## Tooling Constraints
Docs only — no build needed, but run `run_tests` (native) once at the end to confirm you broke nothing. Use `platformio` MCP tools if any build check is needed. `house-style-docs` throughout.

## Line-Item Instructions
- [ ] `modules/LVGLDisplay/README.md` — module overview, interfaces implemented, config schema, adapter model, SquareLine integration (`ui_init` hook), assets-in-binary guidance, ≥16 MB requirement.
- [ ] `framework-expert` skill: add LVGL module coverage under `systems/display/` (peer to U8G2), including the adapter pattern and the boot-branding/system-screen provider (D8).
- [ ] Design spec: add the LVGL module cross-reference where §7 Display Architecture / the module catalogue lives (mirror the U8G2 entry).
- [ ] `docs/developer/using-optional-modules.md` — add an LVGL module entry.
- [ ] Apply the three design-doc amendments to `docs/lvgl-display-design.md` (§7.5, §12.1 ×2) and update its status/last-updated.
- [ ] Ensure all new docs link correctly (relative links only — no absolute OS paths).

## Verification Requirement
- [ ] Links resolve; docs match the shipped code (interfaces, config keys, env name, pins-as-flags).
- [ ] `run_tests` (native) green (sanity — nothing broken).

## Completion Protocol
1. Validate, then **STOP and message the Principal Architect** with results. Surface out-of-boundary findings; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo11-docs/`.
4. Append a status update to this `WORKORDER.md` and terminate.
