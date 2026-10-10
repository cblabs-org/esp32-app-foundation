# feat(display): U8G2GLDisplay board host and the first migrated board (U8G2GL PR-05)

> **Draft for the foundation PR**, written by WO-B3 for the Principal Architect to review. It opens
> from the tip of the `u8g2gl-integration/wo<N>-*` stack against `main`. Not to be confused with this
> repository's own 2026 `pr-05-display-module`: the `u8g2gl-` prefix names the U8G2GL programme.

## Freeze notice: `modules/U8G2Display` is frozen from this merge (PD-4)

Per U8G2GL `docs/implementation_plan.md` §2, PD-4 (strangler-fig), the legacy display module stops
being maintained the day the first board is migrated, and this PR migrates it. **From the merge of
this PR:**

- **`modules/U8G2Display` is frozen.** Correctness and security fixes only.
- **No new features, no new boards, no new widgets** in `modules/U8G2Display`.
- **New work targets `modules/U8G2GLDisplay/`.**
- **A bug fixed in both modules** during the frozen window must reference the same issue in both
  commits, so that divergence is visible in history and not discovered later.
- **Enforced at review:** a PR that adds a feature to the legacy module is rejected and redirected.
- **Nothing is deleted here.** `modules/U8G2Display` still compiles and still runs every other board.
  PR-10 migrates the remaining boards and retires the module, on the schedule in PD-4's table
  (frozen now, deprecated when PR-10 lands, removed when departures-board-v3 advances its
  `framework.lock` past PR-10).

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

**It does the same, defects included.** In the four host scenarios the frame is pixel-identical to the
legacy board's, **bug for bug**, because the gate asks for identity:

- **The clipped descenders.** The legacy label clip keeps one row and one column too few (the far
  corner of U8G2's clip window is exclusive), so "My" renders as "Mv" and the "p" in "Up" and "Heap"
  loses its tail. The XML reproduces this with a label height of 16 and 10 and a data-row width of
  247, each one smaller than the legacy arguments, and says so in the file.
- **The invisible "·".** The legacy fonts are `_tr` and have no glyph `0xB7`, so the separator draws
  nothing and takes no width: the `connected` row shows `MyNetwork`, an 8-column gap, then the IP.
  The migrated board keeps the byte and gets the same gap.
- **The mid-glyph SSID clip.** A long SSID is cut at column 250 with no ellipsis.

**Also identical:** the 1-second refresh cadence (`REFRESH_INTERVAL_MS`, first refresh when
`millis()` reaches 1000, as in legacy), the three text formats byte for byte, and the board name
(`"APP: Diagnostics"`), so the carousel and the web Screenshot tab (R7: no new web diagnostics) see the
same board.

**Not verified on a device yet.** Everything above is host-rendered and builds green; the on-device
capture comparison is WO-B5 and waits for the bench. The on-device frame has not been compared with
the host frame.

**The defects get fixed, in both boards, after the identity check.** The owner has decided the legacy
defects are to be fixed. The fix lands in both the legacy board and the migrated one, as a
correctness fix under the freeze, so the two stay pixel-identical and the gate comparison keeps its
meaning. It is tracked in the PR's workspace.

## Tests

- **Native:** **989 to 1018** (989 existing + 29 new in `test/test_unit/test_u8g2glDisplay/`: the
  board host, the formatter, the screen board and the display). 1018 of 1018 pass, re-run through the
  PlatformIO MCP from the committed tree. The module's tests compile the runtime through a test-local
  unity file, because the runtime's `frameworks: arduino` excludes it from a native environment.
- **Host pixel harness:** `test/render/` renders the legacy and the migrated board against real U8G2
  and compares byte for byte (`cmake -S test/render -B tmp/render-build && cmake --build
  tmp/render-build --target after`). Builds were byte-identical across three clean builds. The
  committed `baseline/` and `after/` files are its output.
- **Builds:** `esp32dev` and `esp32s3nano` build clean through the MCP. `crowpanel-128-rotary`
  gained a `lib_ignore` that was **not built** (the owner directed that it not be).
- **Not run:** anything on hardware.

## Before merge

1. **The U8G2GL PR merges first.** This PR depends on the runtime and the generator that PR lands.
2. **Re-pin `u8g2gl` to the U8G2GL merge commit on `main`**, in both places:
   - `platformio.ini`, `[u8g2gl]` (`lib_deps`);
   - `test/render/CMakeLists.txt`, `U8G2GL_PINNED_SHA`.

   They currently pin `82ec83f2bbce02080c3dd1ab6618bcf6559bfde5`, on the U8G2GL branch
   `u8g2gl-05/wo1-packaging`. After re-pinning, re-run the host harness (`--target after`): the 13
   `after/` files must still match byte for byte, and `esp32dev` and `esp32s3nano` must still build.
3. **Check the flash headroom once more on the re-pinned build** (the figures above are for the
   current pin).

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
