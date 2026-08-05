# PROJECT: skill-currency

**Project ID:** `pr-skill-currency`
**Principal Architect Session:** current
**Base branch:** `main` (`origin/main` @ `44f162d`)
**Origin:** User-requested review of the `framework-expert` skill + docs against the
last two weeks of merged PRs (#42, #44, #47, #49, #51, #53, #56, #58, #61, #65,
#68, #69, #74, #75). The review found the skill's currency stamp stuck at
`sourceDocsVersion: PR-48` while several consumer-facing capabilities merged since.

## Architectural Intent

Bring the `framework-expert` skill and its bundled docs back to full currency with
`main`, and fix one licensing regression that rode in with the EncoderModule
recovery (PR #75). No framework code behaviour changes — this is documentation
currency plus a mechanical license-header correction.

### Findings driving the work

1. **Input subsystem undocumented.** `ButtonModule` (`WITH_BUTTONS`) and
   `EncoderModule` (`WITH_ENCODER`) are on `main` but have no skill coverage.
   - ButtonModule: `ITickable` + `IMCPContributor`; publishes EventBus
     `button.click|double_click|long_press|press_down` (payload `ButtonEvent`);
     MCP tool `button_status`; types in `include/ButtonConfig.h`.
   - EncoderModule: `ITickable` + `IMCPContributor`; dual consumption — push via
     EventBus `encoder.rotate` (`EncoderEvent`) **and** pull via
     `consumeDelta()/consumeDeltaAt()` (RAW/ACCELERATED); MCP tool
     `encoder_status`; multi-encoder (PCNT); types in `include/EncoderConfig.h`.
   - ⚠️ Worker must verify module names/flags against real source — the PA registry
     labelled this "InputModule/`WITH_INPUT`" but it shipped as `ButtonModule`/`WITH_BUTTONS`.
2. **KeyManager MCP tools undocumented** (PR #65): `key_list`, `key_get` (read-only),
   `key_add`, `key_delete` (mutating). Absent from `systems/mcp-server/`.
3. **License-header regression** (rode in with PR #75 encoder recovery): 5 files
   still carry the pre-#69 `Creative Commons Attribution-NonCommercial-ShareAlike 4.0`
   header instead of `SPDX-License-Identifier: Apache-2.0`.
4. **Skill header template stale** — `coding-patterns/style-guide.md` §3 omits the
   SPDX license line entirely.
5. **Build-flag gates undocumented** — `WITH_BUTTONS`, `WITH_ENCODER`,
   `WITH_DISPLAY_LVGL`, `WITH_DISPLAY_U8G2`.
6. **Manifest stale** — `skill_manifest.json` at `sourceDocsVersion: PR-48`.

## Work Order Ledger

- **[x] [wo1-encoder-licensing](wo1-encoder-licensing/WORKORDER.md):** *(mechanical)*
  Replace CC BY-NC-SA headers with Apache-2.0 SPDX in the 5 recovered encoder files
  (source tree). Base: `main`.
- **[ ] [wo2-input-docs](wo2-input-docs/WORKORDER.md):** *(standard)* Create
  `systems/input/` (overview + button-module + encoder-module); add `button.*` and
  `encoder.rotate` to `systems/event-bus/framework-topics.md`; add `button_status`,
  `encoder_status`, and the 4 KeyManager tools to `systems/mcp-server/overview.md`.
  Base: `skill-currency/wo1-encoder-licensing`.
- **[ ] [wo3-skill-hygiene](wo3-skill-hygiene/WORKORDER.md):** *(standard)* Fix the
  `style-guide.md` §3 header template; document `WITH_*` build flags in
  `references/build-system.md`; add `systems/input/` routing rows + input/button/encoder
  trigger vocab to `SKILL.md`; bump `skill_manifest.json` (version, lastUpdated,
  sourceDocsVersion, inventory). Base: `skill-currency/wo2-input-docs`.

## Concurrency & Conflict Analysis

Linear stack, no parallel spawning. WO1 touches only the **source tree**
(`modules/EncoderModule/`, `include/EncoderConfig.h`, `test/`), fully disjoint from
the **skill docs** touched by WO2/WO3. WO3 edits `SKILL.md` + `skill_manifest.json`
to reference the new files WO2 creates, so WO3 is stacked strictly after WO2. No
overlapping-file conflicts across the stack.

## Status Log

- **2026-08-05** — Project created. Decomposition drafted (3-WO linear stack).
- **2026-08-05 — PLAN GATE SATISFIED (human→PA boundary).** User (`matthewmcneill`)
  reviewed the (a)-scope decomposition in session and granted explicit approval:
  *"ok please act as /principal-architect and implement all these items via workers.
  Approval granted to operate autonomously."* This recorded approval is the
  provenance authorising Tier-3 workers to execute their WORKORDERs autonomously
  within their Strict Boundary Boxes.
- **2026-08-05** — WO1 branch `skill-currency/wo1-encoder-licensing` created off `main`; worker spawned.
- **2026-08-05** — ✅ **WO1 COMPLETE.** Worker (haiku) executed the 5-file CC→SPDX
  swap; PA independently verified the diff (5 files, comment-only, zero CC remaining,
  native build + 417/417 tests green). Committed `c6b7ae1`. Per-WO distillery skipped
  (mechanical WO); will distill at project wrap-up.
- **2026-08-05** — WO2 branch `skill-currency/wo2-input-docs` created off WO1; worker spawned.
