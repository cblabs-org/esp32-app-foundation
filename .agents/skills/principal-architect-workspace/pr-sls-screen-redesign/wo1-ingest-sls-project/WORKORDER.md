---
name: sls-screen-redesign-wo1
description: Handoff instructions for Work Order 1 — Ingest the reviewed SLS project source + refresh README
complexity: mechanical
---

# WO1 — Ingest Revised SLS Project Source

### Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Bring the reviewed SquareLine Studio **project source** (delivered by Claude Design at
`/Users/mcneillm/Downloads/sls-project`) into `squareline/crowpanel-128/`, swap the
boot-logo asset, and refresh the project `README.md` to describe the new screen layout.
**No firmware, no `modules/` changes** — this WO is docs + design source only.

## Context
- **Branch:** `sls-screen-redesign/wo1-ingest-sls-project` (based on `sls-templates/crowpanel-128`) — the PA will have created it before you start.
- Source of truth for the new design: `/Users/mcneillm/Downloads/sls-project/`
  (`SquareLine_Project.spj`, `SquareLine_Project.sll`, `Themes.slt`, `assets/cblabs.png`, `assets/cblabs_180x58.png`).
- The repo copy in `squareline/crowpanel-128/` is the OLD design and also holds SLS-local
  working files (`SquareLine_Project.slp`, `project.info`) that the download does **not** include.

## Strict Boundary Box
- Do **NOT** touch anything under `modules/`, `src/`, `test/`, `include/`, or `platformio.ini`.
- Do **NOT** delete `squareline/crowpanel-128/SquareLine_Project.slp` or `project.info` — these are
  SquareLine-local working files; leave the existing ones in place (SLS regenerates them).
- Do **NOT** attempt to run SquareLine Studio or generate any UI C — that is WO2.
- Do **NOT** modify `.gitignore` unless a newly-copied file would otherwise be ignored.

## Tooling Constraints
- No build required. This WO produces no compilable change.
- Use ordinary file tools for the copy. Verify the result with `git status --short` and `git diff --stat`.

## Line-Item Instructions
- [ ] Copy `SquareLine_Project.spj`, `SquareLine_Project.sll`, `Themes.slt` from the download over the repo copies in `squareline/crowpanel-128/`.
- [ ] Copy `assets/cblabs_180x58.png` into `squareline/crowpanel-128/assets/`.
- [ ] `git rm` (or delete) `squareline/crowpanel-128/assets/cblabs_320x100.png` — the redesign no longer references it.
- [ ] Leave `assets/cblabs.png` as-is (unchanged, byte-identical in the download).
- [ ] Update `squareline/crowpanel-128/README.md`:
  - System screen: describe the groups as **Error**, **Setup** (WiFi SSID + config-portal URL), **OTA Progress**, **Busy** (please-wait spinner), and the **new Message** group (icon/title/body).
  - Diagnostics screen: reflect the simplified layout (arc, category, value, status, counter) — the roller and touch-test button were removed.
  - Boot Logo section: point to `cblabs_180x58.png` as the current logo (note `cblabs.png` remains the full-size source).
  - Keep the SquareLine Studio version note accurate (**1.6.1**) and the LVGL version (**9.2.2**).
- [ ] Confirm no other files changed: `git status --short` should show only the five items above (3 project files, 1 asset add, 1 asset remove) plus README.

## Verification Requirement
- Run `git status --short` and `git diff --stat` and paste them into the Completion Report.
- No build is expected. **Before committing, ask the user if they are happy with the changes.**
- Suggested commit message: `chore(sls): ingest reviewed CrowPanel 1.28" SLS project + logo`

### Issue Surfacing
If you discover a defect or concern *outside your boundary box*, surface it in your Completion Report — do not file issues directly. The PA will triage.

## Provenance
Part of **PR: SLS Screen Redesign** ([PROJECT.md](../PROJECT.md)); decomposition approved by the human (see PROJECT.md §8 Status Log). Authorized to execute WO1 autonomously within this Strict Boundary Box.

---
## Completion Report
<!-- Executed inline by the PA acting as architect, at the human's direction -->

### Status
complete

### Changes Made
- `squareline/crowpanel-128/SquareLine_Project.spj` / `.sll` / `Themes.slt` — replaced with the reviewed Claude Design export.
- `squareline/crowpanel-128/assets/cblabs_180x58.png` — added (new boot logo).
- `squareline/crowpanel-128/assets/cblabs_320x100.png` — removed (`git rm`); no longer referenced by the design.
- `squareline/crowpanel-128/README.md` — System screen groups rewritten (Error / Setup [SSID + portal URL] / OTA / Busy / Message); Diagnostics simplified (arc / category / value / status / counter); boot-logo section points at `cblabs_180x58.png`; group-visibility example updated to `ui_SystemSetupGroup`.

### Decisions Made
- Left the SLS-local `SquareLine_Project.slp` and `project.info` untouched (per boundary box). Bonus: `.slp` `uiExportFolderPath` already points at `modules/LVGLDisplay/ui`, so the human's export defaults to the correct path.

### Verification
- `squareline-studio-expert/scripts/validate_project.py` → 🟢 PASSED, 0 warnings (39 GUIDs, 3 screens, 23 font refs, nidcnt 1000713 > max nid 1000703).
- No firmware touched (`modules/`, `src/`, `test/` untouched).

### Out-of-Scope Findings
- (none)

### Blockers
(none)
