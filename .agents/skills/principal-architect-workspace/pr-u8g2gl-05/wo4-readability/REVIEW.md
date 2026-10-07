---
work_order: WO-B4
project: pr-u8g2gl-05
title: "House-style review of the foundation PR (report only)"
reviewed: HEAD a9ab6fae (committed state; see "Reviewed state")
base: e5342373
status: review complete
---

# REVIEW: WO-B4, house-style pass over the foundation PR

## Authorization and method

- **Authorization verified from committed artifacts.** `../PROJECT.md` "Human Approval" quotes the
  owner's 2026-10-06 reply approving the decomposition ("foundation WO-B1..B4"), and the ledger
  lists `wo4-readability`. The WORKORDER names `pr-u8g2gl-05`.
- **Read-only.** The only file written is this one. Git was used read-only (`diff`, `show`,
  `grep`, `status`); no PlatformIO operation; no repository file was executed in place
  (`count_lines.py` was loaded with `exec` and `python3 -B` so no `__pycache__` appeared).
- **Standards used:** `house-style-docs/SKILL.md` and `references/cpp-embedded.md` (cited below as
  *HS §n*: §1 header, §2 function docs, §3 scoped variables, §4 internal flow, §5 arcane logic,
  §6 naming), `CONTRIBUTING.md`, and the precedent in `modules/U8G2Display/`,
  `test/test_unit/test_u8g2Display/`, and foundation's `scripts/*.py`. `python3
  scripts/doc_lint.py` was run: **exit 0, no output** (clean).
- **Scope:** all 34 files in the WORKORDER's `git diff --name-only` list were read in full.
- **Reviewed state.** HEAD (committed). The working tree is dirty from the other worker: it
  modifies `platformio.ini`, `screens/screen_diagnostics.xml` and
  `generated/ui_screen_diagnostics.cpp`. See Observation O1; none of those edits are reviewed.

## Summary

The authored code is in good shape. Every file has the header, SPDX, Doxygen on its public API,
and step comments in the long routines, and the naming follows HS §6 throughout (including the
`U8G2GL` acronym handling and the directory/file/class alignment). The defects are almost all in
**accuracy**: comments written early in the programme that later work orders made false. Every
numeric citation I could check holds, and the ones that do not are listed under Must fix.

**Citations spot-checked and found correct** (so the PA need not redo them):
`U8G2Display.cpp:1413-1415` (clear / render / send), `:143-146` (the four state calls), `:137`
(begin), `:147/148`; `u8g2.h:352/354` (`clip_x1` / `clip_y1` "(excluded)") in the pinned
`extern/u8g2`; `u8g2_font.c:114` (the `u8x8_pgm_read(font)` dereference); `u8g2gl_core.cpp:71-86`
(`u8g2gl_set_active_screen`); `WifiManager.hpp:73, 135, 137, 363, 389, 443`;
`hardware/esp32dev/board.h:93`; root `.gitignore` lines 9 and 20; `Arduino.h:121` stub millis
returns 0; native stubs 192.168.1.100 and 200000; `state_probe.json` values; `clip_probe.json`
"11 removed, 6 if inclusive, so 5 are the corner's doing"; `getMaxCharHeight` 17 and 11 (read from
the font headers, bytes 10); the 8-column gap (measured in `connected.pbm`: a dark run of exactly
8); 256x8 = 2048; "4 bindings against a default of 256" (4 x 1 against 32 x 8); the
"every row sits 1 pixel higher" claim (measured: shifting the probe frame by one row reproduces
the normal frame, to within 0 to 3 pixels per row band).

## Must fix (6)

- [ ] test/render/CMakeLists.txt:119-122 — HS (accuracy; wrong comment) — The comment says the
  runtime is compiled with "the generated config forced in NOT ... so the binding pool has the
  runtime's default capacity, **as it will in the firmware**". That is now false: commit
  `90644596` made `scripts/u8g2gl_generate.py` force the capacities into the 15 runtime sources,
  so the firmware's pool is 4 bindings (96 B), not 256 (6144 B). The harness still builds with the
  defaults, which is harmless to pixels, but it is **not** "as in the firmware". — Replace with:
  "Compiled with the runtime's default capacities. The firmware build does force the generated
  capacities into these sources (scripts/u8g2gl_generate.py), so the binding pool differs; the
  rendered pixels do not depend on it."
- [ ] test/render/main.cpp:496-497 — HS (accuracy) — The string written to the committed
  `after/diagnostics_board/inputs.json` field `runtime_capacities` says "runtime defaults: the
  generated config is not force-included, as platformio.ini does not force it". Literally true of
  `platformio.ini`, but it reads as a statement about the firmware, which is wrong for the same
  reason as the item above, and it is shipped as evidence. — Reword to "harness build: runtime
  default capacities (the firmware build applies the generated capacities via
  scripts/u8g2gl_generate.py; pixels are unaffected)". Then regenerate `after/` (the `after`
  target) so `inputs.json` matches; that is the only change to the render files.
- [ ] test/render/CMakeLists.txt:60 — HS (accuracy; wrong citation) — "pinned by git commit
  exactly as `modules/U8G2GLDisplay/library.json` pins it (R6)". `library.json` does not pin: it
  names `u8g2gl` at version `*`. The pin lives in `platformio.ini` `[u8g2gl]` (moved there in WO-B2
  Part 2). — Replace with "...kept equal to the pin in platformio.ini [u8g2gl] (R6); re-pin both
  before merge".
- [ ] platformio.ini:59 — HS (accuracy) — "the one place its version is pinned" is false:
  `test/render/CMakeLists.txt:61` (`U8G2GL_PINNED_SHA`) is a second, independent pin, and
  `PROJECT.md` already lists both as "to re-pin before merge". Two pins that must agree is a
  hazard exactly because this comment says there is one. — Change to "the one place the firmware
  build pins it; the host render harness pins the same sha in test/render/CMakeLists.txt
  (U8G2GL_PINNED_SHA), and the two must be kept equal". Add the mirror remark at
  `CMakeLists.txt:60`.
- [ ] test/render/shim/Print.h:27-28 — HS (accuracy; wrong claim about another file) — "No virtual
  destructor, matching the Arduino core". The ESP32 core the firmware builds against declares
  one: `~/.platformio/packages/framework-arduinoespressif32/cores/esp32/Print.h:49`
  (`virtual ~Print() {}`). (The AVR core has none, which is probably the source of the belief.)
  — Say what is true: "No virtual destructor: the harness never deletes a U8G2 through a Print
  pointer. (The ESP32 core declares one; it is omitted here as unnecessary.)", or simply add
  `virtual ~Print() = default;` and drop the sentence.
- [ ] modules/U8G2GLDisplay/U8G2GLScreenBoard.cpp:50-52 — HS §5 (accuracy) — "the labels' clip
  removes all but 15 of the 1392 lit pixels". `state_probe.json` shows
  `no_font_pos_top`: `lit_pixels 15, rendered_only 15, normal_only 1392, differing 1407`. The
  normal frame has 1392 lit pixels and **all 1392** are missing; the 15 that are lit are different
  pixels. None of the 1392 survive. — Reword: "the frame has 15 lit pixels and none is where
  the 1392 normal ones are".

## Should fix (13)

- [ ] modules/U8G2GLDisplay/U8G2GLDisplay.cpp:34-37 and :57-59 — HS §5 (accuracy of rationale) —
  "The runtime only uses it for animation deadlines" and "so nothing the generated code does can
  ask the runtime for the time". `u8g2gl_tick_now()` is called only from `ui_handler()` and
  `ui_handler_run_in_period()` (`src/u8g2gl_core.cpp:243, 275, 295`), and R1 says this module never
  calls `ui_handler()`; the board passes `currentMillis` straight to `tick_fn`. So under the board
  host the registered callback is unused. Registering it is still reasonable (a screen or user
  code that does call `ui_handler()` needs it), but the comment implies it matters now. — Say so:
  "Registered for completeness: only ui_handler() reads it, and this module's boards do not call
  that (R1), but anything that does would otherwise see time stuck at 0."
- [ ] modules/U8G2GLDisplay/boards/DiagnosticsScreenBoard.hpp:29-46 — HS §1 (the exported list "must
  summarise EVERY externalized ... variable") — The header lists types, functions and the class
  but omits the exported constants `DIAGNOSTICS_NAME_CAPACITY` (line 67) and
  `DIAGNOSTICS_IP_CAPACITY` (line 70). — Add "Exported Constants:" with a one-line description of
  each.
- [ ] modules/U8G2GLDisplay/U8G2GLDisplay.hpp:12, U8G2GLScreenBoard.hpp:10-11,
  U8G2GLScreenBoard.cpp:43, platformio.ini:59, uidesign/ssd1322-256x64/u8g2gl/screens/screen_diagnostics.xml:14,19
  — HS §1/§5 (comments must be understandable by a reader of the repository) — "PR-05 ruling
  R1/R6" and "EVIDENCE.md section 1" are cited with no path. "PR-05" is also ambiguous in this
  repo (foundation has its own `pr-05-display-module` from 2026). The rulings live in
  `.agents/skills/principal-architect-workspace/pr-u8g2gl-05/PROJECT.md` and the evidence in
  `.../pr-u8g2gl-05/evidence/EVIDENCE.md`, both part of this PR. — Cite them by path, or state the
  rule in the comment (R1 is already restated well in `U8G2GLScreenBoard.hpp`; R6 is not).
- [ ] test/render/main.cpp:480-560 (`writeInputsJson`, about 80 lines) — HS §4 — The one long
  routine in the file with no step breakdown (its siblings `writeSeparatorProbe`,
  `writeClipProbe`, `main` all have them). — Add "--- Step 1: target and toolchain facts ---",
  "--- Step 2: fixed descriptions (state calls, formats, re-run command) ---", "--- Step 3: one
  block per scenario ---".
- [ ] test/render/main.cpp:531 and :551-552 — HS §5 (unexplained magic numbers) —
  `measureBand(r.buffer, 16, 30)` and the literals `"clip_x_range": [4, 251]`, `"band_rows": [16,
  29]` encode "row 2 occupies y 16..29, its clip is x 4..251" with no name. The comment at 547-550
  explains the 251 only. — Name them (`ROW2_TOP`, `ROW3_TOP` = 30, `ROW2_CLIP_X0/X1`) next to
  `DISPLAY_WIDTH`, and use them in both the call and the JSON.
- [ ] test/render/README.md:39-45, :83-88, :140-143 — HS §1 (documentation covers the exported
  surface) — `--probe-clip` and its output `clip_probe.json` are never mentioned in the README. The
  `after` target runs it (`CMakeLists.txt:207-208`) and the XML cites its output
  (`screen_diagnostics.xml:35`). — Add `clip_probe.json` to the "after/" table and `--probe-clip`
  to the direct-run examples and the sentence at line 39-41.
- [ ] test/render/README.md:129-130 — accuracy — "The target writes to `baseline/<output_subdir>/`,
  so a migrated board renders beside the legacy one". Output goes under whatever `--out` names;
  the `u8g2gl` target writes to `after/diagnostics_board/`. — "The target writes to
  `<out>/<output_subdir>/`; the legacy target's `--out` is `baseline/`, the migrated one's is
  `after/`, and the two sets of `.bin` files compare directly."
- [ ] test/render/count_lines.py:114-115 — HS §5 (arcane logic, no comment) — The raw-string
  prefix test is dense and partly dead: `line[max(0, i - 2):i]` is a two-character slice, so the
  alternatives `"L"`, `"u"`, `"U"` can never match it (except at `i == 1`). Only `u8` works. —
  Either add a comment saying what prefixes are handled, or fix it to test the one- and
  two-character prefixes separately, and add a self-test case (`LR"(// x)"`).
- [ ] test/render/count_lines.py:14, 165-166 — accuracy — Header: "A blank line inside a block
  comment counts as blank". The code does so only for a truly empty line; a whitespace-only line
  inside a block comment is labelled `comment` (verified: `"/* a\n   \n b */ int z;"` gives
  `[comment, comment, code]`). It does not change any figure in EVIDENCE §2 (checked on every file
  that was measured), but the comment is wrong as written. — Treat `line.strip() == ""` inside a
  block (and an XML comment) as blank, or say "empty".
- [ ] test/render/README.md:60 and :145-155 — HS (clarity) — Line 60 is a 110-character line
  in a hard-wrapped paragraph (every other line wraps near 80); the `count_lines.py` section's example cites
  `src/main.cpp:91,93,211,213`, line numbers of the **pre-migration** `main.cpp` (the include,
  the declaration, `configure()` and `carousel.add()`), which now point at other code. —
  Re-wrap line 60 and update (or drop) the example line numbers.
- [ ] test/test_unit/test_u8g2glDisplay/test_screen_board.cpp:29-37 and :42-52 — HS §3 — The
  `Counters` struct has a brief but none of its six members is documented, and `activateA`,
  `deactivateA`, `tickA`, `activateB`, `deactivateB` have no comment (only `renderA` does).
  `Scenario` in `test/render/main.cpp` documents every field, so this is below the PR's own
  standard. — One `///<` per member and a one-line comment on each fake.
- [ ] test/render/main.cpp:1096-1125 — accuracy of behaviour vs. the header (`--compare`) —
  `if (outRoot.empty()) return identicalToReference ? 0 : 3;` returns before the
  `g_selfCheckFailures` test at line 1125, so a `--compare` run without `--out` can report success
  while the `formatDiagnostics()` cross-check failed. The `after` target always passes `--out`, so
  the gate is not affected today. — Move the self-check test before the early return.
- [ ] uidesign/ssd1322-256x64/u8g2gl/screens/screen_diagnostics.xml:12-13 — clarity (the file
  is a shipped authored file) — "the gate asks for identity; whether to fix the defects is the
  owner's call" refers to the programme gate and an owner decision that a future reader of this
  XML cannot find. — State the consequence: "These widths and heights reproduce the legacy clip
  bug-for-bug on purpose (PR-05 requires pixel identity). Fixing it later means growing each label
  by one pixel in each direction" and keep the process language in the PR description.

## Nit (12)

- [ ] modules/U8G2GLDisplay/boards/DiagnosticsScreenBoard.hpp:63 — The 32-character limit is cited
  as `WifiManager.hpp:137` (`_currentHostname[33]`); the SSID buffer is `:135`. The sentence
  covers both names, so cite `:135, :137`.
- [ ] modules/U8G2GLDisplay/U8G2GLScreenBoard.hpp:33 (and the `render()` Doxygen at 73-77) —
  "leaving its state as found" is not quite true: when no font is set, `render()` sets
  `AppFonts::defaultSmall()` and does not undo it (`U8G2GLScreenBoard.cpp:84-86`). Harmless and
  documented in the .cpp; say "restoring the font mode (a font is set if none was)".
- [ ] modules/U8G2GLDisplay/U8G2GLScreenBoard.hpp:26-39 — The protected `screen()` accessor
  (line 118) is not in the exported list. It is documented, but nothing in the PR calls it; drop
  it or list it.
- [ ] uidesign/.../screen_diagnostics.xml:21-24, U8G2GLScreenBoard.cpp:57, test/render/main.cpp:885
  — Line-number citations into U8G2 (`u8g2.h:352/354`, `u8g2_font.c:114`) are right for 2.36.18,
  but `platformio.ini` asks for `^2.36.18`, which may resolve later. Append "(U8G2 2.36.18)" once
  per file so the lines do not silently rot.
- [ ] test/render/main.cpp:56-57 — "Access specifiers do not change object layout" is true on GCC
  and Clang but is not guaranteed by the standard before C++23 (and `#define private public` is
  undefined behaviour). The comment is otherwise a good why-comment; add "in practice (GCC/Clang)".
- [ ] test/render/main.cpp:771-775, 791, 807 — `RowPlacement::font` is a string compared by
  `place.font[0] == 'l'`. A `bool large` (or enum) states the intent; the struct's four fields
  also lack comments.
- [ ] test/render/main.cpp:1031 — The usage text shows `--out <dir>` as required; it is optional
  (line 1096).
- [ ] test/render/count_lines.py:23-26 — The exported list names three functions; the module has
  seven non-underscore ones (`classify_c_family`, `classify_xml`, `parse_line_spec`, `self_test`
  missing). Also "cloc is not installed here" (line 12) is a statement about one machine.
- [ ] scripts/u8g2gl_generate.py:75-80 — `resolve_cli`'s docstring has an `Args:` section followed by
  loose prose and no `Returns:`, unlike the other functions in the file. `_apply_capacities`
  (line 202) has no docstring; its explanation is a comment in the body. Foundation's scripts use
  prose docstrings, so this is a consistency nit, not a deviation.
- [ ] scripts/u8g2gl_generate.py:183 — `UI_LIB_DIR in (lib_extra_dirs or "")` is a substring test
  and would also match `uidesign/ssd1322-256x64-other`. Compare whole entries after
  `split(",")` and `strip()`.
- [ ] src/main.cpp:212 — `U8G2GLDisplay::begin();` is the one new line in `setup()` whose
  position matters (after `displayModule.begin()`, before `configure()`), and it has no
  comment, unlike the Doxygen on `begin()` that states the rule. One line saves a future reorder.
- [ ] test/test_unit/test_u8g2glDisplay/u8g2gl_native_stubs.c:1-18 — Header lacks the
  repository URL line the other files have (the legacy stub it includes lacks it too, so this
  matches precedent) and an "Exported Functions" list of the 14 stubs. Optional.

## Files with no findings

- `THIRD-PARTY-NOTICES.md` (new row and paragraph are accurate; section 22 of the U8G2GL spec
  does say MIT for the generated code)
- `modules/U8G2GLDisplay/U8G2GLDisplay.hpp`
- `modules/U8G2GLDisplay/boards/DiagnosticsScreenBoard.cpp`
- `modules/U8G2GLDisplay/library.json`
- `uidesign/ssd1322-256x64/u8g2gl/u8g2gl.xml`
- `uidesign/ssd1322-256x64/u8g2gl/globals.xml`
- `uidesign/ssd1322-256x64/u8g2gl/library.json`
- `test/render/.gitignore`
- `test/render/shim/Arduino.h`
- `test/render/shim/HostEnvironment.hpp`, `HostEnvironment.cpp`
- `test/render/shim/WiFi.h`, `WifiManager.hpp`, `esp_heap_caps.h`
- `test/test_unit/test_u8g2glDisplay/test_main.cpp`, `test_display.cpp`,
  `test_diagnostics_board.cpp`, `test_diagnostics_format.cpp`,
  `u8g2gl_runtime_unity.cpp`

`screen_diagnostics.xml` is accurate throughout (its geometry arithmetic, the 11/5/6 split, the
17/11 heights, the 75/76 centring example and the 135/137 px text widths all reconcile); its only
findings are the two clarity items above. `platformio.ini` comments are clear apart from line 59.

## Counts

| Severity | Count |
|---|---:|
| Must fix | 6 |
| Should fix | 13 |
| Nit | 12 |

Files with findings: 15 of 34. Files with none: 19 (all listed above).

## Observations outside this WORKORDER's scope

- **O1 (in-flight edits, do not commit).** The working tree currently holds two uncommitted
  edits from the concurrent worker: `platformio.ini` gains `custom_u8g2gl_cli =
  /Users/mcneillm/Documents/Projects/U8G2GL-05/cli/dist/node/cli.js` on `env:esp32dev` (an
  absolute machine path that must not reach the PR), and `screens/screen_diagnostics.xml` plus
  `generated/ui_screen_diagnostics.cpp` move the uptime label from `y=30` to `y=31` (this looks
  like the WO-B3b edit-loop mutation; if committed it would break pixel identity and contradict
  the XML's own "Legacy: x=4 y=30" comment).
- **O2 (U8G2GL generator).** Generated files carry only a "GENERATED by u8g2gl" line and no
  `SPDX-License-Identifier: MIT`. `THIRD-PARTY-NOTICES.md` now has to state their licence in prose
  because the files do not. A one-line SPDX in the generator's header would make that
  self-describing.
- **O3 (harness coverage).** Because of the first Must-fix item, the host harness renders the
  runtime with 256-binding capacities while the firmware uses 4. Pixels cannot differ, but nothing
  in the harness exercises the capacity-scoped configuration; only the on-device WO-B5 will.
- **O4 (U8G2GL runtime).** `src/u8g2gl_label.cpp:334` calls `setFontPosTop()` "the framework
  standard" and the spec (`design_spec.md:2216`) calls it "the framework default", yet nothing in
  `src/` sets it (the PA already has this as a finding; the evidence is
  `after/diagnostics_board/state_probe.json`, 1392 lit pixels against 15).
- **O5 (legacy, existing).** `platformio.ini` line 3-8 still says "Three environments" and lists
  four; not part of this PR.
