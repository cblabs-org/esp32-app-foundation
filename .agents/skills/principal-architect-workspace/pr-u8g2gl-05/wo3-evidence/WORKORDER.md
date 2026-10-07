---
work_order: WO-B3
project: pr-u8g2gl-05
title: "The after evidence, and the PR description"
branch: u8g2gl-integration/wo3-evidence
base: u8g2gl-integration/wo2-module
complexity: standard
status: dispatched
---

# WO-B3: measure the migrated board as WO-B1 measured the legacy one

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Before starting, check in the committed
files that:

- `../PROJECT.md` "Human Approval" quotes the owner's approval of the decomposition;
- its ledger lists WO-B3;
- this WORKORDER's `status` is `dispatched`.

Otherwise **stop and report**.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Branch and working tree

- **Worktree:** work only in `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`.
- **Branch:** `u8g2gl-integration/wo3-evidence`, already checked out, with no upstream.
- **Hands off:** never touch `../esp32-app-foundation` or `../esp32-app-foundation-u8g2gl-legacy`.
- **Before every commit**, run `git branch --show-current` as its own command.
- **Scratch files** go in this worktree's `tmp/`.

## COMMIT ON A VERIFIED RESULT

You are authorized to commit when verified. **Do NOT stop to ask**: nobody can answer. Do not
push, tag, flash, or open a PR.

## Rules and tooling

- **Read first:** every file in `.agents/rules/`, in order. **Rule 04 binds:** PlatformIO only
  through the MCP tools `mcp__plugin_platformio-mcp_platformio__*`.
  - Load their schemas with ToolSearch if they are deferred, and load the
    `platformio-mcp:pio-manager` skill first.
  - If the tools are unavailable, stop and report.
  - Never use the CLI, never call `reset_server_state`, and run one build at a time.
- **A PreToolUse hook is installed:** never put `pio`, `platformio` or `esptool` after a `|` or
  inside a `|`-separated regex in a Bash command.
- **Use WO-B1 Part 2's method and flags exactly**, as described in
  `../wo1-baseline/WORKORDER.md` (its Part 2 Completion Report) and `../evidence/EVIDENCE.md` §3
  and §4 Before:
  - `forceExecution: true` and `verbose: true`;
  - clean builds for the size figures;
  - one untimed warm-up, then three timed runs, timed from the edit being written to the task
    reporting success.

  The comparison is only fair if the two sides are measured the same way.

---

## Objective

Fill `EVIDENCE.md` §3 and §4 **After** with numbers measured the same way as **Before**. Draft
the foundation PR description, which announces the PD-4 freeze.

## Strict Boundary Box

**In scope:**
- `../evidence/EVIDENCE.md`
- `../PR-DESCRIPTION.md` (new)
- this WORKORDER's Completion Report
- **temporary, reverted** edits for the timed runs only: `screen_diagnostics.xml`, plus
  `custom_u8g2gl_cli` in `platformio.ini`

**Out of scope:** every source, config and test file. If a measurement exposes a defect, report
it; do not fix it here.

## Line-Item Instructions

1. [ ] **§3 After: flash and RAM, from clean.**
   - Run `clean_project`, then `build_project`, for `esp32dev`, then the same for `esp32s3nano`.
   - Record the exact `RAM:` and `Flash:` lines, and run `size_report` with `expectedElfSha256`
     taken from the freshly built ELF. WO-B1 found it fails without that.
   - Add a **Before → After delta table**: bytes and percent for RAM and flash on both boards,
     and the section-by-section deltas for `esp32dev`.
   - **Attribute the flash delta**, using `nm -S --size-sort` on both ELFs (the toolchain's `nm`,
     by full path, read-only). Name the largest contributors: runtime classes, generated UI,
     module, and what disappeared with the legacy board. Use the legacy ELF if it still exists
     in the legacy worktree's `.pio/build/esp32dev/`; read it only, never rebuild there. If it
     is gone, say so and attribute from the new ELF alone.
   - **The noise floor:** WO-B1 saw 32 B, and WO-B2 saw tens of bytes. State it, and do not
     over-read differences below it.
   - **Headroom:** state what is left of the `esp32dev` app partition after the migration, in
     bytes and percent. Gate criterion 3 asks whether this is "comfortable".
2. [ ] **§4 After: edit → regenerate → rebuild.**
   - **Setup:** temporarily add `custom_u8g2gl_cli =
     /Users/mcneillm/Documents/Projects/U8G2GL-05/cli/dist/node/cli.js` to `[env:esp32dev]`, so
     the hook regenerates on every build.
   - **Warm-up:** one untimed `build_project`.
   - **Three timed runs.** Each changes row 3's `y` in `uidesign/ssd1322-256x64/u8g2gl/screens/
     screen_diagnostics.xml` from 30 to 31, or back again, so every run is a real change. Time
     from the edit being written to the task reporting success.
   - **Split each run into two parts from the build log:** the hook's generate step, and the
     compile and link.
   - **Report** which objects were recompiled, every value, and the median.
   - **Revert both temporary edits.** `git status --short` must be empty before you commit.
   - **The fair comparison is the median against WO-B1's 29.94 s.** Also record, as a second row,
     the **unconfigured** loop: the author runs `u8g2gl generate` by hand, then builds. Time the
     CLI run and the build separately.
3. [ ] **§4 reading.** Add a short, factual paragraph comparing the two loops. Note what the
   timing cannot capture:
   - **Legacy:** a geometry change to a legacy label means touching C++ constructor arguments.
   - **XML:** the same change is one attribute, and the generator re-checks the layout against
     the display.

   Both observations are facts about the code, not opinion. Keep the paragraph neutral: PR-07
   judges.
4. [ ] **`../PR-DESCRIPTION.md`**, the foundation PR description, written to the house standard
   (look at recent merged PR descriptions in `git log` for shape). It must contain:
   - **What the PR does:**
     - the `U8G2GLDisplay` board host inside the legacy carousel (R1), and why it does not own
       the display yet;
     - the generated UI and its pre-build hook;
     - the pinned runtime and the scoped capacities;
     - the `THIRD-PARTY-NOTICES.md` row;
     - the root firmware's `DiagnosticsBoard` switched over, while `sample-app` and
       departures-board-v3 keep the legacy class.
   - **The PD-4 freeze announcement**, verbatim in substance from U8G2GL
     `docs/implementation_plan.md` §2 PD-4: from this merge, `modules/U8G2Display` is
     **frozen**:
     - correctness and security fixes only;
     - no new features, boards or widgets;
     - new work targets `modules/U8G2GLDisplay/`;
     - a fix made in both modules references the same issue in both commits;
     - enforced at review, and nothing is deleted here (PR-10 retires the module).

     Read the plan text with `git -C /Users/mcneillm/Documents/Projects/U8G2GL show
     origin/main:docs/implementation_plan.md`.
   - **The evidence summary:** one table per gate criterion, linking `EVIDENCE.md`.
   - **What the migrated board does and does not do compared with legacy.** Identical pixels in
     the four host scenarios, defects included (the clipped descenders, the invisible "·", the
     mid-glyph SSID clip); same refresh cadence; same board name. It has not been verified on a
     device yet (WO-B5).
   - **Test counts:** native 989 → 1018, and the host pixel harness.
   - **Before merge:** re-pin `u8g2gl` in `platformio.ini` `[u8g2gl]` and in
     `test/render/CMakeLists.txt` to the U8G2GL merge commit on `main`. The U8G2GL PR merges
     first.
   - **Known findings** carried to U8G2GL. List them; link the twin PROJECT, don't paraphrase it.
   - End with `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.
5. [ ] **Commit**, for example `docs(pa): U8G2GL PR-05 WO-B3 after evidence and PR description`,
   ending with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Verification Requirement

- Every number is quoted from a log or tool output, with its source named.
- Temporary edits are reverted, and `git status` is clean.
- Locks are released.
- `git diff --stat u8g2gl-integration/wo2-module..HEAD` shows only the in-scope files.

## Issue Surfacing

Report defects or surprises in the Completion Report. Do not fix or file them.

---

## Completion Report

<!-- Written by the worker: Status, Changes Made, Decisions Made, Dead Ends & Warnings,
Out-of-Scope Findings, Blockers; plus the delta tables, the timing table and the flash
attribution. -->
