# PROJECT — U8G2GL PR-05: the integration module and the first migrated board

**Tier 2 · Principal Architect**
**Programme**: U8G2GL (governed from `cblabs-org/u8g2gl`; this repository is its first consumer)
**Twin project**: `cblabs-org/u8g2gl` →
`.agents/skills/principal-architect-workspace/pr-05-integration/PROJECT.md` (packaging, the
programme view, the full scoping record). This file is the **ledger of record for the foundation
work orders** and their provenance.
**Source**: U8G2GL Chief Architect bridge "PR-05: foundation integration and the first real board"
(pasted by the owner, 2026-10-06)
<!-- Last updated: 2026-10-06 -->

Not to be confused with this workspace's `pr-05-display-module`, which is foundation's own PR-05
from 2026. The `u8g2gl-` prefix names the programme.

## Mandate

Per U8G2GL `docs/implementation_plan.md` §3 and PD-4, following the `lvglpro` precedent:

- **`modules/U8G2GLDisplay/`**, Apache-2.0. It consumes the MIT `u8g2gl` runtime as an external
  PlatformIO dependency, pinned by git commit. Nothing is published.
- **`uidesign/ssd1322-256x64/u8g2gl/`**: the XML project and its committed `generated/`, consumed
  through `lib_extra_dirs`. It is named for the panel because `esp32dev` and `esp32s3nano` share
  it.
- A **pre-build hook** that runs `u8g2gl generate` from a local CLI checkout when one is
  configured, and otherwise builds from the committed `generated/` (U8G2GL [D-11]).
- A **`THIRD-PARTY-NOTICES.md`** row for U8G2GL (MIT).
- **`DiagnosticsBoard` migrated** in the root firmware (`src/main.cpp`). `sample-app` and
  departures-board-v3 keep the legacy class.
- **PR-07's "before" evidence**, captured while the legacy board is live: pixels, flash and RAM on
  `esp32dev`, line counts, and edit-loop timing.

**PD-4:** when this lands, `modules/U8G2Display` is **frozen**: correctness and security fixes only.
The PR description announces it. **Nothing is deleted.**

**Out of scope:** other boards (PR-10), the Wasm preview and pixel-diff harness (PR-06),
publishing or tagging U8G2GL, any change to the U8G2GL generator or runtime (report, don't fix),
and web diagnostics (R7).

## Human Approval

> "Please proceed with the development - I will work with you to connect to the platformio mcp
> (which is already on the machine) and also attache a device later when back at the bench.  For
> now get on with the dev - thanks."
>
> — the owner, 2026-10-06, in reply to the PA's scoping report and proposed decomposition
> (U8G2GL WO-A; foundation WO-B1..B4, plus B5 when hardware is attached) with rulings R1–R7.

## Rulings that bind foundation work

- **R1 — board host, not display owner.** In PR-05, `U8G2GLDisplay` provides an `IDisplayBoard`
  that runs a generated U8G2GL screen inside the **legacy** carousel. It calls the screen's
  `tick_fn` / `render_fn` directly (`u8g2gl_screen` in the runtime's `u8g2gl_core.h`) and never
  calls `ui_handler()`, which clears and sends the buffer itself. The legacy module already does
  both (`U8G2Display.cpp:1413`). The display, carousel and screensaver stay in
  `modules/U8G2Display` until PR-10.
- **R4 — PlatformIO through the MCP only.** Foundation `rules/04` stands. Until the owner connects
  the PlatformIO MCP, any step needing a PlatformIO build is **deferred and reported**, never run
  through the `pio` CLI. Host tools that are not PlatformIO (CMake, a C++ compiler, Python, Node)
  are permitted.
- **R5 — hardware later.** On-device checks wait for WO-B5 (interactive).
- **R6 — pin.** `lib_deps` pins the pushed U8G2GL WO-A commit. Before this PR merges, it is
  re-pinned to the U8G2GL merge commit on `main`, which merges first.
- **R7 — no new web diagnostics.** The legacy Screenshot tab captures the new board.

## Known hazards (for WO-B2)

- **Inherited U8G2 state.** The legacy module sets `setFontPosTop()`, `setFontMode(1)` and
  `setDrawColor(1)` once at begin (`U8G2Display.cpp:141-146`). A U8G2GL screen rendering inside
  the legacy carousel inherits that state. Whatever the U8G2GL runtime assumes must be
  established by the host board, **measured**, not assumed.
- **The "·" separator.** The legacy fonts are `_tr` and have no glyph `0xB7`, so the separator
  probably never renders. Pixel identity means reproducing what legacy actually draws. WO-B1's
  baseline settles it.
- **Live inputs.** The legacy `tick()` reads WiFi, `millis()` and the heap directly. The migrated
  board needs the same values through an injectable path, so the harness can render it with fixed
  inputs.

## Branch Strategy

- **Worktree:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`, from
  `origin/main` @ `e5342373`. The main checkout belongs to other work and to the bug-fixer
  daemon; never touch it.
- **Branches:** `u8g2gl-integration/wo<N>-<topic>`, stacked serially, each based on the previous
  WO's branch by name, with no upstream until submission.
- **The PR:** opens from the stack tip against `main` and is pushed only at submission.

## Work Order Ledger

- **[x] [wo1-baseline](wo1-baseline/WORKORDER.md):** the "before" evidence. **Complete: Part 1
  (host pixel baseline, line counts) and Part 2 (flash/RAM, native 989, edit loop), verified by
  the PA.** Part 2 measured in the detached legacy worktree at `7396e43d`.
- **[x] [wo2-module](wo2-module/WORKORDER.md):** the board host, the generated UI, the pinned
  dependency (`82ec83f`, now in `platformio.ini` `[u8g2gl]`), the hook with its scoped
  capacities, the notices row, the `main.cpp` switch, and the native suite. **Complete: Parts 1
  and 2 verified by the PA.** Pixel-identical in all four scenarios; `esp32dev` and
  `esp32s3nano` build; native 1018/1018.
- **[x] [wo3-evidence](wo3-evidence/WORKORDER.md):** **Complete, verified by the PA.** The
  after-size evidence and the `PR-DESCRIPTION.md` draft are done. Its edit-loop timing is
  superseded by WO-B3b.
- **[x] [wo3b-timing](wo3b-timing/WORKORDER.md):** the legacy and U8G2GL edit loops measured
  interleaved, with load recorded. **Complete, verified by the PA.** It supersedes the WO-B1 and
  WO-B3 timing for the gate.
- **[x] [wo4-readability](wo4-readability/WORKORDER.md):** the review phase is done; `REVIEW.md`
  has 6 must-fix, 13 should-fix and 12 nit findings. The PA confirmed both wrong-citation
  findings in source.
- **[ ] [wo4b-remediation](wo4b-remediation/WORKORDER.md):** fix the Must and Should findings, and
  have the harness build the runtime with the firmware's capacities. *Dispatched 2026-10-07.*
- **[ ] wo5-device (interactive):** host render vs on-device capture, when the owner is at the
  bench.

## Evidence

All four of PR-07's evidence sets live in [evidence/EVIDENCE.md](evidence/EVIDENCE.md). PR-07's PA
reads them there and does not re-derive them.

## Session Log

- **2026-10-06 — dispatch.** Worktree created at `e5342373`. The `house-style-docs` submodule
  initialised in it. WO-B1 Part 1 dispatched, in parallel with U8G2GL WO-A.
- **2026-10-06 — WO-B1 Part 1 verified.**
  - **Reproduced byte for byte:** the PA rebuilt the harness from clean (U8G2 override
    `../U8G2GL-05/extern/u8g2` @ 2.36.18), re-rendered all four scenarios, and every `.bin` and
    `.pbm` matches the committed baseline (`connected.bin` sha256 `50567054…7251`).
  - **Warnings:** 9 build warnings, all `-Wunused-parameter` in unmodified legacy
    `IGfxWidget.hpp:137`. None come from the harness.
  - **Finding (PA, from the rendered image, not in the worker's report): the legacy board clips
    its descenders.** "My" renders as "Mv", and the "p" in "Up" and "Heap" loses its tail.
    - Code reading: `DrawingPrimitives.cpp` drawText passes `y + h - 1` and `x + w - 1` to
      `setClipWindow`, whose far corner U8G2 treats as **exclusive** (`u8g2.h:352/354`, also
      cited in U8G2GL's spec §11). The last row and column of each label are therefore lost. With
      `h = getMaxCharHeight()` under `setFontRefHeightAll()`, that cuts into the descender.
    - The exact rows lost are for WO-B2 to measure.
    - This is a legacy defect that the baseline faithfully records. The gate as written asks for
      pixel identity, so WO-B2 targets **bug-for-bug** identity. Whether to then fix it is an
      owner question, recorded in the twin PROJECT.
  - **Other legacy findings (the worker's):**
    - The "·" separator never renders: an 8-column gap.
    - The long SSID is clipped mid-glyph because truncation is not enabled.
    - `Fonts.hpp` includes `<U8g2lib.h>` inside `namespace AppFonts`.
    - The `DrawingPrimitives.cpp` header names the file in the wrong case.

    All four are legacy, outside scope, and become frozen-module correctness candidates after
    PD-4.
- **2026-10-06 — WO-B2 Part 1 verified.**
  - **Clean rebuild:** from `git archive HEAD` (the WO-B1 lesson: a git-ignored
    `test/render/CMakeLists.txt` had escaped the PA's first check, which built from the working
    tree). U8G2 and U8G2GL are both overridden to `../U8G2GL-05`, whose runtime matches
    `69c37e0`.
  - **Pixels:** all four scenarios byte-identical, legacy against U8G2GL, and both against the
    committed `baseline/` and `after/`.
  - **No degenerate pass:** the `u8g2gl` target links runtime, generated UI and module, and not
    the legacy board library.
  - **Generated code:** the committed `generated/` equals a fresh `generate --target ssd1322`.
  - **Warnings:** only legacy `IGfxWidget.hpp:137`.
  - **Gate signal, criterion 2:** authored code is **138 lines against legacy's 99**. That is XML 33,
    board C++ 99 and `main.cpp` 6, plus a one-off host of 86. Part of the board C++ is the
    hardware-read/format split this WO required for testability. EVIDENCE §2 says so; PR-07 judges
    it.
  - **Gate signal, criterion 1:** bug-for-bug identity needed heights of 16 and 10 and data-row
    width 247, each one smaller than legacy's arguments, to reproduce legacy's off-by-one clip.
    The XML documents why.
  - **U8G2GL findings** (consolidated in the twin PROJECT for the Chief Architect):
    - `setFontPosTop` is never established by the runtime (measured: 15 lit pixels against 1392);
    - a null-font crash in the state saver;
    - the font mode is not restored;
    - screen files not named `screen_*` are skipped silently;
    - the PlatformIO forced-include guidance.
- **2026-10-06 — re-pinned to `0fe144f`.** The owner asked that their email address not be
  published. It was in U8G2GL `library.json`, added by WO-A in `1dee2db`.
  - Removed in `0fe144f` and pushed. The runtime is unchanged: `git diff 69c37e0 0fe144f` touches
    only `library.json`.
  - Foundation now pins `0fe144f` (`modules/U8G2GLDisplay/library.json`,
    `test/render/CMakeLists.txt`). A `FetchContent` of the new sha re-produced all 13 `after/`
    render files byte for byte.
  - **The address remains in the U8G2GL branch's history** (`1dee2db`, `69c37e0`), in a private
    repository. Removing it needs a history rewrite and force-push of that branch, which is the
    owner's call. A squash merge would also keep it out of `main`.
- **2026-10-06 — U8G2GL history rewritten; final pin `82ec83f`.** At the owner's instruction, the
  U8G2GL branch's history was rewritten so the address never appears.
  - `git filter-branch --prune-empty` over `1254eb3..`. The removal commit `0fe144f` became empty
    and was dropped.
  - Force-pushed with lease against `0fe144f`.
  - The old shas `1dee2db`, `69c37e0`, `47618b5` and `0fe144f` are **gone from every ref**, and
    their reflogs are expired.
  - **New pin:** `82ec83f2bbce02080c3dd1ab6618bcf6559bfde5`, the rewritten `69c37e0`. It is
    identical apart from `library.json`.
  - **Verified:** a fresh `FetchContent` checks out `82ec83f`, and all 13 `after/` render files
    match byte for byte.
  - **Scratch cleaned:** the build caches in `tmp/` and the scratchpad that held the old commit
    were deleted.
  - Older entries above that cite `69c37e0` or `0fe144f` describe what was verified at the time;
    those commits no longer exist.
- **2026-10-07 — PlatformIO MCP connected; WO-B1 Part 2 dispatched.**
  - **Connection:** the owner installed the local `platformio-mcp` Claude Code plugin. The PA
    confirmed tool access with `project_envs` (8 envs) and `get_lock_status` (unlocked).
  - **Legacy worktree:** `../esp32-app-foundation-u8g2gl-legacy`, detached at `7396e43d`, whose
    firmware equals `e5342373`.
  - **Scope:** the owner directs focus on the development, build and test loops. The device
    comes later (WO-B5).
  - **Order:** WO-B1 Part 2, then WO-B2 Part 2, then WO-B3, serially, because builds share the
    MCP queue lock.
- **2026-10-07 — WO-B1 Part 2 verified; WO-B2 Part 2 dispatched.**
  - **Legacy figures, re-checked by the PA against the raw MCP build and test logs:**

    | Measurement | Legacy |
    |---|---|
    | `esp32dev` RAM | **57500 B** (17.5%) |
    | `esp32dev` flash | **1336143 B** (84.9%) |
    | `esp32s3nano` RAM | 60124 B |
    | `esp32s3nano` flash | 1370475 B |
    | Native suite | **989/989** |
    | Edit → rebuild, median of 3 | **29.94 s** |

  - **Gate signal, criterion 3: `esp32dev` flash is already 84.9% full**, with about 237 KB left
    of the 1.5 MB app partition before any U8G2GL code. Whatever the runtime and generated UI
    add is measured against that headroom.
  - **The stop and resume:** the owner stopped the worker on a hook prompt. It was the PA's
    plugin hook misreading `esptool` inside a quoted grep regex; the worker never invoked
    esptool. The hook was fixed (quote-aware splitting, 20 test cases) and the worker resumed
    with its measurements intact.
  - **Owner direction for Part 2:** no crowpanel build; bring the module into `env:native` with
    tests on foundation's stubs.
- **2026-10-07 — WO-B2 Part 2 verified.**
  - **Raw logs, cross-checked by the PA:**

    | Build | RAM | Flash |
    |---|---:|---:|
    | `esp32dev` | **57340 B** | **1342279 B** (85.3%) |
    | `esp32s3nano` | 59964 B | 1376723 B |

  - **Native, re-run by the PA through the MCP from the committed tree: 1018/1018** (989 + 29 new
    in `test_u8g2glDisplay`). The newest log in the worktree had been the worker's deliberate
    mutation run, which is why the PA re-ran rather than read it.
  - **Gate signal, criterion 3 (provisional; WO-B3 re-measures from clean):**
    - against legacy, **RAM −160 B** and **flash +6136 B** on `esp32dev`;
    - the scoped capacities are what make RAM come out lower: `s_binding_pool` is 96 B instead
      of 6144 B, and RAM is 63388 B without them against 57340 B with them.
  - **Integration decisions made in Part 2:**
    - **The pin moved to a `[u8g2gl]` section in `platformio.ini`.** PlatformIO does not install
      git dependencies declared by `lib_extra_dirs` libraries, which the worker measured
      ("Ignored `u8g2gl` dependency"). The single pin is still single.
    - **The capacities come from a build middleware** in `scripts/u8g2gl_generate.py`, adding
      the include to exactly the 15 runtime sources. The PA confirmed this from the verbose log.
    - **Native** compiles the runtime through a test-local unity file, because the runtime's
      `frameworks: arduino` excludes it from a native env.
  - **Unverified:** `crowpanel-128-rotary`'s added `lib_ignore`. The owner directed it not be
    built.
  - **To re-pin before merge:** `platformio.ini` `[u8g2gl]`, and `test/render/CMakeLists.txt`
    `U8G2GL_PINNED_SHA`.
- **2026-10-07 — WO-B3 verified; WO-B3b dispatched.**
  - **Size after, `esp32dev`, clean:** RAM 57340 B (**−160 B**), flash 1342247 B (**+6104 B**,
    85.3%). 230617 B of the app partition is left; the migration spends 2.6% of the legacy
    headroom.
  - **Where the flash goes:** the runtime and the U8g2 shape code it links make up about 74% of
    what was added.
  - **Unexplained flash shift:** adding a `custom_` option to `platformio.ini` moved flash by
    −1780 B, in ESP-IDF diagnostics symbols only, with no U8G2GL change. The quoted figures are
    the clean builds. This is a caveat on any delta under about 2 KB.
  - **The timing is not decision-grade.** WO-B3's 48.81 s against WO-B1's 29.94 s was measured
    hours apart, with load averages of 5.5 to 20.7 (the PA saw 8.9 over 15 minutes at
    verification). The generator costs about 0.05 s. WO-B3b re-measures both loops interleaved.
  - **New downstream user:** `template-app` also uses the legacy `DiagnosticsBoard`. It is listed
    in the PR description.
- **2026-10-07 — WO-B3b verified: the edit-loop result for gate criterion 4.** There were five
  interleaved rounds through the MCP, at load averages of 4 to 8.5. Medians of the task time:

  | Build | Legacy | U8G2GL |
  |---|---:|---:|
  | Edit and rebuild | 26.92 s | **31.90 s** |
  | No-change rebuild | 21.60 s | 27.92 s |
  | Per-edit compile cost (edit minus no-change) | 4.65 s | **3.90 s** |

  - **Paired, U8G2GL edit minus legacy edit:** median **+4.99 s**. It survives the pairing.
  - **The whole gap is fixed per-build overhead (+6.16 s), not the generator** (0.05 to 0.07 s; a
    hook-off experiment rules the hook out). The verbose logs show PlatformIO's library
    dependency graph growing by 42.6% (6266 to 8935 lines). `U8G2GLDisplay`, `U8G2GLUI` and
    `u8g2gl` are new, and the `U8G2GLDisplay` subtree alone is 2665 lines under
    `lib_ldf_mode = deep`. That is inferred from the logs; it could not be timed separately
    without the CLI.
  - **Candidate reduction, not done:** `U8G2GLDisplay`'s `library.json` declares `U8G2Display`
    as a dependency, for `IDisplayBoard`. That can make the deep finder walk the legacy module's
    graph a second time. An experiment could cut the overhead; it is put to the owner, not done
    here.
  - **The PA's error:** the PA committed the WO-B4 review files to this worktree during WO-B3b's
    run. Foundation's `inject_version.py` stamps the git state into `esp_app_desc`, so the new
    HEAD forced a recompile and relink in round 4. That caused both outliers. The medians absorb
    it. **Lesson: no commits to a worktree while a timing run is live.**
