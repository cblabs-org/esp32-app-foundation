---
name: skill-currency-wo2
description: Handoff instructions for Work Order 2 — input subsystem docs + EventBus topics + MCP tools
complexity: standard
---

# Parent Project

**PR/Project:** `pr-skill-currency` — see [`../PROJECT.md`](../PROJECT.md). Human
approval of this decomposition is recorded in that file's Status Log (2026-08-05,
"PLAN GATE SATISFIED"). That committed line is your authorisation to execute this
WORKORDER autonomously within the Strict Boundary Box below.

**Active branch:** `skill-currency/wo2-input-docs` (already created, based on the
completed `skill-currency/wo1-encoder-licensing`).

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You
are NOT the Principal Architect. Do not make high-level architectural decisions, do
not create branches, and do not plan. Execute the checklist below.

# Objective

Document the framework's **input subsystem** (`ButtonModule` + `EncoderModule`) in
the `framework-expert` skill, and bring the EventBus-topics and MCP-server docs
current with the button/encoder/KeyManager additions. Documentation only.

# Ground Truth — READ THE SOURCE FIRST (do not trust this summary blindly)

The facts below are the PA's research starting point. **You MUST confirm every name,
flag, topic string, and signature against the actual source before writing** — an
earlier internal registry mislabelled this module "InputModule/`WITH_INPUT`", but it
shipped as **`ButtonModule`/`WITH_BUTTONS`**. Authoritative sources to read:
- `modules/ButtonModule/ButtonModule.hpp` + `.cpp`, `include/ButtonConfig.h`
- `modules/EncoderModule/EncoderModule.hpp` + `.cpp`, `include/EncoderConfig.h`
- `modules/KeyManager/KeyManager.cpp` (`registerMCPTools`)
- `sample-app/src/main.cpp` (registration patterns, `#ifdef WITH_BUTTONS` / `WITH_ENCODER`)
- Existing docs for house style: `systems/display/overview.md`,
  `systems/event-bus/framework-topics.md`, `systems/mcp-server/overview.md`

**ButtonModule** (gated `WITH_BUTTONS`): `ITickable` + `IMCPContributor`; pure
service module. `addButton(BoardButton)` before `begin(app)`; publishes to EventBus.
Types in `include/ButtonConfig.h`: `ButtonType {GPIO, ADC_LADDER, MATRIX}`,
`ButtonGesture {CLICK, DOUBLE_CLICK, LONG_PRESS, PRESS_DOWN}`, `BoardButton`,
`ButtonEvent`. EventBus topics: `button.click`, `button.double_click`,
`button.long_press`, `button.press_down` (payload `ButtonEvent`). MCP tool:
`button_status` (read-only diagnostics).

**EncoderModule** (gated `WITH_ENCODER`): `ITickable` + `IMCPContributor`;
multi-encoder via ESP-IDF PCNT. `addEncoder(BoardEncoder)` before `begin(app)`.
**Dual consumption model**: (1) push — publishes EventBus `encoder.rotate` (payload
`EncoderEvent`); (2) pull — `consumeDelta(mode)` / `consumeDeltaAt(index, mode)` with
`EncoderDelta {RAW, ACCELERATED}` atomic accumulators. Types in
`include/EncoderConfig.h`: `EncoderAccelConfig`, `BoardEncoder`, `EncoderDelta`,
`EncoderEvent`. MCP tool: `encoder_status` (read-only). PR #75 refactored
`adapters/lvgl/CrowPanel128Adapter` to consume this module.

**KeyManager MCP tools** (PR #65, exactly 4): `key_list` (read-only, no args),
`key_get` (read-only; arg `id`), `key_add` (mutating; args `id`≤64, `value`≤1024,
optional `managedBy`, `exclusive`), `key_delete` (mutating; arg `id`, optional
`managedBy` for exclusive keys). `key_add`/`key_delete` respect `systemLocked` and
exclusive owner-matching; metadata tools never return plaintext.

# Strict Boundary Box

- **CREATE** exactly these 3 new files:
  - `.claude/skills/framework-expert/systems/input/overview.md`
  - `.claude/skills/framework-expert/systems/input/button-module.md`
  - `.claude/skills/framework-expert/systems/input/encoder-module.md`
- **EDIT** exactly these 2 existing files:
  - `.claude/skills/framework-expert/systems/event-bus/framework-topics.md`
  - `.claude/skills/framework-expert/systems/mcp-server/overview.md`
- **DO NOT** touch: `SKILL.md`, `skill_manifest.json` (that is WO3), any source code,
  any file under `modules/`/`include/`/`test/`, the `LICENSE`, or any other skill file.
- `.claude/skills/framework-expert` resolves through the `.claude → .agents` symlink;
  edit via the `.claude/...` path. Do NOT create a second copy under `.agents/...`.
- Do NOT run `git add -A`. Stage only the 5 files above. (An unrelated untracked
  `.agents/context_archives/...` directory must NOT be committed.)

# Line-Item Instructions

- [ ] `systems/input/overview.md`: introduce the input subsystem — both modules are
      optional pure service modules (Service Module Pattern), enabled by build flags
      (`WITH_BUTTONS`, `WITH_ENCODER`), consumed via EventBus (push) with the encoder
      additionally offering a pull API. Link to the two module docs and to
      `systems/event-bus/framework-topics.md`.
- [ ] `systems/input/button-module.md`: purpose, enablement (`WITH_BUTTONS`),
      `BoardButton` descriptor types, registration pattern (mirror `sample-app`:
      `addButton()` → `begin(app)` → `registerModule()`; note begin-before-register),
      the 4 `button.*` EventBus topics + `ButtonEvent` payload, and the `button_status`
      MCP tool.
- [ ] `systems/input/encoder-module.md`: purpose, enablement (`WITH_ENCODER`),
      `BoardEncoder`/accel types, registration pattern, the **dual consumption model**
      (EventBus `encoder.rotate` push vs `consumeDelta`/`consumeDeltaAt` pull with
      RAW/ACCELERATED), multi-encoder/PCNT note, `encoder_status` MCP tool, and the
      CrowPanel LVGL adapter consuming it.
- [ ] `systems/event-bus/framework-topics.md`: add the 4 `button.*` topics and
      `encoder.rotate`. These are **module-published** (only when the module is
      compiled in) — do NOT lump them into the always-on framework topic count;
      add a clearly-labelled "Module-Published Topics (optional)" subsection (or
      equivalent) and keep the existing always-on section accurate.
- [ ] `systems/mcp-server/overview.md`: under the existing *Module-Contributed Tools
      (dynamic)* section, add `button_status`, `encoder_status`, and the 4 KeyManager
      tools (`key_list`, `key_get`, `key_add`, `key_delete`), noting which are
      read-only vs mutating (relevant to read-only mode).

# House Style

Match the terse, agent-facing tone and structure of the sibling `systems/*/overview.md`
files. If in doubt about header/section conventions, consult the `house-style-docs`
skill. Keep it a distillation, not a tutorial.

# Tooling Constraints

Pure Markdown documentation — no build/compile required. Verify by re-reading your
files and confirming every relative link resolves to a real path.

# Verification Requirement

- [ ] All 3 new files exist and all 5 files are internally consistent.
- [ ] Every module name, build flag, topic string, type name, and MCP tool name
      matches the actual source you read (not just this WORKORDER).
- [ ] Every relative markdown link resolves.
- [ ] `git diff --stat` (staged: only the 5 files) — show it.

# Completion Protocol

When done, **STOP and send a message back to the Principal Architect** with a short
summary + the `git diff --stat` + a note of anything in the source that contradicted
this WORKORDER's ground-truth summary. Do NOT commit, wrap up, or distill until the
PA explicitly approves.

# Status

- Pending execution.
