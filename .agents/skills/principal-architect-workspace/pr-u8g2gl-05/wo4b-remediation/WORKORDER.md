---
work_order: WO-B4b
project: pr-u8g2gl-05
title: "Remediate the house-style review; the harness builds the runtime as the firmware does"
branch: u8g2gl-integration/wo4-readability
base: u8g2gl-integration/wo3-evidence
complexity: standard
status: dispatched
---

# WO-B4b: fix what WO-B4 found

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Before starting, check in the committed
files that:

- `../PROJECT.md` "Human Approval" quotes the owner's approval;
- its ledger lists WO-B4b;
- this WORKORDER's `status` is `dispatched`.

Otherwise **stop and report**.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Branch and working tree

- **Worktree:** work only in `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`.
- **Branch:** `u8g2gl-integration/wo4-readability`, already checked out, with no upstream.
- **Before every commit**, run `git branch --show-current` as its own command.
- **Hands off:** never touch `../esp32-app-foundation` or `../esp32-app-foundation-u8g2gl-legacy`.
- **Commits:** you are authorized to commit on a verified result. **Do NOT stop to ask.** Do not
  push, tag or flash.

## Rules and tooling

- **Read first:** every file in `.agents/rules/`. **Rule 04 binds:** PlatformIO only through the
  MCP tools `mcp__plugin_platformio-mcp_platformio__*` (load the `platformio-mcp:pio-manager`
  skill first). If the tools are unavailable, stop and report. Never use the CLI.
- **A PreToolUse hook is installed:** never put `pio`, `platformio` or `esptool` after a `|` or
  inside a `|`-separated regex in a Bash command.
- **Standards:** the house style is `.agents/skills/house-style-docs/SKILL.md` and
  `references/cpp-embedded.md`.

## Objective

Every **Must fix** and **Should fix** in `../wo4-readability/REVIEW.md` is resolved, and the
host render harness builds the U8G2GL runtime with the **same capacities the firmware uses**.

## Boundary Box

**You may change:**
- the files named by REVIEW.md findings;
- `test/render/`: the harness capacities, plus regenerating `after/`;
- this WORKORDER's Completion Report;
- a "Remediation" note in REVIEW.md (append only; do not rewrite the review).

**Out of scope:**
- **Behaviour.** Apart from the harness capacities (item 2), this WO changes comments,
  documentation and naming only. No firmware code path changes.
- **U8G2GL sources** (report, don't fix).
- **`generated/`**, which is generator output.
- **The `platformio.ini` header's "Three environments" line.** It predates the PR; leave it.
- **The U8G2GL generated files' missing SPDX.** That is U8G2GL's concern, and is reported there.

## Line-Item Instructions

1. [ ] **Every Must fix and Should fix in REVIEW.md.** Each fix makes the comment **true**, which
   needs no measurement for this work order: the evidence is in REVIEW.md and EVIDENCE.md.
   - **The pin wording.** `platformio.ini` `[u8g2gl]` is the firmware's pin.
     `test/render/CMakeLists.txt` `U8G2GL_PINNED_SHA` is the harness's copy, and must be kept
     equal. Say so in **both** places. No comment may claim that the module's `library.json`
     pins it; that file says `"*"`.
   - **Workspace citations.** Replace a citation like "PR-05 ruling R1/R6" with a self-contained
     reason, or with a path that a reader of this repository can open
     (`.agents/skills/principal-architect-workspace/pr-u8g2gl-05/PROJECT.md`). "PR-05" alone is
     ambiguous here: foundation has its own `pr-05-display-module`. Write "U8G2GL PR-05" where
     the programme must be named.
   - **Nits:** fix them where the fix is a one-liner. List the ones you skip, with the reason.
2. [ ] **The harness capacities.**
   - In `test/render/CMakeLists.txt`, force
     `uidesign/ssd1322-256x64/u8g2gl/generated/u8g2gl_generated_config.h` into the
     `u8g2gl_runtime` target's sources only, the way `scripts/u8g2gl_generate.py`'s middleware
     does in the firmware. Use a compile option on that one target, with an absolute path derived
     from `REPO_ROOT`.
   - **Fix the comment** that claimed otherwise, and the string `test/render/main.cpp` writes
     into `inputs.json`.
   - **Regenerate `after/`** from a clean build. All 13 render files must stay **byte-identical**
     (pixels do not depend on pool size). Only `inputs.json`'s capacity note changes. Show the
     diff.
   - **Prove the capacities reached the runtime.** Show the binding pool's size in the harness
     binary (`nm -S` on the object, or a `static_assert`/print in the harness), with the source
     named.
3. [ ] **Verify.**
   - **Harness:** rebuilt from `git archive HEAD` into `tmp/`, from clean. Both targets, and
     `--compare test/render/baseline` with 0 differing pixels.
   - **Native:** `run_tests` on `env:native` through the MCP. **1018/1018.**
   - **Firmware:** one `clean_project` + `build_project` of `esp32dev`. The `RAM:` line must be
     **57340 B**, since the change is comments only. Quote the `Flash:` line; it may move by tens
     of bytes, which is the known noise floor.
   - **Docs:** `python3 scripts/doc_lint.py` exits clean.
4. [ ] **Commit** in two logical commits: the comment fixes, and
   `test(render): build the runtime with the firmware's capacities`. End each with
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`. Then append "Remediation" to
   REVIEW.md, listing each finding as fixed or skipped with its reason, and commit that.

## Verification Requirement

- Every Must and Should fix is resolved.
- Pixels are unchanged; native passes 1018/1018; `esp32dev` RAM is unchanged.
- `git status` is clean, and the lock is released.

---

## Completion Report

<!-- Written by the worker. -->
