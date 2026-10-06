---
work_order: WO-B2
project: pr-u8g2gl-05
title: "U8G2GLDisplay hosts the migrated DiagnosticsBoard"
branch: u8g2gl-integration/wo2-module
base: u8g2gl-integration/wo1-baseline
complexity: standard
status: dispatched (Part 1); Part 2 pending the PlatformIO MCP
---

# WO-B2: the integration module, and the first board rendered by U8G2GL

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Before starting, verify from committed
files:

- `../PROJECT.md` "Human Approval" quotes the owner's approval of the decomposition;
- its ledger lists this WO as dispatched;
- R1, R4 and R6 state the rules you are building to.

If any of these is missing, **stop and report**. Only Part 1 is authorized now.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Branch and working tree

- **Work only in `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`.** Never touch
  `/Users/mcneillm/Documents/Projects/esp32-app-foundation`.
- **Branch:** `u8g2gl-integration/wo2-module`, already checked out, with no upstream.
- **Before every commit**, run `git branch --show-current` as its own command.
- **Scratch files** go in this worktree's `tmp/`.
- **You may read, and run the CLI from,** `/Users/mcneillm/Documents/Projects/U8G2GL-05`. Never
  write there; it belongs to another PR.

## COMMIT ON A GREEN RESULT

You are authorized to commit Part 1 when it is verified. **Do NOT stop to ask**: nobody can
answer.

- Do not push, tag, flash, or open a PR.
- If a decision is not covered here and cannot be made inside the Boundary Box: commit what is
  green, set the status, write the Completion Report, and stop.

## Rules and standards

- **Read first:** every file in `.agents/rules/`, in order.
- **`04-mcp-tooling.md` binds you.** No `pio`, `platformio` or `esptool` in any form. Firmware
  builds are Part 2.
- **Documentation:** load `.agents/skills/house-style-docs/SKILL.md` and
  `references/cpp-embedded.md`. Every new file carries the foundation header with
  `SPDX-License-Identifier: Apache-2.0`, in the same shape as
  `modules/U8G2Display/boards/DiagnosticsBoard.cpp`.
- **Generated files are exempt.** Files under `generated/` are U8G2GL output, stamped by the
  generator; do not edit them.
- **Lint:** run `python3 scripts/doc_lint.py` before committing.

---

## Context you need

- **The runtime is pinned.** Use `https://github.com/cblabs-org/u8g2gl.git#69c37e01f75198a4aa3fa84b8926fcf139e30f02`.
  - It is a private repository. Git on this machine has credentials for it.
  - Its submodules are `update = none`, so they are never fetched.
  - The runtime at that sha is byte-identical to `src/` and `include/` in
    `/Users/mcneillm/Documents/Projects/U8G2GL-05`. Prove it with
    `git -C ../U8G2GL-05 diff --stat 69c37e0 HEAD -- src include` (it must be empty).
- **The U8G2GL CLI** is built at `/Users/mcneillm/Documents/Projects/U8G2GL-05/cli/dist/node/cli.js`
  (version 0.11.0). Run it with `node <that path> validate|generate <project dir>`.
- **R1: the host board.** `u8g2gl_screen` (runtime `include/u8g2gl/u8g2gl_core.h`) exposes
  `init_fn`, `activate_fn`, `deactivate_fn`, `tick_fn` and `render_fn`.
  - The host board calls `tick_fn` and `render_fn` directly.
  - It **never** calls `ui_handler()`, which clears and sends the buffer. The legacy module does
    both itself, at `U8G2Display.cpp:1413-1415`.
  - `ui_init()`, which is generated, initialises every subject and screen, and must run exactly
    once.
- **The legacy contract.** These are `IDisplayBoard` (`modules/U8G2Display/IDisplayBoard.hpp`)
  virtuals:
  - `render(U8G2&)`
  - `isExpired()`
  - `canAdvance()`
  - `getBoardName()`
  - `onActivate()` / `onDeactivate()`, which `CarouselEngine` calls on switches
  - `tick(uint32_t)`
- **A scoping trial the PA ran** (scratch only, CLI 0.11.0) validated and generated cleanly:

  ```xml
  <!-- globals.xml -->
  <subjects>
      <string name="diag_hostname" max_len="63" />
      <string name="diag_wifi" max_len="63" />
      <string name="diag_uptime" max_len="31" />
      <string name="diag_heap" max_len="47" />
  </subjects>
  <fonts>
      <builtin name="u8g2_font_helvB12_tr" />
      <builtin name="u8g2_font_helvR08_tr" />
  </fonts>
  <!-- screens/screen_diagnostics.xml -->
  <u8g2gl_label name="hostname" x="0" y="0"  width="256" height="16" font="u8g2_font_helvB12_tr" align="center" bind_text="diag_hostname" />
  <u8g2gl_label name="wifi"     x="4" y="16" width="248" height="14" font="u8g2_font_helvR08_tr" bind_text="diag_wifi" />
  ...
  ```

  - The target is `<target name="ssd1322" setup="U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI" rotation="R0" />`.
  - The **geometry is a first guess, not the answer**: `height` is mandatory, and the legacy
    labels have none.
  - A screen file **must** be named `screen_*.xml`. Any other name is silently ignored and
    produces "0 screens" (a CLI finding, already reported).
  - The `max_len` values mirror legacy's buffer sizes (64/32/48 bytes including the terminator).
    The hostname is not a legacy buffer; size it to `WifiManager`'s hostname capacity, and cite
    it.
- **What WO-B1 measured about legacy**, in `test/render/` and `../evidence/EVIDENCE.md`:
  - `0xB7` draws nothing and takes zero width. Row 2 of `connected` shows an 8-column gap.
  - **Descenders are clipped.** Legacy `drawText` (`DrawingPrimitives.cpp`) passes `x + w - 1`
    and `y + h - 1` as the far corner of `setClipWindow`. U8G2 treats that corner as
    **exclusive**, so the last column and row are lost. With `h = getMaxCharHeight()`, that cuts
    into the descenders: "My" renders as "Mv".
  - The long SSID is clipped mid-glyph at x = 251, with no ellipsis.
  - The legacy module sets `setDrawColor(1)`, `setFontMode(1)`, `setFontRefHeightAll()` and
    `setFontPosTop()` once, at `U8G2Display.cpp:143-146`. A U8G2GL screen running inside the
    legacy carousel **inherits** that state.

## The pixel target

The gate (U8G2GL `implementation_plan.md` §6, criterion 1) asks for **pixel identity with the
legacy board**. Target it **bug-for-bug**: the clipped descenders, the invisible separator, and the
mid-glyph clip all included. Whether to fix the legacy defects afterwards is for the owner, not
this WO.

Your levers are:

- the XML: geometry, alignment, `long_mode`, styles;
- the glue's format strings, which must be **byte-identical** to legacy's, including `\xB7`.

**You may not change the U8G2GL runtime or generator.** If identity is unreachable without such a
change, stop iterating. Report the residual difference precisely: differing pixel count per
scenario, the rows and columns, an ASCII diff of the worst case, and the cause **measured** in
runtime code (cite file and line). Do not guess at the cause.

## Strict Boundary Box

**In scope:**

- `modules/U8G2GLDisplay/` (new)
- `uidesign/ssd1322-256x64/u8g2gl/` (new)
- `scripts/u8g2gl_generate.py` (new)
- `platformio.ini`: only the lines that add the generated UI and the hook
- `src/main.cpp`: only the `DiagnosticsBoard` registration
- `THIRD-PARTY-NOTICES.md`: one row
- `test/render/`: a second render target, and its outputs
- `../evidence/EVIDENCE.md`: After for section 1 and section 2
- this Completion Report

**Out of scope:**

- **Anything in `modules/U8G2Display/`.** It is about to be frozen; nothing is deleted.
- `sample-app/`, and every other module.
- The committed legacy baseline files.
- Web diagnostics (R7).
- Any U8G2GL source.

---

## Part 1 — Line-Item Instructions

1. [ ] **`uidesign/ssd1322-256x64/u8g2gl/`.** Create the XML project:
   - `u8g2gl.xml`, `globals.xml`, and `screens/screen_diagnostics.xml`;
   - the committed `generated/`, from the CLI above;
   - a `library.json` (name `U8G2GLUI`, `build.srcDir`/`includeDir` set to `generated`, depending
     on `u8g2gl`), shaped like `uidesign/crowpanel-128-rotary/lvglpro/library.json`.

   Comments in the XML explain the geometry choices **by measurement** (for example, "height 10:
   reproduces legacy's clip at y + getMaxCharHeight() − 1, see EVIDENCE §1"). `validate` and
   `generate` must both be clean.

2. [ ] **`modules/U8G2GLDisplay/`.**
   - **`library.json`.** Name `U8G2GLDisplay`. Dependencies: `u8g2gl` pinned **here, and only
     here**, by the git URL and sha above; `olikraus/U8g2` `^2.36.18`; `U8G2GLUI` `*`; and the
     legacy module, for `IDisplayBoard`, by the name its own `library.json` uses. Add the same
     `-I../../include` flag pattern as the legacy module if the includes need it.
   - **`U8G2GLDisplay.{hpp,cpp}`.** A `U8G2GLDisplay::begin()` that runs once: it registers the tick
     source with `u8g2gl_set_tick_cb`, using a function returning `millis()`, then calls
     `ui_init()`. It is idempotent: a second call is a no-op.
   - **`U8G2GLScreenBoard.{hpp,cpp}`.** A generic `IDisplayBoard` over a `u8g2gl_screen&` and a
     board name.
     - `onActivate` / `onDeactivate` call the screen's `activate_fn` / `deactivate_fn`, and set it
       active with `u8g2gl_set_active_screen`, so subject writes mark the right screen dirty.
     - `tick` calls `tick_fn`, and `render` calls `render_fn`.
     - `isExpired()` returns false by default.
     - **Measure what the runtime assumes about inherited U8G2 state.** Establish in `render`
       whatever the screen needs and the legacy module does not already guarantee. Document each
       setting with its reason.
   - **`boards/DiagnosticsScreenBoard.{hpp,cpp}`.** A `U8G2GLScreenBoard` over
     `ui_screen_diagnostics`, named `"APP: Diagnostics"` as legacy is. It must not collide with
     the legacy `DiagnosticsBoard` symbol, because both link into the same firmware.
     - `configure(WifiManager*)` sets the hostname subject once, as legacy does.
     - `tick` keeps legacy's 1-second throttle semantics exactly.
     - **Split the hardware read from the formatting:**
       - a `DiagnosticsInputs` struct;
       - `readDiagnosticsInputs(WifiManager*)`, the only code touching `WiFi`, `millis()` and
         `heap_caps`;
       - a pure `formatDiagnostics(const DiagnosticsInputs&, …)` using **legacy's format strings
         byte for byte**.

       The harness drives `formatDiagnostics` with fixed inputs.
   - **Native exclusion.** Choose `exclude_on_native.py` in the legacy module's pattern, or a
     `lib_ignore` in `[env:native]`, whichever keeps `env:native` from compiling this module.
     State which and why. The suite itself cannot be run until Part 2.

3. [ ] **`scripts/u8g2gl_generate.py`.** The pre-build hook, added as `pre:` to
   `[env:base_esp32]`'s `extra_scripts`.
   - **Configured.** The CLI path comes from the environment variable `U8G2GL_CLI` (a path to
     `cli.js`), or from a `custom_u8g2gl_cli` option in the env. When set, run
     `node <cli> generate uidesign/ssd1322-256x64/u8g2gl --target ssd1322`, and fail the build on a
     non-zero exit, printing the CLI's output.
   - **Not configured.** Print one line saying the committed `generated/` is used and why, and
     continue ([D-11]).
   - **Never `npx`.** A docstring states `npx u8g2gl` as the eventual shape, once the CLI is
     published.
   - **Testability.** Keep the decision and the invocation in a plain function that runs under
     `python3` outside SCons. Prove all three paths from the shell: not configured, configured
     and succeeding, and configured with a deliberately broken XML in a `tmp/` copy, where it
     fails with exit ≠ 0. The SCons wrapper stays thin.

4. [ ] **`platformio.ini`.** Wire `esp32dev` and `esp32s3nano` (both SSD1322 256×64) to see
   `uidesign/ssd1322-256x64`, without disturbing `crowpanel-128-rotary` (which overrides
   `lib_extra_dirs`) or `native`. Change the minimum lines. Explain each in the Completion
   Report.

5. [ ] **`src/main.cpp`.** Under `WITH_DISPLAY_U8G2`, replace the legacy board's include, instance,
   `configure` and `carousel.add` (lines 91, 93, 211 and 213) with the migrated board. Call
   `U8G2GLDisplay::begin()` before the board's `configure`. Nothing else in the file changes.

6. [ ] **`THIRD-PARTY-NOTICES.md`.** Add one row to External Dependencies:

   | Name | Source | Licence | Description |
   |---|---|---|---|
   | **U8G2GL** | `github.com/cblabs-org/u8g2gl` | MIT | "Declarative UI runtime for U8G2 displays (`modules/U8G2GLDisplay`)" |

   Note that its generated code (under `uidesign/*/u8g2gl/generated/`) is MIT, per U8G2GL
   design_spec §22.

7. [ ] **Render the migrated board: `test/render/`.** Add a second target, `--board u8g2gl`. It
   compiles:
   - the U8G2GL runtime, by `FetchContent` of the pinned sha, or a `-DU8G2GL_SOURCE_DIR`
     override pointing at `../U8G2GL-05`;
   - the committed `generated/`;
   - `DiagnosticsScreenBoard`'s `formatDiagnostics`;
   - the host board.

   Use the same U8G2, the same flags and the same legacy display state as `--board legacy`. Run the
   same four scenarios through `DiagnosticsInputs`. Write outputs to
   `test/render/after/diagnostics_board/` (committed).

   Then compare every scenario byte for byte with `baseline/`:
   - Iterate on the levers until all four are identical, **or** stop under "The pixel target"
     and report.
   - Add one probe scenario rendered **without** `setFontPosTop()`, which a plain U8G2GL user
     calling `ui_handler()` would not have set. Report what it changes. This tests a suspected
     runtime gap: the spec calls font-position-top "the framework default", but nothing in the
     runtime sets it.

8. [ ] **`EVIDENCE.md`.**
   - **§1 After:** the comparison table, per scenario, with identical yes or no, the differing
     pixel count, and sha256. Add the `setFontPosTop` probe result.
   - **§2 After:** line counts with `test/render/count_lines.py`, covering:
     - the XML (`u8g2gl.xml`, `globals.xml`, `screen_diagnostics.xml`);
     - the new C++ the board needs (`DiagnosticsScreenBoard.*`);
     - the generic host (`U8G2GLScreenBoard.*`, `U8G2GLDisplay.*`), **counted separately**,
       because it is paid once for every future board;
     - the `main.cpp` lines.

     `generated/` is counted and reported separately, as machine output, never mixed with
     authored lines. Add a short, factual "reading" paragraph on what each side has to say to
     describe the same board.

9. [ ] **Commit** in logical steps, for example `feat(display): U8G2GLDisplay board host`,
   `feat(uidesign): DiagnosticsBoard in U8G2GL XML`, `build: u8g2gl pre-build hook`,
   `test(render): render the migrated board`, and `docs(pa): …`. End each message with
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Part 2 — PENDING (do not start; resumed when the PlatformIO MCP is connected)

- The `esp32dev` firmware builds with the pin resolved, and `env:native` still builds and passes
  at its baseline count.
- The hook runs during a real PlatformIO build, both configured and not configured.

## Verification Requirement (Part 1)

- `validate` and `generate` are clean.
- The committed `generated/` equals a fresh `generate` (regenerate, then `git diff --exit-code`).
- Both render targets build from clean. The comparison result is reported as measured.
- The hook's three paths are demonstrated.
- `doc_lint.py` is clean.
- `git diff --stat u8g2gl-integration/wo1-baseline..HEAD` shows nothing outside the Boundary Box.

## Issue Surfacing

Report U8G2GL generator or runtime defects, and legacy defects, in the Completion Report, with
file and line. Do not fix or file them.

---

## Completion Report

<!-- Written by the worker: Status, Changes Made, Decisions Made (native exclusion choice, U8G2
state established in render and why, geometry levers chosen), Dead Ends & Warnings, Out-of-Scope
Findings, Blockers. Add the comparison table, the setFontPosTop probe, the hook demonstrations and
the line counts. -->
