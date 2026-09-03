# ESP32 App Foundation — Documentation Authority Map

> ESP32 App Foundation (c) 2026 CB Labs
>
> Canonical documentation index and authority map for the ESP32 App Foundation framework.

---

## Documentation Trees

The repository maintains four distinct documentation trees, each serving a specific audience, abstraction layer, and purpose:

### Tree 1: Design Specification (`docs/design_specification.md`)
- **Link:** [Design Specification](design_specification.md)
- **Role:** The architectural specification — defines what the system *is*, what invariants it maintains, and what design alternatives were considered and rejected.
- **State:** Currently **v2.0 in preparation** (sections being rewritten against the delivered architecture).

### Tree 2: Developer Guide (`docs/developer/`)
- **Link:** [Developer Guide Index](developer/README.md)
- **Role:** 25 maintained documents for human developers building applications or extending the framework.
- **Content:** Practical how-to guides, migration guides, coding patterns, performance analysis, security architecture, and subsystem deep dives.

### Tree 3: Agent Knowledge Base (`.agents/skills/framework-expert/`)
- **Link:** [Agent Knowledge Base](../.agents/skills/framework-expert/)
- **Role:** 78 Markdown files (8,563 lines) structured and optimized for coding agent consumption.
- **Content:** Granular per-interface documents, per-system architectural breakdowns, contracts, and references.
- **Authority:** The most current documentation in the repository for agent workflows.

### Tree 4: Design Records (`docs/*-design.md` + `docs/design/`)
- **Role:** Point design documents for specific subsystems and architectural explorations.
- **Rule:** Every design record declares its own implementation status in a top-level banner (`WIP`, `FINALIZED`, `IMPLEMENTED`, or `SUPERSEDED`).

---

## Conflict-Resolution Orders

When documentation sources diverge or contradict each other, resolve using two distinct authority orders depending on the nature of the question:

| Question type | Authority order | Rationale |
|---|---|---|
| **Factual** (what is this signature? what is the default? how big is that partition?) | `include/interfaces/*.hpp` > `.agents/skills/framework-expert/interfaces/*.md` > `docs/developer/*.md` > `docs/design_specification.md`. **Code is truth.** | A document that contradicts the code is a bug in the document. |
| **Architectural** (what is this system for? what are its invariants? what was rejected?) | `docs/design_specification.md` first, then `docs/design/`, then ADRs | The code cannot answer these. A change that contradicts the specification's stated architecture is a bug in the change. |

---

## Folding Rule for Design Documents

A design document folds into the specification when — and only when — its design has shipped:

- **Shipped** → Fold normative content into v2.0. Keep the design doc only if it carries rationale or rejected alternatives that don't belong in a specification; otherwise retire it.
- **Not yet shipped** (WIP, or finalized-but-unimplemented) → Stays outside. Folding unshipped designs into the spec would make the spec describe code that does not exist.
- **Superseded** → Move to [`docs/archive/`](archive/).

### Design Document Disposition Table

| Document | Lines | Status | Disposition |
|---|---|---|---|
| [`config-gate-design.md`](config-gate-design.md) | 1482 | Implemented | **Fold** → v2.0 §8/§9 (WO4) |
| [`lvgl-display-design.md`](archive/lvgl-display-design.md) | 1342 | SUPERSEDED | **Archive** → `docs/archive/` (this WO) |
| [`microkernel-architecture-design.md`](microkernel-architecture-design.md) | 681 | WIP — not shipped | **Keep outside** |
| [`hardware-configuration-design.md`](hardware-configuration-design.md) | 743 | FINALIZED — ready for implementation | **Keep outside** (Risk R2) |
| [`design/logging.md`](design/logging.md) | 24 | Implemented | **Fold** → §11A (WO4), retire |

---

## Historical Archives

Point-in-time assessments, early evaluations, and superseded designs preserved for historical provenance:

- [Security Evaluation Report](archive/security-evaluation-report.md) — v0.1.0 audit (2026-06-01)
- [Embedded Best Practices Assessment](archive/embedded-best-practices-assessment.md) — initial review (2026-05-31)
- [Comparative Analysis Report](archive/comparative_analysis_report.md) — T1/T2 agent comparison
- [LVGL Display Module — Design Document (Historical)](archive/lvgl-display-design.md) — SquareLine Studio era (superseded by LVGL Pro Editor architecture)
