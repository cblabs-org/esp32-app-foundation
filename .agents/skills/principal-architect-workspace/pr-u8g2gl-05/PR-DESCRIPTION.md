# feat(display): U8G2GLDisplay board host and the first migrated board (U8G2GL PR-05)

> **Draft for the foundation PR**, written by WO-B3 for the Principal Architect to review. It opens
> from the tip of the `u8g2gl-integration/wo<N>-*` stack against `main`. Not to be confused with this
> repository's own 2026 `pr-05-display-module`: the `u8g2gl-` prefix names the U8G2GL programme.

## Held off `main` until a planned cut-over (owner ruling R10)

This stack is **not to be merged yet**. U8G2GL keeps developing and merging in its own
repository. Foundation `main` stays on the legacy display module until a planned cut-over, once
the renderer is more complete. The PR stays a draft as the verified record of the first migrated
board, and as PR-07's evidence.

## Freeze notice: `modules/U8G2Display` is frozen from the cut-over (PD-4)

Per U8G2GL `docs/implementation_plan.md` §2, PD-4 (strangler-fig), the legacy display module stops
being maintained when the first migrated board lands on `main`. Under R10 that is the cut-over,
not this draft. **From the cut-over merge:**

- **`modules/U8G2Display` is frozen.** Correctness and security fixes only.
- **No new features, no new boards, no new widgets** in `modules/U8G2Display`.
- **New work targets `modules/U8G2GLDisplay/`.**
- **A bug fixed in both modules** during the frozen window must reference the same issue in both
  commits.
- **Enforced at review:** a PR adding a feature to the legacy module is rejected and redirected.
- **Nothing is deleted here.** PR-10 retires the module, on PD-4's schedule.

Until the cut-over, `modules/U8G2Display` on `main` is active as normal. The correctness fixes
below (descenders, separator, ellipsis, default label height) live on this branch and reach
`main` with the cut-over.

## What this PR does

It adds the integration layer between this repository and the **U8G2GL** declarative-UI runtime
(MIT, `cblabs-org/u8g2gl`), and moves one real board onto it: the root firmware's `DiagnosticsBoard`.

### 1. `modules/U8G2GLDisplay/`: a board host inside the legacy carousel (R1)

`U8G2GLDisplay` provides an `IDisplayBoard` (`U8G2GLScreenBoard`) that runs a generated U8G2GL screen
**inside the legacy carousel**. It calls the screen's `tick_fn` and `render_fn` directly and never
calls the runtime's `ui_handler()`.

**Why it does not own the display yet (ruling R1).** `ui_handler()` clears and sends the buffer
itself. The legacy module already does both (`U8G2Display.cpp:1413`), and the carousel, the
screensaver and the other boards all live there until PR-10. A second owner of the buffer would draw
over the first. So in this PR the legacy module stays the display owner and `U8G2GLDisplay` is a
guest in it. A guest must leave the display as it found it, so the host does three jobs, each found
and measured on the host harness and recorded in the code:

- it inherits the legacy module's `setFontPosTop()` and `setFontRefHeightAll()` (the runtime assumes
  the first and never sets it);
- it saves and restores U8G2's font mode, which the runtime's labels leave solid (without this, every
  legacy board drawn after it would render in the wrong mode);
- it makes sure a font is current before the runtime's state saver runs (otherwise a null font is
  restored, and U8G2 dereferences it).

`DiagnosticsScreenBoard` is the board itself: the platform is read by `readDiagnosticsInputs()`, and
the text is made by a pure `formatDiagnostics()`, so the host harness and the native tests can drive
it with fixed inputs.

### 2. The generated UI and its pre-build hook

- **`uidesign/ssd1322-256x64/u8g2gl/`**: the XML project (`u8g2gl.xml`, `globals.xml`,
  `screens/screen_diagnostics.xml`) and its **committed `generated/`**, consumed through
  `lib_extra_dirs`. It is named for the panel because `esp32dev` and `esp32s3nano` share it.
- **`scripts/u8g2gl_generate.py`**, a PlatformIO pre-build script (U8G2GL [D-11]):
  - **configured** (`U8G2GL_CLI` in the environment, or `custom_u8g2gl_cli` in `platformio.ini`): it
    runs `u8g2gl generate` from a local CLI checkout on every build, and a broken XML file fails the
    build with the generator's diagnostic;
  - **not configured** (the default, and what anyone without the CLI gets): it builds from the
    committed `generated/`. Nobody needs Node to build this firmware.

### 3. The pinned runtime and the scoped capacities

- **The pin.** The runtime is an external PlatformIO dependency pinned **by git commit**, in one place:
  the `[u8g2gl]` section of `platformio.ini` (and `U8G2GL_PINNED_SHA` for the host harness). Nothing
  is published. PlatformIO installs git dependencies only for the project's own `lib_deps`, which is
  why the pin lives in `platformio.ini` and not in the module's `library.json`.
- **The scoped capacities.** The generated `u8g2gl_generated_config.h` sizes the runtime's pools for
  this project. Its documented form, a project-wide `-include`, **fails to build here** (it reaches the
  Arduino `FS` library and `PsychicHttp`, which cannot resolve `u8g2gl/`). The hook script therefore
  adds the include to exactly the runtime's own 15 sources with a build middleware. Effect on
  `esp32dev`: `s_binding_pool` is 96 B instead of 6144 B, and RAM is 57340 B instead of 63388 B.

### 4. `THIRD-PARTY-NOTICES.md`

One row for U8G2GL (MIT), and a line noting that the C++ `u8g2gl generate` writes is MIT too, so the
files under `generated/` carry no Apache-2.0 header. The XML they come from is this project's own and
is Apache-2.0.

### 5. Who is switched over, and who is not

| | Board class |
|---|---|
| Root firmware (`src/main.cpp`) | **`DiagnosticsScreenBoard`** (U8G2GL), 6 lines changed |
| `sample-app` | `DiagnosticsBoard` (legacy), unchanged |
| `template-app` | `DiagnosticsBoard` (legacy), unchanged |
| departures-board-v3 | legacy, unchanged; it stays on its pinned `framework.lock` |

## Evidence for the gate

These are the four PR-07 criteria (U8G2GL `docs/implementation_plan.md` §6). The numbers are measured
the same way on both sides and live in
[`EVIDENCE.md`](.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md), which
quotes each from its log or tool output. This PR does not judge the gate; PR-07 does. The tables
below are signals.

### 1. Pixel-identical, by automated diff: [EVIDENCE §1](.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md)

Host harness (`test/render/`, CMake, no PlatformIO), real U8G2 2.36.18 sources, the firmware's flags,
the legacy module's display state. Legacy and migrated frames compared byte for byte.

| Scenario | Identical | Differing pixels | Lit pixels |
|---|---|---:|---:|
| `connected` | yes | 0 | 1392 |
| `ap_mode` | yes | 0 | 1465 |
| `disconnected` | yes | 0 | 1318 |
| `long_ssid` | yes | 0 | 1695 |

### 2. XML materially shorter and clearer: [EVIDENCE §2](.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md)

| (code lines, `test/render/count_lines.py`) | Legacy | Migrated |
|---|---:|---:|
| The layout | in C++ | XML: 33 |
| Board C++ (both files) | 95 | 99 |
| `src/main.cpp` lines for the board | 4 | 6 |
| **Authored, this board** | **99** | **138** |
| Generic host, paid once for every later board | n/a | 86 |
| `generated/` (machine output, not authored) | n/a | 140 |

For this one board the migrated version has **more** authored code, not less. EVIDENCE §2 gives the
two main reasons (the hardware-read / pure-format split that the harness needs, and the subject
declarations the XML adds) and a reading. It is stated here so nobody discovers it later.

### 3. Flash and RAM comfortable on `esp32dev`: [EVIDENCE §3](.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md)

Clean builds through the PlatformIO MCP, same toolchain on both sides.

| `esp32dev` | Legacy | Migrated | Delta |
|---|---:|---:|---:|
| RAM | 57500 B (17.5%) | 57340 B (17.5%) | **-160 B** |
| Flash | 1336143 B (84.9%) | 1342247 B (85.3%) | **+6104 B** |
| App partition left | 236721 B (15.05%) | **230617 B (14.66%)** | -6104 B |

`esp32s3nano`: RAM 60124 to 59964 B (-160), flash 1370475 to 1376723 B (+6248; 32.7% to 32.8% of a
4 MB partition).

Where the +6104 B of flash comes from (`nm -S`, both ELFs): the runtime's label, clip, binding and
subject code +3178 B; U8g2 shape code that only the new image links +1354 B; the generated UI +865 B;
the board and host module +709 B; less 657 B for the legacy `DiagnosticsBoard` that this removes; 622 B
is in no named symbol. The runtime and the U8g2 code it links are 74% of what was added; a second
board using the same widgets would not pay them again. **Noise floor:** tens of bytes between builds
of unchanged source, but a 1.8 KB variation (ESP-IDF diagnostics code the linker keeps in some builds
and not others) appeared in this tree, so the clean-build figures above are the ones to quote.
`esp32dev` flash was already 84.9% full before this PR.

### 4. Edit, regenerate, rebuild faster than editing C++: [EVIDENCE §4](.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md)

Same task on both sides: move row 3 by one pixel. The same flags were used throughout (WO-B3b).

- **Interleaved:** five rounds of four builds. Each round was a legacy edit, a migrated edit, a
  legacy no-change build and a migrated no-change build.
- **Load:** recorded before every build, with load averages of 4.0 to 8.5.
- **Figures:** medians of the build task's own start-to-end time.

| Build | Legacy | Migrated (hook regenerates) |
|---|---:|---:|
| Edit, rebuild | 26.92 s | **31.90 s** |
| No-change rebuild | 21.60 s | 27.92 s |
| Compile work for the edit itself (edit − no-change) | 4.65 s | **3.90 s** |

- **The gap is real.** Paired within each round, the migrated loop is **+4.99 s** slower (median),
  and the difference survives the pairing.
- **The XML workflow does not cause it.** The edit itself costs slightly *less* than the C++ edit,
  and the generator runs in 0.05 to 0.07 s. A run with the hook switched off still shows the gap.
- **The whole gap is fixed per-build overhead (+6.16 s on a no-change build).** The verbose logs
  show PlatformIO's library dependency graph growing by 42.6% under `lib_ldf_mode = deep`, from the
  three new libraries (`U8G2GLDisplay`, `U8G2GLUI`, `u8g2gl`). That attribution is inferred from the
  logs; timing the scan on its own would need the `pio` CLI, which rule 04 forbids.
- **What the timing cannot capture:** a geometry change is a constructor argument in C++ and one
  attribute in XML, and the generator re-checks the layout against the display. EVIDENCE §4 says so,
  and leaves the judgement to PR-07.
- **Superseded figures:** WO-B1's (29.94 s) and WO-B3's (48.81 s) timings remain in EVIDENCE as the
  record. WO-B3's were taken hours apart, under much heavier load.

## What the migrated board does and does not do, compared with legacy

**First, bug for bug.** The four host scenarios, and the device running `v0.0.12-dev.91`, show the
migrated board pixel-identical to the legacy one, **defects included**: clipped descenders, an
invisible "·", and a long SSID cut mid-glyph. On the device, the frame and the host render of
the same live values differ in **0 pixels for both renderers**.

**Then fixed, in both boards (owner ruling R8)**, so they stay identical. These are correctness
fixes to the legacy module under PD-4:

- **Descenders.** The legacy `drawText` clip lost the box's last row and column, because U8G2's
  clip far corner is exclusive. That is fixed. The diagnostics labels get
  `getMaxCharHeight() + 1` rows (heights 18/14/12/12), and so does `drawText`'s default height,
  which helps the other legacy boards.
- **Separator.** Row 2 uses `u8g2_font_helvR08_tf`, with a UTF-8 `·`.
- **Long text.** An ellipsis on rows 2 to 4 (legacy `setTruncated(true)`, U8G2GL
  `long_mode="dots"`). A latent legacy bug went with it: truncation could cut a UTF-8 character
  in half.

**Identity after the fixes:**
- **Host:** 0/5 scenarios differ, and 181/181 truncation lengths are identical.
- **Device (`dev.92`):** the hostname and WiFi rows equal the host render in 5 of 6 captures. The
  sixth was a torn capture: the screenshot reads the buffer mid-redraw, and the panel itself does
  not tear.
- **Owner's check on the panel:** correct, including the screensaver.

**Other legacy boards, through the shared `drawText` fixes (`dev.93`):**
- **Rendered:** 9 frames. Every change is pixels added in the formerly clipped descender row.
- **No overlaps.** Two near-misses (pixels touching, none shared) are left as layout findings:
  `ConfiguringStatusBoard`'s header and the `BootBoard` splash title.
- **Owner's check on the boot screen:** correct.

**Also identical:** the 1-second refresh cadence, the text formats, and the board name.

**Not addressed:** with a 32-character hostname *and* a truncated WiFi line, the two boards differ
in 4 pixels. Row 2's text box blanks the hostname's last two rows. That is a layout question.

## Tests

- **Native:** **989 to 1018** (29 new in `test/test_unit/test_u8g2glDisplay/`). 1018/1018 pass
  through the PlatformIO MCP.
- **Host pixel harness:** `test/render/`. `render_harness` renders the legacy and migrated
  diagnostics boards, in 5 scenarios including the device's live values, and compares byte for
  byte. `render_legacy_boards` renders the other legacy boards before and after the default-height
  fix. `compare_png.py` compares a harness frame with a device capture.
- **Builds (final):**
  - `esp32dev`: RAM 57340 B, flash 1344575 B (85.5%);
  - `esp32s3nano`: RAM 59964 B, flash 1378979 B.

  The defect fixes cost +2.3 KB of flash, mostly the `_tf` font. `crowpanel-128-rotary` was not
  built (owner direction).
- **Hardware:** an ESP32-S3 Nano with an SSD1322, flashed with `dev.91`, `dev.92` and `dev.93`.
  All booted cleanly, and the owner confirmed the panel each time.

## Before the cut-over merge

1. **Merge foundation `main` into this stack.** It is held off `main` and will have drifted.
2. **Re-pin `u8g2gl` to the then-current U8G2GL `main`,** in both places:
   - `platformio.ini`, `[u8g2gl]`;
   - `test/render/CMakeLists.txt`, `U8G2GL_PINNED_SHA`.

   The current pin is `82ec83f`, reachable from U8G2GL `main` since #11 merged as `3bc2a08`.
3. **Re-verify everything:**
   - the host harness (0/5 identity);
   - native;
   - `esp32dev` and `esp32s3nano` builds, and the flash headroom;
   - a device flash and a panel check.

## Known findings, carried to U8G2GL

Found while integrating. None is fixed in this PR (this PR changes nothing in the runtime or the
generator); each is reported to the U8G2GL Chief Architect in the twin project's
[findings](https://github.com/cblabs-org/u8g2gl/blob/u8g2gl-05/wo1-packaging/.agents/skills/principal-architect-workspace/pr-05-integration/PROJECT.md),
section "Findings for the Chief Architect", which is the authoritative text:

1. The runtime never establishes `setFontPosTop()`, but every text widget assumes it (a plain
   `begin()` + `ui_handler()` user gets 15 lit pixels instead of 1392).
2. The state saver restores a null font, and U8G2 dereferences it (a segfault on the first frame).
3. The state saver does not restore the font mode.
4. A screen file not named `screen_*.xml` is silently ignored; `validate` and `generate` exit 0.
5. The generated config's PlatformIO guidance (`-include`) fails in a multi-library project.
6. `library.json` `frameworks: "arduino"` excludes the runtime from native PlatformIO builds.

From the evidence work, for whoever owns the tooling:

7. The MCP `size_report` relinks the ELF before comparing, so `expectedElfSha256` must be the sha of
   the relinked file (the built file's sha fails with `ANALYSIS_ELF_MISMATCH`).
8. `esp32dev` flash for unchanged source varied by 1.8 KB between builds (ESP-IDF diagnostics code
   kept or dropped by the linker); the mechanism is not established.

Legacy defects now in a frozen module, candidates for correctness fixes under the freeze: the `0xB7`
separator that never renders, the mid-glyph SSID clip, the descender clipping (an owner question),
`Fonts.hpp` including `<U8g2lib.h>` inside `namespace AppFonts`, and the wrong-case file name in the
`DrawingPrimitives.cpp` header.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
