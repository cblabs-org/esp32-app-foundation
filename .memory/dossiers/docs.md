# docs

## Invariants & contracts

### [docseed-5ead17e4] **Section 17 (`docs/design_specification.md`)** is the canonical summary ledger and index for all framework architectural decisions.
`invariant · kind: semantic · anchors: docs/design_specification.md · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-78ac0958] **State:** Currently **v2.0 in preparation** (sections being rewritten against the delivered architecture).
`invariant · kind: semantic · anchors: docs/design_specification.md · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-84500e97] **Link:** [Design Specification](design_specification.md)
`invariant · kind: semantic · anchors: docs/design_specification.md · source_doc: docs/README.md · evidence: doc:docs/README.md`

### [docseed-899ba4f1] **Role:** The architectural specification — defines what the system *is*, what invariants it maintains, and what design alternatives were considered and rejected.
`invariant · kind: semantic · anchors: docs/design_specification.md · source_doc: docs/README.md · evidence: doc:docs/README.md`

## Decisions

### [seed-40d61690] **Relative link repairs in archived doc:** When moving `lvgl-display-design.md` into `docs/archive/`, adjusted relative paths (`../modules/LVGLDisplay/README.md` → `../../modules/LVGLDisplay/README.md`, `developer/*.md` → `../developer/*.md`, `design_specification.md` → `../design_specification.md`) to avoid broken links.
`decision · kind: episodic · anchors: docs/README.md, CONTRIBUTING.md, docs/README.md, docs/design_specification.md, docs/archive/lvgl-display-design.md, modules/LVGLDisplay/README.md`

### [seed-9d8627c5] **Disposition table placement:** Positioned immediately following the front matter metadata block (`Last Updated: 2026-09-03` / `---`) and before `## Table of Contents` so that any agent or human reviewing the specification is warned prior to navigating into individual sections.
`decision · kind: episodic · anchors: docs/README.md, CONTRIBUTING.md, docs/README.md, docs/design_specification.md, docs/archive/lvgl-display-design.md, modules/LVGLDisplay/README.md`

### [seed-a6a016c8] **Link protocol for conversation audit source:** Replaced `../conversation://8b733dfa-645a-478e-b6cc-e08c50620730` in the disposition table preamble with `conversation://8b733dfa-645a-478e-b6cc-e08c50620730` so the custom URI scheme parses cleanly as an active link rather than a broken relative filesystem path.
`decision · kind: episodic · anchors: docs/README.md, CONTRIBUTING.md, docs/README.md, docs/design_specification.md, docs/archive/lvgl-display-design.md, modules/LVGLDisplay/README.md`

### [seed-c08dbb93] Cleanly separated historical design rationale (`docs/design/config-gate.md`) from normative technical specification (§8/§9).
`decision · kind: episodic · anchors: docs/design_specification.md, lib/ConfigManifest`

### [seed-d3b52309] Confined all specification changes strictly to §7–§11A (lines 1085–2061, 978 lines). §1–§6, §12–§17, source code, and agent skills remained completely untouched.
`decision · kind: episodic · anchors: docs/design_specification.md, lib/ConfigManifest`

### [seed-f465187b] **Severity bands relabeling:** Checked repository for in-repo quotations of the severity classification. None found, so skipped as instructed.
`decision · kind: episodic · anchors: docs/README.md, CONTRIBUTING.md, docs/README.md, docs/design_specification.md, docs/archive/lvgl-display-design.md, modules/LVGLDisplay/README.md`

### [seed-fded3dff] Documented `markAppValid()` as explicitly delegated to the downstream application rather than called during the boot sequence.
`decision · kind: episodic · anchors: docs/design_specification.md, lib/ConfigManifest`

## Open issues

### [seed-8f26371f] Lines 391 and 617 of `docs/design_specification.md` still contain links to `lvgl-display-design.md`. Per the Strict Boundary Box, body content was left untouched for WO3/WO4 to update as part of the section rewrites.
`open-issue · kind: semantic · anchors: docs/README.md, CONTRIBUTING.md, docs/README.md, docs/design_specification.md, docs/archive/lvgl-display-design.md, modules/LVGLDisplay/README.md`

### [seed-97e5ccc8] Lines 2092, 2107, 2155, 3341, 3343 in §12 and §17 still reference `AsyncWebServer` and legacy `AppState` values; these will be addressed in Work Order 5 (§12–§17).
`open-issue · kind: semantic · anchors: docs/design_specification.md, lib/ConfigManifest`

### [seed-f55ee1fa] `docs/hardware-configuration-design.md` line 724 references `lvgl-display-design.md`, but this file was explicitly marked as out-of-scope (Risk R2, feature branch) and was not modified.
`open-issue · kind: semantic · anchors: docs/README.md, CONTRIBUTING.md, docs/README.md, docs/design_specification.md, docs/archive/lvgl-display-design.md, modules/LVGLDisplay/README.md`
