---
name: spec-v2-wo2
description: Documentation inventory, authority map, spec triage — mechanical doc triage
complexity: mechanical
---

# WO2 — Documentation Inventory, Authority Map, Spec Triage

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Establish the documentation authority map, update the spec front matter to be honest about its state, publish the folding rule for design docs, and archive the superseded LVGL design doc. This is all mechanical work — no judgement calls.

**Parent issue:** [#215](https://github.com/matthewmcneill/esp32-app-foundation/issues/215)  
**WO issue:** [#217](https://github.com/matthewmcneill/esp32-app-foundation/issues/217)  
**Branch:** `spec-v2/wo2-inventory-and-triage`

## Strict Boundary Box

**In scope:** Only the files listed below.  
**Out of scope:**
- `docs/design_specification.md` **body content** (WO3–WO5 handle that). You edit ONLY the front matter and per-section disposition table.
- Any source code files
- `.agents/skills/framework-expert/` content (not this WO)
- `docs/hardware-configuration-design.md` — **DO NOT MOVE OR EDIT** (Risk R2: lives on a feature branch)
- Do NOT create any new branches

## Tooling Constraints

- No build or test tools needed — this WO is documentation only.
- Read `.agents/rules/` files before starting.

## Line-Item Instructions

### 1. Rewrite `docs/README.md` as the Documentation Authority Map

- [ ] Replace the current `docs/README.md` content with a comprehensive authority map that names all four documentation trees:

  **Tree 1: Design Specification** (`docs/design_specification.md`)
  - The architectural specification — what the system *is*, what invariants it maintains, what was rejected
  - Currently v2.0 in preparation (sections being rewritten)

  **Tree 2: Developer Guide** (`docs/developer/`)
  - 25 maintained documents for human developers
  - Practical how-to guides, migration guides, coding patterns, performance, security

  **Tree 3: Agent Knowledge Base** (`.agents/skills/framework-expert/`)
  - 78 Markdown files, 8,563 lines
  - Optimised for coding agent consumption — per-interface documents, per-system breakdowns
  - The most current documentation in the repository

  **Tree 4: Design Records** (`docs/` root-level `*-design.md` files + `docs/design/`)
  - Point design documents for specific subsystems
  - Each declares its own implementation status in a banner

- [ ] Publish the **conflict-resolution orders** (two orders, not one):

  | Question type | Authority order | Rationale |
  |---|---|---|
  | **Factual** (what is this signature? what is the default? how big is that partition?) | `include/interfaces/*.hpp` > `.agents/skills/framework-expert/interfaces/*.md` > `docs/developer/*.md` > `docs/design_specification.md`. **Code is truth.** | A document that contradicts the code is a bug in the document. |
  | **Architectural** (what is this system for? what are its invariants? what was rejected?) | `docs/design_specification.md` first, then `docs/design/`, then ADRs | The code cannot answer these. A change that contradicts the specification's stated architecture is a bug in the change. |

- [ ] Publish the **folding rule**:

  > A design document folds into the specification when — and only when — its design has shipped.
  > - **Shipped** → fold normative content into v2.0. Keep the design doc only if it carries rationale or rejected alternatives that don't belong in a specification; otherwise retire it.
  > - **Not yet shipped** (WIP, or finalized-but-unimplemented) → stays outside. Folding unshipped designs into the spec would make the spec describe code that does not exist.
  > - **Superseded** → `docs/archive/`.

- [ ] List the design doc disposition table:

  | Document | Lines | Status | Disposition |
  |---|---|---|---|
  | `config-gate-design.md` | 1482 | Implemented | **Fold** → v2.0 §8/§9 (WO4) |
  | `lvgl-display-design.md` | 1342 | SUPERSEDED | **Archive** → `docs/archive/` (this WO) |
  | `microkernel-architecture-design.md` | 681 | WIP — not shipped | **Keep outside** |
  | `hardware-configuration-design.md` | 743 | FINALIZED — ready for implementation | **Keep outside** (Risk R2) |
  | `design/logging.md` | 24 | Implemented | **Fold** → §11A (WO4), retire |

### 2. Update `CONTRIBUTING.md`

- [ ] Add a section titled `## Documentation` that:
  - References `docs/README.md` as the authority map
  - States the folding rule (brief version with a link to the full rule in `docs/README.md`)
  - States that every new design doc at `docs/` or `docs/design/` **must** carry a STATUS banner: one of `WIP`, `FINALIZED`, `IMPLEMENTED`, or `SUPERSEDED`

### 3. Spec Front Matter Update

- [ ] In `docs/design_specification.md`, update ONLY the front matter (the first ~30 lines before §1):
  - Change `Status` to: `v2.0 (in preparation — sections being rewritten)`
  - Change `Last Updated` to today's date: `2026-09-03`
  - Add immediately after the front matter, before §1, a **per-section disposition table**:

  ```markdown
  > [!WARNING]
  > **This specification is being rebuilt as v2.0.** Sections marked ✅ have been rewritten
  > against the delivered architecture. Sections marked ⚠️ contain content from v1.1
  > (2026-05-22) that does not reflect the current codebase. See the
  > [design spec audit report](../conversation://8b733dfa-645a-478e-b6cc-e08c50620730)
  > for the full findings.

  | Section | Status | Notes |
  |---|---|---|
  | §1 Purpose & Vision | ⚠️ v1.1 | Missing LVGL, AuthManager, PreconditionEngine |
  | §2 Design Principles | ⚠️ v1.1 | ConfigManager-as-Gateway not reflected |
  | §3 System Architecture | ⚠️ v1.1 | Component diagram severely outdated |
  | §4 Framework Kernel | ⚠️ v1.1 | Kernel table missing 7 components |
  | §5 Module System | ⚠️ v1.1 | Module catalog severely outdated |
  | §6 Extension Points | ⚠️ v1.1 | **Interface signatures will not compile** |
  | §7 Display Architecture | ⚠️ v1.1 | Missing LVGL, IDisplayModule |
  | §8 Configuration | ⚠️ v1.1 | Describes pre-PR-11 pattern |
  | §9 Application Lifecycle | ⚠️ v1.1 | AppState has 3 states not 4 |
  | §10 Web Portal | ⚠️ v1.1 | Wrong web server library |
  | §11 Inter-Module Communication | ⚠️ v1.1 | EventBus dispatch model wrong |
  | §11A Logging & Observability | ✅ v2.0 | Written 2026-09-01 during PR-211 |
  | §12 Repository & Build | ⚠️ v1.1 | Directory structure outdated |
  | §13 Security | ⚠️ v1.1 | Missing AuthManager, OTA v2 |
  | §14 Resource Budget | ⚠️ v1.1 | Partition sizes and locations wrong |
  | §15 Module Authoring | ⚠️ v1.1 | Code samples won't compile |
  | §16 Testing | ⚠️ v1.1 | Test infrastructure evolved |
  | §17 Decision Log | ⚠️ v1.1 | Missing D40+ decisions |
  ```

### 4. Archive `lvgl-display-design.md`

- [ ] Move `docs/lvgl-display-design.md` → `docs/archive/lvgl-display-design.md`
  - Use `git mv` to preserve history
- [ ] Check for any inbound links from other docs that reference `lvgl-display-design.md` and update them to point to `docs/archive/lvgl-display-design.md` (or to `modules/LVGLDisplay/README.md` if the link was seeking current info)

### 5. Relabel Severity Bands (Cosmetic)

- [ ] In the audit report artifact at the conversation level, the severity bands were relabelled per [PA-11]. If the audit report or its severity classification is quoted or referenced anywhere in the repo files that you can see, ensure the labels read:
  - 🔴 **Critical — Actively Misleading** (not just "Critical — Misleading")
  - 🟠 **Major — Undocumented Systems** (not just "Major — Missing Systems")
  - Add a one-line preamble: "This audit found zero code defects. All items below are documentation integrity findings."

  If there are no in-repo quotations of the severity bands, skip this step.

## Verification Requirement

- All edited Markdown files must be well-formed (no broken links, no orphaned table rows)
- `git mv` used for the archive move (not copy-delete)
- Front matter date matches today

**CRITICAL:** Before committing, ask the user if they are happy with the changes.

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
Complete

### Changes Made
- `docs/README.md`: Completely rewritten as the Documentation Authority Map. Enumerated all four documentation trees (Design Specification, Developer Guide, Agent Knowledge Base, Design Records), established the two-tier conflict resolution orders (Factual vs. Architectural), published the design document folding rule, added the 5-row design document disposition table, and indexed historical archives.
- `CONTRIBUTING.md`: Added `## Documentation` section linking to the Authority Map in `docs/README.md`, mandated status banners (`WIP`, `FINALIZED`, `IMPLEMENTED`, `SUPERSEDED`) on all design documents in `docs/` and `docs/design/`, and summarized the folding rule with link to full policy.
- `docs/design_specification.md`: Updated front matter metadata (`Status: v2.0 (in preparation — sections being rewritten)`, `Last Updated: 2026-09-03`), added a `> [!WARNING]` callout, and inserted the 18-row per-section disposition table before the Table of Contents. All body sections (§1–§17) untouched.
- `docs/lvgl-display-design.md`: Moved to `docs/archive/lvgl-display-design.md` using `git mv`. Repaired relative links in header, section 1.2, and references to match the new subdirectory depth.
- `modules/LVGLDisplay/README.md`: Updated inbound reference link at line 187 to point to `../../docs/archive/lvgl-display-design.md`.

### Decisions Made
- **Link protocol for conversation audit source:** Replaced `../conversation://8b733dfa-645a-478e-b6cc-e08c50620730` in the disposition table preamble with `conversation://8b733dfa-645a-478e-b6cc-e08c50620730` so the custom URI scheme parses cleanly as an active link rather than a broken relative filesystem path.
- **Disposition table placement:** Positioned immediately following the front matter metadata block (`Last Updated: 2026-09-03` / `---`) and before `## Table of Contents` so that any agent or human reviewing the specification is warned prior to navigating into individual sections.
- **Relative link repairs in archived doc:** When moving `lvgl-display-design.md` into `docs/archive/`, adjusted relative paths (`../modules/LVGLDisplay/README.md` → `../../modules/LVGLDisplay/README.md`, `developer/*.md` → `../developer/*.md`, `design_specification.md` → `../design_specification.md`) to avoid broken links.
- **Severity bands relabeling:** Checked repository for in-repo quotations of the severity classification. None found, so skipped as instructed.

### Dead Ends & Warnings
- None encountered. All operations completed cleanly.

### Out-of-Scope Findings
- Lines 391 and 617 of `docs/design_specification.md` still contain links to `lvgl-display-design.md`. Per the Strict Boundary Box, body content was left untouched for WO3/WO4 to update as part of the section rewrites.
- `docs/hardware-configuration-design.md` line 724 references `lvgl-display-design.md`, but this file was explicitly marked as out-of-scope (Risk R2, feature branch) and was not modified.

### Blockers
- None.
