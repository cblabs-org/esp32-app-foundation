---
work_order: WO-B4
project: pr-u8g2gl-05
title: "House-style review of the foundation PR (review only)"
branch: u8g2gl-integration/wo3-evidence (read-only; no commits by this WO)
base: e5342373
complexity: standard
status: dispatched (review phase)
---

# WO-B4: house-style review, report only

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Before starting, check in the committed
`../PROJECT.md` that "Human Approval" quotes the owner's approval and that the ledger lists WO-B4.
Otherwise **stop and report**.

## Role Definition

> You are a code reviewer. You do not edit source, you do not commit, you do not branch. You read,
> judge against the written standards, and write one report.

## Constraints

- **Read-only.** Another worker is building and committing in this worktree,
  `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`. Do not modify any file except
  your report, and **do not run any git command that writes** (no `add`, `commit`, `checkout` or
  `stash`).
- **No PlatformIO operations** of any kind. You don't need any.
- **Your one output:** write `REVIEW.md` beside this WORKORDER. The PA commits it.

## Standards to judge against

- **The house style:** `.agents/skills/house-style-docs/SKILL.md` and its
  `references/cpp-embedded.md`. Covers module headers with SPDX, function documentation, scoped
  variables, internal step comments for functions over about 15 lines, arcane-logic comments, and
  naming.
- **Repository conventions:** `CONTRIBUTING.md`, and `scripts/doc_lint.py`. Run it; it is
  read-only.
- **The precedent:** files of the same kind in `modules/U8G2Display/` and `modules/LVGLDisplay/`.
  Match what the repository already does. Do not invent stricter rules.
- **Python scripts:** judge `scripts/u8g2gl_generate.py` against foundation's existing PlatformIO
  scripts (`scripts/*.py`, and the modules' `exclude_on_native.py`).

## Scope: the authored files changed since `e5342373`

Get the list with:

```
git diff --name-only e5342373..HEAD -- ':!.agents' ':!test/render/baseline' ':!test/render/after' ':!uidesign/ssd1322-256x64/u8g2gl/generated'
```

**Excluded:**
- `uidesign/ssd1322-256x64/u8g2gl/generated/`: machine output from U8G2GL, MIT, stamped by its
  generator;
- the baseline and after render files;
- the workspace.

**Also review for clarity, not style:**
- the XML comments in `screens/screen_diagnostics.xml`, which explain the pixel-identity
  geometry;
- the `platformio.ini` comments.

**Out of scope:** U8G2GL's runtime and generator. A defect you notice there goes under
"Out-of-scope" in the report.

## The report: `REVIEW.md`

- **Format:** one finding per entry, in WORKORDER checklist form, so it can be handed straight to
  a remediation worker:
  `- [ ] path:line — the rule (cite the section of the house style) — what is wrong — the exact fix`.
- **Severity:**
  - **Must fix:** missing header or SPDX, an undocumented public API, a wrong or misleading
    comment, a naming violation.
  - **Should fix:** a missing step breakdown, an unexplained magic number.
  - **Nit.**
- **Proportion:** keep it honest. Say plainly when a file is clean. Don't pad.
- **Accuracy over style:** also check that every comment is **true**. Comments in this PR cite
  measurements, line numbers and other files. Spot-check the citations: does `U8G2Display.cpp:1413`
  hold the frame sequence, does `u8g2.h:352/354` say the far corner is excluded (the pinned U8G2
  source is at `/Users/mcneillm/Documents/Projects/U8G2GL-05/extern/u8g2/csrc/u8g2.h`), and so on.
  A wrong citation is a **Must fix**.
- **End with:** a count per severity, and a list of the files with no findings.

When done, send the PA a short summary: the counts, the five most important findings, and any
out-of-scope observations.
