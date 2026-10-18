---
name: skill-currency-wo3
description: Handoff instructions for Work Order 3 — header template + build flags + SKILL.md routing + manifest
complexity: standard
---

# Parent Project

**PR/Project:** `pr-skill-currency` — see [`../PROJECT.md`](../PROJECT.md). Human
approval of this decomposition is recorded in that file's Status Log (2026-08-05,
"PLAN GATE SATISFIED"). That committed line is your authorisation to execute this
WORKORDER autonomously within the Strict Boundary Box below.

**Active branch:** `skill-currency/wo3-skill-hygiene` (already created, based on the
completed `skill-currency/wo2-input-docs` — the new `systems/input/` docs already
exist in the tree).

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You
are NOT the Principal Architect. Do not create branches or plan. Execute the checklist.

# Objective

Final currency pass on the `framework-expert` skill: fix the stale file-header
template, document the input/display build flags, wire the new `systems/input/`
docs into `SKILL.md` routing + triggers, and bump the manifest. Docs/config only.

# Strict Boundary Box

- **EDIT** exactly these 4 files (all under `.claude/skills/framework-expert/`, which
  resolves via the `.claude → .agents` symlink — if `git add` rejects the `.claude`
  pathspec, stage the identical `.agents/skills/framework-expert/...` path instead):
  - `coding-patterns/style-guide.md`
  - `references/build-system.md`
  - `SKILL.md`
  - `skill_manifest.json`
- **DO NOT** touch: the `systems/input/` docs, `framework-topics.md`, or
  `mcp-server/overview.md` (WO2 owns those — already complete), any source code, or
  the `LICENSE`.
- Do NOT run `git add -A`. Stage only the 4 files. (Unrelated untracked
  `.agents/context_archives/...` must NOT be committed.)

# Line-Item Instructions

### 1. `coding-patterns/style-guide.md` §3 "Module Header Template"
The example header block is stale — it shows `* ESP32 App Foundation (c) 2026` with
**no license line**. Update the `/* ... */` code fence's header lines to the
canonical block used across the repo (verify against `modules/ButtonModule/ButtonModule.hpp`):
```
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
```
- [ ] Add the ` CB Labs` org suffix, the repo-URL line, and the
      `SPDX-License-Identifier: Apache-2.0` line.
- [ ] Leave the `* Module:`, `* Description:`, and the interface-grouped
      `Exported Classes` example below it **unchanged**.

### 2. `references/build-system.md` — document optional-module build flags
- [ ] Find the build-flags / feature-gate section (read the file first). Add a short
      subsection documenting the optional-feature `build_flags` a consumer sets in
      `platformio.ini`. Verify exact flag names against `platformio.ini`,
      `sample-app/platformio.ini`, and `sample-app/src/main.cpp`:
  - `WITH_BUTTONS` — compile in `ButtonModule` (→ `systems/input/button-module.md`)
  - `WITH_ENCODER` — compile in `EncoderModule` (→ `systems/input/encoder-module.md`)
  - `WITH_DISPLAY_LVGL` — select the LVGL colour-TFT display module
  - `WITH_DISPLAY_U8G2` — select the U8G2 monochrome display module
- [ ] Keep it terse; link to the relevant systems docs.

### 3. `SKILL.md` — routing + triggers
- [ ] In the "Detailed References" routing table, add a row for the input subsystem,
      e.g. `| **Input System** (buttons, rotary encoders, gestures) | \`systems/input/\` |`.
- [ ] In the "Trigger Reference" section, add an input bullet covering: `button`,
      `ButtonModule`, `WITH_BUTTONS`, `rotary encoder`, `EncoderModule`, `WITH_ENCODER`,
      `PCNT`, `quadrature`, `gesture`, `button.click`, `encoder.rotate`, `iot_button`,
      `consumeDelta`.
- [ ] Do not disturb unrelated rows/bullets.

### 4. `skill_manifest.json` — bump + inventory
- [ ] `version`: `2.2.3` → `2.3.0`.
- [ ] `lastUpdated`: → `2026-08-05`.
- [ ] `sourceDocsVersion`: `PR-48` → `PR-75`.
- [ ] Add the 3 new files to the `references` array (place them logically with the
      other `systems/...` entries): `systems/input/overview.md`,
      `systems/input/button-module.md`, `systems/input/encoder-module.md`.
- [ ] Ensure the file remains valid JSON.

# Tooling Constraints

Docs + JSON only — no build required. Validate `skill_manifest.json` parses (e.g.
`python3 -m json.tool skill_manifest.json` or equivalent).

# Verification Requirement

- [ ] `skill_manifest.json` is valid JSON and lists the 3 new files.
- [ ] `style-guide.md` §3 header block now contains the SPDX line.
- [ ] Build flags documented with correct names verified against `platformio.ini`.
- [ ] `SKILL.md` routing row + trigger bullet present; all relative links resolve.
- [ ] `git diff --stat` (staged: only the 4 files) — show it.

# Completion Protocol

When done, **STOP and send a message back to the Principal Architect** with a short
summary + `git diff --stat`. Do NOT commit, wrap up, or distill until the PA
explicitly approves.

# Status

- Pending execution.
