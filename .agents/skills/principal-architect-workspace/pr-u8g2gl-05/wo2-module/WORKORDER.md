---
work_order: WO-B2
project: pr-u8g2gl-05
title: "U8G2GLDisplay hosts the migrated DiagnosticsBoard"
branch: u8g2gl-integration/wo2-module
base: u8g2gl-integration/wo1-baseline
complexity: standard
status: Part 1 complete (committed, verified on host); Part 2 pending the PlatformIO MCP
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

1. [x] **`uidesign/ssd1322-256x64/u8g2gl/`.** Create the XML project:
   - `u8g2gl.xml`, `globals.xml`, and `screens/screen_diagnostics.xml`;
   - the committed `generated/`, from the CLI above;
   - a `library.json` (name `U8G2GLUI`, `build.srcDir`/`includeDir` set to `generated`, depending
     on `u8g2gl`), shaped like `uidesign/crowpanel-128-rotary/lvglpro/library.json`.

   Comments in the XML explain the geometry choices **by measurement** (for example, "height 10:
   reproduces legacy's clip at y + getMaxCharHeight() − 1, see EVIDENCE §1"). `validate` and
   `generate` must both be clean.

2. [x] **`modules/U8G2GLDisplay/`.**
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

3. [x] **`scripts/u8g2gl_generate.py`.** The pre-build hook, added as `pre:` to
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

4. [x] **`platformio.ini`.** Wire `esp32dev` and `esp32s3nano` (both SSD1322 256×64) to see
   `uidesign/ssd1322-256x64`, without disturbing `crowpanel-128-rotary` (which overrides
   `lib_extra_dirs`) or `native`. Change the minimum lines. Explain each in the Completion
   Report.

5. [x] **`src/main.cpp`.** Under `WITH_DISPLAY_U8G2`, replace the legacy board's include, instance,
   `configure` and `carousel.add` (lines 91, 93, 211 and 213) with the migrated board. Call
   `U8G2GLDisplay::begin()` before the board's `configure`. Nothing else in the file changes.

6. [x] **`THIRD-PARTY-NOTICES.md`.** Add one row to External Dependencies:

   | Name | Source | Licence | Description |
   |---|---|---|---|
   | **U8G2GL** | `github.com/cblabs-org/u8g2gl` | MIT | "Declarative UI runtime for U8G2 displays (`modules/U8G2GLDisplay`)" |

   Note that its generated code (under `uidesign/*/u8g2gl/generated/`) is MIT, per U8G2GL
   design_spec §22.

7. [x] **Render the migrated board: `test/render/`.** Add a second target, `--board u8g2gl`. It
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

8. [x] **`EVIDENCE.md`.**
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

9. [x] **Commit** in logical steps, for example `feat(display): U8G2GLDisplay board host`,
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

### Status

**Part 1 complete, verified on the host. All four scenarios are pixel-identical to the legacy
board, bug for bug. No U8G2GL change was needed.** Part 2 was not attempted (it is not authorised,
and no PlatformIO MCP tools are in this worker's toolset). Nothing here has been built by
PlatformIO: the `platformio.ini` wiring, the dependency resolution of the pin and the hook inside a
real build are all unexercised (see Blockers).

Authorization was verified from committed artifacts: this WORKORDER names `pr-u8g2gl-05`;
`../PROJECT.md` "Human Approval" quotes the owner's approval of the decomposition; its ledger lists
`wo2-module` as dispatched; R1, R4 and R6 are stated there. `git -C ../U8G2GL-05 diff --stat 69c37e0
HEAD -- src include` is empty. The pinned sha resolves to the full
`69c37e01f75198a4aa3fa84b8926fcf139e30f02`.

Commits on `u8g2gl-integration/wo2-module` (`git branch --show-current` run as its own command
before every commit; every message ends with the `Co-Authored-By` line):

| Commit | Subject |
|---|---|
| `3a338a4b` | feat(uidesign): DiagnosticsBoard in U8G2GL XML |
| `03716082` | feat(display): U8G2GLDisplay board host (module, `THIRD-PARTY-NOTICES.md` row) |
| `8679d083` | build: u8g2gl pre-build hook (script and `platformio.ini`) |
| `161c4b82` | feat(app): DiagnosticsBoard migrated to U8G2GL in main |
| `781135b5` | test(render): render the migrated board |
| `94ebfee0` | fix(render): track test/render/CMakeLists.txt (see Out-of-Scope Findings, item 1) |
| (the commit that holds this file) | docs(pa): U8G2GL PR-05 WO-B2 Part 1 evidence |

### Changes Made

- `uidesign/ssd1322-256x64/u8g2gl/`: `u8g2gl.xml`, `globals.xml`, `screens/screen_diagnostics.xml`,
  `library.json` (`U8G2GLUI`, `srcDir` and `includeDir` `generated`, depends on `u8g2gl` with `*`, so
  the pin lives only in `U8G2GLDisplay`), and the committed `generated/` (8 files, CLI 0.11.0).
- `modules/U8G2GLDisplay/`: `library.json` (pins `u8g2gl` by URL and sha, here only),
  `U8G2GLDisplay.{hpp,cpp}`, `U8G2GLScreenBoard.{hpp,cpp}`, `boards/DiagnosticsScreenBoard.{hpp,cpp}`.
- `scripts/u8g2gl_generate.py`: the hook.
- `platformio.ini` (four hunks, below), `src/main.cpp` (6 lines),
  `THIRD-PARTY-NOTICES.md` (one row, plus one sentence saying the generated code is MIT).
- `test/render/`: `CMakeLists.txt`, `main.cpp`, `README.md`, `.gitignore`, and the committed
  `after/diagnostics_board/` (`.bin`, `.pbm`, `inputs.json`, `state_probe.json`, `clip_probe.json`,
  three `probe_*.pbm`).
- `../evidence/EVIDENCE.md`: section 1 After and section 2 After. This Completion Report.

`git diff --stat u8g2gl-integration/wo1-baseline..HEAD` lists only the Boundary Box paths (plus this
WORKORDER, which the PA's dispatch commit created on this branch). Nothing under
`modules/U8G2Display/`, `sample-app/`, the legacy baseline files or any U8G2GL source changed.

### Verification (all measured)

- **`validate` and `generate`**: `Validation passed: 0 errors, 0 warnings`; `generate` exits 0.
- **`generated/` equals a fresh `generate`**: regenerated with the CLI, then `git diff --exit-code --
  uidesign` is clean; `generate --check` reports "up to date".
- **Both render targets build from clean** (default `FetchContent` of U8G2 and of the pinned U8G2GL
  sha, which is private and fetched with this machine's git credentials). Warnings: only the two
  `-Wunused-parameter` ones in unmodified legacy `IGfxWidget.hpp:137`. None from new code.
- **Reproducible from the committed tree**: `git archive HEAD` of the needed paths into a scratch
  directory, built from clean there, reproduces the committed `baseline/` and `after/` exactly.
- **Determinism**: clean builds with the `*_SOURCE_DIR` overrides, and two with `FetchContent`, gave
  identical `.bin`, `.pbm` and JSON (apart from the recorded source description). Scenarios rendered
  in reverse order gave the same four `.bin`.
- **The legacy baseline is unchanged**: re-rendered after my harness edits, `git status` shows no
  change under `baseline/`.
- **The fetched runtime** equals `U8G2GL-05`'s `src/` and `include/` (`diff -r` empty).
- **`doc_lint.py`**: no findings (it scans `docs/` Markdown only, so it does not exercise the new
  files). `scripts/lint_includes.py`: no violations.

### The comparison

`render_harness --board u8g2gl --compare test/render/baseline` (the `after` target), byte for byte:

| Scenario | Identical | Differing pixels | `.bin` sha256 (after = baseline) |
|---|---|---:|---|
| `connected` | yes | 0 | `50567054cfcad77c4fecb2519a1cc564b9bfd3a218fb756bebd1838dc7b27251` |
| `ap_mode` | yes | 0 | `110974bf911419aa709a69634e228802fba4b85de092ee519bf3674f197add86` |
| `disconnected` | yes | 0 | `8d3ba960a35442433ab6b51fbe96955bad3dbc6934f1f82d1426d359bb3cf847` |
| `long_ssid` | yes | 0 | `6a9b4231659f467b713db12b5fdde790b520055f0f03c1df34102ba17f16a334` |

The `.pbm` files are also identical. The comparison does detect differences: while the SSID field was
33 bytes, the same run reported `long_ssid` as 208 differing pixels (rows 18..25, columns 183..250).

### The `setFontPosTop` probe

The `connected` frame rendered **without** `setFontPosTop()`: **15 lit pixels instead of 1392**
(1407 pixels differ, rows 0..53, columns 5..194; the 15 are descender tips). The text is not
shifted, it is nearly gone: in U8G2's default baseline mode a label's `y` is the baseline, the
glyphs are drawn above it and the label's clip removes them. A plain U8G2GL user (`begin()` only,
`ui_handler()`) gets the identical near-blank frame. **The suspected runtime gap is real.**
The design_spec states font-position-top as "the framework default" (`docs/design_spec.md:2216`) and
`src/u8g2gl_label.cpp:334` and `src/u8g2gl_clock.cpp:221` assume it, but nothing in `src/` sets it.
Also measured: without `setFontRefHeightAll()` every row sits one pixel higher (1137 pixels differ).
Detail and the three frames: `EVIDENCE.md` section 1, `state_probe.json`, `probe_*.pbm`.

### Hook demonstrations (all three paths, from the shell)

1. **Not configured** (`env -u U8G2GL_CLI python3 scripts/u8g2gl_generate.py`), exit 0:
   `[u8g2gl_generate] U8G2GL_CLI is not set and custom_u8g2gl_cli is empty: using the committed
   uidesign/ssd1322-256x64/u8g2gl/generated`.
2. **Configured and succeeding** (`U8G2GL_CLI=.../U8G2GL-05/cli/dist/node/cli.js`), exit 0: the CLI
   output follows, all eight files reported `unchanged`, and `git diff` of `uidesign/` is empty.
3. **Configured, broken XML in a `tmp/` copy** (`tmp/hook-broken/`, an unknown attribute on one
   label), **exit 1**: the CLI's error (`Unknown attribute 'bogus_attr' on <u8g2gl_label>`,
   "Generation refused ... Nothing was written") is printed, then
   `[u8g2gl_generate] FAILED (exit 1): the build stops rather than use a stale generated/`.

Also: a configured path that does not exist exits 127 with a message (an error, not a silent skip).
The SCons wrapper is short; it skips with a message for an env whose `lib_extra_dirs` lacks the
UI directory (`crowpanel-128-rotary`).

### `platformio.ini` changes, and why each

1. `base_esp32` `lib_extra_dirs = modules` becomes `modules, uidesign/ssd1322-256x64`: `esp32dev` and
   `esp32s3nano` inherit it and so find the `U8G2GLUI` library. `crowpanel-128-rotary` overrides the
   line and is untouched; `native` has its own.
2. `base_esp32` `extra_scripts` gains `pre:scripts/u8g2gl_generate.py`.
3. `crowpanel-128-rotary` `lib_ignore = U8G2Display` gains `U8G2GLDisplay`. `base_esp32` uses
   `lib_ldf_mode = deep`, which does not evaluate `#if`, and `main.cpp` now includes
   `U8G2GLDisplay.hpp` inside the `WITH_DISPLAY_U8G2` branch. The existing `lib_ignore = U8G2Display`
   in that env exists for the same reason. Without this the LDF would pull the module into the LVGL
   board and fail to resolve `U8G2GLUI`. **Unverified without the MCP.**
4. `native` `lib_ignore` gains `U8G2GLDisplay` (`native_ha` inherits it).

### Decisions Made

- **Native exclusion: `lib_ignore` in `[env:native]`, not an `exclude_on_native.py`.** A source
  filter script runs only after a library is selected, so it cannot stop the library's own
  `dependencies` (a private git URL) from being resolved. `lib_ignore` stops it being selected at all.
  The module has no source that should be compiled on native in any case (everything in it touches
  `WiFi`, `millis()` or the runtime). The suite cannot be run until Part 2.
- **`onActivate` / `onDeactivate`**: the WORKORDER says to call the screen's `activate_fn` /
  `deactivate_fn` and set the screen active. `u8g2gl_set_active_screen()` already calls the outgoing
  screen's `deactivate_fn` and the incoming `activate_fn` (`src/u8g2gl_core.cpp:71-86`), so calling
  them as well would run each twice. The board calls only `u8g2gl_set_active_screen(&screen)` on
  activate, and `u8g2gl_set_active_screen(nullptr)` on deactivate, if its screen is still the active
  one. Same effect, once.
- **U8G2 state established in `render()`, and why** (read from the code, then measured):
  - *Inherited, left alone*: `setFontPosTop()`, `setFontRefHeightAll()`, `setDrawColor(1)`,
    `setFontMode(1)`, set once at `U8G2Display.cpp:143-146` and changed by no legacy board. Measured
    above: the screen cannot draw without the first two.
  - *Saved and restored*: the **font mode**. The runtime's label sets it on every draw
    (`src/u8g2gl_label.cpp:331`, default solid) and the state saver restores only colour and font
    (`u8g2gl_state_saver.h:80-83`), so after the runtime's `render_fn` the mode is **0**; the legacy
    module relies on 1 for every other board. After the board's `render()` it is 1 again. Measured.
  - *Established*: a current **font**. With none set the state saver restores a null font and U8G2
    dereferences it (below). `begin()` sets none; legacy boards' saver skips a null font. The board
    sets `AppFonts::defaultSmall()` (already linked) if none is current. No pixel changes.
- **Geometry levers** (all measured; each explained in the XML): label heights 16 (hostname) and 10
  (data rows), data-row width 247, hostname width 256, `long_mode` left at `clip`, no styles.
  Legacy clips to `y + getMaxCharHeight() - 1` as an exclusive corner; the runtime's rect keeps its
  last row; hence one less. `ascent - descent` equals `getMaxCharHeight()` (17, 11) so the label's
  vertical centring truncates to 0. The hostname width stays 256: 255 would shift the framework's
  default hostname `"esp32-device"` one pixel left of legacy.
- **`DiagnosticsInputs` string capacity is 64**, not WifiManager's 33. The harness's `long_ssid` is 38
  characters, longer than WifiManager can hold (`char _wifiSsid[33]`); at 33 the SSID was truncated
  and `long_ssid` differed. 64 is the size of the line the SSID feeds. WifiManager itself still caps
  real SSIDs at 32.
- **`max_len` values**: hostname 32 (WifiManager `_currentHostname[33]`, `WifiManager.hpp:137`), wifi
  63, uptime 31, heap 47 (legacy `char[64]`, `[32]`, `[48]`; `max_len` excludes the terminator).
- **Include order**: each module `.cpp` includes the generated `ui.hpp` before any runtime header,
  because `ui.hpp` includes the generated capacities first and they only apply if seen before
  `u8g2gl_config.h`. `U8G2GLScreenBoard.hpp` forward-declares `u8g2gl_screen` rather than including
  the core header, for the same reason. My first build had four macro-redefinition warnings from
  getting this wrong.
- **The generated config is not force-included** (see Blockers, item 2). The runtime's binding pool is
  therefore sized by its defaults, not by the 4 bindings counted. Only `u8g2gl_obj.cpp` reads those
  capacities, so there is no layout mismatch; it costs RAM and nothing else.
- The harness compiles the runtime's every `src/*.cpp` the way PlatformIO would, and never regenerates
  `generated/`.

### Line counts (`count_lines.py`)

| Part | Total | Code |
|---|---:|---:|
| XML (3 files) | 137 | 33 |
| `DiagnosticsScreenBoard.{hpp,cpp}` | 330 | 99 |
| `src/main.cpp` (lines 91, 92, 94, 212, 213, 215) | 6 | 6 |
| **Authored, this board** | **473** | **138** |
| Generic host (`U8G2GLScreenBoard.*`, `U8G2GLDisplay.*`), once | 387 | 86 |
| `generated/` (machine output, separate) | 370 | 140 |
| Legacy, for comparison | 292 | 99 |

For this one board the migrated version has more authored code, not less (138 against 99). The
reading paragraph is in `EVIDENCE.md` section 2.

### Dead Ends & Warnings

- The first render crashed (segfault). Cause: the state saver's null font, below. Found with an
  ASan/UBSan build of the harness in `tmp/`.
- The first `long_ssid` run differed (the 33-byte SSID, above).
- Four macro-redefinition warnings from the include order (above).
- `lldb` hung in this sandbox; ASan was used instead.
- PlatformIO was not used and could not be: no `pio`, `platformio` or `esptool` was run.

### Out-of-Scope Findings (reported, not fixed)

**Foundation repository**
1. **`test/render/CMakeLists.txt` was never committed by WO-B1.** The root `.gitignore:20` ignores
   every `CMakeLists.txt`, so the baseline harness could not be rebuilt from a clean clone. Fixed
   inside this Boundary Box (negation in `test/render/.gitignore`, file added in `94ebfee0`).
2. Legacy defects already on the ledger remain, and one is sharpened. Descenders: of the 11 pixels the
   clip removes from `connected`, 5 come from the exclusive corner and 6 would be lost even with an
   inclusive corner, because the `p`, `y` and `g` tips sit one row below the `getMaxCharHeight()` cell
   (`clip_probe.json`). Fixing the corner alone does not restore them.
3. Legacy `U8G2Display.cpp` `begin()` (`:137-148`) sets no font. Harmless to legacy boards, which
   guard a null font in `U8G2StateSaver`, but it is the reason a runtime widget can crash (R-1).

**U8G2GL runtime and generator (`69c37e0`, CLI 0.11.0)**
- **R-1. Null-font crash.** `include/u8g2gl/u8g2gl_state_saver.h:80-83`: the destructor calls
  `setFont(saved_font_)` with the font that was current at construction. On a U8G2 that has never had
  a font set that is `nullptr`, and `u8g2_SetFont` reads it (`u8g2_font.c:114`, via
  `u8g2_read_font_info`, `u8g2_font.c:134`). Reproduced: first frame, segfault, call chain
  `ui_screen_diagnostics_render` -> `u8g2gl_render_object` (`u8g2gl_core.cpp:150`) -> state saver. A
  plain `begin()`; `ui_handler()` user crashes on their first frame. The legacy `U8G2StateSaver`
  guards it with `if (_oldFont != nullptr)`.
- **R-2. `setFontPosTop()` is assumed and never set** (measured above). `docs/design_spec.md:2216`,
  `src/u8g2gl_label.cpp:334`, `src/u8g2gl_clock.cpp:221`. A plain user gets a near-blank screen.
  `setFontRefHeightAll()` matters too (one pixel).
- **R-3. The state saver leaves font mode changed, and says it cannot read it.**
  `u8g2gl_state_saver.h` file header ("no getter for either"); `src/u8g2gl_label.cpp:331` sets the mode
  every draw, default solid. U8G2 does expose it in the struct,
  `u8g2->font_decode.is_transparent` (`u8g2_font.c:378`), which this module reads. Measured: after the
  runtime's `render_fn` the mode is 0 where the host had 1.
- **R-4. Include-order fragility.** `u8g2gl_generated_config.h` must be seen before `u8g2gl_config.h` in
  every TU; a TU that includes a runtime header first gets four `-Wmacro-redefined` warnings and the
  runtime defaults first. Documented, but easy to get wrong. Hypothesis, **not tested**: the
  documented PlatformIO integration (`-include .../u8g2gl_generated_config.h` in `build_flags`) would
  force that header, which uses `static_assert`, into every C translation unit of the build.

### Blockers (need the PlatformIO MCP, or an owner decision)

1. **Part 2 is unrun**: the `esp32dev` and `esp32s3nano` builds with the pin resolved; `env:native`
   still building and passing at its baseline count; the hook inside a real build, configured and not.
   Specific things to look for: whether `lib_extra_dirs` plus `lib_ldf_mode = deep` finds `U8G2GLUI`
   and resolves its `u8g2gl` dependency (`*`) to the pinned one; whether `lib_compat_mode = strict`
   accepts `u8g2gl` (WO-A's consumer used strict and passed); whether the `crowpanel-128-rotary`
   `lib_ignore` is needed or sufficient; whether `ui.hpp` collides with another header in the include
   path; and the firmware's flash and RAM (sections 3 and 4).
2. **Capacities (owner/PA decision).** Applying the generated capacities to the library needs the
   generated config forced into the runtime's own translation unit (`u8g2gl_obj.cpp`). I did not add a
   forced `-include` to `platformio.ini`: it would reach every C and C++ unit of the build, and I
   could not test it. Without it the binding pool has the runtime default (`U8G2GL_MAX_TOTAL_BINDINGS`
   = 256 slots, 32 objects x 8 bindings, `u8g2gl_config.h:90`) instead of the 4 counted. Measure the
   RAM effect in Part 2 before deciding.
3. Not decided, owner's question already on the ledger: whether to fix the legacy defects (descenders,
   the invisible separator, the mid-glyph SSID clip) now that the migrated board reproduces them.
